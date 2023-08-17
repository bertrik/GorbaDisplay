#include <stdint.h>

#include "framebuffer.h"

void draw_init(uint8_t *framebuffer, int width, int height, double gamma);
void draw_flip(bool horizontal, bool vertical);

void draw_clear(void);
bool draw_pixel(int x, int y, uint8_t p);
int draw_text(int x, int y, uint8_t fg, const char *text);


