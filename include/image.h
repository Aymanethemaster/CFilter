#ifndef IMAGE_H
#define IMAGE_H

#include <stddef.h>

typedef struct {
    int width;
    int height;
    int channels;
    unsigned char *data;
} Image;

// Helper to determine number of color channels (excluding alpha)
static inline int get_color_channels(const Image *img) {
    return (img->channels == 2 || img->channels == 4) ? img->channels - 1 : img->channels;
}

// Clamp integer to unsigned char [0, 255]
static inline unsigned char clamp_u8(int val) {
    return (unsigned char)(val > 255 ? 255 : (val < 0 ? 0 : val));
}

// Clamp integer to range [min_val, max_val]
static inline int clamp_int(int val, int min_val, int max_val) {
    return val < min_val ? min_val : (val > max_val ? max_val : val);
}

// Load limits: guard against oversized / decompression-bomb images.
#define IMAGE_MAX_DIM    65535
#define IMAGE_MAX_PIXELS 268435456LL // 256 MP; ~= 1 GiB of RGBA data

// Image lifecycle & I/O
int load_image(const char *filename, Image *img);
int load_image_ex(const char *filename, Image *img, int max_dim, long long max_pixels);
int save_image(const char *filename, const Image *img);
void free_image(Image *img);

#endif // IMAGE_H
