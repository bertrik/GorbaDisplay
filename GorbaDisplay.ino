#include <Arduino.h>

#include <stdbool.h>
#include <stdint.h>

#include "WiFiManager.h"

#include "leddriver.h"
#include "framebuffer.h"

#include "cmdproc.h"
#include "editline.h"

#define print Serial.printf

static char espid[32];
static char editline[128];
static int frame_counter;

static uint8_t framebuffer[LED_HEIGHT][LED_WIDTH];

static int do_pix(int argc, char *argv[])
{
    if (argc < 4) {
        return CMD_ARG;
    }
    int x = atoi(argv[1]);
    int y = atoi(argv[2]);
    uint8_t c = atoi(argv[3]);
    if ((x >= LED_WIDTH) || (y >= LED_HEIGHT)) {
        return CMD_ARG;
    }
    framebuffer[y][x] = c;
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
    Serial.println("\nGORBA");

    led_init(vsync);

    snprintf(espid, sizeof(espid), "gorba-%06x", ESP.getChipId());
    Serial.begin(115200);
    print("\n%s\n", espid);

    for (int y = 5; y < 10; y++) {
        for (int x = 10; x < 22; x++) {
            framebuffer[y][x] = 32;
        }
    }

    EditInit(editline, sizeof(editline));

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
