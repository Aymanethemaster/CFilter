/* test_filters.c - unit, regression and CLI integration tests for CFilter.
 *
 * Usage: test_filters [fixture.bmp] [path-to-cfilter-binary]
 *   - fixture.bmp defaults to tests/fixture.bmp
 *   - when the cfilter binary path is given, CLI integration tests run too
 *     (negative cases must fail, valid rotate must swap dimensions)
 *
 * Exit code 0 = all checks passed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "../include/image.h"
#include "../include/filters.h"
#include "fixture_pixels.h"

static int checks = 0;
static int failures = 0;

#define CHECK(cond) do { \
    checks++; \
    if (!(cond)) { \
        failures++; \
        printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

static Image load_fresh(const char *path) {
    Image im = {0};
    if (!load_image(path, &im)) {
        fprintf(stderr, "FATAL: could not load fixture '%s'\n", path);
        exit(2);
    }
    return im;
}

static void check_px(const Image *im, int x, int y, int r, int g, int b, const char *what) {
    const unsigned char *p = im->data + ((size_t)y * (size_t)im->width + (size_t)x) * (size_t)im->channels;
    checks++;
    if ((int)p[0] != r || (int)p[1] != g || (int)p[2] != b) {
        failures++;
        printf("FAIL %s @(%d,%d): expected (%d,%d,%d), got (%d,%d,%d)\n",
               what, x, y, r, g, b, (int)p[0], (int)p[1], (int)p[2]);
    }
}

/* Expected grayscale value of every fixture pixel (row-major).
 * gray = (77*R + 150*G + 29*B) >> 8 */
static const unsigned char GRAY_EXPECTED[FIX_W * FIX_H] = {
    76, 149, 28, 255,
    18, 48, 78, 108,
    0, 128, 159, 150
};

static void test_grayscale(const char *f) {
    Image im = load_fresh(f);
    apply_grayscale(&im);
    for (int y = 0; y < FIX_H; y++)
        for (int x = 0; x < FIX_W; x++) {
            int g = GRAY_EXPECTED[y * FIX_W + x];
            check_px(&im, x, y, g, g, g, "grayscale");
        }
    free_image(&im);
}

static void test_invert(const char *f) {
    Image im = load_fresh(f);
    apply_invert(&im);
    check_px(&im, 0, 0, 0, 255, 255, "invert A");
    check_px(&im, 3, 0, 0, 0, 0, "invert D");
    check_px(&im, 0, 1, 245, 235, 225, "invert E");
    free_image(&im);
}

static void test_brightness(const char *f) {
    Image im = load_fresh(f);
    apply_brightness(&im, 30);
    check_px(&im, 0, 1, 40, 50, 60, "brightness+30 E");
    check_px(&im, 3, 0, 255, 255, 255, "brightness+30 clamps at 255");
    check_px(&im, 0, 2, 30, 30, 30, "brightness+30 I");
    free_image(&im);

    im = load_fresh(f);
    apply_brightness(&im, -30);
    check_px(&im, 0, 1, 0, 0, 0, "brightness-30 clamps at 0");
    check_px(&im, 1, 2, 98, 98, 98, "brightness-30 J");
    free_image(&im);
}

static void test_contrast(const char *f) {
    /* factor = 259*(205) / (255*309) = 0.67384; v' = f*(v-128)+128 (truncated) */
    Image im = load_fresh(f);
    apply_contrast(&im, -50);
    check_px(&im, 0, 0, 213, 41, 41, "contrast-50 A");
    check_px(&im, 3, 0, 213, 213, 213, "contrast-50 D");
    check_px(&im, 0, 2, 41, 41, 41, "contrast-50 I");
    check_px(&im, 1, 2, 128, 128, 128, "contrast-50 J (midpoint unchanged)");
    free_image(&im);
}

static void test_sepia(const char *f) {
    Image im = load_fresh(f);
    apply_sepia(&im);
    check_px(&im, 0, 0, 100, 88, 69, "sepia A (pure red)");
    check_px(&im, 3, 0, 255, 255, 238, "sepia D (white, blue clipped)");
    check_px(&im, 0, 2, 0, 0, 0, "sepia I (black stays black)");
    free_image(&im);
}

static void test_flips(const char *f) {
    Image im = load_fresh(f);
    apply_flip_horizontal(&im);
    check_px(&im, 0, 0, 255, 255, 255, "flip-h D -> (0,0)");
    check_px(&im, 3, 0, 255, 0, 0, "flip-h A -> (3,0)");
    check_px(&im, 0, 1, 100, 110, 120, "flip-h H -> (0,1)");
    free_image(&im);

    im = load_fresh(f);
    apply_flip_vertical(&im);
    check_px(&im, 0, 0, 0, 0, 0, "flip-v I -> (0,0)");
    check_px(&im, 0, 2, 255, 0, 0, "flip-v A -> (0,2)");
    check_px(&im, 1, 1, 40, 50, 60, "flip-v middle row intact");
    free_image(&im);
}

static void test_rotate(const char *f) {
    Image im = load_fresh(f);
    CHECK(apply_rotate(&im, 90) == 1);
    CHECK(im.width == FIX_H && im.height == FIX_W);
    check_px(&im, 0, 0, 0, 0, 0, "rot90 I -> (0,0)");
    check_px(&im, 2, 0, 255, 0, 0, "rot90 A -> (2,0)");
    check_px(&im, 0, 3, 5, 250, 25, "rot90 L -> (0,3)");
    check_px(&im, 2, 3, 255, 255, 255, "rot90 D -> (2,3)");
    free_image(&im);

    im = load_fresh(f);
    CHECK(apply_rotate(&im, 180) == 1);
    CHECK(im.width == FIX_W && im.height == FIX_H);
    check_px(&im, 0, 0, 5, 250, 25, "rot180 L -> (0,0)");
    check_px(&im, 3, 2, 255, 0, 0, "rot180 A -> (3,2)");
    free_image(&im);

    im = load_fresh(f);
    CHECK(apply_rotate(&im, 270) == 1);
    CHECK(im.width == FIX_H && im.height == FIX_W);
    check_px(&im, 0, 0, 255, 255, 255, "rot270 D -> (0,0)");
    check_px(&im, 2, 3, 0, 0, 0, "rot270 I -> (2,3)");
    free_image(&im);

    /* Invalid angle must be rejected and leave the image untouched. */
    im = load_fresh(f);
    CHECK(apply_rotate(&im, 45) == 0);
    CHECK(im.width == FIX_W && im.height == FIX_H);
    check_px(&im, 0, 0, 255, 0, 0, "rot45 leaves image untouched");
    free_image(&im);
}

static void test_convolutions(const char *f) {
    Image im = load_fresh(f);
    apply_blur(&im);
    check_px(&im, 0, 0, 120, 66, 13, "blur corner (edge-clamped)");
    check_px(&im, 1, 1, 78, 75, 73, "blur center");
    free_image(&im);

    im = load_fresh(f);
    apply_sharpen(&im);
    check_px(&im, 0, 0, 255, 0, 0, "sharpen corner");
    check_px(&im, 1, 1, 0, 0, 52, "sharpen center");
    free_image(&im);

    im = load_fresh(f);
    apply_edge(&im);
    check_px(&im, 1, 1, 0, 0, 0, "edge center");
    check_px(&im, 1, 2, 255, 255, 255, "edge at gray discontinuity");
    free_image(&im);
}

/* Filters must never touch the alpha channel (RGBA path). */
static void test_alpha_preserved(void) {
    unsigned char buf[8] = { 200, 100, 50, 77,   10, 20, 30, 200 };
    Image im = { 2, 1, 4, buf }; /* stack buffer: do NOT free_image() */

    apply_grayscale(&im);
    CHECK(buf[3] == 77 && buf[7] == 200);
    apply_invert(&im);
    CHECK(buf[3] == 77 && buf[7] == 200);
    apply_brightness(&im, 50);
    CHECK(buf[3] == 77 && buf[7] == 200);
    apply_contrast(&im, 50);
    CHECK(buf[3] == 77 && buf[7] == 200);
    apply_sepia(&im);
    CHECK(buf[3] == 77 && buf[7] == 200);
    apply_blur(&im);
    CHECK(buf[3] == 77 && buf[7] == 200);
}

/* load_image_ex dimension/pixel caps (decompression-bomb guard). */
static void test_load_limits(const char *f) {
    Image im = {0};
    CHECK(load_image_ex(f, &im, 2, 1000000) == 0); /* width 4 > limit 2 */
    CHECK(im.data == NULL);

    CHECK(load_image_ex(f, &im, 4, 12) == 1);      /* exactly at limits */
    free_image(&im);
    im.data = NULL; im.width = im.height = im.channels = 0;

    CHECK(load_image_ex(f, &im, 4, 11) == 0);      /* 12 pixels > 11 */
    CHECK(im.data == NULL);
}

/* Run the cfilter CLI and return its process exit code.
 * On Windows, system() routes through a shell that mis-handles quoted paths
 * containing spaces (this workspace lives under "...\Aiman Mokhtari\..."), so
 * we launch the binary directly with CreateProcess instead. */
static int cli_exit_code(const char *cli, const char *args) {
#ifdef _WIN32
    char cmdline[2048];
    snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", cli, args);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return (int)code;
#else
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "\"%s\" %s >/dev/null 2>&1", cli, args);
    return system(cmd);
#endif
}

static void test_cli(const char *cli, const char *fixture) {
    char args[1100];
    printf("Running CLI integration tests against '%s'...\n", cli);

    CHECK(cli_exit_code(cli, "missing-input.xyz tmp_out.png grayscale") != 0);

    snprintf(args, sizeof(args), "\"%s\" tmp_out.png bogus-op", fixture);
    CHECK(cli_exit_code(cli, args) != 0);

    snprintf(args, sizeof(args), "\"%s\" tmp_out.png rotate 45", fixture);
    CHECK(cli_exit_code(cli, args) != 0);

    snprintf(args, sizeof(args), "\"%s\" tmp_out.png brightness 9999", fixture);
    CHECK(cli_exit_code(cli, args) != 0);

    snprintf(args, sizeof(args), "\"%s\" tmp_out.png brightness abc", fixture);
    CHECK(cli_exit_code(cli, args) != 0);

    /* A valid rotate must succeed and swap dimensions. */
    snprintf(args, sizeof(args), "\"%s\" tmp_out.png rotate 90", fixture);
    CHECK(cli_exit_code(cli, args) == 0);
    Image im = {0};
    CHECK(load_image("tmp_out.png", &im) == 1);
    if (im.data) {
        CHECK(im.width == FIX_H && im.height == FIX_W);
        check_px(&im, 0, 0, 0, 0, 0, "CLI rot90 I -> (0,0)");
        free_image(&im);
    }
    remove("tmp_out.png");
}

int main(int argc, char **argv) {
    const char *fixture = (argc > 1) ? argv[1] : "tests/fixture.bmp";
    const char *cli = (argc > 2) ? argv[2] : NULL;

    /* Fixture sanity */
    Image base = load_fresh(fixture);
    CHECK(base.width == FIX_W && base.height == FIX_H);
    CHECK(base.channels == 3);
    check_px(&base, 0, 0, 255, 0, 0, "fixture top-left");
    check_px(&base, 3, 2, 5, 250, 25, "fixture bottom-right");
    free_image(&base);

    test_grayscale(fixture);
    test_invert(fixture);
    test_brightness(fixture);
    test_contrast(fixture);
    test_sepia(fixture);
    test_flips(fixture);
    test_rotate(fixture);
    test_convolutions(fixture);
    test_alpha_preserved();
    test_load_limits(fixture);
    if (cli) test_cli(cli, fixture);

    printf("%d checks, %d failure(s)\n", checks, failures);
    return (failures == 0) ? 0 : 1;
}


