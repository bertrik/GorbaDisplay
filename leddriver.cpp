#include <string.h>

#include <Arduino.h>

#include "framebuffer.h"
#include "leddriver.h"

#define PIN_COL_DATA    D1
#define PIN_COL_ENABLE  D2
#define PIN_COL_CLOCK   D3
#define PIN_COL_LATCH   D4

#define PIN_ROW_SEL0    D5
#define PIN_ROW_SEL1    D6
#define PIN_ROW_SEL2    D7

#define MUX_MASK ((1 << PIN_ROW_SEL0) | (1 << PIN_ROW_SEL1) | (1 << PIN_ROW_SEL2))

static const vsync_fn_t *vsync_fn;
static uint8_t framebuffer[LED_HEIGHT][LED_WIDTH];
static uint8_t pwmbuf[LED_HEIGHT][LED_WIDTH];
static int phase = 0;
static int frame = 0;

#define FAST_GPIO_WRITE(pin,val) if (val) GPOS = 1<<(pin); else GPOC = 1<<(pin)

static void IRAM_ATTR mux_clear(void)
{
    // set to invisible row
    GPOS = MUX_MASK;
}

static void IRAM_ATTR mux_set(int select)
{
    uint16_t val = 0;
    val |= (select & 1) ? (1 << PIN_ROW_SEL0) : 0;
    val |= (select & 2) ? (1 << PIN_ROW_SEL1) : 0;
    val |= (select & 4) ? (1 << PIN_ROW_SEL2) : 0;
    GPOC = ~val & MUX_MASK;
}

// "horizontal" interrupt routine, displays one line
static void IRAM_ATTR led_hsync(void)
{
    // blank the display by selecting row 0
    mux_clear();

    // write column data
    for (int row = phase; row < LED_HEIGHT; row += 4) {
        FAST_GPIO_WRITE(PIN_COL_LATCH, 0);
        for (int col = 0; col < LED_WIDTH; col++) {
            int val = framebuffer[row][col];
            FAST_GPIO_WRITE(PIN_COL_CLOCK, 0);
            FAST_GPIO_WRITE(PIN_COL_DATA, val > 0);
            FAST_GPIO_WRITE(PIN_COL_CLOCK, 1);
        }
        FAST_GPIO_WRITE(PIN_COL_LATCH, 1);
    }

    // select mux
    mux_set(phase);

    // next phase    
    phase++;
    if (phase >= 4) {
        // last line, indicate vsync
        vsync_fn(frame++);
        phase = 0;
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
    mux_clear();

    pinMode(PIN_COL_DATA, OUTPUT);
    digitalWrite(PIN_COL_DATA, 0);

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

void led_enable(void)
{
    // enable columns
    digitalWrite(PIN_COL_ENABLE, 0);

    // set up timer interrupt
    timer1_disable();
    timer1_attachInterrupt(led_hsync);
    timer1_write(1000);         // fps = 555555/number
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
}

void led_disable(void)
{
    // detach the interrupt routine
    timer1_detachInterrupt();
    timer1_disable();

    // select invisible row
    mux_clear();

    // disable columns
    digitalWrite(PIN_COL_ENABLE, 1);
}
