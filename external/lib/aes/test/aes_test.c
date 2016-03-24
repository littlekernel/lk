/*
 * Unit test for HW AES encryption.
 */

#include <lib/aes.h>

#include <stdint.h>
#include <string.h>
#include <platform.h>
#include <debug.h>
#include <trace.h>
#include <arch/ops.h>
#include <lib/console.h>

/*
 * These sample values come from publication "FIPS-197", Appendix C.1
 * "AES-128" See
 * http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
 */
static const uint8_t plaintext[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

static const uint8_t key[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

static const uint8_t expected_ciphertext[] = {
    0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
    0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a
};

static int aes_command(int argc, const cmd_args *argv)
{
    AES_KEY aes_key;
    uint8_t ciphertext[AES_BLOCK_SIZE];

    TRACEF("Testing AES encryption.\n");
    memset(ciphertext, 0, sizeof(ciphertext));
    AES_set_encrypt_key(key, 128, &aes_key);
    lk_bigtime_t start = current_time_hires();
    AES_encrypt(plaintext, ciphertext, &aes_key);
    lk_bigtime_t end = current_time_hires();
    int elapsed = end - start;
    TRACEF("Elapsed time: %d us for 16 bytes (%d.%03d us per byte)\n",
           elapsed, elapsed / AES_BLOCK_SIZE,
           ((elapsed * 1000) / AES_BLOCK_SIZE) % 1000);
    int not_equal = memcmp(expected_ciphertext, ciphertext, AES_BLOCK_SIZE);
    if (not_equal) {
        TRACEF("Encryption failed.  Expected:\n");
        hexdump8(expected_ciphertext, sizeof(expected_ciphertext));
        TRACEF("Actual:\n");
        hexdump8(ciphertext, sizeof(ciphertext));
        TRACEF("FAILED AES encryption\n");
    } else {
        TRACEF("PASSED AES encryption\n");
    }
    return 0;
}

static int aes_bench(int argc, const cmd_args *argv)
{
    uint32_t c;
    int i;
    AES_KEY aes_key;
    uint8_t ciphertext[AES_BLOCK_SIZE];
#define ITER 1000

    memset(ciphertext, 0, sizeof(ciphertext));

    c = arch_cycle_count();

    for (i = 0; i < ITER; i++) {
        AES_set_encrypt_key(key, 128, &aes_key);
    }

    c = arch_cycle_count() - c;
    printf("%u cycles to set encryption key\n", c / ITER);

    c = arch_cycle_count();
    for (i = 0; i < ITER; i++) {
        AES_encrypt(plaintext, ciphertext, &aes_key);
    }
    c = arch_cycle_count() - c;

    printf("%u cycles to encrypt block of 16 bytes\n", c / ITER);

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("aes_test", "test AES encryption", &aes_command)
STATIC_COMMAND("aes_bench", "bench AES encryption", &aes_bench)
STATIC_COMMAND_END(aes_test);
