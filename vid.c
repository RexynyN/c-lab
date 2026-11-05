#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "vid.h"

typedef enum {
    FILTER_GRAYSCALE = 0,
    FILTER_INVERSE = 1,
    FILTER_SEPIA = 2,
} ImageFilter;

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

double lanczosKernel(double x, int a) {
    if (x == 0.0) 
        return 1.0;

    if (x <= -a || x >= a)
        return 0.0;
    
    double cardinal = PI * x; 
    return (sin(cardinal) * sin(cardinal / a)) / ((pow(PI, 2) * pow(x, 2)) / a);
}

Image* resizeImg(Image* img, int width, int height, int kernel) {
    if (kernel <= 1) {
        kernel = 3;
    }

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


Image* grayscaleFilter(Image* img, int channels) {
    channels = (channels != 2) ? 1 : 2; 
    printf("Graychannels: %d\n", channels);
    Image* img_gray = initImage(img->width, img->height, channels, NULL);


    int index = 0;
    double rFactor = 299.0/1000; printf("rFactor: %f | ", rFactor);
    double gFactor = 587.0/1000; printf("gFactor: %f | ", gFactor);
    double bFactor = 114.0/1000; printf("bFactor: %f\n\n", bFactor);
    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            int grayIndex = PIXEL_IDX(img_gray, x, y);
            
            if (img->channels >= 4 && channels == 2) {
                img_gray->data[grayIndex + 1] = img->data[index + 3];
            }
            
            // Grayscale
            double gray = (img->data[index] * rFactor) + (img->data[index + 1] * gFactor) + (img->data[index + 2] * bFactor);
            img_gray->data[grayIndex] = (unsigned char) (gray + 0.5);
            
            printf(" %d ", img_gray->data[grayIndex]);
            index += img->channels;

            if (index > img->width * 3) exit(69);
        }
    }


    return img_gray;
}

// int main(void) {
//     // char * path = "images/curaits-vGaAjWIODxc-unsplash.jpg";
//     char * path = "./nsfw.png";

//     Image* img = loadImage(path);
//     printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", img->width, img->height, img->channels);

//     float aspect = (float)img->width / img->height; 
//     float hei = (int) img->height * 3;
//     img = resizeImg(img, (int)floor(aspect * hei), hei, 3);

//     saveImage(img, "resized1.jpg");

  
//     printf("Hello2\n");
//     // saveImage(img, "test1.png");
//     printf("saveImage => returns %d\n", saveImage(inverseFilter(img), "test1.png"));
//     printf("saveImage => returns %d\n", saveImage(grayscaleFilter(img), "testg1.png"));
//     printf("saveImage => returns %d\n", saveImage(sepiaFilter(img), "testg2.png"));

//     return 0;
// }

#include <fftw3.h>


void imgFree(Image* img) {
    free(img->data);
    free(img);
}


#include <string.h>
int compFuncDouble(const void *a, const void *b) {
    double fa = *(const double*) a;
    double fb = *(const double*) b;
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

double doubleMedian(double *data, size_t size) {
    size_t len = sizeof(double) * size; 
    double* dummy = (double*) malloc(len);
    if (!dummy) return 0.0;
    
    memcpy(dummy, data, len);
    qsort(dummy, size, sizeof(double), compFuncDouble);
    
    double median = 0;
    if (size % 2 == 0) {
        median = (dummy[(size / 2) - 1] + dummy[size / 2]) / 2.0;
    } else {    
        median = dummy[size / 2]; 
    }
    
    free(dummy);
    return median; 
}




// Recebe o array de bits (0 ou 1) e o tamanho total (ex: 64 para 8x8, 256 para 16x16)
char* binaryArrayToHex(const char* mask, int hashShape) {
    // Calculamos o tamanho necessário com arredondamento para cima (ceil(size / 4))
    int num_hex_chars = (int) (hashShape + 3) / 4;
    
    char* hex_out = (char*) malloc(num_hex_chars + 1); // + \0
    if (!hex_out) return NULL;

    int remainder = hashShape % 4;
    int bit_idx = 0;
    int hex_idx = 0;

    // Se o tamanho total não for divisível por 4, o primeiro bloco pega os bits restantes
    if (remainder != 0) {
        int val = 0;
        for (int i = 0; i < remainder; i++) {
            val = (val << 1) | (mask[bit_idx++] & 1);
        }
        sprintf(&hex_out[hex_idx++], "%x", val);
    }

    // Processa o restante em blocos perfeitos de 4 bits
    while (bit_idx < hashShape) {
        int val = 0;
        for (int i = 0; i < 4; i++) {
            val = (val << 1) | (mask[bit_idx++] & 1);
        }
        sprintf(&hex_out[hex_idx++], "%x", val);
    }

    hex_out[num_hex_chars] = '\0';
    return hex_out;
}

void phash(Image* img, int hashSize, int highFreqFactor) {
    if (hashSize < 2) return;
    
    int imgSize = hashSize * highFreqFactor; 
    int hashShape = hashSize * hashSize; 

    Image* gs = grayscaleFilter(img, 1);
    exit(69);

    Image* bastard = resizeImg(gs, imgSize, imgSize, -1);

    saveImage(gs, "gs.jpg");
    saveImage(bastard, "bastard.jpg");

    imgFree(gs);
    
    // Aloca espaço para a imagem redimensionada em double (32x32)
    double* in = (double*) malloc(imgSize * imgSize * sizeof(double));
    double* outImg = (double*) malloc(imgSize * imgSize * sizeof(double));
    for(int i = 0; i < imgSize * imgSize; i++) {
        in[i] = (double)bastard->data[i];
    }

    imgFree(bastard);
    fftw_plan plan = fftw_plan_r2r_2d(imgSize, imgSize, in, outImg, FFTW_REDFT10, FFTW_REDFT10, FFTW_ESTIMATE);
    
    fftw_execute(plan);
    fftw_destroy_plan(plan);
    
    double* dctlowfreq = (double*) malloc(hashShape * sizeof(double));

    int index = 0;
    printf("DCT Low Freq (Top-Left %dx%d):\n", hashSize, hashSize);
    for(int i = 0; i < hashSize; i++) {
        printf("[ ");
        for(int j = 0; j < hashSize; j++) {
            int src_index = (i * imgSize) + j; 
            dctlowfreq[index] = outImg[src_index];
            printf(" %f ", dctlowfreq[index]);
            index++;
        }        
        printf(" ]\n");
    }



    double median = doubleMedian(dctlowfreq, hashShape); 
    printf("\nMedian: %f\n", median);

    printf("Binary Mask:\n[ ");
    char* binaryMask = (char*) malloc(sizeof(char) * hashShape);
    for(int i = 0; i < hashShape; i++) {
        binaryMask[i] = (dctlowfreq[i] > median) ? 1 : 0;
        printf(" %d ", binaryMask[i]);
    }
    printf(" ]\n");


    printf("Final String: %s\n", binaryArrayToHex(binaryMask, hashShape));
    free(in);
    free(outImg);
    free(dctlowfreq);
    free(binaryMask);
}

int main() {
    Image* img = loadImage("nsfw.png"); 

    phash(img, 8, 4);
}
