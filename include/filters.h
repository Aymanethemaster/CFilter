#ifndef FILTERS_H
#define FILTERS_H

#include "image.h"

// Color & Lighting Filters
void apply_grayscale(Image *img);
void apply_invert(Image *img);
void apply_sepia(Image *img);
void apply_brightness(Image *img, int amount);
void apply_contrast(Image *img, int contrast);

// Spatial Transformations
void apply_flip_horizontal(Image *img);
void apply_flip_vertical(Image *img);
int  apply_rotate(Image *img, int degrees);

// Convolution Filters
void apply_blur(Image *img);
void apply_sharpen(Image *img);
void apply_edge(Image *img);

#endif // FILTERS_H
