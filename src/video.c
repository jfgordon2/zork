#include "video.h"

#define REG16(address) (*(volatile uint16_t *)(address))
#define REG32(address) (*(volatile uint32_t *)(address))
#define VRAM ((Color *)0x06000000)

static Color framebuffer[SCREEN_W * SCREEN_H] __attribute__((aligned(4)));

static void copy_words(void *dst, const void *src, unsigned words) {
    REG32(0x040000D4) = (uint32_t)(uintptr_t)src;
    REG32(0x040000D8) = (uint32_t)(uintptr_t)dst;
    __asm__ volatile("" ::: "memory");
    REG32(0x040000DC) = 0x84000000 | words;
    __asm__ volatile("" ::: "memory");
}

void video_init(void) {
    REG16(0x04000208) = 0;
    REG16(0x04000000) = 0x0080;
}

void wait_frame(void) {
    while (REG16(0x04000006) >= 160) {}
    while (REG16(0x04000006) < 160) {}
}

void present(void) {
    wait_frame();
    copy_words(VRAM, framebuffer, SCREEN_W * SCREEN_H / 2);
    REG16(0x04000000) = 0x0403;
}

uint16_t buttons(void) {
    return ~REG16(0x04000130) & 0x03FF;
}

void scene(int y, const Color *pixels) {
    scene_rows(y, pixels, SCENE_H);
}

void scene_rows(int y, const Color *pixels, int rows) {
    copy_words(framebuffer + y * SCREEN_W, pixels, SCREEN_W * rows / 2);
}

void draw_masked_pixels(int x, int y, int width, int height, const Color *pixels, bool mirrored) {
    int left = x < 0 ? -x : 0;
    int right = x + width > SCREEN_W ? SCREEN_W - x : width;
    int top = y < 0 ? -y : 0;
    int bottom = y + height > SCREEN_H ? SCREEN_H - y : height;
    if (left >= right || top >= bottom) return;
    int direction = mirrored ? -1 : 1;
    int start = mirrored ? width - 1 - left : left;
    for (int row = top; row < bottom; row++) {
        const Color *source = pixels + row * width;
        Color *dest = framebuffer + (y + row) * SCREEN_W + (x + left);
        for (int col = left, i = start; col < right; col++, i += direction, dest++) {
            // Packed sprites mark opaque RGB555 pixels with bit 15.
            if (source[i] & 0x8000) *dest = source[i] & 0x7fff;
        }
    }
}

// Four fractional bits, with 16 representing full opacity.
static Color blend(Color foreground, Color background, unsigned opacity) {
    enum { RED_BLUE = 0x7c1f, GREEN = 0x03e0 };
    unsigned behind = 16 - opacity;
    unsigned rb = ((foreground & RED_BLUE) * opacity + (background & RED_BLUE) * behind) >> 4;
    unsigned g = ((foreground & GREEN) * opacity + (background & GREEN) * behind) >> 4;
    return (Color)((rb & RED_BLUE) | (g & GREEN));
}

void tint_rect(int x, int y, int width, int height, Color color, unsigned opacity) {
    for (int row = y; row < y + height; row++) {
        Color *line = framebuffer + row * SCREEN_W + x;
        for (int col = 0; col < width; col++) line[col] = blend(color, line[col], opacity);
    }
}

void ground_shadow(int x, int y, int width) {
    static const uint8_t opacity[] = {2, 4, 5, 3, 1};
    static const uint8_t inset[] = {5, 2, 0, 2, 5};
    for (int row = 0; row < 5; row++) {
        if (y + row < 0 || y + row >= SCREEN_H) continue;
        Color *line = framebuffer + (y + row) * SCREEN_W;
        int left = x + inset[row], right = x + width - inset[row];
        if (left < 0) left = 0;
        if (right > SCREEN_W) right = SCREEN_W;
        for (int col = left; col < right; col++) line[col] = blend(RGB(15, 12, 16), line[col], opacity[row]);
    }
}

void pixel(int x, int y, Color color) {
    framebuffer[y * SCREEN_W + x] = color;
}

#ifdef VISUAL_QA
// Host-test hook: lets the replay QA check pixels the renderer drew.
Color visual_qa_pixel(int x, int y) { return framebuffer[y * SCREEN_W + x]; }
#endif

void fill(int x, int y, int w, int h, Color color) {
    for (int row = y; row < y + h; row++) {
        Color *line = framebuffer + row * SCREEN_W + x;
        for (int col = 0; col < w; col++) line[col] = color;
    }
}

void box(int x, int y, int w, int h, Color background, Color edge) {
    fill(x + 1, y, w - 2, h, background);
    fill(x, y + 1, w, h - 2, background);
    fill(x + 1, y, w - 2, 1, edge);
    fill(x + 1, y + h - 1, w - 2, 1, edge);
    fill(x, y + 1, 1, h - 2, edge);
    fill(x + w - 1, y + 1, 1, h - 2, edge);
}

typedef struct { uint8_t width, row[7]; } Glyph;
static const Glyph glyphs[128] = {
    [' '] = {3, {0}},
    ['!'] = {1, {1,1,1,1,1,0,1}},
    ['"'] = {3, {5,5,0,0,0,0,0}},
    ['#'] = {5, {10,31,10,10,31,10,0}},
    ['\''] = {2, {2,2,1,0,0,0,0}},
    ['('] = {3, {4,2,1,1,1,2,4}},
    [')'] = {3, {1,2,4,4,4,2,1}},
    ['*'] = {5, {0,21,14,31,14,21,0}},
    ['+'] = {5, {0,4,4,31,4,4,0}},
    [','] = {2, {0,0,0,0,0,2,1}},
    ['-'] = {3, {0,0,0,7,0,0,0}},
    ['.'] = {1, {0,0,0,0,0,0,1}},
    ['/'] = {4, {8,8,4,4,2,2,1}},
    ['0'] = {5, {14,17,19,21,25,17,14}},
    ['1'] = {3, {2,3,2,2,2,2,7}},
    ['2'] = {5, {14,17,16,8,4,2,31}},
    ['3'] = {5, {15,16,16,14,16,16,15}},
    ['4'] = {5, {8,12,10,9,31,8,8}},
    ['5'] = {5, {31,1,1,15,16,16,15}},
    ['6'] = {5, {14,1,1,15,17,17,14}},
    ['7'] = {5, {31,16,8,4,2,2,2}},
    ['8'] = {5, {14,17,17,14,17,17,14}},
    ['9'] = {5, {14,17,17,30,16,16,14}},
    [':'] = {1, {0,0,1,0,0,1,0}},
    [';'] = {2, {0,0,2,0,0,2,1}},
    ['<'] = {4, {0,8,4,2,4,8,0}},
    ['='] = {5, {0,0,31,0,31,0,0}},
    ['>'] = {4, {0,1,2,4,2,1,0}},
    ['?'] = {5, {14,17,16,8,4,0,4}},
    ['A'] = {5, {14,17,17,31,17,17,17}}, ['B'] = {5, {15,17,17,15,17,17,15}},
    ['C'] = {5, {14,17,1,1,1,17,14}},    ['D'] = {5, {15,17,17,17,17,17,15}},
    ['E'] = {5, {31,1,1,15,1,1,31}},    ['F'] = {5, {31,1,1,15,1,1,1}},
    ['G'] = {5, {14,17,1,29,17,17,14}},  ['H'] = {5, {17,17,17,31,17,17,17}},
    ['I'] = {3, {7,2,2,2,2,2,7}},       ['J'] = {5, {28,8,8,8,8,9,6}},
    ['K'] = {5, {17,9,5,3,5,9,17}},     ['L'] = {5, {1,1,1,1,1,1,31}},
    ['M'] = {5, {17,27,21,21,17,17,17}},['N'] = {5, {17,19,21,25,17,17,17}},
    ['O'] = {5, {14,17,17,17,17,17,14}},['P'] = {5, {15,17,17,15,1,1,1}},
    ['Q'] = {5, {14,17,17,17,21,9,22}},  ['R'] = {5, {15,17,17,15,5,9,17}},
    ['S'] = {5, {30,1,1,14,16,16,15}},  ['T'] = {5, {31,4,4,4,4,4,4}},
    ['U'] = {5, {17,17,17,17,17,17,14}},['V'] = {5, {17,17,17,17,17,10,4}},
    ['W'] = {5, {17,17,17,21,21,27,17}},['X'] = {5, {17,17,10,4,10,17,17}},
    ['Y'] = {5, {17,17,10,4,4,4,4}},    ['Z'] = {5, {31,16,8,4,2,1,31}},
    ['['] = {3, {7,1,1,1,1,1,7}},       [']'] = {3, {7,4,4,4,4,4,7}},
    ['^'] = {5, {4,10,17,0,0,0,0}},
    ['&'] = {5, {6,9,5,2,21,9,22}},
    ['_'] = {5, {0,0,0,0,0,0,31}},
    ['a'] = {4, {0,0,6,8,14,9,14}},      ['b'] = {4, {1,1,7,9,9,9,7}},
    ['c'] = {4, {0,0,14,1,1,1,14}},      ['d'] = {4, {8,8,14,9,9,9,14}},
    ['e'] = {4, {0,0,6,9,15,1,14}},      ['f'] = {3, {4,2,7,2,2,2,2}},
    ['g'] = {4, {0,0,14,9,14,8,7}},      ['h'] = {4, {1,1,7,9,9,9,9}},
    ['i'] = {1, {1,0,1,1,1,1,1}},       ['j'] = {2, {2,0,2,2,2,3,1}},
    ['k'] = {4, {1,1,9,5,3,5,9}},       ['l'] = {2, {1,1,1,1,1,1,2}},
    ['m'] = {5, {0,0,11,21,21,21,21}},  ['n'] = {4, {0,0,7,9,9,9,9}},
    ['o'] = {4, {0,0,6,9,9,9,6}},       ['p'] = {4, {0,0,7,9,7,1,1}},
    ['q'] = {4, {0,0,14,9,14,8,8}},      ['r'] = {3, {0,0,5,3,1,1,1}},
    ['s'] = {4, {0,0,14,1,6,8,7}},      ['t'] = {3, {0,2,7,2,2,2,4}},
    ['u'] = {4, {0,0,9,9,9,9,14}},      ['v'] = {5, {0,0,17,17,17,10,4}},
    ['w'] = {5, {0,0,17,21,21,21,10}},  ['x'] = {4, {0,0,9,9,6,9,9}},
    ['y'] = {4, {0,0,9,9,14,8,7}},      ['z'] = {4, {0,0,15,8,6,1,15}},
};

static const Glyph *glyph(unsigned char ch) {
    if (ch < 128 && glyphs[ch].width) return &glyphs[ch];
    return &glyphs['?'];
}

static int advance(char ch) {
    return glyph(ch)->width + 1;
}

static int letter(int x, int y, char ch, Color color, int scale) {
    const Glyph *g = glyph(ch);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < g->width; col++) {
            if (g->row[row] & (1 << col)) {
                if (scale == 1) pixel(x + col, y + row, color);
                else fill(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
    return x + (g->width + 1) * scale;
}

void label(int x, int y, const char *text, Color color) {
    while (*text) x = letter(x, y, *text++, color, 1);
}

void title(int x, int y, const char *text, Color color, int scale) {
    while (*text) x = letter(x, y, *text++, color, scale);
}

int label_width(const char *text) {
    int width = 0;
    while (*text) width += advance(*text++);
    return width ? width - 1 : 0;
}

const char *wrap_line(const char *text, int width, const char **line_end) {
    const char *end = text;
    const char *last_space = 0;
    int used = 0;
    while (*end && *end != '\n' && used + advance(*end) <= width) {
        if (*end == ' ') last_space = end;
        used += advance(*end++);
    }
    if (*end && *end != '\n' && *end != ' ' && last_space) end = last_space;
    // Always make progress: a glyph wider than the line is placed on its own row.
    if (end == text && *end && *end != '\n' && *end != ' ') end++;
    *line_end = end;
    if (*end == '\n') end++;
    while (*end == ' ') end++;
    return end;
}

void paragraph(int x, int y, int width, int lines, const char *text, Color color) {
    for (int row = 0; *text && row < lines; row++, y += 10) {
        const char *end;
        const char *next = wrap_line(text, width, &end);
        int px = x;
        while (text < end) px = letter(px, y, *text++, color, 1);
        text = next;
    }
}
