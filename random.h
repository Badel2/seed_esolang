#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "pylong.h"

#define eprintf(format, ...) do {                 \
    if (VERBOSE_LEVEL == 1)                       \
        fprintf(stderr, format, ##__VA_ARGS__);   \
    if (VERBOSE_LEVEL == 2)                       \
        fprintf(stdout, format, ##__VA_ARGS__);   \
} while(0)

extern int VERBOSE_LEVEL;

/* Period parameters -- These are all magic.  Don't change. */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfU    /* constant vector a */
#define UPPER_MASK 0x80000000U  /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffU  /* least significant r bits */

typedef struct {
    int index;
    uint32_t state[N];
} RandomObject;

extern RandomObject init_genrand_19650218;

void getstate(RandomObject *self);
int same_state(RandomObject *r1, RandomObject *r2);
void init_genrand(RandomObject *self, uint32_t s);
void init_by_array(RandomObject *self, uint32_t init_key[], size_t key_length);
uint32_t genrand_int32(RandomObject *self);
void pregenerate_genrand(RandomObject * self, size_t x);
double random_random(RandomObject *self);
void seed_random_object(RandomObject *self, uint64_t n);
uint32_t reverse_genrand_last_step(uint32_t target);

double test_random_random(uint32_t a, uint32_t b);

uint32_t get_seed_if_key_length_1(RandomObject *self);
void reverse_init_last_step(uint32_t *mt);
uint32_t* generic_get_seed(const uint32_t *s, size_t key_length);
uint32_t* get_seed_if_key_length_N_3(uint32_t* s);
uint32_t* get_seed_if_key_length_N_1(uint32_t* s);
uint32_t* reverse_init(uint32_t* s, size_t key_length);
void into_zero_generator_N_3(RandomObject *self);
void into_zero_generator_M(RandomObject *self, size_t key_len);
