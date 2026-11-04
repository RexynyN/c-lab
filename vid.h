#ifndef IMG_H
#define IMG_H 

#define PI 3.14159265358979323846

#define CHANNEL_IDX(img, x, y, chan) (((y * img->width + x) * img->channels) + chan)
#define PIXEL_IDX(img, x, y) ((y * img->width + x) * img->channels)
#define max(a, b) ((a) > (b) ? (a) : (b)) 
#define min(a, b) ((a) < (b) ? (a) : (b)) 
#define clip(v_min, value, v_max) max(v_min, min(v_max, value))
#define clamp(idx, max_size)  max(0, min((max_size) - 1, idx))
#define pixel_clip(value) clip(0, value, 255)

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

int str_endswith(const char* str, const char* suffix);
int saveImage(Image* img, char* filename);

double lanczosKernel(double x, int a);

Image* initImage(int width, int height, int channels, unsigned char *img_data); 
Image* loadImage(char* imgPath);


Image* resizeImg(Image* img, int width, int height, int kernel);
Image* sepiaFilter(Image* img);
Image* inverseFilter(Image* img);
Image* grayscaleFilter(Image* img);

#endif