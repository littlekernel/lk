/*
 * Copyright (c) 2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BOOTBLOCK_SIZE 1024
#define ADF_SIZE       901120

static uint32_t read_be(const unsigned char *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) |
           ((uint32_t)p[3] << 0);
}

static void write_be(unsigned char *p, uint32_t val) {
    p[0] = (val >> 24);
    p[1] = (val >> 16);
    p[2] = (val >> 8);
    p[3] = (val >> 0);
}

static uint32_t generate_checksum(unsigned char *block) {
    uint32_t sum = 0;

    // Clear checksum
    write_be(block + 4, 0);

    for (int i = 0; i < BOOTBLOCK_SIZE; i += 4) {
        uint32_t word = read_be(block + i);
        uint32_t old = sum;

        sum += word;

        if (sum < old) {
            sum++;
        }
    }

    return ~sum;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <bootcode> <adf_output>\n", argv[0]);
        return 1;
    }

    unsigned char buf[ADF_SIZE];
    memset(buf, 0, sizeof(buf));

    FILE *bootcode = fopen(argv[1], "rb");

    if (!bootcode) {
        perror("Error opening input file");
        return 1;
    }

    size_t bytes_read = fread(buf + 12, 1, BOOTBLOCK_SIZE - 12, bootcode);

    if (ferror(bootcode)) {
        perror("Error reading input file");
        fclose(bootcode);
        return 1;
    }

    fclose(bootcode);

    if (bytes_read == 0) {
        fprintf(stderr, "Error: input bootcode binary is empty.\n");
        return 1;
    }

    // 'DOS0' type, 'DOS1' also works
    buf[0] = 'D';
    buf[1] = 'O';
    buf[2] = 'S';
    buf[3] = 0x00;

    write_be(buf + 4, 0);   // checksum
    write_be(buf + 8, 880); // root block

    uint32_t checksum = generate_checksum(buf);
    write_be(buf + 4, checksum);

    printf("Calculated checksum: 0x%08X\n", checksum);

    FILE *adf_out = fopen(argv[2], "wb");

    if (!adf_out) {
        perror("Error opening output file");
        return 1;
    }

    size_t written = fwrite(buf, 1, ADF_SIZE, adf_out);

    if (written != ADF_SIZE) {
        perror("Error writing output file");
        fclose(adf_out);
        return 1;
    }

    fclose(adf_out);

    printf("Successfully generated ADF image: %s\n", argv[2]);

    return 0;
}
