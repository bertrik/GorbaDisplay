#include <Arduino.h>

#include <stdbool.h>
#include <stdint.h>

#include "WiFiManager.h"

#include "leddriver.h"
#include "framebuffer.h"
#include "draw.h"

#include "cmdproc.h"
#include "editline.h"

#define print Serial.printf

static char espid[32];
static char editline[128];
static int frame_counter;

static uint8_t framebuffer[LED_HEIGHT][LED_WIDTH];

static int do_pix(int argc, char *argv[])
{
    if (argc < 3) {
        return CMD_ARG;
    }
    int x = atoi(argv[1]);
    int y = atoi(argv[2]);
    uint8_t c = 128;
    if (argc > 3) {
        c = atoi(argv[3]);
    }
    bool r = draw_pixel(x, y, c);
    return r ? CMD_OK : CMD_ARG;
}

static int do_clear(int argc, char *argv[])
{
    draw_clear();
    return CMD_OK;
}

static int do_text(int argc, char *argv[])
{
    if (argc < 2) {
        return CMD_ARG;
    }
    char *text = argv[1];
    print("Drawing '%s'\n", text);
    draw_text(0, 0, 32, text);
    return CMD_OK;
}

static int do_fps(int argc, char *argv[])
{
    print("Measuring ...");
    uint32_t count = frame_counter;
    delay(1000);
    int fps = frame_counter - count;
    print("FPS = %d\n", fps);
    return CMD_OK;
}

static int do_enable(int argc, char *argv[])
{
    bool enable = true;
    if (argc > 1) {
        enable = atoi(argv[1]) != 0;
    }
    led_enable(enable);
    return CMD_OK;
}

static int do_reboot(int argc, char *argv[])
{
    led_enable(false);
    ESP.restart();
    return CMD_OK;
}

static int do_help(int argc, char *argv[]);
static const cmd_t commands[] = {
    { "pix", do_pix, "<col> <row> [intensity] Set pixel" },
    { "text", do_text, "<text> Draw text" },
    { "clear", do_clear, "Clear display" },
    { "fps", do_fps, "Show FPS" },
    { "enable", do_enable, "[0|1] Enable/disable" },
    { "reboot", do_reboot, "Reboot" },
    { "help", do_help, "Show help" },
    { NULL, NULL, NULL }
};

static void show_help(const cmd_t * cmds)
{
    for (const cmd_t * cmd = cmds; cmd->cmd != NULL; cmd++) {
        print("%10s: %s\n", cmd->name, cmd->help);
    }
}

static int do_help(int argc, char *argv[])
{
    show_help(commands);
    return CMD_OK;
}

// vsync callback
static void IRAM_ATTR vsync(int frame_nr)
{
    led_write_framebuffer(framebuffer);
    frame_counter = frame_nr;
}

void setup(void)
{
    Serial.begin(115200);
    Serial.println("\nHello world!");

    led_init(vsync);
    draw_init(&framebuffer[0][0], LED_WIDTH, LED_HEIGHT, 1.0);
    EditInit(editline, sizeof(editline));

    snprintf(espid, sizeof(espid), "gorba-%06x", ESP.getChipId());
    print("\n%s\n", espid);

    draw_text(0, 4, 32, "Hello!");
    led_enable(true);
}

void loop(void)
{
    // parse command line
    if (Serial.available()) {
        char c = Serial.read();
        bool haveLine = EditLine(c, &c);
        Serial.write(c);
        if (haveLine) {
            int result = cmd_process(commands, editline);
            switch (result) {
            case CMD_OK:
                print("OK\n");
                break;
            case CMD_NO_CMD:
                break;
            case CMD_UNKNOWN:
                print("Unknown command, available commands:\n");
                show_help(commands);
                break;
            case CMD_ARG:
                print("Invalid argument(s)\n");
                break;
            default:
                print("%d\n", result);
                break;
            }
            print(">");
        }
    }
}
