#include <string.h>
#include <math.h>

#include "font.h"
#include "draw.h"

static uint8_t *_framebuffer;
static int _width, _height;
static uint8_t gamma_corr[256];
static bool fliph, flipv;

void draw_init(uint8_t *framebuffer, int width, int height, double gamma)
{
    _framebuffer = framebuffer;
    _width = width;
    _height = height;
    for (int i = 0; i < 256; i++) {
        gamma_corr[i] = round(pow(i / 255.0, gamma) * 255);
    }
    fliph = false;
    flipv = false;
}

void draw_flip(bool horizontal, bool vertical)
{
    fliph = horizontal;
    flipv = vertical;
}

void draw_clear(void)
{
    memset(_framebuffer, 0, _width * _height);
}

bool draw_pixel(int x, int y, uint8_t c)
{
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) {
        return false;
    }
    if (fliph) {
        x = _width - 1 - x;
    }
    if (flipv) {
        y = _height - 1 - y;
    }
    _framebuffer[y * _width + x] = gamma_corr[c];
    return true;
}

static int draw_glyph(int g, int x, int y, uint8_t fg)
{
    // ASCII?
    if (g > 127) {
        return x;
    }

    // draw glyph
    unsigned char aa = 0;
    for (int col = 0; col < 5; col++) {
        unsigned char a = font[g][col];

        // skip repeating space
        if ((aa == 0) && (a == 0)) {
            continue;
        }
        aa = a;

        // draw column
        for (int i = 0; i < 7; y++) {
            if (a & 1) {
                draw_pixel(x, y + i, fg);
            }
            a >>= 1;
        }
        x++;
    }

    // skip space until next character
    if (aa != 0) {
        x++;
    }

    return x;
}

int draw_text(const char *text, int x, int y, uint8_t fg)
{
    for (size_t i = 0; i < strlen(text); i++) {
        x = draw_glyph(text[i], x, y, fg);
    }
    return x;
}

