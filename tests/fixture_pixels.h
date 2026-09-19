#ifndef FIXTURE_PIXELS_H
#define FIXTURE_PIXELS_H

/* Deterministic 4x3 RGB test fixture shared by generate_fixture.c and
 * test_filters.c. Row 0 is the TOP row (canonical stb_image orientation).
 * Values are chosen so every filter produces a distinct, exactly
 * predictable result. */
#define FIX_W 4
#define FIX_H 3
#define FIX_CH 3

static const unsigned char FIX_PIXELS[FIX_W * FIX_H * FIX_CH] = {
    /* y=0:  A red          B green        C blue         D white */
    255, 0, 0,     0, 255, 0,     0, 0, 255,     255, 255, 255,
    /* y=1:  E..H dark ramps */
    10, 20, 30,    40, 50, 60,    70, 80, 90,    100, 110, 120,
    /* y=2:  I black  J mid-gray  K warm        L high-g */
    0, 0, 0,       128, 128, 128, 200, 150, 100, 5, 250, 25
};

#endif // FIXTURE_PIXELS_H
