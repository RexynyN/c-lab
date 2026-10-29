#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PI 3.14159265358979323846

#define CHANNEL_IDX(img, x, y, chan) (((y * img->width + x) * img->channels) + chan)
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

    if (str_len < suffix_len) 
        return 0;
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
        printf("[ERRO] (loadImage) Couldn't load image!\n");
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
    printf("B\n");
    return 0; 
}

#define max(a, b) ((a) > (b) ? (a) : (b)) 
#define min(a, b) ((a) < (b) ? (a) : (b)) 
#define clip(v_min, value, v_max) max(v_min, min(v_max, value))
#define clamp(idx, max_size)  max(0, min((max_size) - 1, idx))
#define pixel_clip(value) clip(0, value, 255)


double lanczosKernel(double x, int a) {
    if (x == 0.0) 
        return 1.0;

    if (x <= -a || x >= a)
        return 0.0;
    
    double cardinal = PI * x; 
    return (sin(cardinal) * sin(cardinal / a)) / ((pow(PI, 2) * pow(x, 2)) / a);
}

Image* resizeImg(Image* img, int width, int height, int kernel) {
    int scan_a = kernel * 2; 
    unsigned char tmpPixels[scan_a * img->channels];

    double xFactor = ((double)img->width / width);
    double yFactor = ((double)img->height / height);

    printf("[DEBUG] Inicializando imagem de destino...\n");
    Image* destImg = initImage(width, height, img->channels, NULL);
    printf("[DEBUG] Processando pixels...\n");
    
    for(int y_dest = 0; y_dest < height; y_dest++) {
        for(int x_dest = 0; x_dest < width; x_dest++) {
            
            double x_s = (x_dest + 0.5) * xFactor; 
            double y_s = (y_dest + 0.5) * yFactor; 
            
            int x_int = (int) floor(x_s);
            int y_int = (int) floor(y_s);

            // 1. Interpolação Horizontal (Linhas)
            for(int i = 0; i < scan_a; i++) {
                int y_nghbr = clamp(y_int - kernel + 1 + i, img->height);

                double sumColor[img->channels];
                double sumWeight[img->channels];
                for(int p = 0; p < img->channels; p++) {
                    sumColor[p] = 0.0;
                    sumWeight[p] = 0.0;
                }

                for(int j = 0; j < scan_a; j++) {
                    int x_nghbr = clamp(x_int - kernel + 1 + j, img->width);
                    
                    double x_delta = x_s - x_nghbr;
                    double x_weight = lanczosKernel(x_delta, kernel);
                    
                    int pixel_idx = PIXEL_IDX(img, x_nghbr, y_nghbr);
                    for(int p = 0; p < img->channels; p++) {
                        sumColor[p] += img->data[pixel_idx + p] * x_weight;
                        sumWeight[p] += x_weight;
                    }
                }

                for(int p = 0; p < img->channels; p++) { 
                    int tmp_idx = (i * img->channels) + p;
                    if(sumWeight[p] > 0.00001) { 
                        tmpPixels[tmp_idx] = (unsigned char) clip(0, sumColor[p] / sumWeight[p], 255);
                    } else {
                        int fallback_idx = CHANNEL_IDX(img, x_int, y_nghbr, p);
                        tmpPixels[tmp_idx] = img->data[fallback_idx];
                    }
                }
            }
            
            // 2. Interpolação Vertical (Colunas)
            double sumColor[img->channels];
            double sumWeight[img->channels];
            for(int p = 0; p < img->channels; p++) {
                sumColor[p] = 0.0;
                sumWeight[p] = 0.0;
            }

            for(int i = 0; i < scan_a; i++) {
                int y_nghbr = clamp(y_int - kernel + 1 + i, img->height);
                
                double y_delta = y_s - y_nghbr;
                double y_weight = lanczosKernel(y_delta, kernel);
                
                for(int p = 0; p < img->channels; p++) {
                    int tmp_idx = (i * img->channels) + p;
                    sumColor[p] += tmpPixels[tmp_idx] * y_weight;
                    sumWeight[p] += y_weight;
                }
            }

            // 3. Escrita na Imagem Final
            for(int p = 0; p < img->channels; p++) { 
                int dest_idx = CHANNEL_IDX(destImg, x_dest, y_dest, p); 
                if(sumWeight[p] > 0.00001) {
                    destImg->data[dest_idx] = (unsigned char) clip(0, sumColor[p] / sumWeight[p], 255);
                } else {
                    int fallback_tmp_idx = (kernel * img->channels) + p;
                    destImg->data[dest_idx] = tmpPixels[fallback_tmp_idx];
                }
            }
        }
    }
    printf("[DEBUG] Concluído com sucesso!\n");
    return destImg;
}


Pixel* initPixel(int channels, unsigned char* data) {
    if(data == NULL) {
        data = (unsigned char*)malloc(channels * sizeof(unsigned char));
    }

    Pixel* pix = (Pixel*)malloc(sizeof(Pixel));
    pix->data = data;
    pix->channels = channels;

    return pix;
}



Image* sepiaFilter(Image* img) {
    Image* img_sepia = initImage(img->width, img->height, img->channels, NULL);

    int index = 0;
    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            unsigned char red = img->data[index];
            unsigned char green = img->data[index + 1];
            unsigned char blue = img->data[index + 2];
            
            unsigned char alpha = 255; // Default if not RGBA
            if (img->channels >= 4) {
                img_sepia->data[index + 3] = img->data[index + 3];
            }

            img_sepia->data[index] = pixel_clip(red * 0.393 + green * 0.769 + blue * 0.189);
            img_sepia->data[index + 1] = pixel_clip(red * 0.349 + green * 0.686 + blue * 0.168);
            img_sepia->data[index + 2] = pixel_clip(red * 0.272 + green * 0.534 + blue * 0.131);
            
            index += img->channels;
        }
    }

    return img_sepia;
}


Image* inverseFilter(Image* img) {
    Image* img_inv = initImage(img->width, img->height, img->channels, NULL);

    int index = 0;
    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            unsigned char alpha = 255; // Default if not RGBA
            if (img->channels >= 4) {
                img_inv->data[index + 3] = img->data[index + 3];
            }
            
            // Inverse
            img_inv->data[index] = 255 - img->data[index];
            img_inv->data[index + 1] = 255 - img->data[index + 1];
            img_inv->data[index + 2] = 255 - img->data[index + 2];
            
            index += img->channels;
        }
    }

    return img_inv;
}


Image* grayscaleFilter(Image* img) {
    int grayChannels = img->channels == 4 ? 2 : 1; // 4 means there's the transparency lmao 
    Image* img_gray = initImage(img->width, img->height, grayChannels, NULL);


    int index = 0;
    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            int grayIndex = PIXEL_IDX(img_gray, x, y);

            unsigned char alpha = 255; // Default if not RGBA
            if (img->channels >= 4) {
                img_gray->data[index + 1] = img->data[index + 3];
            }
            
            // Grayscale
            img_gray->data[grayIndex] = (img->data[index] + img->data[index + 1] + img->data[index + 2]) / 3;
            index += img->channels;
        }
    }


    return img_gray;
}

int main(void) {
    // char * path = "images/curaits-vGaAjWIODxc-unsplash.jpg";
    char * path = "./nsfw.png";

    Image* img = loadImage(path);
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", img->width, img->height, img->channels);

    float aspect = (float)img->width / img->height; 
    float hei = (int) img->height * 3;
    img = resizeImg(img, (int)floor(aspect * hei), hei, 3);

    saveImage(img, "resized1.jpg");

  
    printf("Hello2\n");
    // saveImage(img, "test1.png");
    printf("saveImage => returns %d\n", saveImage(inverseFilter(img), "test1.png"));
    printf("saveImage => returns %d\n", saveImage(grayscaleFilter(img), "testg1.png"));
    printf("saveImage => returns %d\n", saveImage(sepiaFilter(img), "testg2.png"));

    return 0;
}