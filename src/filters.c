#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/filters.h"

// 1. Grayscale Filter (Fixed-point perceptual luminance: 0.299R + 0.587G + 0.114B)
void apply_grayscale(Image *img) {
    if (!img || !img->data || img->channels < 3) return;

    int num_pixels = img->width * img->height;
    for (int i = 0; i < num_pixels; i++) {
        unsigned char *px = &img->data[i * img->channels];
        unsigned char r = px[0];
        unsigned char g = px[1];
        unsigned char b = px[2];

        // Integer approximation: (R*77 + G*150 + B*29) >> 8
        unsigned char gray = (unsigned char)((r * 77 + g * 150 + b * 29) >> 8);

        px[0] = gray;
        px[1] = gray;
        px[2] = gray;
        // Alpha channel is preserved
    }
}

// 2. Invert Filter (Negative) - Safe for RGBA and Gray+Alpha
void apply_invert(Image *img) {
    if (!img || !img->data) return;

    int color_ch = get_color_channels(img);
    int num_pixels = img->width * img->height;

    for (int i = 0; i < num_pixels; i++) {
        unsigned char *px = &img->data[i * img->channels];
        for (int c = 0; c < color_ch; c++) {
            px[c] = 255 - px[c];
        }
    }
}

// 3. Brightness Adjustment (-255 to +255)
void apply_brightness(Image *img, int amount) {
    if (!img || !img->data) return;

    int color_ch = get_color_channels(img);
    int num_pixels = img->width * img->height;

    for (int i = 0; i < num_pixels; i++) {
        unsigned char *px = &img->data[i * img->channels];
        for (int c = 0; c < color_ch; c++) {
            px[c] = clamp_u8((int)px[c] + amount);
        }
    }
}

// 4. Contrast Adjustment (-255 to +255)
void apply_contrast(Image *img, int contrast) {
    if (!img || !img->data) return;

    contrast = clamp_int(contrast, -255, 255);
    float factor = (259.0f * (contrast + 255.0f)) / (255.0f * (259.0f - contrast));

    int color_ch = get_color_channels(img);
    int num_pixels = img->width * img->height;

    for (int i = 0; i < num_pixels; i++) {
        unsigned char *px = &img->data[i * img->channels];
        for (int c = 0; c < color_ch; c++) {
            int new_val = (int)(factor * ((int)px[c] - 128) + 128);
            px[c] = clamp_u8(new_val);
        }
    }
}

// 5. Sepia Filter
void apply_sepia(Image *img) {
    if (!img || !img->data || img->channels < 3) return;

    int num_pixels = img->width * img->height;
    for (int i = 0; i < num_pixels; i++) {
        unsigned char *px = &img->data[i * img->channels];
        int r = px[0];
        int g = px[1];
        int b = px[2];

        int tr = (int)(0.393f * r + 0.769f * g + 0.189f * b);
        int tg = (int)(0.349f * r + 0.686f * g + 0.168f * b);
        int tb = (int)(0.272f * r + 0.534f * g + 0.131f * b);

        px[0] = clamp_u8(tr);
        px[1] = clamp_u8(tg);
        px[2] = clamp_u8(tb);
    }
}

// 6. Horizontal Flip (Mirror)
void apply_flip_horizontal(Image *img) {
    if (!img || !img->data) return;

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width / 2; x++) {
            int left_idx = (y * img->width + x) * img->channels;
            int right_idx = (y * img->width + (img->width - 1 - x)) * img->channels;

            for (int c = 0; c < img->channels; c++) {
                unsigned char temp = img->data[left_idx + c];
                img->data[left_idx + c] = img->data[right_idx + c];
                img->data[right_idx + c] = temp;
            }
        }
    }
}

// 7. Vertical Flip
void apply_flip_vertical(Image *img) {
    if (!img || !img->data) return;

    int row_bytes = img->width * img->channels;
    unsigned char *temp_row = (unsigned char *)malloc(row_bytes);
    if (!temp_row) {
        fprintf(stderr, "Error: Memory allocation failed for vertical flip.\n");
        return;
    }

    for (int y = 0; y < img->height / 2; y++) {
        int top_offset = y * row_bytes;
        int bottom_offset = (img->height - 1 - y) * row_bytes;

        memcpy(temp_row, &img->data[top_offset], row_bytes);
        memcpy(&img->data[top_offset], &img->data[bottom_offset], row_bytes);
        memcpy(&img->data[bottom_offset], temp_row, row_bytes);
    }

    free(temp_row);
}

// 8. Rotate Clockwise (90, 180, 270 degrees)
int apply_rotate(Image *img, int degrees) {
    if (!img || !img->data) return 0;

    degrees = (degrees % 360 + 360) % 360;
    if (degrees == 0) return 1;

    if (degrees != 90 && degrees != 180 && degrees != 270) {
        fprintf(stderr, "Error: Rotation must be 90, 180, or 270 degrees.\n");
        return 0;
    }

    int new_width = (degrees == 180) ? img->width : img->height;
    int new_height = (degrees == 180) ? img->height : img->width;
    size_t total_bytes = (size_t)new_width * new_height * img->channels;

    unsigned char *new_data = (unsigned char *)malloc(total_bytes);
    if (!new_data) {
        fprintf(stderr, "Error: Memory allocation failed for image rotation.\n");
        return 0;
    }

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int src_idx = (y * img->width + x) * img->channels;
            int dest_x = 0;
            int dest_y = 0;

            if (degrees == 90) {
                dest_x = img->height - 1 - y;
                dest_y = x;
            } else if (degrees == 180) {
                dest_x = img->width - 1 - x;
                dest_y = img->height - 1 - y;
            } else if (degrees == 270) {
                dest_x = y;
                dest_y = img->width - 1 - x;
            }

            int dest_idx = (dest_y * new_width + dest_x) * img->channels;
            memcpy(&new_data[dest_idx], &img->data[src_idx], img->channels);
        }
    }

    free(img->data);
    img->data = new_data;
    img->width = new_width;
    img->height = new_height;
    return 1;
}

// 9. Generic 3x3 Convolution Helper (with clamped border handling)
static void apply_convolution_3x3(Image *img, const float kernel[3][3], float factor, float bias) {
    if (!img || !img->data) return;

    size_t size = (size_t)img->width * img->height * img->channels;
    unsigned char *copy = (unsigned char *)malloc(size);
    if (!copy) {
        fprintf(stderr, "Error: Memory allocation failed for convolution operation.\n");
        return;
    }
    memcpy(copy, img->data, size);

    int color_ch = get_color_channels(img);

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < color_ch; c++) {
                float sum = 0.0f;
                for (int ky = -1; ky <= 1; ky++) {
                    int py = clamp_int(y + ky, 0, img->height - 1);
                    for (int kx = -1; kx <= 1; kx++) {
                        int px_coord = clamp_int(x + kx, 0, img->width - 1);
                        int idx = (py * img->width + px_coord) * img->channels + c;
                        sum += copy[idx] * kernel[ky + 1][kx + 1];
                    }
                }
                int dest_idx = (y * img->width + x) * img->channels + c;
                int final_val = (int)(sum * factor + bias);
                img->data[dest_idx] = clamp_u8(final_val);
            }
        }
    }

    free(copy);
}

// 10. Box Blur (Edge-clamped 3x3 kernel)
void apply_blur(Image *img) {
    const float box_kernel[3][3] = {
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f}
    };
    apply_convolution_3x3(img, box_kernel, 1.0f / 9.0f, 0.0f);
}

// 11. Sharpen Filter
void apply_sharpen(Image *img) {
    const float sharpen_kernel[3][3] = {
        { 0.0f, -1.0f,  0.0f},
        {-1.0f,  5.0f, -1.0f},
        { 0.0f, -1.0f,  0.0f}
    };
    apply_convolution_3x3(img, sharpen_kernel, 1.0f, 0.0f);
}

// 12. Edge Detection Filter (Laplacian)
void apply_edge(Image *img) {
    const float edge_kernel[3][3] = {
        {-1.0f, -1.0f, -1.0f},
        {-1.0f,  8.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f}
    };
    apply_convolution_3x3(img, edge_kernel, 1.0f, 0.0f);
}
