/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Compiles the lib/libc/string sources themselves under generic_ names, so the
// baseline is exactly the C that libc would otherwise link. The real headers are
// included first; each source's own includes are then no-ops and only its
// definition is renamed.

#include "generic_string.h"

#include <string.h>
#include <sys/types.h>

#define memcpy generic_memcpy
#include "../../lib/libc/string/memcpy.c"
#undef memcpy

#define memmove generic_memmove
#include "../../lib/libc/string/memmove.c"
#undef memmove

#define memset generic_memset
#include "../../lib/libc/string/memset.c"
#undef memset

#define memcmp generic_memcmp
#include "../../lib/libc/string/memcmp.c"
#undef memcmp

#define memchr generic_memchr
#include "../../lib/libc/string/memchr.c"
#undef memchr

#define strlen generic_strlen
#include "../../lib/libc/string/strlen.c"
#undef strlen

#define strnlen generic_strnlen
#include "../../lib/libc/string/strnlen.c"
#undef strnlen

#define strcmp generic_strcmp
#include "../../lib/libc/string/strcmp.c"
#undef strcmp

#define strncmp generic_strncmp
#include "../../lib/libc/string/strncmp.c"
#undef strncmp

#define strchr generic_strchr
#include "../../lib/libc/string/strchr.c"
#undef strchr

#define strrchr generic_strrchr
#include "../../lib/libc/string/strrchr.c"
#undef strrchr

#define strcpy generic_strcpy
#include "../../lib/libc/string/strcpy.c"
#undef strcpy
