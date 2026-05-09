/*
 * Copyright 2026 The LK Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "lib/cmdline.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Sentinel values for uninitialized state
static const char *cmdline_buf = NULL;
static size_t cmdline_len = 0;
static bool cmdline_initialized = false;

/*
 * Check if character is valid in an unquoted variable name
 * Variable names: alphanumeric, underscore, hyphen, dot
 */
static inline bool is_var_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.';
}

/*
 * Skip whitespace and return pointer to next non-whitespace or end
 */
static const char *skip_whitespace(const char *p, const char *end) {
    while (p < end && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

/* Result of a find_variable lookup. */
struct var_match {
    bool found;
    const char *value; /* raw (not yet unescaped) value bytes; not null-terminated */
    size_t value_len;  /* length of raw value */
};

/*
 * Search cmdline_buf for a variable named var_name.  Returns a struct with
 * found=true and the raw value on success, or found=false on failure.
 */
static struct var_match find_variable(const char *var_name) {
    struct var_match result = { .found = false };

    if (!cmdline_buf || !var_name) {
        return result;
    }

    const char *p = cmdline_buf;
    const char *end = cmdline_buf + cmdline_len;

    // Scan for variable
    while (p < end) {
        // Skip whitespace
        p = skip_whitespace(p, end);
        if (p >= end) {
            break;
        }

        // Start of potential variable
        const char *var_start = p;

        // Read variable name
        while (p < end && is_var_char(*p)) {
            p++;
        }

        size_t len = (size_t)(p - var_start);

        // Check if this matches the variable we're looking for
        if (len == strlen(var_name) && strncmp(var_start, var_name, len) == 0) {
            // Check what comes after: '=', whitespace, or end
            if (p < end && *p == '=') {
                p++; // skip '='

                // Now extract the value
                const char *val_start = p;

                // Determine if value is quoted
                if (p < end && (*p == '"' || *p == '\'')) {
                    char quote = *p;
                    p++; // skip opening quote

                    // Find closing quote (handle escapes)
                    while (p < end && *p != quote) {
                        if (*p == '\\' && p + 1 < end) {
                            p += 2; // skip escape sequence
                        } else {
                            p++;
                        }
                    }

                    bool quote_closed = (p < end && *p == quote);
                    if (quote_closed) {
                        p++; // skip closing quote
                    }

                    // Value is from (val_start+1) to end of quoted content.
                    // If the closing quote was missing we treat everything up to
                    // the end of the buffer as the value rather than underflowing
                    // the length to SIZE_MAX.
                    result.found = true;
                    result.value = val_start + 1;
                    result.value_len =
                        quote_closed ? (size_t)(p - val_start - 2) : (size_t)(p - val_start - 1);
                } else {
                    // Unquoted value: read until whitespace
                    while (p < end && !isspace((unsigned char)*p)) {
                        if (*p == '\\' && p + 1 < end) {
                            p += 2; // skip escape sequence
                        } else {
                            p++;
                        }
                    }

                    result.found = true;
                    result.value = val_start;
                    result.value_len = (size_t)(p - val_start);
                }

                return result;
            } else if (p >= end || isspace((unsigned char)*p)) {
                // Bare variable with no value
                result.found = true;
                result.value = "";
                result.value_len = 0;
                return result;
            }
            // If followed by something else (like ':'), not a match, keep searching
        }

        // Skip to next token boundary, respecting quoted values so that spaces
        // inside quotes don't split the token and confuse subsequent iterations.
        while (p < end && !isspace((unsigned char)*p)) {
            if (*p == '\"' || *p == '\'') {
                char q = *p++;
                while (p < end && *p != q) {
                    if (*p == '\\' && p + 1 < end) {
                        p += 2;
                    } else {
                        p++;
                    }
                }
                if (p < end) {
                    p++; // skip closing quote
                }
            } else if (*p == '\\' && p + 1 < end) {
                p += 2;
            } else {
                p++;
            }
        }
    }

    return result; // not found
}

static void cmdline_reset_state(void) {
    cmdline_buf = NULL;
    cmdline_len = 0;
    cmdline_initialized = false;
}

#if WITH_TESTS
/*
 * TEST HELPER: Reset cmdline state for testing
 * This is only used by unit tests to re-initialize cmdline between tests
 */
void cmdline_reset_for_testing(void) {
    cmdline_reset_state();
}

void cmdline_save_state_for_testing(struct cmdline_saved_state *state) {
    if (!state) {
        return;
    }

    state->buf = cmdline_buf;
    state->len = cmdline_len;
    state->initialized = cmdline_initialized;
}

void cmdline_restore_state_for_testing(const struct cmdline_saved_state *state) {
    if (!state) {
        return;
    }

    if (!state->initialized) {
        cmdline_reset_state();
        return;
    }

    cmdline_buf = state->buf;
    cmdline_len = state->len;
    cmdline_initialized = true;
}
#endif

status_t cmdline_init(const char *buf, size_t len) {
    if (cmdline_initialized) {
        return ERR_ALREADY_STARTED;
    }

    cmdline_buf = buf;
    cmdline_len = len;
    cmdline_initialized = true;

    return 0;
}

status_t cmdline_get_value(const char *var, const char **value_out_ptr, size_t *out_size) {
    if (!var || !value_out_ptr || !out_size) {
        return ERR_INVALID_ARGS;
    }

    if (!cmdline_initialized) {
        return ERR_NOT_FOUND;
    }

    struct var_match m = find_variable(var);
    if (m.found) {
        *value_out_ptr = m.value;
        *out_size = m.value_len;
        return 0;
    }

    return ERR_NOT_FOUND;
}

bool cmdline_is_present(const char *var) {
    if (!var || !cmdline_initialized) {
        return false;
    }

    return find_variable(var).found;
}

/*
 * Helper: unescape and optionally unquote a value
 * Returns number of bytes written to dst (not including null terminator if added)
 */
static size_t unescape_value(const char *src, size_t src_len, char *dst, size_t dst_len) {
    const char *src_end = src + src_len;
    const char *dst_end = dst + dst_len;
    size_t written = 0;

    while (src < src_end && dst < dst_end) {
        if (*src == '\\' && src + 1 < src_end) {
            src++;
            char c = *src;

            switch (c) {
                case 'n':
                    *dst++ = '\n';
                    break;
                case 'r':
                    *dst++ = '\r';
                    break;
                case 't':
                    *dst++ = '\t';
                    break;
                case '\\':
                    *dst++ = '\\';
                    break;
                case '"':
                    *dst++ = '"';
                    break;
                case '\'':
                    *dst++ = '\'';
                    break;
                case '0':
                    *dst++ = '\0';
                    break;
                default:
                    // Unknown escape: treat as literal
                    *dst++ = c;
                    break;
            }
            src++;
            written++;
        } else {
            *dst++ = *src++;
            written++;
        }
    }

    return written;
}

status_t cmdline_get_string(const char *var, char *buf, size_t buf_len, size_t *actual_len_out) {
    if (!var || !buf || buf_len == 0) {
        return ERR_INVALID_ARGS;
    }

    if (!cmdline_initialized) {
        return ERR_NOT_FOUND;
    }

    const char *value;
    size_t value_len;

    status_t st = cmdline_get_value(var, &value, &value_len);
    if (st != 0) {
        return st;
    }

    // Unescape the value into the output buffer, leaving one byte for the null
    // terminator.  Pass buf_len-1 as the limit so unescape_value never touches
    // buf[buf_len-1].
    size_t unescaped_len = unescape_value(value, value_len, buf, buf_len - 1);

    if (unescaped_len == buf_len - 1) {
        // The buffer is exactly full.  We cannot tell from the return value alone
        // whether the source was fully consumed (escape sequences shrink output,
        // so the raw value_len is not a reliable proxy).  Re-run with one extra
        // byte of space — buf[buf_len-1] is safe to probe here because it will
        // be unconditionally null-terminated below.
        if (unescape_value(value, value_len, buf, buf_len) > unescaped_len) {
            // The source had more data; the first pass was cut short.
            return ERR_NOT_ENOUGH_BUFFER;
        }
    }

    if (actual_len_out) {
        *actual_len_out = unescaped_len;
    }

    // Null-terminate
    buf[unescaped_len] = '\0';

    return 0;
}

status_t cmdline_get_bool(const char *var, bool *out) {
    if (!var || !out) {
        return ERR_INVALID_ARGS;
    }

    if (!cmdline_initialized) {
        return ERR_NOT_FOUND;
    }

    const char *value;
    size_t value_len;

    status_t st = cmdline_get_value(var, &value, &value_len);
    if (st != 0) {
        return st;
    }

    // Empty value (bare variable) means true
    if (value_len == 0) {
        *out = true;
        return 0;
    }

    // Parse value strings (case-insensitive)
    // True cases: "true", "yes", "1"
    if ((value_len == 4 && strncasecmp(value, "true", 4) == 0) ||
        (value_len == 3 && strncasecmp(value, "yes", 3) == 0) ||
        (value_len == 1 && strncasecmp(value, "1", 1) == 0)) {
        *out = true;
        return 0;
    }

    // False cases: "false", "no", "0"
    if ((value_len == 5 && strncasecmp(value, "false", 5) == 0) ||
        (value_len == 2 && strncasecmp(value, "no", 2) == 0) ||
        (value_len == 1 && strncasecmp(value, "0", 1) == 0)) {
        *out = false;
        return 0;
    }

    // Unrecognized value
    return ERR_INVALID_ARGS;
}

/*
 * Helper: parse unsigned integer with base detection
 * Handles: decimal, 0x/0X (hex), 0 prefix (octal)
 */
static status_t parse_unsigned_int(const char *str, size_t len, unsigned long long *out) {
    if (len == 0) {
        return ERR_INVALID_ARGS;
    }

    int base = 10;
    const char *p = str;
    const char *end = str + len;

    // Auto-detect base
    if (len >= 2 && (*p == '0') && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        base = 16;
        p += 2;
    } else if (len >= 2 && (*p == '0') && isdigit(*(p + 1))) {
        base = 8;
        p++;
    }

    // Parse the number
    unsigned long long value = 0;
    bool parsed_any = false;

    while (p < end) {
        int digit;

        if (isdigit(*p)) {
            digit = *p - '0';
        } else if (base == 16 && isxdigit(*p)) {
            digit = tolower(*p) - 'a' + 10;
        } else {
            break; // Stop at first invalid character
        }

        if (digit >= base) {
            break; // Invalid for this base
        }

        value = value * base + digit;
        parsed_any = true;
        p++;
    }

    if (!parsed_any) {
        return ERR_INVALID_ARGS;
    }

    // Reject trailing non-numeric garbage (e.g. "42abc")
    if (p < end) {
        return ERR_INVALID_ARGS;
    }

    *out = value;
    return 0;
}

status_t cmdline_get_uint32(const char *var, uint32_t *out) {
    if (!var || !out) {
        return ERR_INVALID_ARGS;
    }

    if (!cmdline_initialized) {
        return ERR_NOT_FOUND;
    }

    const char *value;
    size_t value_len;

    status_t st = cmdline_get_value(var, &value, &value_len);
    if (st != 0) {
        return st;
    }

    unsigned long long parsed;
    st = parse_unsigned_int(value, value_len, &parsed);
    if (st != 0) {
        return st;
    }

    // Check for overflow
    if (parsed > UINT32_MAX) {
        return ERR_INVALID_ARGS;
    }

    *out = (uint32_t)parsed;
    return 0;
}

status_t cmdline_get_uint64(const char *var, uint64_t *out) {
    if (!var || !out) {
        return ERR_INVALID_ARGS;
    }

    if (!cmdline_initialized) {
        return ERR_NOT_FOUND;
    }

    const char *value;
    size_t value_len;

    status_t st = cmdline_get_value(var, &value, &value_len);
    if (st != 0) {
        return st;
    }

    unsigned long long parsed;
    st = parse_unsigned_int(value, value_len, &parsed);
    if (st != 0) {
        return st;
    }

    *out = (uint64_t)parsed;
    return 0;
}

status_t cmdline_get_int(const char *var, int *out) {
    if (!var || !out) {
        return ERR_INVALID_ARGS;
    }

    if (!cmdline_initialized) {
        return ERR_NOT_FOUND;
    }

    const char *value;
    size_t value_len;

    status_t st = cmdline_get_value(var, &value, &value_len);
    if (st != 0) {
        return st;
    }

    if (value_len == 0) {
        return ERR_INVALID_ARGS;
    }

    // Handle sign
    bool negative = false;
    const char *p = value;
    size_t remaining = value_len;

    if (*p == '-') {
        negative = true;
        p++;
        remaining--;
    } else if (*p == '+') {
        p++;
        remaining--;
    }

    unsigned long long parsed;
    st = parse_unsigned_int(p, remaining, &parsed);
    if (st != 0) {
        return st;
    }

    // Check for overflow
    if (negative) {
        if (parsed > (unsigned long long)INT_MAX + 1) {
            return ERR_INVALID_ARGS;
        }
        // Avoid UB: casting INT_MAX+1 to int is undefined; use INT_MIN directly.
        *out = (parsed == (unsigned long long)INT_MAX + 1) ? INT_MIN : -(int)parsed;
    } else {
        if (parsed > INT_MAX) {
            return ERR_INVALID_ARGS;
        }
        *out = (int)parsed;
    }

    return 0;
}

const char *cmdline_get_raw_string(void) {
    return cmdline_buf;
}

size_t cmdline_get_raw_size(void) {
    return cmdline_len;
}
