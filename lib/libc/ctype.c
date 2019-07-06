/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <ctype.h>

int isblank(int c) {
    return (c == ' ' || c == '\t');
}

int isspace(int c) {
    return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v');
}

int islower(int c) {
    return ((c >= 'a') && (c <= 'z'));
}

int isupper(int c) {
    return ((c >= 'A') && (c <= 'Z'));
}

int isdigit(int c) {
    return ((c >= '0') && (c <= '9'));
}

int isalpha(int c) {
    return isupper(c) || islower(c);
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isxdigit(int c) {
    return isdigit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F'));
}

int isgraph(int c) {
    return ((c > ' ') && (c < 0x7f));
}

int iscntrl(int c) {
    return ((c < ' ') || (c == 0x7f));
}

int isprint(int c) {
    return ((c >= 0x20) && (c < 0x7f));
}

int ispunct(int c) {
    return isgraph(c) && (!isalnum(c));
}

int tolower(int c) {
    if ((c >= 'A') && (c <= 'Z'))
        return c + ('a' - 'A');
    return c;
}

int toupper(int c) {
    if ((c >= 'a') && (c <= 'z'))
        return c + ('A' - 'a');
    return c;
}

