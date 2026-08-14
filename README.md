# CFilter - Interactive Image Studio

[![C99](https://img.shields.io/badge/Language-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)]()
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()
[![GitHub Releases](https://img.shields.io/github/v/release/Aymanethemaster/CFilter?include_prereleases&color=brightgreen&label=Release)](https://github.com/Aymanethemaster/CFilter/releases)

A fast, lightweight, and interactive command-line image processing studio written in pure C99. It features an intuitive terminal menu, drag-and-drop file loading, filter chaining, and multi-format export (**PNG, JPEG, BMP, TGA**) with zero external runtime dependencies.

---

## About The Project

**CFilter** is an open-source, minimalist image processor built for developers, students, and command-line enthusiasts who want fast, bloat-free image manipulations without the overhead of heavy desktop software.

### Why CFilter?
* **Zero Runtime Dependencies**: Unlike Python or Electron-based tools, CFilter is compiled into a single, standalone native binary (~350 KB) that runs instantly without installing runtimes, packages, or dynamic link libraries (DLLs).
* **Minimalist Ergonomics**: Operates right inside your terminal with clean frame-by-frame screen refreshes, drag-and-drop file inputs, and interactive filter chaining.
* **Direct Hardware-Level Performance**: Written in pure C99 with `-O3` compiler optimization, direct contiguous memory strides, fixed-point bit-shift math, and edge-clamped $3\times3$ spatial convolution kernels.
* **Format Flexibility**: Built-in support for reading and exporting to any popular raster format (**PNG, JPEG, BMP, TGA**) with automatic extension detection and full alpha channel protection.

---

## Download Pre-Built Executables (Releases)

You do **not** need to install C compilers or build tools to use this application! Ready-to-run standalone binaries are provided for every major platform:

**[Download the Latest Release on GitHub](https://github.com/Aymanethemaster/CFilter/releases/latest)**

| Platform | Binary Name | How to Run |
| :--- | :--- | :--- |
| **Windows (x64)** | `cfilter-windows-x64.exe` | Double-click or run `.\cfilter-windows-x64.exe` in PowerShell / CMD |
| **Linux (x64)** | `cfilter-linux-x64` | `chmod +x cfilter-linux-x64 && ./cfilter-linux-x64` |
| **macOS (Apple Silicon / Intel)** | `cfilter-macos-arm64` | `chmod +x cfilter-macos-arm64 && ./cfilter-macos-arm64` |

---

## Features

- **Simple Interactive Menu**: Screen-by-screen frame navigation with drag-and-drop file loading.
- **Filter Chaining**: Apply multiple filters sequentially (e.g. Sepia + Sharpen + Brightness) before exporting.
- **Ultra Fast Performance**: Pure C with `-O3` vectorization — processes 4K images in milliseconds.
- **Multi-Format Export**: Choose to export to `.png`, `.jpg`, `.bmp`, or `.tga` on the fly.
- **Safe Alpha Handling**: Preserves transparency for RGBA (4-channel) and Grayscale+Alpha (2-channel) images.
- **Zero Dependencies**: Built using lightweight single-header `stb_image` and `stb_image_write`.

---

## Available Filters & Transformations

| # | Action | Description |
| :---: | :--- | :--- |
| **1** | `Grayscale` | Converts image to perceptual black & white luminance ($0.299R + 0.587G + 0.114B$) |
| **2** | `Invert Colors` | Inverts color channels (photo negative effect) |
| **3** | `Sepia Tone` | Applies warm vintage photographic sepia tone |
| **4** | `Flip Horizontal` | Mirrors image left-to-right |
| **5** | `Flip Vertical` | Flips image upside down |
| **6** | `Rotate` | Rotates clockwise by 90°, 180°, or 270° |
| **7** | `Brightness` | Adjusts brightness level ($-255$ to $+255$) |
| **8** | `Contrast` | Scales dynamic contrast ($-255$ to $+255$) |
| **9** | `Box Blur` | Smooth $3\times3$ box blur filter with seamless edge-clamping |
| **10** | `Sharpen` | $3\times3$ unsharp mask sharpening filter |
| **11** | `Edge Detection` | Laplacian outline extraction filter |

---

## How to Use

Launch the application:

```bash
./cfilter.exe
```

### Interactive Flow:

1. **Upload**: Drag and drop your image file into the terminal (or type the path) and press `Enter`.
2. **Choose Filter**: Pick any filter (`1` to `11`).
3. **Chain Filters**: Apply additional filters or choose `0` to proceed to export.
4. **Select Format**: Pick your preferred output format (`PNG`, `JPEG`, `BMP`, `TGA`).
5. **Name File**: Press `Enter` for default filename (`output.png`) or type a custom name.
6. **Edit More**: Easily edit another image without restarting!

---

## Under the Hood: Architecture & Internal Mechanics

Understanding how CFilter represents and processes images in memory:

### 1. In-Memory Image Structure

Images are represented as a contiguous 1D array of 8-bit unsigned bytes (`unsigned char`):

```c
typedef struct {
    int width;            // Image width in pixels
    int height;           // Image height in pixels
    int channels;         // Number of 8-bit components per pixel (1 to 4)
    unsigned char *data;  // Heap-allocated pixel buffer of size (width * height * channels)
} Image;
```

#### Pixel Memory Layout:
For an image of dimension $W \times H$ with $C$ channels, pixel $(x, y)$ is accessed via 1D indexing:

$$\text{index}(x, y, c) = (y \times W + x) \times C + c$$

* **RGB (3 channels)**: `[R0, G0, B0, R1, G1, B1, ...]`
* **RGBA (4 channels)**: `[R0, G0, B0, A0, R1, G1, B1, A1, ...]`
* **Grayscale (1 channel)**: `[Y0, Y1, Y2, ...]`
* **Grayscale + Alpha (2 channels)**: `[Y0, A0, Y1, A1, ...]`

### 2. Alpha Channel Transparency Preservation

To prevent modifying opacity when adjusting colors, the helper function `get_color_channels()` isolates color components:

```c
static inline int get_color_channels(const Image *img) {
    return (img->channels == 2 || img->channels == 4) ? img->channels - 1 : img->channels;
}
```

This guarantees the alpha byte (channel index 3 in RGBA or 1 in Gray+Alpha) remains untouched across all filters.

### 3. Algorithm Implementations

* **Grayscale (Fixed-Point Luminance)**:
  Instead of expensive floating-point arithmetic ($0.299R + 0.587G + 0.114B$), CFilter uses fast integer bit-shifts:
  $$\text{gray} = (R \times 77 + G \times 150 + B \times 29) \gg 8$$

* **Contrast Scaling**:
  Uses standard digital image contrast enhancement:
  $$F = \frac{259 \times (\text{contrast} + 255)}{255 \times (259 - \text{contrast})}, \quad V_{\text{out}} = \text{clamp}(F \times (V_{\text{in}} - 128) + 128)$$

* **$3\times3$ Spatial Convolutions with Edge Clamping**:
  Filters like `blur`, `sharpen`, and `edge` pass a $3\times3$ kernel matrix over the image. Boundary pixels are clamped rather than skipped, preventing dark or unblurred border frames:
  $$x_{\text{clamped}} = \max(0, \min(x + k_x, W - 1))$$
  $$y_{\text{clamped}} = \max(0, \min(y + k_y, H - 1))$$

---

## Developer Guide: How to Extend and Add New Filters

Extending CFilter with new filters or transformations is simple and modular. Follow this 4-step tutorial:

### Step 1: Declare the Function Prototype

Open [`include/filters.h`](include/filters.h) and add your new function signature:

```c
// In include/filters.h
void apply_tint(Image *img, int r_offset, int g_offset, int b_offset);
```

---

### Step 2: Implement the Algorithm

Open [`src/filters.c`](src/filters.c) and implement the function:

```c
// In src/filters.c
void apply_tint(Image *img, int r_offset, int g_offset, int b_offset) {
    if (!img || !img->data || img->channels < 3) return;

    int num_pixels = img->width * img->height;
    for (int i = 0; i < num_pixels; i++) {
        unsigned char *px = &img->data[i * img->channels];
        px[0] = clamp_u8((int)px[0] + r_offset); // Red
        px[1] = clamp_u8((int)px[1] + g_offset); // Green
        px[2] = clamp_u8((int)px[2] + b_offset); // Blue
        // Alpha (px[3]) is left untouched!
    }
}
```

*Tip: For custom 3x3 convolution filters (e.g. Emboss, Motion Blur), you can directly reuse the internal `apply_convolution_3x3()` helper in `src/filters.c`.*

---

### Step 3: Register the Option in the Menu

Open [`src/main.c`](src/main.c):

1. Add your filter to `print_filter_menu()`:
   ```c
   printf("  [12] Color Tint         (Add custom RGB color offset)\n");
   ```
2. Add a new `case` to `apply_chosen_filter()`:
   ```c
   case 12:
       apply_tint(img, 30, 0, 30); // Example: Purple tint
       snprintf(status_msg, stat_size, "Applied Color Tint.");
       if (strlen(history) > 0) strncat(history, " -> Tint", hist_size - strlen(history) - 1);
       else strncpy(history, "Tint", hist_size - 1);
       break;
   ```

---

### Step 4: Recompile and Test

```bash
# Rebuild using Make
mingw32-make

# Run automated tests
mingw32-make test
```

---

## Project Structure

```
CFilter/
├── .github/
│   └── workflows/
│       ├── ci.yml             # Automated CI workflow (Windows, Linux, macOS)
│       └── release.yml        # Automatic multi-platform release builder
├── .gitignore                 # Git ignore rules for C & build artifacts
├── LICENSE                    # MIT License
├── README.md                  # Project documentation & reference
├── Makefile                   # Multi-target build script
├── assets/
│   └── sample.bmp             # Sample asset for testing
├── include/
│   ├── image.h                # Image data structures & lifecycle prototypes
│   ├── filters.h              # Filter declarations
│   ├── stb_image.h            # STB image decoder
│   └── stb_image_write.h      # STB image encoder
└── src/
    ├── main.c                 # Interactive menu & CLI dispatcher
    ├── image.c                # Format detection & memory management
    └── filters.c              # Filter algorithms & convolution engine
```

---

## License

This project is open-source and available under the [MIT License](LICENSE).
