#include <stdio.h>

#define WIDTH 1920
#define HEIGHT 1080
#define DEPTH 3

#define gaussian_accel_base (volatile int *) 0xFF202040 // gaussian filter acceletaor base address
#define component_factor 1000000.0                      // Component calculates value multiplied by a factor of 10^6

volatile unsigned *compression_acc = (volatile unsigned *)0x10c0; //compression accelerator base address
volatile unsigned *compression_out = (volatile unsigned *)0x10f0; //output of the compression accelerator


void apply_gaussian(unsigned char input[HEIGHT][WIDTH][DEPTH], unsigned char output[HEIGHT][WIDTH][DEPTH]) {
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            for (int d = 0; d < DEPTH; d++) {
                int top = (i == 0) ? 0 : i - 1;
                int bottom = (i == HEIGHT - 1) ? (HEIGHT - 1) : (i + 1);
                int left = (j == 0) ? 0 : j - 1;
                int right = (j == WIDTH - 1) ? (WIDTH - 1) : (j + 1);
                *(gaussian_accel_base + 1) = input[top][left][d];
                *(gaussian_accel_base + 2) = input[top][j][d];
                *(gaussian_accel_base + 3) = input[top][right][d];
                *(gaussian_accel_base + 4) = input[i][left][d];
                *(gaussian_accel_base + 5) = input[i][j][d];
                *(gaussian_accel_base + 6) = input[i][right][d];
                *(gaussian_accel_base + 7) = input[bottom][left][d];
                *(gaussian_accel_base + 8) = input[bottom][j][d];
                *(gaussian_accel_base + 9) = input[bottom][right][d];


                // *(gaussian_accel_base) will return output value scaled up by 10^6
                output[i][j][d] = (int) round(*(gaussian_accel_base) / component_factor); 
            }
        }
    }
}

void compression(unsigned char input[HEIGHT][WIDTH][DEPTH]) {
    *(compression_acc + 1) = &input[0][0][0];
    *(compression_acc) = 0;
    *compression_acc; //make sure compression is finished

    //the hardware should place the output starting from the address compression_out
    //the first four bytes should be the size in words?
    //the first two bytes should be width in big endian
    //the second two bytes shoule be length in big endian
}

int main() {

    //read the image input from the bluetooth file 
    FILE* fp;

    unsigned char img[HEIGHT][WIDTH][DEPTH];
    fp = fopen("bytes.txt", "r+");

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            for (int k = 0; k < DEPTH; k++) {
                char buff;
                img[i][j][k] = fgetc(fp);
            }
        }
    }

    unsigned char out[HEIGHT][WIDTH][DEPTH];

    apply_gaussian(img, out);

    compression(out);

    //write the compressed image to the WiFi port
    FILE* newFp;
    newFp = fopen("out.txt", "w+");
    
    fwrite((const void *)(compression_out + 1), sizeof(compression_out), (size_t)(*(compression_out)), newFp);

    fflush(newFp);
    fclose(fp);
    fclose(newFp);
    return 0;
}