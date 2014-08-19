#include <stdio.h>

#define countof(a) (sizeof(a) / sizeof((a)[0]))

#include "float_test_vec.c"

int main(void)
{
    printf("floating point printf tests\n");

    for (size_t i = 0; i < float_test_vec_size; i++) {
        PRINT_FLOAT;
    }

    return 0;
}

