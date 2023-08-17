// simple ascii pixelflood processor
// written by folkert van heusden
// released under MIT license

#include <ctype.h>

#include "pixelflood.h"


static uint8_t cnv_hex_nibble(char c)
{
    c = toupper(c);

    return c >= 'A' ? c - 'A' + 10 : c - '0';
}

pixelflood::pixelflood(const int listen_port_ascii, const int listen_port_binary,
        const int width, const int height,
        bool (* draw_pixel)(int x, int y, uint8_t gray),
        uint8_t (* rgb_to_gray)(uint8_t r, uint8_t g, uint8_t b)) :
    listen_port_ascii(listen_port_ascii),
    listen_port_binary(listen_port_binary),
    width(width), height(height),
    draw_pixel(draw_pixel),
    rgb_to_gray(rgb_to_gray)
{
}

pixelflood::~pixelflood()
{
}

bool pixelflood::begin()
{
    if (listen_port_ascii == listen_port_binary)
        return false;

    if (listen_port_ascii != -1)
        port_ascii.begin(listen_port_ascii);

    if (listen_port_binary != -1)
        port_binary.begin(listen_port_binary);

    return true;
}

void pixelflood::poll_ascii()
{
    // any data?
    unsigned packet_size = port_ascii.parsePacket();

    if (packet_size == 0)
        return;

    // retrieve packet, make sure it is 0x00 terminated
    // for easy processing
    char buffer[1472 + 1];

    if (packet_size >= sizeof buffer)
        packet_size = sizeof buffer - 1;

    port_ascii.read(buffer, packet_size);

    buffer[packet_size] = 0x00;

    // Serial.println(F("ASCII packet"));

    char *p                = buffer;
    char *const buffer_end = &buffer[packet_size];

    for(;;) {
        // format: PX x y RRGGBB
        // with RR being a hex value

        // sanity check
        if (buffer_end - p < 13)
            return;

        if (p[0] != 'P' || p[1] != 'X' || p[2] != ' ')
            return;

        int x = atoi(&p[3]);

        if (x >= width)
            return;

        char *space = strchr(&p[3], ' ');
        if (!space)
            return;

        int y = atoi(space + 1);

        if (y >= height)
            return;

        space = strchr(space + 1, ' ');
        if (!space)
            return;

        char *rgb = space + 1;

        if (buffer_end - rgb < 6)
            return;

        uint8_t r = (cnv_hex_nibble(rgb[0]) << 4) | cnv_hex_nibble(rgb[1]);
        uint8_t g = (cnv_hex_nibble(rgb[2]) << 4) | cnv_hex_nibble(rgb[3]);
        uint8_t b = (cnv_hex_nibble(rgb[4]) << 4) | cnv_hex_nibble(rgb[5]);

        uint8_t gray = rgb_to_gray(r, g, b);

        draw_pixel(x, y, gray);

        p = rgb + 6;

        // p now points to either 0x00 or the \n at the end of a pixel

        if (*p != '\n')
            return;

        p++;  // skip lf
    }
}

void pixelflood::draw_binary_0(const uint8_t *const start, const uint8_t *const end, const bool alpha)
{
    const uint8_t *p = start;

    const int len = 7 + alpha;

    while(end - p >= len) {
        int x = p[0] + (p[1] << 8);

        if (x >= width)
            break;

        int y = p[2] + (p[3] << 8);

        if (y >= height)
            break;

        uint8_t r = p[4];
        uint8_t g = p[5];
        uint8_t b = p[6];

        uint8_t gray = rgb_to_gray(r, g, b);

        draw_pixel(x, y, gray);

        p += len;
    }
}

void pixelflood::draw_binary_1(const uint8_t *const start, const uint8_t *const end, const bool alpha)
{
    const uint8_t *p = start;

    const int len = 6 + alpha;

    while(end - p >= len) {
        int x = p[0] + ((p[1] & 15) << 8);

        if (x >= width)
            break;

        int y = (p[1] >> 4) + (p[2] << 4);

        if (y >= height)
            break;

        uint8_t r = p[3];
        uint8_t g = p[4];
        uint8_t b = p[5];

        uint8_t gray = rgb_to_gray(r, g, b);

        draw_pixel(x, y, gray);

        p += len;
    }
}

void pixelflood::draw_binary_2(const uint8_t *const start, const uint8_t *const end, const bool alpha)
{
    const uint8_t *p = start;

    const int len = 4;

    while(end - p >= len) {
        int x = p[0] + ((p[1] & 15) << 8);

        if (x >= width)
            break;

        int y = (p[1] >> 4) + (p[2] << 4);

        if (y >= height)
            break;

        uint8_t r, g, b;

        if (alpha) {
            r = (p[3] >> 6) * 63;
            g = ((p[3] >> 4) & 3) * 63;
            b = ((p[3] >> 2) & 3) * 63;
        }
        else {
            r = (p[3] >> 5) * 31;
            g = ((p[3] >> 2) & 7) * 31;
            b = (p[3] & 3) * 63;
        }

        uint8_t gray = rgb_to_gray(r, g, b);

        draw_pixel(x, y, gray);

        p += len;
    }
}

void pixelflood::draw_binary_3(const uint8_t *const start, const uint8_t *const end, const uint8_t rgb)
{
    const uint8_t *p = start;

    const int len = 3;

    uint8_t r = (p[3] >> 5) * 31;
    uint8_t g = ((p[3] >> 2) & 7) * 31;
    uint8_t b = (p[3] & 3) * 63;

    uint8_t gray = rgb_to_gray(r, g, b);

    while(end - p >= len) {
        int x = p[0] + ((p[1] & 15) << 8);

        if (x >= width)
            break;

        int y = (p[1] >> 4) + (p[2] << 4);

        if (y >= height)
            break;

        draw_pixel(x, y, gray);

        p += len;
    }
}

void pixelflood::poll_binary()
{
    // any data?
    unsigned packet_size = port_binary.parsePacket();

    if (packet_size == 0)
        return;

    // retrieve packet
    uint8_t buffer[1500];

    if (packet_size >= sizeof buffer)
        packet_size = sizeof buffer - 1;

    port_binary.read(buffer, packet_size);

    if (packet_size < 2)
        return;

    // Serial.println(F("Binary packet"));

    uint8_t *p                = buffer;
    uint8_t *const buffer_end = &buffer[packet_size];

    if (p[0] == 0)
        draw_binary_0(p + 2, buffer_end, (p[1] & 1) == 1);
    else if (p[0] == 1)
        draw_binary_1(p + 2, buffer_end, (p[1] & 1) == 1);
    else if (p[0] == 2)
        draw_binary_2(p + 2, buffer_end, (p[1] & 1) == 1);
    else if (p[0] == 3)
        draw_binary_3(p + 2, buffer_end, p[1]);
}

void pixelflood::poll()
{
    poll_ascii();

    poll_binary();

    uint32_t now = millis();

    if (now - last_announcement >= 5000) {
        // Serial.println(F("BC announce"));

        IPAddress ip_addr   = WiFi.localIP();
        IPAddress netmask   = WiFi.subnetMask();
        IPAddress broadcast = {
            uint8_t(ip_addr[0] | (netmask[0] ^ 255)),
            uint8_t(ip_addr[1] | (netmask[1] ^ 255)),
            uint8_t(ip_addr[2] | (netmask[2] ^ 255)),
            uint8_t(ip_addr[3] | (netmask[3] ^ 255))
        };

        char buffer[128];
        int buf_len = snprintf(buffer, sizeof buffer, "pixelvloed:0.00 %d.%d.%d.%d:%d %d*%d",
                ip_addr[0], ip_addr[1], ip_addr[2], ip_addr[3],
                listen_port_binary,
                width, height);

        WiFiUDP bc_udp;

        int bc_port = 5006;

        bc_udp.begin(bc_port);

        bc_udp.beginPacket(broadcast, bc_port);
        bc_udp.write(buffer, buf_len);
        bc_udp.endPacket();

        last_announcement = now;
    }
}
