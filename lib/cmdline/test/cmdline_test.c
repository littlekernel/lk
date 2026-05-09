/*
 * Copyright 2026 The LK Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/cmdline.h>
#include <lib/unittest.h>
#include <stdio.h>
#include <string.h>

static const char *test_cmdline_basic = "foo=bar debug nofoo count=42";

static bool test_cmdline_basic_init(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    // Reset state by calling init
    status_t st = cmdline_init(test_cmdline_basic, strlen(test_cmdline_basic));
    EXPECT_EQ(st, 0, "cmdline_init should succeed");

    // Second init should fail
    st = cmdline_init("other", 5);
    EXPECT_EQ(st, ERR_ALREADY_STARTED, "second init should fail with ERR_ALREADY_STARTED");

    END_TEST;
}

static bool test_cmdline_basic_get_value(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    cmdline_init(test_cmdline_basic, strlen(test_cmdline_basic));

    const char *val;
    size_t len;

    // Get existing value
    status_t st = cmdline_get_value("foo", &val, &len);
    EXPECT_EQ(st, 0, "should find foo");
    EXPECT_EQ(len, 3U, "foo value should be 3 bytes");
    EXPECT_BYTES_EQ((const uint8_t *)"bar", (const uint8_t *)val, 3, "foo value should be 'bar'");

    // Get bare variable
    st = cmdline_get_value("debug", &val, &len);
    EXPECT_EQ(st, 0, "should find debug");
    EXPECT_EQ(len, 0U, "debug value should be empty");

    // Get nonexistent
    st = cmdline_get_value("missing", &val, &len);
    EXPECT_EQ(st, ERR_NOT_FOUND, "missing var should return ERR_NOT_FOUND");

    // NULL arguments
    st = cmdline_get_value(NULL, &val, &len);
    EXPECT_EQ(st, ERR_INVALID_ARGS, "NULL var should return ERR_INVALID_ARGS");

    END_TEST;
}

static bool test_cmdline_is_present(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    cmdline_init(test_cmdline_basic, strlen(test_cmdline_basic));

    EXPECT_TRUE(cmdline_is_present("foo"), "foo should be present");
    EXPECT_TRUE(cmdline_is_present("debug"), "debug should be present");
    EXPECT_TRUE(cmdline_is_present("nofoo"), "nofoo should be present");
    EXPECT_FALSE(cmdline_is_present("missing"), "missing should not be present");

    END_TEST;
}

static bool test_cmdline_get_bool(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "flag1 noflag flag2=true flag3=false flag4=yes flag5=no flag6=1 flag7=0";
    cmdline_init(cmdline, strlen(cmdline));

    bool b;

    // Bare flag
    EXPECT_EQ(cmdline_get_bool("flag1", &b), 0, "flag1 should exist");
    EXPECT_TRUE(b, "bare flag should be true");

    // Bare negation
    EXPECT_EQ(cmdline_get_bool("noflag", &b), 0, "noflag should exist");
    EXPECT_TRUE(b, "noflag bare word is true");

    // Explicit true
    EXPECT_EQ(cmdline_get_bool("flag2", &b), 0, "flag2 should exist");
    EXPECT_TRUE(b, "flag2=true should be true");

    EXPECT_EQ(cmdline_get_bool("flag4", &b), 0, "flag4 should exist");
    EXPECT_TRUE(b, "flag4=yes should be true");

    EXPECT_EQ(cmdline_get_bool("flag6", &b), 0, "flag6 should exist");
    EXPECT_TRUE(b, "flag6=1 should be true");

    // Explicit false
    EXPECT_EQ(cmdline_get_bool("flag3", &b), 0, "flag3 should exist");
    EXPECT_FALSE(b, "flag3=false should be false");

    EXPECT_EQ(cmdline_get_bool("flag5", &b), 0, "flag5 should exist");
    EXPECT_FALSE(b, "flag5=no should be false");

    EXPECT_EQ(cmdline_get_bool("flag7", &b), 0, "flag7 should exist");
    EXPECT_FALSE(b, "flag7=0 should be false");

    // Non-existent
    EXPECT_EQ(cmdline_get_bool("missing", &b), ERR_NOT_FOUND,
              "missing should return ERR_NOT_FOUND");

    // Invalid value
    cmdline_reset_for_testing();
    const char *bad_cmdline = "invalid=maybe";
    cmdline_init(bad_cmdline, strlen(bad_cmdline));
    EXPECT_EQ(cmdline_get_bool("invalid", &b), ERR_INVALID_ARGS,
              "unparseable value should return ERR_INVALID_ARGS");

    END_TEST;
}

static bool test_cmdline_get_string(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "simple=hello msg=\"hello world\" escaped=\"foo\\nbar\"";
    cmdline_init(cmdline, strlen(cmdline));

    char buf[256];
    size_t len;

    // Simple string
    EXPECT_EQ(cmdline_get_string("simple", buf, sizeof(buf), &len), 0, "should find simple");
    EXPECT_EQ(len, 5U, "simple should be 5 bytes");
    EXPECT_EQ(0, strcmp(buf, "hello"), "simple should be 'hello'");

    // Quoted string
    EXPECT_EQ(cmdline_get_string("msg", buf, sizeof(buf), &len), 0, "should find msg");
    EXPECT_EQ(len, 11U, "msg should be 11 bytes");
    EXPECT_EQ(0, strcmp(buf, "hello world"), "msg should be 'hello world'");

    // Escaped string
    EXPECT_EQ(cmdline_get_string("escaped", buf, sizeof(buf), &len), 0, "should find escaped");
    EXPECT_EQ(len, 7U, "escaped should be 7 bytes (foo\\nbar = 3+1+3)");
    EXPECT_EQ(buf[3], '\n', "escaped should contain actual newline at position 3");

    // Buffer too small
    EXPECT_EQ(cmdline_get_string("simple", buf, 3, NULL), ERR_NOT_ENOUGH_BUFFER,
              "small buffer should return ERR_NOT_ENOUGH_BUFFER");

    END_TEST;
}

static bool test_cmdline_get_uint32(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "dec=42 hex=0x2a HEX=0xFF octal=052 leading_zero=0xFF";
    cmdline_init(cmdline, strlen(cmdline));

    uint32_t val;

    // Decimal
    EXPECT_EQ(cmdline_get_uint32("dec", &val), 0, "should parse dec");
    EXPECT_EQ(val, 42U, "dec=42");

    // Hex (lowercase 0x)
    EXPECT_EQ(cmdline_get_uint32("hex", &val), 0, "should parse hex");
    EXPECT_EQ(val, 0x2aU, "hex=0x2a");

    // Hex (uppercase 0X)
    EXPECT_EQ(cmdline_get_uint32("HEX", &val), 0, "should parse HEX");
    EXPECT_EQ(val, 0xFFU, "HEX=0xFF");

    // Octal (leading zero)
    EXPECT_EQ(cmdline_get_uint32("octal", &val), 0, "should parse octal");
    EXPECT_EQ(val, 052U, "octal=052");

    // Non-existent
    EXPECT_EQ(cmdline_get_uint32("missing", &val), ERR_NOT_FOUND,
              "missing should return ERR_NOT_FOUND");

    // Invalid
    cmdline_reset_for_testing();
    const char *bad = "invalid=abc";
    cmdline_init(bad, strlen(bad));
    EXPECT_EQ(cmdline_get_uint32("invalid", &val), ERR_INVALID_ARGS,
              "invalid should return ERR_INVALID_ARGS");

    END_TEST;
}

static bool test_cmdline_get_uint64(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "big=0x123456789abcdef";
    cmdline_init(cmdline, strlen(cmdline));

    uint64_t val;

    EXPECT_EQ(cmdline_get_uint64("big", &val), 0, "should parse large hex");
    EXPECT_EQ(val, 0x123456789abcdefULL, "big hex value");

    END_TEST;
}

static bool test_cmdline_get_int(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "pos=42 neg=-42 zero=0 hex=0x10 neg_hex=-0x10";
    cmdline_init(cmdline, strlen(cmdline));

    int val;

    // Positive
    EXPECT_EQ(cmdline_get_int("pos", &val), 0, "should parse pos");
    EXPECT_EQ(val, 42, "pos=42");

    // Negative
    EXPECT_EQ(cmdline_get_int("neg", &val), 0, "should parse neg");
    EXPECT_EQ(val, -42, "neg=-42");

    // Zero
    EXPECT_EQ(cmdline_get_int("zero", &val), 0, "should parse zero");
    EXPECT_EQ(val, 0, "zero=0");

    // Hex
    EXPECT_EQ(cmdline_get_int("hex", &val), 0, "should parse hex");
    EXPECT_EQ(val, 0x10, "hex=0x10");

    // Negative hex
    EXPECT_EQ(cmdline_get_int("neg_hex", &val), 0, "should parse neg_hex");
    EXPECT_EQ(val, -0x10, "neg_hex=-0x10");

    END_TEST;
}

static bool test_cmdline_quoted_values(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "a=\"quoted\" b='single' c=unquoted d=\"with spaces\"";
    cmdline_init(cmdline, strlen(cmdline));

    char buf[256];

    EXPECT_EQ(cmdline_get_string("a", buf, sizeof(buf), NULL), 0, "should get double quoted");
    EXPECT_EQ(0, strcmp(buf, "quoted"), "double quoted value");

    EXPECT_EQ(cmdline_get_string("b", buf, sizeof(buf), NULL), 0, "should get single quoted");
    EXPECT_EQ(0, strcmp(buf, "single"), "single quoted value");

    EXPECT_EQ(cmdline_get_string("c", buf, sizeof(buf), NULL), 0, "should get unquoted");
    EXPECT_EQ(0, strcmp(buf, "unquoted"), "unquoted value");

    EXPECT_EQ(cmdline_get_string("d", buf, sizeof(buf), NULL), 0, "should get quoted with spaces");
    EXPECT_EQ(0, strcmp(buf, "with spaces"), "quoted value with spaces");

    END_TEST;
}

static bool test_cmdline_escapes(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "newline=\"hello\\nworld\" quote=\"say \\\"hi\\\"\" "
                          "backslash=\"path\\\\to\\\\file\" tab=\"a\\tb\"";
    cmdline_init(cmdline, strlen(cmdline));

    char buf[256];
    size_t len;

    EXPECT_EQ(cmdline_get_string("newline", buf, sizeof(buf), &len), 0, "should get newline");
    EXPECT_EQ(len, 11U, "newline length should be 11");
    EXPECT_EQ(buf[5], '\n', "should have actual newline");

    EXPECT_EQ(cmdline_get_string("quote", buf, sizeof(buf), &len), 0, "should get quote");
    EXPECT_EQ(0, strcmp(buf, "say \"hi\""), "escaped quotes should be unescaped");

    EXPECT_EQ(cmdline_get_string("backslash", buf, sizeof(buf), &len), 0, "should get backslash");
    EXPECT_EQ(0, strcmp(buf, "path\\to\\file"), "escaped backslashes");

    EXPECT_EQ(cmdline_get_string("tab", buf, sizeof(buf), &len), 0, "should get tab");
    EXPECT_EQ(buf[1], '\t', "should have actual tab");

    END_TEST;
}

static bool test_cmdline_raw_access(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "foo=bar debug";
    size_t cmdline_len = strlen(cmdline);

    cmdline_init(cmdline, cmdline_len);

    const char *raw = cmdline_get_raw_string();
    EXPECT_EQ(raw, cmdline, "raw string should match");

    size_t len = cmdline_get_raw_size();
    EXPECT_EQ(len, cmdline_len, "raw size should match");

    END_TEST;
}

static bool test_cmdline_empty_values(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "empty= other=value";
    cmdline_init(cmdline, strlen(cmdline));

    const char *val;
    size_t len;

    EXPECT_EQ(cmdline_get_value("empty", &val, &len), 0, "should find empty");
    EXPECT_EQ(len, 0U, "empty should have 0 length");

    END_TEST;
}

static bool test_cmdline_duplicates(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "var=first var=second";
    cmdline_init(cmdline, strlen(cmdline));

    const char *val;
    size_t len;

    EXPECT_EQ(cmdline_get_value("var", &val, &len), 0, "should find var");
    EXPECT_BYTES_EQ((const uint8_t *)"first", (const uint8_t *)val, 5,
                    "first occurrence should win");

    END_TEST;
}

/*
 * Regression: unterminated quoted value must not overflow.
 * Before the fix, `var="` produced value_len = SIZE_MAX.
 */
static bool test_cmdline_unterminated_quote(void) {
    BEGIN_TEST;

    // Lone opening quote - no closing quote at all
    cmdline_reset_for_testing();
    const char *cmdline = "var=\"";
    cmdline_init(cmdline, strlen(cmdline));

    char buf[32];
    size_t len;
    // Should succeed with an empty (or truncated-to-buffer) value, not crash.
    status_t st = cmdline_get_string("var", buf, sizeof(buf), &len);
    EXPECT_EQ(st, 0, "unterminated quote: should return a value, not crash");
    EXPECT_EQ(len, 0U, "unterminated quote with no content should have zero length");

    // Value with content but no closing quote
    cmdline_reset_for_testing();
    const char *cmdline2 = "var=\"hello";
    cmdline_init(cmdline2, strlen(cmdline2));

    st = cmdline_get_string("var", buf, sizeof(buf), &len);
    EXPECT_EQ(st, 0, "unterminated non-empty quote: should return value");
    EXPECT_EQ(len, 5U, "unterminated quote: value length should be 5");
    EXPECT_EQ(0, strcmp(buf, "hello"), "unterminated quote: value should be 'hello'");

    END_TEST;
}

/*
 * Regression: a variable following a token whose value contains quoted spaces
 * must still be found correctly.
 * Before the fix, the naive whitespace-skip broke on the space inside quotes.
 */
static bool test_cmdline_quoted_spaces_then_lookup(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "first=\"one two\" target=99 last=end";
    cmdline_init(cmdline, strlen(cmdline));

    uint32_t val;
    EXPECT_EQ(cmdline_get_uint32("target", &val), 0,
              "target after quoted-space token should be found");
    EXPECT_EQ(val, 99U, "target value should be 99");

    char buf[32];
    EXPECT_EQ(cmdline_get_string("last", buf, sizeof(buf), NULL), 0,
              "last after quoted-space token should be found");
    EXPECT_EQ(0, strcmp(buf, "end"), "last value should be 'end'");

    // Also verify the quoted value itself is correct
    EXPECT_EQ(cmdline_get_string("first", buf, sizeof(buf), NULL), 0, "first should be found");
    EXPECT_EQ(0, strcmp(buf, "one two"), "first value should be 'one two'");

    END_TEST;
}

/*
 * Regression: cmdline_get_string must return ERR_NOT_ENOUGH_BUFFER when the
 * value length exactly equals buf_len (no room for null terminator).
 * Before the fix the condition used '>' instead of '>=' and silently truncated.
 */
static bool test_cmdline_string_exact_buffer_size(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    // value "hello" is 5 bytes
    const char *cmdline = "var=hello";
    cmdline_init(cmdline, strlen(cmdline));

    // Buffer of exactly 5 bytes: fits 4 chars + null — must truncate "hello"
    char buf5[5];
    EXPECT_EQ(cmdline_get_string("var", buf5, sizeof(buf5), NULL), ERR_NOT_ENOUGH_BUFFER,
              "buf exactly = value length should return ERR_NOT_ENOUGH_BUFFER");

    // Buffer of 6 bytes: just enough for "hello\0"
    char buf6[6];
    size_t len;
    EXPECT_EQ(cmdline_get_string("var", buf6, sizeof(buf6), &len), 0,
              "buf = value length + 1 should succeed");
    EXPECT_EQ(len, 5U, "length should be 5");
    EXPECT_EQ(0, strcmp(buf6, "hello"), "value should be 'hello'");

    END_TEST;
}

/*
 * Regression: INT_MIN (-2147483648) must parse without undefined behaviour.
 */
static bool test_cmdline_get_int_min(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "minval=-2147483648 maxval=2147483647 overflow=-2147483649";
    cmdline_init(cmdline, strlen(cmdline));

    int val;

    EXPECT_EQ(cmdline_get_int("minval", &val), 0, "INT_MIN should parse");
    EXPECT_EQ(val, -2147483648, "minval should be INT_MIN");

    EXPECT_EQ(cmdline_get_int("maxval", &val), 0, "INT_MAX should parse");
    EXPECT_EQ(val, 2147483647, "maxval should be INT_MAX");

    EXPECT_EQ(cmdline_get_int("overflow", &val), ERR_INVALID_ARGS,
              "value below INT_MIN should overflow");

    END_TEST;
}

/*
 * Regression: trailing non-numeric garbage must be rejected.
 */
static bool test_cmdline_int_trailing_garbage(void) {
    BEGIN_TEST;

    cmdline_reset_for_testing();
    const char *cmdline = "bad=42abc ok=42";
    cmdline_init(cmdline, strlen(cmdline));

    uint32_t val;
    EXPECT_EQ(cmdline_get_uint32("bad", &val), ERR_INVALID_ARGS,
              "trailing alpha chars should return ERR_INVALID_ARGS");
    EXPECT_EQ(cmdline_get_uint32("ok", &val), 0, "clean integer should still parse");
    EXPECT_EQ(val, 42U, "ok should be 42");

    END_TEST;
}

/*
 * Regression: escape sequences that shrink output must not produce a
 * false-positive ERR_NOT_ENOUGH_BUFFER.
 *
 * Each escape sequence is 2 raw bytes but produces 1 decoded byte.  The old
 * truncation check used `value_len >= buf_len` as a proxy, which fired even
 * when the decoded result fit in the buffer.  The fix re-runs unescape_value
 * with one extra byte of headroom to detect actual truncation.
 */
static bool test_cmdline_escape_shrink_no_false_truncation(void) {
    BEGIN_TEST;

    // "\\n" is 2 raw bytes but decodes to 1 byte ('\n').
    // A 2-byte buffer (1 data byte + null terminator) must succeed.
    cmdline_reset_for_testing();
    const char *cmdline1 = "key=\\n";
    cmdline_init(cmdline1, strlen(cmdline1));

    char buf2[2];
    size_t len;
    EXPECT_EQ(cmdline_get_string("key", buf2, sizeof(buf2), &len), 0,
              "single escaped byte fits in 2-byte buffer");
    EXPECT_EQ(len, 1U, "decoded length should be 1");
    EXPECT_EQ(buf2[0], '\n', "decoded value should be newline");

    // Three escapes decode to 3 bytes.  A 4-byte buffer must succeed.
    cmdline_reset_for_testing();
    const char *cmdline2 = "key=\\n\\r\\t";
    cmdline_init(cmdline2, strlen(cmdline2));

    char buf4[4];
    EXPECT_EQ(cmdline_get_string("key", buf4, sizeof(buf4), &len), 0,
              "three escaped bytes fit in 4-byte buffer");
    EXPECT_EQ(len, 3U, "decoded length should be 3");
    EXPECT_EQ(buf4[0], '\n', "first decoded byte should be newline");
    EXPECT_EQ(buf4[1], '\r', "second decoded byte should be CR");
    EXPECT_EQ(buf4[2], '\t', "third decoded byte should be tab");

    // Same three escapes must NOT fit in a 3-byte buffer (3 data bytes leave
    // no room for the null terminator).
    cmdline_reset_for_testing();
    cmdline_init(cmdline2, strlen(cmdline2));

    char buf3[3];
    EXPECT_EQ(cmdline_get_string("key", buf3, sizeof(buf3), NULL), ERR_NOT_ENOUGH_BUFFER,
              "three decoded bytes do not fit in 3-byte buffer");

    END_TEST;
}

/*
 * Regression: bytes >= 0x80 in the cmdline must not invoke undefined
 * behaviour through the ctype.h functions.
 *
 * Before the fix, plain `char` values were passed directly to isalnum() and
 * isspace().  On platforms where `char` is signed that is UB for bytes > 0x7f.
 * The fix adds `(unsigned char)` casts throughout.
 *
 * Observable behaviour checked here:
 *   - A high byte is not a valid variable-name character, so it acts as a
 *     token boundary inside a name.
 *   - A high byte inside an unquoted value is included in the value (it is
 *     not whitespace in the C locale).
 */
static bool test_cmdline_high_byte_chars(void) {
    BEGIN_TEST;

    // High byte embedded in an unquoted value: the entire 3-byte sequence
    // {a, 0x80, b} should be returned as the value.
    cmdline_reset_for_testing();
    const char cmdline_val[] = { 'k', 'e', 'y', '=', 'a', '\x80', 'b', '\0' };
    cmdline_init(cmdline_val, strlen(cmdline_val));

    const char *val;
    size_t len;
    EXPECT_EQ(cmdline_get_value("key", &val, &len), 0,
              "variable with high-byte value should be found");
    EXPECT_EQ(len, 3U, "value should be 3 bytes (a, 0x80, b)");

    // High byte inside a variable name breaks the name at that position, so
    // "k\x80ey=val" does not define a variable named "key".
    cmdline_reset_for_testing();
    const char cmdline_name[] = { 'k', '\x80', 'e', 'y', '=', 'v', 'a', 'l',
                                  ' ', 'r',    'e', 'a', 'l', '=', '1', '\0' };
    cmdline_init(cmdline_name, strlen(cmdline_name));

    EXPECT_EQ(cmdline_get_value("key", &val, &len), ERR_NOT_FOUND,
              "broken name with high-byte must not match 'key'");
    // A well-formed variable after the malformed token must still be found.
    EXPECT_EQ(cmdline_get_value("real", &val, &len), 0,
              "normal variable after high-byte token should be found");
    EXPECT_EQ(len, 1U, "real value length should be 1");

    END_TEST;
}

// Test suite
BEGIN_TEST_CASE(cmdline_tests)
RUN_TEST(test_cmdline_basic_init)
RUN_TEST(test_cmdline_basic_get_value)
RUN_TEST(test_cmdline_is_present)
RUN_TEST(test_cmdline_get_bool)
RUN_TEST(test_cmdline_get_string)
RUN_TEST(test_cmdline_get_uint32)
RUN_TEST(test_cmdline_get_uint64)
RUN_TEST(test_cmdline_get_int)
RUN_TEST(test_cmdline_quoted_values)
RUN_TEST(test_cmdline_escapes)
RUN_TEST(test_cmdline_raw_access)
RUN_TEST(test_cmdline_empty_values)
RUN_TEST(test_cmdline_duplicates)
RUN_TEST(test_cmdline_unterminated_quote)
RUN_TEST(test_cmdline_quoted_spaces_then_lookup)
RUN_TEST(test_cmdline_string_exact_buffer_size)
RUN_TEST(test_cmdline_get_int_min)
RUN_TEST(test_cmdline_int_trailing_garbage)
RUN_TEST(test_cmdline_escape_shrink_no_false_truncation)
RUN_TEST(test_cmdline_high_byte_chars)
END_TEST_CASE(cmdline_tests)
