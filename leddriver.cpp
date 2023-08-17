#include <string.h>

#include <Arduino.h>

#include "framebuffer.h"
#include "leddriver.h"

#define PIN_ROW_SEL0    D1
#define PIN_ROW_SEL1    D2
#define PIN_ROW_SEL2    D3

#define PIN_COL_DATA    D5
#define PIN_COL_ENABLE  D6
#define PIN_COL_CLOCK   D7
#define PIN_COL_LATCH   D8

#define MUX_MASK ((1 << PIN_ROW_SEL0) | (1 << PIN_ROW_SEL1) | (1 << PIN_ROW_SEL2))

static const vsync_fn_t *vsync_fn;
static uint8_t framebuffer[LED_HEIGHT][LED_WIDTH];
static uint8_t pwmbuf[LED_HEIGHT][LED_WIDTH];
static int phase = 0;
static int frame = 0;

// do not use on pin D0
#define FAST_GPIO_WRITE(pin,val) if (val) GPOS = 1<<(pin); else GPOC = 1<<(pin)

// "horizontal" interrupt routine, displays one full row of data
static void IRAM_ATTR led_hsync(void)
{
    // disable row multiplexer
    FAST_GPIO_WRITE(PIN_ROW_SEL2, 1);

    // write column shift register
    FAST_GPIO_WRITE(PIN_COL_LATCH, 0);
    for (int row = phase; row < LED_HEIGHT; row += 4) {
        for (int col = 0; col < LED_WIDTH; col++) {
            int val = pwmbuf[row][col];
            val += framebuffer[row][col];
            FAST_GPIO_WRITE(PIN_COL_CLOCK, 0);
            FAST_GPIO_WRITE(PIN_COL_DATA, val > 255);
            FAST_GPIO_WRITE(PIN_COL_CLOCK, 1);
            pwmbuf[row][col] = val;
        }
    }
    FAST_GPIO_WRITE(PIN_COL_LATCH, 1);

    // activate row
    FAST_GPIO_WRITE(PIN_ROW_SEL0, (phase & 1) == 0);
    FAST_GPIO_WRITE(PIN_ROW_SEL1, (phase & 2) == 0);
    FAST_GPIO_WRITE(PIN_ROW_SEL2, 0);   // enable

    // increment phase for new row    
    phase = (phase + 1) & 3;
    if (phase == 0) {
        // last line, indicate vsync
        vsync_fn(frame++);
    }
}

void IRAM_ATTR led_write_framebuffer(const void *data)
{
    memcpy(framebuffer, data, sizeof(framebuffer));
}

void led_init(const vsync_fn_t * vsync)
{
    // set all pins to a defined state
    pinMode(PIN_ROW_SEL0, OUTPUT);
    pinMode(PIN_ROW_SEL1, OUTPUT);
    pinMode(PIN_ROW_SEL2, OUTPUT);
    digitalWrite(PIN_ROW_SEL0, 0);
    digitalWrite(PIN_ROW_SEL1, 0);
    digitalWrite(PIN_ROW_SEL2, 0);

    pinMode(PIN_COL_DATA, OUTPUT);
    digitalWrite(PIN_COL_DATA, 0);
    pinMode(PIN_COL_ENABLE, OUTPUT);
    digitalWrite(PIN_COL_ENABLE, 0);
    pinMode(PIN_COL_CLOCK, OUTPUT);
    digitalWrite(PIN_COL_CLOCK, 0);
    pinMode(PIN_COL_LATCH, OUTPUT);
    digitalWrite(PIN_COL_LATCH, 0);

    // copy vsync pointer
    vsync_fn = vsync;

    // clear the frame buffer
    memset(framebuffer, 0, sizeof(framebuffer));

    // initialise PWM buffer
    for (int y = 0; y < LED_HEIGHT; y++) {
        for (int x = 0; x < LED_WIDTH; x++) {
            pwmbuf[y][x] = random(255);
        }
    }

    // initialise timer
    timer1_isr_init();

}

void led_enable(bool enable)
{
    timer1_disable();
    if (enable) {
        // enable timer interrupt
        timer1_attachInterrupt(led_hsync);
        timer1_write(1000);
        timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
    }
    digitalWrite(PIN_COL_ENABLE, enable ? 0 : 1);
}

