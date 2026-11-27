#ifndef VIDEO_H
#define VIDEO_H

#include <stdbool.h>
#include <stdint.h>

enum { SCREEN_W = 240, SCREEN_H = 160, SCENE_H = 102 };
typedef uint16_t Color;
#define RGB(r, g, b) ((Color)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))

void video_init(void);
void wait_frame(void);
void present(void);
uint16_t buttons(void);
// PROFILE builds only: a 16-bit 16.384 kHz timer around the primary menu build.
// Both calls are no-ops in normal and VISUAL_QA builds.
void video_profile_timer_start(void);
uint16_t video_profile_timer_read(void);
void video_profile_token_timer_start(void);
uint16_t video_profile_token_timer_read(void);
void video_profile_trace_start(void);
uint16_t video_profile_trace_read(void);
enum {
    KEY_A = 1 << 0, KEY_B = 1 << 1, KEY_SELECT = 1 << 2, KEY_START = 1 << 3,
    KEY_RIGHT = 1 << 4, KEY_LEFT = 1 << 5, KEY_UP = 1 << 6, KEY_DOWN = 1 << 7,
    KEY_R = 1 << 8, KEY_L = 1 << 9
};

void scene(int y, const Color *pixels);
void scene_rows(int y, const Color *pixels, int rows);
void draw_masked_pixels(int x, int y, int width, int height, const Color *pixels, bool mirrored);
void ground_shadow(int x, int y, int width);
void tint_rect(int x, int y, int width, int height, Color color, unsigned opacity);
void fill(int x, int y, int w, int h, Color color);
void box(int x, int y, int w, int h, Color background, Color edge);
void dpad_badge(int x, int y, int size, uint16_t direction, Color background, Color edge, Color ink);
void pixel(int x, int y, Color color);
void label(int x, int y, const char *text, Color color);
void title(int x, int y, const char *text, Color color, int scale);
int label_width(const char *text);
const char *wrap_line(const char *text, int width, const char **line_end);
void paragraph(int x, int y, int width, int lines, const char *text, Color color);

#endif
