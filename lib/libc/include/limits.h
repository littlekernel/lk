#pragma once

/* use the GCC builtins until we need something ourselves */
#if !defined(_GCC_LIMITS_H_)
#include_next <limits.h>
#endif

#ifndef LLONG_MIN
#define LLONG_MIN (-__LONG_LONG_MAX__ - 1)
#endif

#ifndef LLONG_MAX
#define LLONG_MAX __LONG_LONG_MAX__
#endif

#ifndef ULLONG_MAX
#define ULLONG_MAX (__LONG_LONG_MAX__ * 2ULL + 1)
#endif
