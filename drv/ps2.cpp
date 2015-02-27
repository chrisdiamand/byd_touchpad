#include <assert.h>
#include <stdio.h>

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

FILE *open_serio(void) {
    for (int i = 0; i < 16; i++) {
        std::string fname = "/dev/serio_raw" + std::to_string(i);
        const char *str = fname.c_str();
        FILE *ret = fopen(str, "rwb");
        if (ret != nullptr)
            return ret;
    }
    fprintf(stderr, "Error: Cannot open '/dev/serio_raw*'\n");
    exit(1);
    return nullptr;
}

void ps2_align(FILE *fp) {
    while (true) {
        uint8_t c = fgetc(fp);
        if (!(c & PS2_Y_OVERFLOW) &&
            !(c & PS2_X_OVERFLOW) &&
            (c & PS2_ALWAYS_1)) {
            ungetc(c, fp);
            return;
        }
    }
}

void read_ps2_packet(FILE *fp) {
    uint8_t pkt[4];
    size_t size = fread(pkt, 1, 4, fp);
    uint8_t b1 = pkt[0];

    if (b1 & PS2_Y_OVERFLOW || b1 & PS2_X_OVERFLOW) {
        printf("Overflow: Discarding\n");
        return;
    }

    if (b1 & PS2_ALWAYS_1) {
        printf("align ");
    } else {
        ps2_align(fp);
    }

    printf("%02x %02x %02x %02x ", pkt[0], pkt[1], pkt[2], pkt[3]);

    printf("%s %s %s\n", b1 & PS2_LEFT   ? "left" : "_",
                         b1 & PS2_MIDDLE ? "middle" : "_",
                         b1 & PS2_RIGHT  ? "right" : "_");
}

int main(int argc, char **argv) {
    FILE *fp = open_serio();

    while (true) {
        read_ps2_packet(fp);
    }

    return 0;
}
