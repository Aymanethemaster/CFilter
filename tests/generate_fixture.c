/* generate_fixture.c - builds tests/fixture.bmp from fixture_pixels.h.
 *
 * Only built by `make test` / CI; not part of the shipped application.
 * The generator self-verifies the written file by loading it back with
 * stb_image; if the BMP round-trips flipped, it rewrites a pre-flipped
 * copy so the on-disk file always reads back exactly as FIX_PIXELS.
 *
 * Usage: generate_fixture [output-path]   (default: tests/fixture.bmp)
 */
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#include "fixture_pixels.h"

static int write_and_verify(const char *path, const unsigned char *pixels) {
    if (!stbi_write_bmp(path, FIX_W, FIX_H, FIX_CH, pixels)) {
        fprintf(stderr, "failed to write %s\n", path);
        return 0;
    }
    int w = 0, h = 0, ch = 0;
    unsigned char *back = stbi_load(path, &w, &h, &ch, FIX_CH);
    if (!back) {
        fprintf(stderr, "failed to re-read %s: %s\n", path, stbi_failure_reason());
        return 0;
    }
    int ok = (w == FIX_W && h == FIX_H && memcmp(back, FIX_PIXELS, sizeof(FIX_PIXELS)) == 0);
    stbi_image_free(back);
    return ok;
}

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "tests/fixture.bmp";

    if (write_and_verify(path, (const unsigned char *)FIX_PIXELS)) {
        printf("generated %s (%dx%d)\n", path, FIX_W, FIX_H);
        return 0;
    }

    /* Round-trip came back flipped: write a pre-flipped copy instead. */
    unsigned char flipped[sizeof(FIX_PIXELS)];
    const size_t row_bytes = (size_t)FIX_W * FIX_CH;
    for (int y = 0; y < FIX_H; y++) {
        memcpy(flipped + (size_t)y * row_bytes,
               FIX_PIXELS + (size_t)(FIX_H - 1 - y) * row_bytes, row_bytes);
    }
    if (write_and_verify(path, flipped)) {
        printf("generated %s (%dx%d, flipped)\n", path, FIX_W, FIX_H);
        return 0;
    }

    fprintf(stderr, "could not produce a self-consistent fixture at %s\n", path);
    return 1;
}
