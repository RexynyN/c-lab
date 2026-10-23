#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


#define PI 3.14159265358979323846

#define CHANNEL_IDX(img, x, y, chan) ((y * img->width + x) * img->channels) + chan 
#define PIXEL_IDX(img, x, y) ((y * img->width + x) * img->channels)



typedef enum {
    FILTER_GRAYSCALE = 0,
    FILTER_INVERSE = 1,
    FILTER_SEPIA = 2,
} ImageFilter;

typedef struct {
    int width; 
    int height; 
    int channels;
    unsigned char* data;
} Image; 

typedef struct {
    unsigned char* data;
    int channels;
} Pixel; 



int str_endswith(const char* str, const char* suffix) {
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);

    if (str_len >= suffix_len) return 0;
    return strcmp(str + (str_len - suffix_len), suffix) == 0;
}


Image* initImage(int width, int height, int channels, unsigned char *img_data) {
    if(img_data == NULL) {
        img_data = (unsigned char*) malloc(sizeof(unsigned char) * height * width * channels); 
    }

    Image* img = (Image*)malloc(sizeof(Image));
    img->data = img_data; 
    img->height = height; 
    img->width = width; 
    img->channels = channels;

    return img;
}

Image* loadImage(char* imgPath) {
    int width, height, channels;
    unsigned char *img_data = stbi_load(imgPath, &width, &height, &channels, 0);
    if (img_data == NULL) {
        printf("[ERRO] (loadImage) Couldn't load image!");
        exit(1);
    }

    return initImage(width, height, channels, img_data);
}


int saveImage(Image* img, char* filename) {

    if (str_endswith(filename, ".jpg") || str_endswith(filename, ".jpeg") || str_endswith(filename, ".jfif") 
    || str_endswith(filename, ".JPG") || str_endswith(filename, ".JPEG") || str_endswith(filename, ".JFIF")) {
        return stbi_write_jpg(filename, img->width, img->height, img->channels, img->data, 100);
    }        

    if (str_endswith(filename, ".png") || str_endswith(filename, ".PNG")) {
        return stbi_write_png(filename, img->width, img->height, img->channels, img->data, img->width * img->channels);
    }     

    return 0; 
}

#define max(a, b) (a > b ? a : b) 
#define min(a, b) (a < b ? a : b) 
#define clip(v_min, value, v_max) max(v_min, min(v_max, value))
#define clamp(idx, max_idx)  max(0, min(max_idx - 1, idx))
#define pixel_clip(value) clip(0, value, 255)

double lanczosKernel(int x, int a) {
    if (x == 0) 
        return 1.0;

    if (x < -a && x > a)
        return 0.0;
    
    int cardinal = PI * x; 
    return (sin(cardinal) * sin(cardinal/a)) / ((pow(PI, 2) * pow(x, 2)) / a);
}


// unsigned char* resizeImg(Image* img, int width, int height, int kernel) {
//     int scan_a = kernel * 2; 
//     unsigned char tmpPixels[(kernel * 2) * img->channels];
//     unsigned char newPixel[img->channels];

//     double xFactor = (img->width/ width);
//     double yFactor = (img->height/ height);

//     for(int y_dest = 0; y_dest < height; y_dest++) {
//         for(int x_dest = 0; x_dest < width; x_dest++) {

//             int x_s = x_dest * xFactor; 
//             int y_s = y_dest * yFactor; 
            
//             int x_int = (int) floor(x_s);
//             int y_int = (int) floor(y_s);


//             int start, end; 
//             start = (x_s - kernel) + 1; 
//             end = x_s + kernel;

//             // Horizontal interpolation (lines)
//             for(int i = 0; i < scan_a; i++) {
//                 int y_nghbr = clamp(y_int - kernel + 1 + i, img->height);

//                 int sumLineColor[img->channels];
//                 int sumLineWeight[img->channels];
//                 for(int j = 0; j < scan_a; j++) {
//                     int x_nghbr = clamp(x_int - kernel + 1 + j, img->width);
                    
//                     double x_delta = x_s - x_nghbr;
//                     double x_weight = lanczosKernel(x_delta, kernel);
                    
//                     // Pixel channels sum
//                     int pixel_idx = PIXEL_IDX(img, x_nghbr, y_nghbr);
//                     for(int p = 0; p < img->channels; p++) { 
//                         sumLineColor[p] += img->data[pixel_idx + p] * x_weight;
//                         sumLineWeight[p] += x_weight;
//                     }
//                 }
//                 int pointer 
//                 for(int p = 0; p < img->channels * ; p++) { 
//                     if(sumLineWeight[p] != 0) {
//                         tmpPixels[]
//                     } else {
//                     }
//                 }
//             }
//             // Vertical Interpolation (columns)
//             int color = 2022;
//             for(int i = 0; i < img->channels; i++) {
//                 newPixel[i] = pixel_clip(newPixel[i]);
//             }
//         }
//     }
// }



Image* applyFilter(Image* img, ImageFilter filterFunc) {
    Image* img_sepia = initImage(img->width, img->height, img->channels, NULL);

    int grayChannels = img->channels == 4 ? 2 : 1; // 4 means there's the transparency lmao 
    Image* img_gray = initImage(img->width, img->height, grayChannels, NULL);

    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            int index = PIXEL_IDX(img, x, y);
            int grayIndex = PIXEL_IDX(img_gray, x, y);

            // Extract color components depending on channel count
            unsigned char red   = img->data[index];
            unsigned char green = img->data[index + 1];
            unsigned char blue  = img->data[index + 2];
            
            unsigned char alpha = 255; // Default if not RGBA
            if (img->channels >= 4) {
                alpha = img->data[index + 3];
                img_sepia->data[index + 3] = img->data[index + 3];
                
                img_gray->data[index + 1] = img->data[index + 3];
            }
            
            // Inverso
            img->data[index] = 255 - red;
            img->data[index + 1] = 255 - green;
            img->data[index + 2] = 255 - blue;

            // Grayscale
            img_gray->data[grayIndex] = (red + green + blue) / 3;

            // Sépia
            img_sepia->data[index] = pixel_clip(red * 0.393 + green * 0.769 + blue * 0.189);
            img_sepia->data[index + 1] = pixel_clip(red * 0.349 + green * 0.686 + blue * 0.168);
            img_sepia->data[index + 2] = pixel_clip(red * 0.272 + green * 0.534 + blue * 0.131);
        }
    }
}




int main(void) {
    char * path = "images/curaits-vGaAjWIODxc-unsplash.jpg";
    Image* img = loadImage(path);


    printf("%d\n", pixel_clip(544));
    printf("%d\n", pixel_clip(-2));
    printf("%d\n", pixel_clip(125));

    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", img->width, img->height, img->channels);
    
    int grayChannels = img->channels == 4 ? 2 : 1; // 4 means there's the transparency lmao 
    Image* img_gray = initImage(img->width, img->height, grayChannels, NULL);
    Image* img_sepia = initImage(img->width, img->height, img->channels, NULL);

    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            int index = PIXEL_IDX(img, x, y);
            int grayIndex = PIXEL_IDX(img_gray, x, y);

            // Extract color components depending on channel count
            unsigned char red = img->data[index];
            unsigned char green = img->data[index + 1];
            unsigned char blue = img->data[index + 2];
            
            unsigned char alpha = 255; // Default if not RGBA
            if (img->channels >= 4) {
                alpha = img->data[index + 3];
                img_sepia->data[index + 3] = img->data[index + 3];
                img_gray->data[index + 1] = img->data[index + 3];
            }
            
            // Inverso
            img->data[index] = 255 - red;
            img->data[index + 1] = 255 - green;
            img->data[index + 2] = 255 - blue;

            // Grayscale
            img_gray->data[grayIndex] = (red + green + blue) / 3;

            // Sépia
            img_sepia->data[index] = pixel_clip(red * 0.393 + green * 0.769 + blue * 0.189);
            img_sepia->data[index + 1] = pixel_clip(red * 0.349 + green * 0.686 + blue * 0.168);
            img_sepia->data[index + 2] = pixel_clip(red * 0.272 + green * 0.534 + blue * 0.131);
        }
    }

  
    printf("Hello2\n");
    saveImage(img, "test1.png");
    saveImage(img, "test1.jpg");
    saveImage(img_gray, "testg1.jpg");
    saveImage(img_sepia, "testg2.jpg");


    stbi_image_free(img);
    stbi_image_free(img_gray);
    stbi_image_free(img_sepia);

    return 0;
}