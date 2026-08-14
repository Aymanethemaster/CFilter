#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

// Case-insensitive string suffix check
static int ends_with_case_insensitive(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;

    const char *p_str = str + (str_len - suffix_len);
    for (size_t i = 0; i < suffix_len; i++) {
        if (tolower((unsigned char)p_str[i]) != tolower((unsigned char)suffix[i])) {
            return 0;
        }
    }
    return 1;
}

// Load an image from disk (returns 1 on success, 0 on failure)
int load_image(const char *filename, Image *img) {
    if (!filename || !img) return 0;
    img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 0);
    if (!img->data) {
        fprintf(stderr, "Error loading image '%s': %s\n", filename, stbi_failure_reason());
        return 0;
    }
    return 1;
}

// Save image to disk with automatic format detection by extension
int save_image(const char *filename, const Image *img) {
    if (!filename || !img || !img->data) return 0;

    int result = 0;
    if (ends_with_case_insensitive(filename, ".jpg") || ends_with_case_insensitive(filename, ".jpeg")) {
        result = stbi_write_jpg(filename, img->width, img->height, img->channels, img->data, 90);
    } else if (ends_with_case_insensitive(filename, ".bmp")) {
        result = stbi_write_bmp(filename, img->width, img->height, img->channels, img->data);
    } else if (ends_with_case_insensitive(filename, ".tga")) {
        result = stbi_write_tga(filename, img->width, img->height, img->channels, img->data);
    } else {
        // Default to PNG (covers .png and unspecified extensions)
        result = stbi_write_png(filename, img->width, img->height, img->channels, img->data, img->width * img->channels);
    }

    if (!result) {
        fprintf(stderr, "Error saving image to '%s'\n", filename);
        return 0;
    }
    return 1;
}

// Free image memory
void free_image(Image *img) {
    if (img && img->data) {
        stbi_image_free(img->data);
        img->data = NULL;
    }
}
