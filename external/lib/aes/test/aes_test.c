/*
 * Unit tests for AES encryption/decryption.
 *
 * Test vectors from FIPS-197 Appendix C (C.1, C.2, C.3):
 * http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
 *
 * All three key sizes use the same plaintext block.
 */

#include <lib/aes.h>
#include <lib/unittest.h>

#include <stdint.h>
#include <string.h>

/* FIPS-197 Appendix C - plaintext shared by all three test cases */
static const uint8_t plaintext[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

/* C.1 AES-128 */
static const uint8_t key_128[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};
static const uint8_t ciphertext_128[] = {
    0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
    0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a
};

/* C.2 AES-192 */
static const uint8_t key_192[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
};
static const uint8_t ciphertext_192[] = {
    0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
    0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d, 0x71, 0x91
};

/* C.3 AES-256 */
static const uint8_t key_256[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
static const uint8_t ciphertext_256[] = {
    0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
    0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89
};

static bool aes128_encrypt(void) {
    BEGIN_TEST;
    AES_KEY aes_key;
    uint8_t out[AES_BLOCK_SIZE];
    memset(out, 0, sizeof(out));
    AES_set_encrypt_key(key_128, 128, &aes_key);
    AES_encrypt(plaintext, out, &aes_key);
    EXPECT_BYTES_EQ(ciphertext_128, out, AES_BLOCK_SIZE, "AES-128 encrypt");
    END_TEST;
}

static bool aes128_decrypt(void) {
    BEGIN_TEST;
    AES_KEY aes_key;
    uint8_t out[AES_BLOCK_SIZE];
    memset(out, 0, sizeof(out));
    AES_set_decrypt_key(key_128, 128, &aes_key);
    AES_decrypt(ciphertext_128, out, &aes_key);
    EXPECT_BYTES_EQ(plaintext, out, AES_BLOCK_SIZE, "AES-128 decrypt");
    END_TEST;
}

static bool aes192_encrypt(void) {
    BEGIN_TEST;
    AES_KEY aes_key;
    uint8_t out[AES_BLOCK_SIZE];
    memset(out, 0, sizeof(out));
    AES_set_encrypt_key(key_192, 192, &aes_key);
    AES_encrypt(plaintext, out, &aes_key);
    EXPECT_BYTES_EQ(ciphertext_192, out, AES_BLOCK_SIZE, "AES-192 encrypt");
    END_TEST;
}

static bool aes192_decrypt(void) {
    BEGIN_TEST;
    AES_KEY aes_key;
    uint8_t out[AES_BLOCK_SIZE];
    memset(out, 0, sizeof(out));
    AES_set_decrypt_key(key_192, 192, &aes_key);
    AES_decrypt(ciphertext_192, out, &aes_key);
    EXPECT_BYTES_EQ(plaintext, out, AES_BLOCK_SIZE, "AES-192 decrypt");
    END_TEST;
}

static bool aes256_encrypt(void) {
    BEGIN_TEST;
    AES_KEY aes_key;
    uint8_t out[AES_BLOCK_SIZE];
    memset(out, 0, sizeof(out));
    AES_set_encrypt_key(key_256, 256, &aes_key);
    AES_encrypt(plaintext, out, &aes_key);
    EXPECT_BYTES_EQ(ciphertext_256, out, AES_BLOCK_SIZE, "AES-256 encrypt");
    END_TEST;
}

static bool aes256_decrypt(void) {
    BEGIN_TEST;
    AES_KEY aes_key;
    uint8_t out[AES_BLOCK_SIZE];
    memset(out, 0, sizeof(out));
    AES_set_decrypt_key(key_256, 256, &aes_key);
    AES_decrypt(ciphertext_256, out, &aes_key);
    EXPECT_BYTES_EQ(plaintext, out, AES_BLOCK_SIZE, "AES-256 decrypt");
    END_TEST;
}

BEGIN_TEST_CASE(aes_tests)
RUN_TEST(aes128_encrypt)
RUN_TEST(aes128_decrypt)
RUN_TEST(aes192_encrypt)
RUN_TEST(aes192_decrypt)
RUN_TEST(aes256_encrypt)
RUN_TEST(aes256_decrypt)
END_TEST_CASE(aes_tests)
