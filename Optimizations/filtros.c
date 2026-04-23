#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <immintrin.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct {
    int size;
    float *data_1dx;
    float *data_1dy;
    float *data_2d;
} Kernel;

typedef struct __attribute__((packed)){
    unsigned char R, G, B;
} RGB;

typedef struct {
    int width;
    int height;
    int channels;
    RGB *data;
} Image;

typedef struct {
    int width;
    int height;
    int channels;
    unsigned char *raw;
} RawImage;

RawImage LoadImage(const char *filename);
Image LoadRGBImage(RawImage raw_img);
void SeparateImageIntoRGB(Image *img, unsigned char *raw);
Kernel CreateGaussianKernel(double sigma, int size);
void Normalize2dArray(Kernel* K);
void Normalize1dArrays(Kernel* K);
void FilterImageWith2dKernel(Image *img, Kernel K, const char *filter);
void FilterImageWith1dKernel(Image *img, Kernel K, const char *filter);
Image FilterImageWithSharpen(RawImage raw_img, Image *img, float alpha);
int SaveImage(Image *img, const char *filename);
void FreeRawImage(RawImage *raw_img);
void FreeImage(Image *img);
void FreeKernel(Kernel *K);
void log_elapsed_time(struct timespec begin, struct timespec end);

// gcc -o filtros filtros.c -Wextra -pedantic -lm -O3 -march=native -ffast-math -mfma -mavx2 && ./filtros "input/image2.jpg" "output/"

int main(int argc, char **argv) {

    if (argc<4) {
        printf("Usage: filtros input output (kernel_type)1d/2d\n");
        return 1;
	}
    
    if (strcmp(argv[3], "1d") != 0 && strcmp(argv[3], "2d") != 0) {
        printf("Unrecognized kernel type. Use 1d or 2d\n");
        return 2;
	}
    
    RawImage raw_blur = LoadImage(argv[1]);
    Image img_blur = LoadRGBImage(raw_blur);
    Kernel K_blur = CreateGaussianKernel(1.0, 3);

    if (strcmp(argv[3], "1d") == 0) Normalize1dArrays(&K_blur);
    if (strcmp(argv[3], "2d") == 0) Normalize2dArray(&K_blur);
    
	struct timespec begin = {0};
    int ret = clock_gettime(CLOCK_MONOTONIC, &begin);
    assert(ret==0);
    
    if (strcmp(argv[3], "1d") == 0) FilterImageWith1dKernel(&img_blur, K_blur, "blur");
    if (strcmp(argv[3], "2d") == 0) FilterImageWith2dKernel(&img_blur, K_blur, "blur");

    struct timespec end = {0};
    ret = clock_gettime(CLOCK_MONOTONIC, &end);
    assert(ret==0);

    log_elapsed_time(begin, end);

    if (!SaveImage(&img_blur, argv[2])) {
        printf("Failed to save the blurred image\n");
    } else {
        printf("Saved blurred image\n");
	}

    FreeRawImage(&raw_blur);
    FreeImage(&img_blur);
    FreeKernel(&K_blur);
    return 0;
}

RawImage LoadImage(const char *filename) {
    int width, height, channels;
    unsigned char *raw = stbi_load(filename, &width, &height, &channels, 3);
    if (!raw) {
        printf("Error loading image: ");
        printf("%s\n", stbi_failure_reason());
        RawImage fail = {0};
        return fail;
	}
    RawImage img = {
        .width = width,
        .height = height,
        .channels = channels,
        .raw = raw
	};
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
    return img;
}

Image LoadRGBImage(RawImage raw_img) {
    Image img = {
        .width = raw_img.width,
        .height = raw_img.height,
        .channels = raw_img.channels,
        .data = malloc(sizeof(RGB) * raw_img.width * raw_img.height)
	};
    SeparateImageIntoRGB(&img, raw_img.raw);    
    return img;
}

void SeparateImageIntoRGB(Image *img, unsigned char *raw) {
    RGB (*pixels)[img->width] = (RGB (*)[img->width]) img->data;
	int row_offset = img->width * img->channels;
    int column_offset = 1 * img->channels;    
    for (int y=0; y<img->height; y++) {
        for (int x=0; x<img->width; x++) {
            int idx = (y*row_offset) + (x*column_offset);
            pixels[y][x].R = raw[idx+0];
            pixels[y][x].G = raw[idx+1];
            pixels[y][x].B = raw[idx+2];
        }
    }
}

Kernel CreateGaussianKernel(double sigma, int size) {
	Kernel K = {
        .size = size,
        .data_1dx = malloc(size * sizeof *K.data_1dx),
        .data_1dy = malloc(size * sizeof *K.data_1dy),
        .data_2d = malloc(sizeof(float) * size * size)
    };
	double sum = 0.0;
    int center = size/2;
	double gauss_x[size], gauss_y[size];
    double inv_sigma_sq = 1.0 / (sigma*sigma);
    for (int i=0; i<size; i++) {
        double x = i - center;
        gauss_x[i] = exp(-0.5 * (x*x) * inv_sigma_sq);
        gauss_y[i] = gauss_x[i];
        K.data_1dx[i] = gauss_x[i];
        K.data_1dy[i] = gauss_y[i];
        sum += gauss_x[i] * gauss_y[i];
	}
    float (*kernel2d)[K.size] = (float (*)[K.size]) K.data_2d;
    float test_sum = 0;
    for (int i=0; i<size; i++) {
        for (int j=0; j<size; j++) {
            kernel2d[i][j] = gauss_x[j] * gauss_y[i] / sum;
            test_sum += kernel2d[i][j];
		}
	}
    return K;
}

void Normalize2dArray(Kernel* K) {
    float sum_of_K = 0;
    int size = K->size;
    float (*kernel2d)[size] = (float (*)[size]) K->data_2d;
    for (int i=0; i<size; i++) {
        for (int j=0; j<size; j++) {
			sum_of_K += kernel2d[i][j];
		}
	}
    for (int i=0; i<size; i++) {
        for (int j=0; j<size; j++) {
		    kernel2d[i][j] /= sum_of_K;
		}
	}
}

void Normalize1dArrays(Kernel* K) {
    float sum_of_Kx = 0;
    float sum_of_Ky = 0;
    int size = K->size;
    for (int i=0; i<size; i++) {
			sum_of_Kx += K->data_1dx[i];
			sum_of_Ky += K->data_1dy[i];
	}
    for (int i=0; i<size; i++) {
	    K->data_1dx[i] /= sum_of_Kx;
	    K->data_1dy[i] /= sum_of_Ky;
	}
}

void FilterImageWith2dKernel(Image *img, Kernel K, const char *filter) {
    RGB (*pixels)[img->width] = (RGB (*)[img->width]) img->data;
    float (*kernel)[K.size] = (float (*)[K.size]) K.data_2d;
    int height = img->height;
    int width = img->width;
    int K_size = K.size;
    int center = K_size / 2;
    float sum_B = 0;
	float sum_G = 0;
	float sum_R = 0;
    RGB *temp = malloc(sizeof(RGB) * width * height);
    memcpy(temp, img->data, sizeof(RGB) * width * height);
    RGB (*temp_pixels)[width] = (RGB (*)[width]) temp;
    
	printf("Filtering image with %s filter\n", filter);
    for (int i=center; i<height-center; i++) {
        for (int j=center; j<width-center; j++) {
            sum_B = sum_G = sum_R = 0;
            for (int a=0; a<K_size; a++) {
                for (int b=0; b<K_size; b++) {
                    int row = i + (a - (K_size / 2));
                    int col = j + (b - (K_size / 2));
                    sum_R += temp_pixels[row][col].R * kernel[a][b];
                    sum_G += temp_pixels[row][col].G * kernel[a][b];
                    sum_B += temp_pixels[row][col].B * kernel[a][b];
				}
			}
		    pixels[i][j].R = (sum_R < 0) ? 0 : (sum_R > 255) ? 255 : sum_R;
		    pixels[i][j].G = (sum_G < 0) ? 0 : (sum_G > 255) ? 255 : sum_G;
		    pixels[i][j].B = (sum_B < 0) ? 0 : (sum_B > 255) ? 255 : sum_B;
		}
	}
    free(temp);
}

void FilterImageWith1dKernel(Image *img, Kernel K, const char *filter) {
    RGB *pixels = (RGB *) img->data;
    int height = img->height;
    int width = img->width;
    int K_size = K.size;
    int center = K_size / 2;
    RGB *temp = malloc(sizeof(RGB) * width * height);
    memcpy(temp, img->data, sizeof(RGB) * width * height);
    RGB *temp_pixels = (RGB *) temp;

    for (int i=0; i<height; i++) {
        RGB *row = &pixels[i * width];
        RGB *out = &temp_pixels[i * width];

        for (int j=center; j<width-center; j++) {
            float sum_R = K.data_1dy[center] * row[j].R;
            float sum_G = K.data_1dy[center] * row[j].G;
            float sum_B = K.data_1dy[center] * row[j].B;

            for (int a=1; a<=center; a++) {
                float w = K.data_1dy[center - a];
                RGB left = row[j-a];
                RGB right = row[j+a];

                sum_R += w * (left.R + right.R);
                sum_G += w * (left.G + right.G);
                sum_B += w * (left.B + right.B);
            }
            out[j].R = sum_R;
            out[j].G = sum_G;
            out[j].B = sum_B;
        }
        
        for (int j=center; j<height-center; j++) {
            float sum_R = K.data_1dx[center] * temp_pixels[j * width + i].R;
            float sum_G = K.data_1dx[center] * temp_pixels[j * width + i].G;
            float sum_B = K.data_1dx[center] * temp_pixels[j * width + i].B;

            for (int b=1; b<=center; b++) {
                float w = K.data_1dx[center-b];
                RGB top = temp_pixels[(j-b)*width+i];
                RGB bottom = temp_pixels[(j+b)*width+i];

                sum_R += w * (top.R + bottom.R);
                sum_G += w * (top.G + bottom.G);
                sum_B += w * (top.B + bottom.B);
            }
            pixels[j * width + i].R = (sum_R < 0) ? 0 : (sum_R > 255) ? 255 : sum_R;
            pixels[j * width + i].G = (sum_G < 0) ? 0 : (sum_G > 255) ? 255 : sum_G;
            pixels[j * width + i].B = (sum_B < 0) ? 0 : (sum_B > 255) ? 255 : sum_B;
        }
    }
    free(temp);
}

Image FilterImageWithSharpen(RawImage raw_img, Image *img, float alpha) {
    Image output = {
        .width = img->width,
        .height = img->height,
        .channels = img->channels,
        .data = malloc(sizeof(RGB) * img->width * img->height)
	};
	RGB (*pixels)[img->width] = (RGB (*)[img->width]) img->data;
	RGB (*output_pixels)[output.width] = (RGB (*)[output.width]) output.data;
    int row_offset = raw_img.width * raw_img.channels;
    int column_offset = 1 * raw_img.channels;
	float sum_B = 0;
	float sum_G = 0;
	float sum_R = 0;
    float a = alpha + 1.0f;
    
    for (int i=0; i<img->height; i++) {
        for (int j=0; j<img->width; j++) {
            int idx = (i*row_offset) + (j*column_offset);
			sum_R = (a * raw_img.raw[idx+0]) - (alpha * pixels[i][j].R);
            sum_G = (a * raw_img.raw[idx+1]) - (alpha * pixels[i][j].G);
            sum_B = (a * raw_img.raw[idx+2]) - (alpha * pixels[i][j].B);
            output_pixels[i][j].R = (sum_R < 0) ? 0 : (sum_R > 255) ? 255 : sum_R;
            output_pixels[i][j].G = (sum_G < 0) ? 0 : (sum_G > 255) ? 255 : sum_G;
            output_pixels[i][j].B = (sum_B < 0) ? 0 : (sum_B > 255) ? 255 : sum_B;
		}
	}
    return output;
}

int SaveImage(Image *img, const char *filename) {
    unsigned char *output = (unsigned char *)img->data;
    int success = stbi_write_jpg(filename, img->width, img->height, img->channels, output, 100);
    return success;
}

void FreeImage(Image *img) {
    if (!img) return;

    if (img->data) {
        free(img->data);
        img->data = NULL;
	}

    img->width = 0;
    img->height = 0;
    img->channels = 0;
}

void FreeRawImage(RawImage *raw_img) {
    if (!raw_img) return;

    if (raw_img->raw) {
        free(raw_img->raw);
        raw_img->raw = NULL;
	}

    raw_img->width = 0;
    raw_img->height = 0;
    raw_img->channels = 0;
}

void FreeKernel(Kernel *K) {
    if (!K) return;

    if (K->data_1dx) {
        free(K->data_1dx);
        K->data_1dx = NULL;
	}
    if (K->data_1dy) {
        free(K->data_1dy);
        K->data_1dy = NULL;
	}
    if (K->data_2d) {
        free(K->data_2d);
        K->data_2d = NULL;
	}

    K->size = 0;
}

void log_elapsed_time(struct timespec begin, struct timespec end) {
    double a = (double)begin.tv_sec*1e3 + begin.tv_nsec*1e-6;
    double b = (double)end.tv_sec*1e3 + end.tv_nsec*1e-6;
    printf("Time elapsed: %lfms.\n", b-a);
}
