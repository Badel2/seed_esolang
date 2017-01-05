#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define eprintf(format, ...) do {                 \
    if (VERBOSE_LEVEL == 1)                       \
        fprintf(stderr, format, ##__VA_ARGS__);   \
    if (VERBOSE_LEVEL == 2)                       \
        fprintf(stdout, format, ##__VA_ARGS__);   \
} while(0)

extern int VERBOSE_LEVEL;
extern void remove_useless_newline(char * c);

// This is nice but it must be heap-allocated, so I currently don't use it
typedef struct _PyLong {
    size_t len;
    uint32_t x[];
} PyLong;

void next_pylong(uint32_t * x, size_t key_len);
void byte_array_to_python_number(uint32_t* x, size_t lens);
int byte_array_to_decimal_number(uint32_t* the_seed, size_t seed_len, char * out);
