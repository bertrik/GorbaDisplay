#include <stdint.h>
#include <stdbool.h>

typedef void (vsync_fn_t) (int frame_nr);

void led_init(vsync_fn_t * vsync);
void led_write_framebuffer(const void *data);

void led_enable(bool enable);
