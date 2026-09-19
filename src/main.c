#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "../include/image.h"
#include "../include/filters.h"

#define MAX_PATH_LEN 1024

// Cross-platform screen clearing
static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Trim leading and trailing whitespace and surrounding quotes (from drag-and-drop)
static void sanitize_path(char *str) {
    if (!str) return;

    // Remove trailing newline / carriage return
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r' || isspace((unsigned char)str[len - 1]))) {
        str[--len] = '\0';
    }

    // Skip leading whitespace
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    // Remove surrounding double or single quotes
    if ((*start == '"' || *start == '\'') && len > 1) {
        char quote = *start;
        if (str[len - 1] == quote) {
            str[len - 1] = '\0';
            start++;
        }
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// Safely read a line from standard input
static int get_user_input(char *buffer, size_t size) {
    if (!fgets(buffer, (int)size, stdin)) {
        return 0;
    }
    sanitize_path(buffer);
    return 1;
}

static void print_banner(void) {
    printf("========================================================\n");
    printf("                 CFILTER - IMAGE STUDIO                 \n");
    printf("========================================================\n");
}

// Parse a base-10 integer strictly: the whole string must be a valid number.
// Returns 1 and stores the value in *out on success, 0 otherwise.
static int parse_int_strict(const char *s, int *out) {
    if (!s || !out) return 0;
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return 0;
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (s == end || errno == ERANGE || v < -2147483647L - 1 || v > 2147483647L) return 0;
    while (isspace((unsigned char)*end)) end++;
    if (*end != '\0') return 0;
    *out = (int)v;
    return 1;
}

// Append a tag to the filter history chain ("A -> B -> C").
static void history_append(char *history, size_t hist_size, const char *tag) {
    if (!history || !tag || hist_size == 0) return;
    size_t used = strlen(history);
    if (used > 0) {
        strncat(history, " -> ", hist_size - used - 1);
        used = strlen(history);
    }
    strncat(history, tag, hist_size - used - 1);
}

static void print_status_header(const char *filename, const Image *img, const char *history, const char *status_msg) {
    print_banner();

    const char *basename = strrchr(filename, '/');
    if (!basename) basename = strrchr(filename, '\\');
    basename = (basename) ? basename + 1 : filename;

    const char *ch_desc = "Unknown";
    if (img->channels == 1) ch_desc = "Grayscale";
    else if (img->channels == 2) ch_desc = "Grayscale + Alpha";
    else if (img->channels == 3) ch_desc = "RGB Color";
    else if (img->channels == 4) ch_desc = "RGBA (Transparency)";

    printf(" [File: %s | %dx%d | %s]\n", basename, img->width, img->height, ch_desc);
    printf(" [Filters: %s]\n", (history && strlen(history) > 0) ? history : "None");
    printf("========================================================\n");

    if (status_msg && strlen(status_msg) > 0) {
        printf("\n >> %s\n", status_msg);
    }
}

static void print_filter_menu(void) {
    printf("\n--------------------------------------------------------\n");
    printf("                 SELECT A FILTER / ACTION               \n");
    printf("--------------------------------------------------------\n");
    printf("  [1]  Grayscale          (Black & White perceptual luminance)\n");
    printf("  [2]  Invert Colors      (Photo negative)\n");
    printf("  [3]  Sepia Tone         (Warm vintage photographic effect)\n");
    printf("  [4]  Flip Horizontal    (Mirror left-to-right)\n");
    printf("  [5]  Flip Vertical      (Upside down)\n");
    printf("  [6]  Rotate             (90, 180, or 270 degrees clockwise)\n");
    printf("  [7]  Brightness         (Adjust brightness -255 to +255)\n");
    printf("  [8]  Contrast           (Adjust dynamic contrast -255 to +255)\n");
    printf("  [9]  Box Blur           (Smooth 3x3 box blur)\n");
    printf("  [10] Sharpen            (Enhance detail and clarity)\n");
    printf("  [11] Edge Detection     (Laplacian outline extraction)\n");
    printf("--------------------------------------------------------\n");
    printf("  [0]  Export Image & Save\n");
    printf("--------------------------------------------------------\n");
    printf("Enter choice (0-11): ");
}

static int apply_chosen_filter(Image *img, int choice, char *history, size_t hist_size, char *status_msg, size_t stat_size) {
    char input_buf[128];

    switch (choice) {
        case 1:
            apply_grayscale(img);
            snprintf(status_msg, stat_size, "Applied Grayscale filter.");
            history_append(history, hist_size, "Grayscale");
            break;
        case 2:
            apply_invert(img);
            snprintf(status_msg, stat_size, "Applied Invert Colors.");
            history_append(history, hist_size, "Invert");
            break;
        case 3:
            apply_sepia(img);
            snprintf(status_msg, stat_size, "Applied Sepia Tone.");
            history_append(history, hist_size, "Sepia");
            break;
        case 4:
            apply_flip_horizontal(img);
            snprintf(status_msg, stat_size, "Applied Horizontal Flip.");
            history_append(history, hist_size, "Flip-H");
            break;
        case 5:
            apply_flip_vertical(img);
            snprintf(status_msg, stat_size, "Applied Vertical Flip.");
            history_append(history, hist_size, "Flip-V");
            break;
        case 6: {
            printf("Enter rotation angle (90, 180, or 270): ");
            if (!get_user_input(input_buf, sizeof(input_buf))) return 0;
            int deg = 0;
            if (!parse_int_strict(input_buf, &deg)) {
                snprintf(status_msg, stat_size, "Invalid rotation angle '%s' (Must be 90, 180, or 270).", input_buf);
                return 0;
            }
            if (deg != 90 && deg != 180 && deg != 270) {
                snprintf(status_msg, stat_size, "Invalid rotation angle '%s' (Must be 90, 180, or 270).", input_buf);
                return 0;
            }
            if (apply_rotate(img, deg)) {
                snprintf(status_msg, stat_size, "Rotated %d degrees clockwise (%dx%d).", deg, img->width, img->height);
                char tag[64];
                snprintf(tag, sizeof(tag), "Rotate(%d deg)", deg);
                history_append(history, hist_size, tag);
            }
            break;
        }
        case 7: {
            printf("Enter brightness change (-255 to +255, e.g. 50 or -30): ");
            if (!get_user_input(input_buf, sizeof(input_buf))) return 0;
            int amount = 0;
            if (!parse_int_strict(input_buf, &amount)) {
                snprintf(status_msg, stat_size, "Invalid brightness value '%s'.", input_buf);
                return 0;
            }
            if (amount < -255 || amount > 255) {
                snprintf(status_msg, stat_size, "Brightness value must be between -255 and +255.");
                return 0;
            }
            apply_brightness(img, amount);
            snprintf(status_msg, stat_size, "Applied Brightness (%+d).", amount);
            char tag[64];
            snprintf(tag, sizeof(tag), "Brightness(%+d)", amount);
            history_append(history, hist_size, tag);
            break;
        }
        case 8: {
            printf("Enter contrast change (-255 to +255, e.g. 40 or -20): ");
            if (!get_user_input(input_buf, sizeof(input_buf))) return 0;
            int amount = 0;
            if (!parse_int_strict(input_buf, &amount)) {
                snprintf(status_msg, stat_size, "Invalid contrast value '%s'.", input_buf);
                return 0;
            }
            if (amount < -255 || amount > 255) {
                snprintf(status_msg, stat_size, "Contrast value must be between -255 and +255.");
                return 0;
            }
            apply_contrast(img, amount);
            snprintf(status_msg, stat_size, "Applied Contrast (%+d).", amount);
            char tag[64];
            snprintf(tag, sizeof(tag), "Contrast(%+d)", amount);
            history_append(history, hist_size, tag);
            break;
        }
        case 9:
            apply_blur(img);
            snprintf(status_msg, stat_size, "Applied Box Blur.");
            history_append(history, hist_size, "Blur");
            break;
        case 10:
            apply_sharpen(img);
            snprintf(status_msg, stat_size, "Applied Sharpen filter.");
            history_append(history, hist_size, "Sharpen");
            break;
        case 11:
            apply_edge(img);
            snprintf(status_msg, stat_size, "Applied Edge Detection filter.");
            history_append(history, hist_size, "Edge");
            break;
        default:
            snprintf(status_msg, stat_size, "Invalid choice! Please select 0 to 11.");
            return 0;
    }
    return 1;
}

static void export_image_interactive(const char *filename, const Image *img, const char *history) {
    char input_buf[MAX_PATH_LEN];
    char output_path[MAX_PATH_LEN];
    const char *default_ext = ".png";

    clear_screen();
    print_status_header(filename, img, history, "");

    printf("\n--------------------------------------------------------\n");
    printf("                 SELECT EXPORT FORMAT                   \n");
    printf("--------------------------------------------------------\n");
    printf("  [1] PNG   (.png) - Lossless, recommended for graphics\n");
    printf("  [2] JPEG  (.jpg) - Compressed photo format\n");
    printf("  [3] BMP   (.bmp) - Uncompressed standard bitmap\n");
    printf("  [4] TGA   (.tga) - Truevision Targa format\n");
    printf("--------------------------------------------------------\n");
    printf("Enter format choice (1-4, default=1 PNG): ");

    int format_choice = 1;
    if (get_user_input(input_buf, sizeof(input_buf)) && strlen(input_buf) > 0) {
        format_choice = atoi(input_buf);
    }

    switch (format_choice) {
        case 2: default_ext = ".jpg"; break;
        case 3: default_ext = ".bmp"; break;
        case 4: default_ext = ".tga"; break;
        case 1:
        default:
            default_ext = ".png";
            break;
    }

    printf("\nEnter output filename (Press Enter for 'output%s'): ", default_ext);
    if (get_user_input(input_buf, sizeof(input_buf)) && strlen(input_buf) > 0) {
        strncpy(output_path, input_buf, sizeof(output_path) - 1);
        output_path[sizeof(output_path) - 1] = '\0';

        // Append extension if user didn't provide one
        if (strrchr(output_path, '.') == NULL) {
            strncat(output_path, default_ext, sizeof(output_path) - strlen(output_path) - 1);
        }
    } else {
        snprintf(output_path, sizeof(output_path), "output%s", default_ext);
    }

    clear_screen();
    print_banner();
    printf("\nSaving image to '%s'...\n", output_path);
    if (save_image(output_path, img)) {
        printf("\n========================================================\n");
        printf(" SUCCESS: Image successfully exported!\n");
        printf(" Output file: %s\n", output_path);
        printf(" Dimensions:  %d x %d pixels\n", img->width, img->height);
        printf("========================================================\n");
    } else {
        printf("\n========================================================\n");
        printf(" FAILED: Could not save image to '%s'.\n", output_path);
        printf("========================================================\n");
    }
}

// Batch / CLI fallback mode
static int run_cli_mode(int argc, char *argv[]) {
    const char *input_path = argv[1];
    const char *output_path = argv[2];
    const char *operation = argv[3];

    Image img = {0};
    if (!load_image(input_path, &img)) {
        return 1;
    }

    if (strcmp(operation, "grayscale") == 0) apply_grayscale(&img);
    else if (strcmp(operation, "invert") == 0) apply_invert(&img);
    else if (strcmp(operation, "sepia") == 0) apply_sepia(&img);
    else if (strcmp(operation, "flip") == 0 || strcmp(operation, "flip-h") == 0) apply_flip_horizontal(&img);
    else if (strcmp(operation, "flip-v") == 0) apply_flip_vertical(&img);
    else if (strcmp(operation, "rotate") == 0) {
        int deg = 90;
        if (argc >= 5 && !parse_int_strict(argv[4], &deg)) {
            fprintf(stderr, "Invalid rotation angle '%s' (must be 90, 180, or 270).\n", argv[4]);
            free_image(&img);
            return 1;
        }
        if (deg != 90 && deg != 180 && deg != 270) {
            fprintf(stderr, "Invalid rotation angle %d (must be 90, 180, or 270).\n", deg);
            free_image(&img);
            return 1;
        }
        if (!apply_rotate(&img, deg)) {
            free_image(&img);
            return 1;
        }
    } else if (strcmp(operation, "brightness") == 0 || strcmp(operation, "contrast") == 0) {
        int amt = 0;
        if (argc >= 5 && !parse_int_strict(argv[4], &amt)) {
            fprintf(stderr, "Invalid %s value '%s' (must be an integer from -255 to 255).\n",
                    operation, argv[4]);
            free_image(&img);
            return 1;
        }
        if (amt < -255 || amt > 255) {
            fprintf(stderr, "Invalid %s value %d (must be between -255 and +255).\n", operation, amt);
            free_image(&img);
            return 1;
        }
        if (operation[0] == 'b') apply_brightness(&img, amt);
        else apply_contrast(&img, amt);
    } else if (strcmp(operation, "blur") == 0) apply_blur(&img);
    else if (strcmp(operation, "sharpen") == 0) apply_sharpen(&img);
    else if (strcmp(operation, "edge") == 0) apply_edge(&img);
    else {
        fprintf(stderr, "Unknown operation: %s\n", operation);
        free_image(&img);
        return 1;
    }

    if (!save_image(output_path, &img)) {
        free_image(&img);
        return 1;
    }
    free_image(&img);
    return 0;
}

int main(int argc, char *argv[]) {
    // If command-line arguments are provided, run in non-interactive batch mode
    if (argc >= 4) {
        return run_cli_mode(argc, argv);
    }

    char input_path[MAX_PATH_LEN];
    char user_choice_buf[64];
    char history[512] = {0};
    char status_msg[MAX_PATH_LEN + 128] = {0};

    while (1) {
        clear_screen();
        print_banner();

        if (strlen(status_msg) > 0) {
            printf("\n %s\n", status_msg);
            status_msg[0] = '\0';
        }

        printf("\nUpload Image: Enter image file path (or drag & drop here, 'q' to quit):\n> ");
        if (!get_user_input(input_path, sizeof(input_path))) {
            break;
        }

        if (strcmp(input_path, "q") == 0 || strcmp(input_path, "Q") == 0 || strcmp(input_path, "exit") == 0) {
            clear_screen();
            print_banner();
            printf("\nThank you for using CFilter. Goodbye!\n\n");
            break;
        }

        if (strlen(input_path) == 0) {
            continue;
        }

        Image img = {0};
        if (!load_image(input_path, &img)) {
            snprintf(status_msg, sizeof(status_msg), "Error: Failed to load '%s'. Please try again.", input_path);
            continue;
        }

        history[0] = '\0';
        status_msg[0] = '\0';

        // Filter application loop with clean frame refresh
        while (1) {
            clear_screen();
            print_status_header(input_path, &img, history, status_msg);
            status_msg[0] = '\0';

            print_filter_menu();
            if (!get_user_input(user_choice_buf, sizeof(user_choice_buf))) {
                break;
            }

            if (strlen(user_choice_buf) == 0) {
                continue;
            }

            int choice = atoi(user_choice_buf);
            if (choice == 0) {
                break;
            }

            apply_chosen_filter(&img, choice, history, sizeof(history), status_msg, sizeof(status_msg));
        }

        // Export screen
        export_image_interactive(input_path, &img, history);
        free_image(&img);

        // Edit another image prompt
        printf("Would you like to edit another image? (y/n, default=y): ");
        if (get_user_input(user_choice_buf, sizeof(user_choice_buf))) {
            if (user_choice_buf[0] == 'n' || user_choice_buf[0] == 'N') {
                clear_screen();
                print_banner();
                printf("\nThank you for using CFilter. Goodbye!\n\n");
                break;
            }
        }
    }

    return 0;
}
