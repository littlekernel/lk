/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Copy input file to output, prepending a MOP header enough to satisfy
// the mopd daemon used to netboot an image on a VAX.
//
// Developed against mopd daemon at https://github.com/qu1j0t3/mopd

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("not enough args");
usage:
        printf("usage %s <infile> <outfile>\n", argv[0]);
        return 1;
    }

    FILE *infp = fopen(argv[1], "rb");
    if (!infp) {
        printf("error opening file %s\n", argv[1]);
        goto usage;
    }

    FILE *outfp = fopen(argv[2], "wb");
    if (!infp) {
        printf("error opening file %s\n", argv[2]);
        goto usage;
    }

    // get length of the input file
    fseek(infp, 0, SEEK_END);
    long int len = ftell(infp);
    //printf("input file length %#lx\n", len);
    rewind(infp);

    // round up to next 512 byte boundary
    len = (len + 511) / 512 * 512;
    //printf("rounded %#lx\n", len);

    // generate the header
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));

    // main header
    uint16_t isd = 0xd4;
    uint16_t iha = 0x30;
    uint8_t hbcnt = 1;
    uint16_t image_type = 0xffff;
    memcpy(buffer + 0, &isd, sizeof(isd));
    memcpy(buffer + 2, &iha, sizeof(iha));
    memcpy(buffer + 16, &hbcnt, sizeof(hbcnt));
    memcpy(buffer + 510, &image_type, sizeof(image_type));

    // image header (at offset isd)
    uint16_t isize = len / 512;
    uint16_t load_addr = 0 / 512;

    memcpy(buffer + isd + 2, &isize, sizeof(isize));
    memcpy(buffer + isd + 4, &load_addr, sizeof(load_addr));

    // something else at iha
    uint32_t xfer_addr = 0;
    memcpy(buffer + iha, &xfer_addr, sizeof(xfer_addr));

    // write the header out
    fseek(outfp, 0, SEEK_SET);
    fwrite(buffer, sizeof(buffer), 1, outfp);

    // copy the input file to the output file
    int written = 0;
    while (!feof(infp)) {
        char buf[512];
        int count = fread(buf, 1, sizeof(buf), infp);
        if (count == 0) {
            break;
        }
        fwrite(buf, 1, count, outfp);
        written += count;
    }

    // pad it out with zeros
    while (written < len) {
        char zero = 0;
        fwrite(&zero, 1, 1, outfp);
        written++;
    }

    fclose(infp);
    fclose(outfp);

    return 0;
}
