#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

typedef struct {
    int rows;
    int columns;
    float data[3][3];
} Kernel;

typedef struct {
    unsigned char R, G, B;
} RGB;

typedef struct {
    int width;
    int height;
    int channels;
    RGB *data;
} Image;

Image LoadRGBImage(const char *filename);
void SeparateImageIntoRGB(Image *img, unsigned char *raw);
void Normalize2dArray(Kernel* K);
void FilterImageWithKernel(Image *img, Kernel K, const char *filter);
int SaveImage(Image *img, const char *filename);

int main() {
    Image img_blur = LoadRGBImage("image.jpg");
	Image img_sharpen = LoadRGBImage("image.jpg");
    
    Kernel K_blur = {
        .rows = 3,
        .columns = 3,
        .data = {
            {1, 2, 1},
            {2, 4, 2},
            {1, 2, 1}
		}
    };

    Kernel K_sharpen = {
        .rows = 3,
        .columns = 3,
        .data = {
            { 0, -1,  0},
            {-1,  5, -1},
            { 0, -1,  0}
		}
	};

    Normalize2dArray(&K_blur);
    FilterImageWithKernel(&img_blur, K_blur, "blur");

    Normalize2dArray(&K_sharpen);
    FilterImageWithKernel(&img_sharpen, K_sharpen, "sharpen");
    
    if (!SaveImage(&img_blur, "filtered_image_blur.jpg")) {
        printf("Failed to save the blurred image\n");
    } else {
        printf("Saved blurred image\n");
	}
    if (!SaveImage(&img_sharpen, "filtered_image_sharpen.jpg")) {
        printf("Failed to save the sharpened image\n");
    } else {
        printf("Saved sharpened image\n");
	}
    return 0;
}

Image LoadRGBImage(const char *filename) {
    int width, height, channels;
    Image img = {0};
    
    unsigned char *raw = stbi_load(filename, &width, &height, &channels, 0);
    if (!raw) {
        printf("Error loading image: ");
        printf("%s\n", stbi_failure_reason());
        return img;
	}
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

    img.width = width;
    img.height = height;
    img.channels = channels;
    img.data = malloc(sizeof(RGB) * width * height);
    SeparateImageIntoRGB(&img, raw);

    stbi_image_free(raw);
    return img;
}

void SeparateImageIntoRGB(Image *img, unsigned char *raw) {
    RGB (*pixels)[img->width] = (RGB (*)[img->width]) img->data;
	int row_offset = img->width * img->channels;
    int column_offset = 1 * img->channels;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {

            int idx = (y*row_offset) + (x*column_offset);

            pixels[y][x].R = raw[idx + 0];
            pixels[y][x].G = raw[idx + 1];
            pixels[y][x].B = raw[idx + 2];
        }
    }
}

void Normalize2dArray(Kernel* K) {
	int sum_of_K = 0;
    for (int i=0; i<K->rows; i++) {
        for (int j=0; j<K->columns; j++) {
			sum_of_K += K->data[i][j];
		}
	}
    for (int i=0; i<K->rows; i++) {
        for (int j=0; j<K->columns; j++) {
		    K->data[i][j] /= sum_of_K;
		}
	}
}

void FilterImageWithKernel(Image *img, Kernel K, const char *filter) {
    RGB (*pixels)[img->width] = (RGB (*)[img->width]) img->data;
    int height = img->height;
    int width = img->width;
    float sum_B = 0;
	float sum_G = 0;
	float sum_R = 0;

    RGB temp[height][width];

	printf("Filtering image with %s filter\n", filter);
    for (int i=0; i<height; i++) {
        for (int j=0; j<width; j++) {
            temp[i][j] = pixels[i][j];
		}
	}
    
    for (int j=0; j<width-1; j++) {
        for (int i=0; i<height-1; i++) {
            sum_B = sum_G = sum_R = 0;
            for (int a=-1; a<=1; a++) {
                for (int b=-1; b<=1; b++) {
                    int row = i + a;
                    int col = j + b;
                    if (row>=0 && row<height && col>=0 && col<width) {
                        sum_R += temp[row][col].R * K.data[a+1][b+1];
                        sum_G += temp[row][col].G * K.data[a+1][b+1];
                        sum_B += temp[row][col].B * K.data[a+1][b+1];
					}
				}
			}
		    pixels[i][j].R = (sum_R < 0) ? 0 : (sum_R > 255) ? 255 : sum_R;
		    pixels[i][j].G = (sum_G < 0) ? 0 : (sum_G > 255) ? 255 : sum_G;
		    pixels[i][j].B = (sum_B < 0) ? 0 : (sum_B > 255) ? 255 : sum_B;
		}
	}
}

int SaveImage(Image *img, const char *filename) {
    unsigned char *output = (unsigned char *)img->data;
    int success = stbi_write_jpg(filename, img->width, img->height, img->channels, output, 100);
    return success;
}
