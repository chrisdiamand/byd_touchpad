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

const char *interpret_scroll(uint8_t scroll) {
    switch (scroll) {
        case 0xCA:      return "scrollup";
        case 0x36:      return "scrolldown";
        case 0x2B:      return "2down";
        case 0xD5:      return "2up";
        case 0xD6:      return "2left";
        case 0x2A:      return "2right";
        case 0xD8:      return "zoomout";
        case 0x28:      return "zoomin";
        case 0xD3:      return "3up";
        case 0x2D:      return "3down";
        case 0xD4:      return "3left";
        case 0x2C:      return "3right";
    }
    return "_";
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

    printf("%02x %03d %03d %02x ", pkt[0], pkt[1], pkt[2], pkt[3]);

    printf("%s %s %s : ", b1 & PS2_LEFT   ? "left" : "_",
                          b1 & PS2_MIDDLE ? "middle" : "_",
                          b1 & PS2_RIGHT  ? "right" : "_");
    printf("%s\n", interpret_scroll(pkt[3]));
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
    printf("Waiting for ACK...");
    wait_byte(fd, PS2_ACK);
    printf("done.\n");
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
    stat[0] = read_byte(fd);
    stat[1] = read_byte(fd);
    stat[2] = read_byte(fd);
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

// Send a sequence of bytes - each one should be ACKed before the next is sent.
static void send_seq(int fd, const uint8_t *seq, size_t len) {
    for (unsigned i = 0; i < len; ++i) {
        send_byte(fd, seq[i]);
        ps2_wait_ack(fd);
    }
}

void tp_init(int fd) {
    const uint8_t seq1[] = {0xE2, 0x00, 0xE0, 0x02, 0xE0};
    send_seq(fd, seq1, sizeof(seq1) / sizeof(*seq1));

    send_byte(fd, 0x01);
    ps2_wait_ack(fd);
    printf("R0x01: %02x\n", read_byte(fd));
    printf("R0x01: %02x\n", read_byte(fd));
    printf("R0x01: %02x\n", read_byte(fd));
    printf("R0x01: %02x\n", read_byte(fd));

    const uint8_t seq2[] = { 0xD3, 0x01, 0xD0, 0x00, 0xD0, 0x04, 0xD4, 0x01,
    0xD5, 0x01, 0xD7, 0x03, 0xD8, 0x04, 0xDA, 0x03, 0xDB, 0x02, 0xE4, 0x05,
    0xD6, 0x01, 0xDE, 0x04, 0xE3, 0x01, 0xCF, 0x00, 0xD2, 0x03, 0xE5, 0x04,
    0xD9, 0x02, 0xD9, 0x07, 0xDC, 0x03, 0xDD, 0x03, 0xDF, 0x03, 0xE1, 0x03,
    0xD1, 0x00, 0xCE, 0x00, 0xCC, 0x00, 0xE0, 0x00, 0xE2, 0x01 };

    send_seq(fd, seq2, sizeof(seq2) / sizeof(*seq2));
}

int main(int argc, char **argv) {
    int fd = open_serio();

    /*send_byte(fd, 0xE2);
    send_byte(fd, 0x00);
    ps2_wait_ack(fd);
    */

    ps2_init(fd);
    printf("    --- Changing mode ---\n");
    tp_init(fd);

    printf("Reading\n");

    while (true) {
        read_ps2_packet(fd);
        //uint8_t c = read_byte(fd);
        //printf("R: %02x = %d\n", c, c);
    }

    return 0;
}
