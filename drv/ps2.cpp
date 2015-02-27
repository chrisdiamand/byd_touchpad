#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <iostream>

#define PS2_Y_OVERFLOW (0x1 << 7)
#define PS2_X_OVERFLOW (0x1 << 6)
#define PS2_Y_SIGN (0x1 << 5)
#define PS2_X_SIGN (0x1 << 4)
#define PS2_ALWAYS_1 (0x1 << 3)
#define PS2_MIDDLE (0x1 << 2)
#define PS2_RIGHT (0x1 << 1)
#define PS2_LEFT (0x1 << 0)

#define PS2_ACK 0xFA
#define PS2_RESET 0xFF
#define PS2_ENABLE_STREAMING 0xF4

int open_serio(void) {
    for (int i = 0; i < 64; i++) {
        std::string fname = "/dev/serio_raw" + std::to_string(i);
        const char *str = fname.c_str();
        int ret = open(str, O_RDWR);
        if (ret >= 0)
            return ret;
    }
    fprintf(stderr, "Error: Cannot open '/dev/serio_raw*'\n");
    exit(1);
    return -1;
}

uint8_t ps2_align(int fd) {
    while (true) {
        uint8_t c;
        read(fd, &c, 1);
        printf("align: R %02x\n", c);
        if (!(c & PS2_Y_OVERFLOW) &&
            !(c & PS2_X_OVERFLOW) &&
            (c & PS2_ALWAYS_1)) {
            return c;
        }
    }
}

uint8_t read_byte(int fd) {
    uint8_t c = 0;
    read(fd, &c, 1);
    return c;
}

void read_ps2_packet(int fd) {
    uint8_t pkt[4];
    pkt[0] = read_byte(fd);
    pkt[1] = read_byte(fd);
    pkt[2] = read_byte(fd);
    pkt[3] = read_byte(fd);
    uint8_t b1 = pkt[0];

    if (b1 & PS2_Y_OVERFLOW || b1 & PS2_X_OVERFLOW) {
        //printf("Overflow: Discarding\n");
    }

    if (!(b1 & PS2_ALWAYS_1)) {
        //printf("Aligning:\n");
        //b1 = pkt[0] = ps2_align(fd);
    }

    printf("%02x %02x %02x %02x ", pkt[0], pkt[1], pkt[2], pkt[3]);

    printf("%s %s %s\n", b1 & PS2_LEFT   ? "left" : "_",
                         b1 & PS2_MIDDLE ? "middle" : "_",
                         b1 & PS2_RIGHT  ? "right" : "_");
}

void wait_byte(int fd, uint8_t byte) {
    while (true) {
        uint8_t c;
        read(fd, &c, 1);
        if (c == byte)
            return;
        else
            printf("Skipping byte %02x\n", c);
    }
}

void ps2_wait_ack(int fd) {
    wait_byte(fd, PS2_ACK);
}

void send_byte(int fd, uint8_t byte) {
    write(fd, &byte, 1);
}

void ps2_reset(int fd) {
    send_byte(fd, PS2_RESET);
    ps2_wait_ack(fd);
    wait_byte(fd, 0xAA);
    printf("Touchpad reset: Extra byte %02x\n", read_byte(fd));
}

void ps2_get_status(int fd) {
    send_byte(fd, 0xE9);
    ps2_wait_ack(fd);
    uint8_t stat[3];
    read(fd, stat, 3);
    printf("status: %02x %02x %02x\n", stat[0], stat[1], stat[2]);
}

void ps2_set_res(int fd, uint8_t res) {
    printf("setres(%02x)\n", res);
    send_byte(fd, 0xE8); // set resolution
    ps2_wait_ack(fd);
    printf("res acked\n");
    send_byte(fd, res);
    ps2_wait_ack(fd);
}

void ps2_set_sample_rate(int fd, uint8_t rate) {
    printf("sending sample rate to 0x%02x = %d\n", rate, rate);
    send_byte(fd, 0xF3); // Sample rate
    ps2_wait_ack(fd);
    send_byte(fd, rate);
    ps2_wait_ack(fd);
}

void ps2_get_mouse_ID(int fd) {
    send_byte(fd, 0xF2); // get mouse ID
    ps2_wait_ack(fd);
    printf("ID = %d\n", read_byte(fd));
}

void ps2_init(int fd) {
    ps2_reset(fd);
    ps2_reset(fd);

    ps2_get_mouse_ID(fd);

    ps2_set_res(fd, 0x00);

    printf("scaling\n");
    send_byte(fd, 0xE6);
    ps2_wait_ack(fd);
    printf("scaling\n");
    send_byte(fd, 0xE6);
    ps2_wait_ack(fd);
    printf("scaling\n");
    send_byte(fd, 0xE6);
    ps2_wait_ack(fd);

    ps2_get_status(fd);

    ps2_set_res(fd, 0x03);

    ps2_set_sample_rate(fd, 0xC8);
    ps2_set_sample_rate(fd, 0x64);
    ps2_set_sample_rate(fd, 0x50);

    ps2_get_mouse_ID(fd);

    ps2_set_sample_rate(fd, 0xC8);
    ps2_set_sample_rate(fd, 0xC8);
    ps2_set_sample_rate(fd, 0x50);
    ps2_get_mouse_ID(fd);
    ps2_set_sample_rate(fd, 0x64);
    ps2_set_res(fd, 0x03);

    send_byte(fd, PS2_ENABLE_STREAMING);
    ps2_wait_ack(fd);

    printf("Streaming enabled.\n");
}

int main(int argc, char **argv) {
    int fd = open_serio();

    ps2_init(fd);

    while (true) {
        read_ps2_packet(fd);
    }

    return 0;
}
