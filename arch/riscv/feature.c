/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "arch/riscv/feature.h"

#include <lk/trace.h>
#include <lk/debug.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define LOCAL_TRACE 0

// Make feature bitmap
uint32_t riscv_feature_bitmap[ROUNDUP(RISCV_FEAT_COUNT, 32) / 32];

static void set_feature(enum riscv_feature feature) {
    riscv_feature_bitmap[feature / 32] |= (1U << feature % 32);
}

// match a one word feature from a known list
static void match_feature(const char *str, size_t start, size_t end) {
    const struct {
        const char *str;
        enum riscv_feature feat;
    } oneword[] = {
        { "zba", RISCV_FEAT_ZBA },
        { "zbb", RISCV_FEAT_ZBB },
        { "zbc", RISCV_FEAT_ZBC },
        { "zbs", RISCV_FEAT_ZBS },
        { "zicbom", RISCV_FEAT_ZICBOM },
        { "zicbop", RISCV_FEAT_ZICBOP },
        { "zicboz", RISCV_FEAT_ZICBOZ },
        { "sstc", RISCV_FEAT_SSTC },
        { "svadu", RISCV_FEAT_SVADU },
        { "zicsr", RISCV_FEAT_ZICSR },
        { "zifencei", RISCV_FEAT_ZIFENCEI },
    };

    if (LOCAL_TRACE) {
        char feat[128];
        strlcpy(feat, &str[start], end - start + 1);
        printf("feature '%s'\n", feat);
    }

    for (size_t i = 0; i < countof(oneword); i++) {
        if (strlen(oneword[i].str) != end - start)
            continue;

        if (strncasecmp(oneword[i].str, &str[start], end - start) == 0) {
            dprintf(INFO, "riscv: found feature '%s'\n", oneword[i].str);
            set_feature(oneword[i].feat);
        }
    }
}

void riscv_set_isa_string(const char *str) {
    LTRACEF("%s\n", str);

    const size_t slen = strlen(str);

    // Handle simple features first.
    // Feature string must start with either 'rv64' or 'rv32' followed by
    // one or more one character features until _ or null.
    if (slen < 4 || str[0] != 'r' || str[1] != 'v') {
        return;
    }

    // We're going to continue, so wipe out the existing default feature list
    memset(riscv_feature_bitmap, 0, 4 * countof(riscv_feature_bitmap));

    size_t pos = 4;
    while (str[pos] != 0 && str[pos] != '_') {
        bool found = true;
        switch (tolower(str[pos])) {
            // TODO: make sure this is the complete list
            case 'i': set_feature(RISCV_FEAT_I); break;
            case 'm': set_feature(RISCV_FEAT_M); break;
            case 'a': set_feature(RISCV_FEAT_A); break;
            case 'q': set_feature(RISCV_FEAT_Q);
                // fallthrough
            case 'd':
feat_d:
                set_feature(RISCV_FEAT_D);
                // fallthrough
            case 'f': set_feature(RISCV_FEAT_F); break;
            case 'c': set_feature(RISCV_FEAT_C); break;
            case 'b': set_feature(RISCV_FEAT_B); break;
            case 'p': set_feature(RISCV_FEAT_P); break;
            case 'h': set_feature(RISCV_FEAT_H); break;
            case 'v':
                set_feature(RISCV_FEAT_V);
                goto feat_d;
            case 'g':
                // 'g' is a special case that implies IMAFDZisr_Zifenci
                set_feature(RISCV_FEAT_I);
                set_feature(RISCV_FEAT_M);
                set_feature(RISCV_FEAT_A);
                set_feature(RISCV_FEAT_ZICSR);
                set_feature(RISCV_FEAT_ZIFENCEI);
                goto feat_d;
            default:
                found = false;
        }
        if (found) {
            dprintf(INFO, "riscv: found feature '%c'\n", tolower(str[pos]));
        }

        pos++;
    }

    // walk the one-word features
    bool in_word = false;
    size_t start;
    for (; pos <= slen; pos++) {
        if (!in_word) {
            if (str[pos] == '_') {
                continue;
            } else if (str[pos] == 0) {
                break;
            }
            start = pos;
            in_word = true;
        } else {
            // we're in a word
            if (str[pos] == '_' || str[pos] == 0) {
                // end of word
                in_word = false;

                // process the feature word, between str[start...pos]
                match_feature(str, start, pos);
            }
        }
    }
}

void riscv_feature_early_init(void) {
    // set the default features based on the compiler switches
#if __riscv_i
    set_feature(RISCV_FEAT_I);
#endif
#if __riscv_m
    set_feature(RISCV_FEAT_M);
#endif
#if __riscv_a
    set_feature(RISCV_FEAT_A);
#endif
#if __riscv_f
    set_feature(RISCV_FEAT_F);
#endif
#if __riscv_d
    set_feature(RISCV_FEAT_D);
#endif
#if __riscv_q
    set_feature(RISCV_FEAT_Q);
#endif
#if __riscv_c
    set_feature(RISCV_FEAT_C);
#endif
#if __riscv_zba
    set_feature(RISCV_FEAT_ZBA);
#endif
#if __riscv_zbb
    set_feature(RISCV_FEAT_ZBB);
#endif
#if __riscv_zba
    set_feature(RISCV_FEAT_ZBA);
#endif
#if __riscv_zbc
    set_feature(RISCV_FEAT_ZBC);
#endif
#if __riscv_zbs
    set_feature(RISCV_FEAT_ZBS);
#endif
}

const char *riscv_feature_to_string(enum riscv_feature feature) {
    switch (feature) {
        case RISCV_FEAT_I: return "i";
        case RISCV_FEAT_M: return "m";
        case RISCV_FEAT_A: return "a";
        case RISCV_FEAT_F: return "f";
        case RISCV_FEAT_D: return "d";
        case RISCV_FEAT_Q: return "q";
        case RISCV_FEAT_C: return "c";
        case RISCV_FEAT_B: return "b";
        case RISCV_FEAT_P: return "p";
        case RISCV_FEAT_V: return "v";
        case RISCV_FEAT_H: return "h";
        case RISCV_FEAT_ZBA: return "zba";
        case RISCV_FEAT_ZBB: return "zbb";
        case RISCV_FEAT_ZBC: return "zbc";
        case RISCV_FEAT_ZBS: return "zbs";
        case RISCV_FEAT_ZICBOM: return "zicbom";
        case RISCV_FEAT_ZICBOP: return "zicbop";
        case RISCV_FEAT_ZICBOZ: return "zicboz";
        case RISCV_FEAT_ZICSR: return "zicsr";
        case RISCV_FEAT_ZIFENCEI: return "zifencei";
        case RISCV_FEAT_SSTC: return "sstc";
        case RISCV_FEAT_SVADU: return "svadu";

        // keep this in so the compiler warns if something is missing
        case RISCV_FEAT_COUNT: return "";
    }
    return "";
}

void riscv_feature_init(void) {
    dprintf(INFO, "RISCV: detected features");
    for (size_t i = 0; i < countof(riscv_feature_bitmap); i++) {
        for (size_t j = 0; j < sizeof(riscv_feature_bitmap[i]) * 8; j++) {
            if (riscv_feature_bitmap[i] & (1U << j)) {
                dprintf(INFO, " %s", riscv_feature_to_string(i * 32 + j));
            }
        }
    }
    dprintf(INFO, "\n");
}