/*
 * Unit test for HW AES encryption.
 */

#include <lib/aes.h>
#include <lib/unittest.h>

#include <stdint.h>
#include <string.h>

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

static bool aes_encrypt(void) {
    BEGIN_TEST;

    AES_KEY aes_key;
    uint8_t ciphertext[AES_BLOCK_SIZE];

    memset(ciphertext, 0, sizeof(ciphertext));
    AES_set_encrypt_key(key, 128, &aes_key);
    AES_encrypt(plaintext, ciphertext, &aes_key);

    EXPECT_BYTES_EQ(expected_ciphertext, ciphertext, AES_BLOCK_SIZE, "AES-128 encryption");

    END_TEST;
}

static bool aes_decrypt(void) {
    BEGIN_TEST;

    AES_KEY aes_key;
    uint8_t decrypted[AES_BLOCK_SIZE];

    memset(decrypted, 0, sizeof(decrypted));
    AES_set_decrypt_key(key, 128, &aes_key);
    AES_decrypt(expected_ciphertext, decrypted, &aes_key);

    EXPECT_BYTES_EQ(plaintext, decrypted, AES_BLOCK_SIZE, "AES-128 decryption");

    END_TEST;
}

BEGIN_TEST_CASE(aes_tests)
RUN_TEST(aes_encrypt)
RUN_TEST(aes_decrypt)
END_TEST_CASE(aes_tests)
