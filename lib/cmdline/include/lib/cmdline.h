/*
 * Copyright 2026 The LK Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <lk/err.h>
#include <lk/compiler.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

__BEGIN_CDECLS

/*
 * cmdline: Command-line argument parser library
 *
 * Parses Linux-style kernel command lines in format:
 *   var=value var2 nofoo console="foo; bar" debug=0x42
 *
 * Features:
 * - Lazy parsing: only scans what's needed per query
 * - Handles quoted strings ("..." or '...') with escape sequences
 * - Auto-detects integer bases (hex 0x..., octal 0..., decimal)
 * - Immutable buffer: input persists unchanged for boot lifetime
 * - Thread-safe for queries after initialization
 *
 * Escape sequences (basic ASCII):
 *   \" → "   \' → '   \\ → \   \n → newline
 *   \r → CR   \t → tab   \0 → null byte
 */

/*
 * Initialize the command-line parser with a buffer
 *
 * @buf: pointer to command line (may not be null-terminated)
 * @len: length in bytes
 *
 * Returns:
 *   0 on success
 *   ERR_ALREADY_STARTED if cmdline_init() was already called
 */
status_t cmdline_init(const char *buf, size_t len);

/*
 * Get raw value for a variable (low-level)
 *
 * Returns pointer directly into original buffer (no copy).
 * Value is NOT null-terminated or unquoted.
 *
 * @var: variable name (case-sensitive)
 * @value_out: output pointer to value
 * @out_size: output size in bytes
 *
 * Returns:
 *   0 if found
 *   ERR_NOT_FOUND if variable not present
 *   ERR_INVALID_ARGS if var or output pointers are NULL
 *
 * Example:
 *   const char *raw;
 *   size_t sz;
 *   if (cmdline_get_value("console", &raw, &sz) == 0) {
 *       // raw points to the value, may include quotes
 *   }
 */
status_t cmdline_get_value(const char *var, const char **value_out, size_t *out_size);

/*
 * Check if variable is present
 *
 * Returns true if the variable appears in command line
 * (either as bare word "foo" or with value "foo=...").
 *
 * Returns false if not present or not initialized.
 */
bool cmdline_is_present(const char *var);

/*
 * Get variable as boolean
 *
 * Dual-mode parsing:
 *   - Bare presence (e.g., "nofoo" or "debug") → true
 *   - Explicit value: "foo=true|yes|1" → true
 *                     "foo=false|no|0" → false
 *   - Value comparison is case-insensitive
 *
 * @var: variable name
 * @out: output bool pointer
 *
 * Returns:
 *   0 if found and converted
 *   ERR_NOT_FOUND if variable not present
 *   ERR_INVALID_ARGS if unparseable value or NULL arguments
 */
status_t cmdline_get_bool(const char *var, bool *out);

/*
 * Get variable as null-terminated string
 *
 * Copies value to buffer, unquoting and unescaping as needed.
 * Output is always null-terminated.
 *
 * @var: variable name
 * @buf: destination buffer
 * @buf_len: size of buffer (must include room for null terminator)
 * @actual_len_out: (optional) if non-NULL, returns unquoted value length
 *                  (not including null terminator)
 *
 * Returns:
 *   0 if found
 *   ERR_NOT_FOUND if variable not present
 *   ERR_NOT_ENOUGH_BUFFER if buffer too small for value
 *   ERR_INVALID_ARGS if NULL arguments (except actual_len_out)
 *
 * Example:
 *   char buf[256];
 *   size_t len;
 *   if (cmdline_get_string("msg", buf, sizeof(buf), &len) == 0) {
 *       printf("msg='%s' (len=%zu)\n", buf, len);
 *   }
 */
status_t cmdline_get_string(const char *var, char *buf, size_t buf_len, size_t *actual_len_out);

/*
 * Get variable as unsigned 32-bit integer
 *
 * Auto-detects base:
 *   - "0x..." or "0X..." → hexadecimal
 *   - "0..." (leading zero) → octal
 *   - otherwise → decimal
 *
 * @var: variable name
 * @out: output pointer to uint32_t
 *
 * Returns:
 *   0 if found and converted
 *   ERR_NOT_FOUND if variable not present
 *   ERR_INVALID_ARGS if unparseable or NULL arguments
 */
status_t cmdline_get_uint32(const char *var, uint32_t *out);

/*
 * Get variable as unsigned 64-bit integer
 *
 * Same base detection as cmdline_get_uint32().
 *
 * @var: variable name
 * @out: output pointer to uint64_t
 *
 * Returns:
 *   0 if found and converted
 *   ERR_NOT_FOUND if variable not present
 *   ERR_INVALID_ARGS if unparseable or NULL arguments
 */
status_t cmdline_get_uint64(const char *var, uint64_t *out);

/*
 * Get variable as signed integer
 *
 * Handles sign prefix (+/- prefix) and base detection like uint32.
 *
 * @var: variable name
 * @out: output pointer to int
 *
 * Returns:
 *   0 if found and converted
 *   ERR_NOT_FOUND if variable not present
 *   ERR_INVALID_ARGS if unparseable or NULL arguments
 */
status_t cmdline_get_int(const char *var, int *out);

/*
 * Get raw command line buffer
 *
 * Returns pointer to original command line buffer as passed to cmdline_init().
 * Guaranteed immutable for the duration of boot.
 *
 * Returns NULL if not yet initialized.
 */
const char *cmdline_get_raw_string(void);

/*
 * Get raw command line size
 *
 * Returns the length in bytes of the command line as passed to cmdline_init().
 * Returns 0 if not yet initialized.
 */
size_t cmdline_get_raw_size(void);

#if WITH_TESTS
struct cmdline_saved_state {
	const char *buf;
	size_t len;
	bool initialized;
};

/*
 * TEST ONLY: Reset command line state
 *
 * This function is for unit testing only. It resets the internal state
 * to allow re-initialization. Should never be called in production.
 * Defined for testing framework compatibility.
 */
void cmdline_reset_for_testing(void);

/*
 * TEST ONLY: Save command line parser state.
 */
void cmdline_save_state_for_testing(struct cmdline_saved_state *state);

/*
 * TEST ONLY: Restore command line parser state previously saved.
 */
void cmdline_restore_state_for_testing(const struct cmdline_saved_state *state);
#endif

__END_CDECLS
