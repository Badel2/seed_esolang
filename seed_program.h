#pragma once

#include "random.h"

void remove_useless_newline(char * c);
int write_program_to_file(FILE * output, size_t program_len, uint32_t * the_seed, size_t seed_len);
size_t seed_program_generator(const char *program, uint32_t *seed);
size_t verify_program(RandomObject *self, const char *program);
size_t verify_program_fast(RandomObject *self, const char *program_index, size_t target_len);
size_t seed_program_generator_bruteforce(const char *program, uint32_t *seed, uint16_t bruteforce);

void high_quality_bruteforce(const char *program);