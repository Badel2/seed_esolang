#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "random.h"
#include "pylong.h"
#include "seed_program.h"

#define eprintf(format, ...) do {                 \
    if (VERBOSE_LEVEL == 1)                       \
        fprintf(stderr, format, ##__VA_ARGS__);   \
    if (VERBOSE_LEVEL == 2)                       \
        fprintf(stdout, format, ##__VA_ARGS__);   \
} while(0)

int VERBOSE_LEVEL = 0;

#define DEFAULT_BRUTEFORCE_LEVEL 3
#define MAX_PROGRAM_LEN 127


void test_performance_init_by_array(){
    RandomObject r;
    uint32_t key[N] = {0xbade12};
    size_t i,j;
    for(j=0; j<1000; j++)
    for(i=0; i<N; i++){
        init_by_array(&r, key, i);
    }
}

void test_zero_generators(){
    RandomObject r;
    uint32_t key[N] = {0xbade12};
    size_t i = 0;
    for(i=0; i<1000; i++){
        eprintf("i = %zu\n", i);
        size_t keylen = M + i;
        init_by_array(&r, key, keylen);
        into_zero_generator_M(&r, keylen);
    }

    /*
    uint32_t *key3 = generic_get_seed(r.state, keylen);
    RandomObject rr;
    init_by_array(&rr, key3, keylen);
    int a = genrand_int32(&rr);
    getstate(&rr);
    free(key3);
    */


}

void test_consecutive_states(){
    RandomObject r;
    uint32_t key[N] = {0};
    for(int i=0; i<10; i++){
        key[0] = i << 29;
        init_by_array(&r, key, 1);
        getstate(&r);
    }
}

void print_help(){
    printf("Common usage:\n"
            "\techo 'befunge_program' | ./seed_esolang > seed_program.txt\n"
            "\tcat befunge_program.txt | ./seed_esolang > seed_program.txt\n"
            "\t./seed_esolang -i befunge_program.txt -o seed_program.txt\n"
            "\n"
            "Supported flags:\n"
            "\t-b\t\tenable bruteforce\n"
            "\t-B N\t\tenable bruteforce for N chars (N is an integer, default 3)\n"
            "\t-h\t\tprint this help\n"
            "\t-i filename\tuse this file as input\n"
            "\t-o filename\tuse this file as output\n"
            "\t-q\t\tquiet, don't print useless information\n"
            "\t-v\t\tverbose, print all the useless information\n"
            );
}

int main(int argc, char *argv[]){
    FILE *input = stdin;
    FILE *output = stdout;
    uint16_t bruteforce_level = 0;
    int opt;

    while((opt = getopt(argc, argv, ":B:bhi:o:qv")) != -1){
        switch(opt){
            case 'B':
                bruteforce_level = atoi(optarg);
                printf("Bruteforce on, level %d\n", bruteforce_level);
                break;
            case 'b':
                bruteforce_level = DEFAULT_BRUTEFORCE_LEVEL;
                printf("Bruteforce on, level %d\n", bruteforce_level);
                break;
            case 'h':
                print_help();
                return 0;
                break;
            case 'i':
                printf("Input: %s\n", optarg);
                input = fopen(optarg, "r");
                if(input == NULL){
                    perror("Error opening input file");
                    return 1;
                }
                break;
            case 'o':
                // when writing to a file we can be more verbose
                VERBOSE_LEVEL = 1;
                printf("Output: %s\n", optarg);
                output = fopen(optarg, "w");
                if(output == NULL){
                    perror("Error opening output file");
                    return 1;
                }
                break;
            case 'q':
                VERBOSE_LEVEL = 0;
                break;
            case 'v':
                VERBOSE_LEVEL = 2;
                break;
            case ':':
                switch(optopt){
                    // -B with no argument == -b 3
                    case 'B':
                        bruteforce_level = DEFAULT_BRUTEFORCE_LEVEL;
                        printf("Bruteforce on, level %d\n", bruteforce_level);
                        break;
                    default:
                        fprintf(stderr, "option -%c is missing a required argument\n", optopt);
                        return 1;
                }
                break;
        }
    }

    init_genrand(&init_genrand_19650218, 19650218U);

    uint32_t the_seed[N] = {0};
    char starget[MAX_PROGRAM_LEN + 1];
    //fgets(starget, sizeof(starget), stdin);
    size_t strlen_starget = fread(starget, sizeof(starget[0]), sizeof(starget), input);
    if(strlen_starget == 0){
        // On 0 byte files this used to say "Error reading input: Success"
        if(ferror(input)){
            perror("Error reading input");
            return 2;
        }else{  // The user wants a 0 byte seed program? Ok...
            eprintf("Do you really want an empty program?\n");
            fprintf(output, "0 0");
            return 0;
        }
    }
    
    //eprintf("strlen_starget: %zu\n", strlen_starget);

    if(strlen_starget >= MAX_PROGRAM_LEN){
        fprintf(stderr, "Warning: program too long, truncating to %d chars\n", MAX_PROGRAM_LEN);
        starget[MAX_PROGRAM_LEN] = '\0';
    }else{ // fread does not set the null terminator
        starget[strlen_starget] = '\0';
    }

    remove_useless_newline(starget);
    strlen_starget = strlen(starget);
    eprintf("(%zu) %s\n", strlen_starget, starget);

    size_t seed_len;
    if(bruteforce_level == 0){ 
        seed_len = seed_program_generator(starget, the_seed);
    }else{
        seed_len = seed_program_generator_bruteforce(starget, the_seed, bruteforce_level);
    }

    if(seed_len == 0){
        fprintf(stderr, "An error ocurred, probably the program is too long :(\n");
        return 0;
    }
    //byte_array_to_python_number(the_seed, seed_len);
    write_program_to_file(output, strlen_starget, the_seed, seed_len);
    //eprintf("Ok?\n");

    return 0;

}
