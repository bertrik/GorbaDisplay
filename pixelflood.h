// simple ascii pixelflood processor
// written by folkert van heusden
// released under MIT license

#include <cstdint>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

class pixelflood
{
private:
    const int listen_port_ascii;
    const int listen_port_binary;
    const int width;
    const int height;
    bool    (* draw_pixel )(int x, int y, uint8_t gray);
    uint8_t (* rgb_to_gray)(uint8_t r, uint8_t g, uint8_t b);

    uint32_t last_announcement { 0 };

    WiFiUDP port_ascii;
    WiFiUDP port_binary;

    void poll_ascii();

    void draw_binary_0(const uint8_t *const start, const uint8_t *const end, const bool alpha);
    void draw_binary_1(const uint8_t *const start, const uint8_t *const end, const bool alpha);
    void draw_binary_2(const uint8_t *const start, const uint8_t *const end, const bool alpha);
    void draw_binary_3(const uint8_t *const start, const uint8_t *const end, const uint8_t color);
    void poll_binary();

public:
    pixelflood(const int listen_port_ascii,
            const int listen_port_binary,
            const int width, const int height,
            bool (*draw_pixel)(int x, int y, uint8_t gray),
            uint8_t (* rgb_to_gray)(uint8_t r, uint8_t g, uint8_t b));
    virtual ~pixelflood();

    bool begin();

    void poll();
};
