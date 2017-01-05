#include "seed_program.h"

void remove_useless_newline(char * c){
    size_t i = strlen(c);
    if(i > 0 && c[i-1] == '\n'){
        c[i-1] = '\0';
    }
}

// assumes the char is valid for a seed program
char map_char_to_index(char c){
    if(c=='\n'){
        return 95;
    }
    return c-' ';
}

// assumes i <= 95
char map_index_to_char(char i){
    if(i==95){
        return '\n';
    }
    return i+' ';
}

int write_program_to_file(FILE * output, size_t program_len, uint32_t * seed, size_t seed_len){
    int err = 0;    
    const size_t bufsize = seed_len*10;
    char seed_decimal[bufsize];
    err = byte_array_to_decimal_number(seed, seed_len, seed_decimal);
    if(err){    
        // error converting to decimal
        return err;
    }

    eprintf("Got seed! %s\n", seed_decimal);
    eprintf("Writing program to file...\n");
    err = fprintf(output, "%zu %s", program_len, seed_decimal);
    if(err < 0){
        // error writing to file
        return err;
    }
    eprintf("Done.\n");
    return 0;
}

uint32_t reverse_random_random_96(int t){
    // return an "a" such that test_random_random(a,b)*96 gives t
    // for any value of b, and only needs the 9 MSB of a
    uint32_t l;
    double limit = (double)(t+1)/96.0;
    l = limit*9007199254740992.0/67108864.0;
    l -= 0x20;
    //eprintf("l: %08X\n", l);
    // 7 bits should be enough for 96 posibilities
    // but for reasons we actually need 9...
    // so here we have an accurate representation of a and b
    // aaaaaaa00xxxxxxxxxxxxxxxxxxxxxxx xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // this may be useful in the future, when the bruteforce technique improves
    // and we can control individual bits or something
    uint32_t result = (l<<5) & 0xfe000000;
    return result;
}

int test_reverse_random_random(){
    int i;
    for(i=0; i<96; i++){
        if((int)(test_random_random(reverse_random_random_96(i),0)*96.0) != i){
            eprintf("FAil at %d\n", i);
            return i;
        }
        if((int)(test_random_random(reverse_random_random_96(i)|0x007fffff,-1)*96.0) != i){
            eprintf("FaIL at %d\n", i);
            return i+100;
        }
    }
    return 0;
}

void bruteforce_seed(uint32_t* target_mt, uint32_t* key, size_t key_length){
    // 2^32 is not that much
    // EDIT: yes it is
    if(key_length>1) return;
    const uint32_t *original_mt = init_genrand_19650218.state;
    uint32_t* mt = malloc(N*sizeof(uint32_t));
    key[0] = 1;
    int found = 0;
    int tried = 0;
    while(!found){
        memcpy(mt, original_mt, N*sizeof(uint32_t));
        size_t i=1, j=0;
        size_t k = (N>key_length ? N : key_length);
        for (; k; k--) {
            //eprintf("target mt[%d] == %lld\t", i, target_mt[i]);
            mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U))
                     + key[j] + (uint32_t)j; /* non linear */
            if(i>1 && mt[i] != target_mt[i]){
                //eprintf("i:%d\n", i);
                break;
            }
            //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
            i++; j++;
            if (i>=N) { mt[0] = mt[N-1]; i=1; }
            if (j>=key_length) j=0;
        }
        if(k==0){
            found = 1;
            //eprintf("k==0: i==%d\n", i);
        }else{
            tried++;
            if(tried%0x100000==0)
            eprintf("Not %d\n", key[0]);
            key[0]++;
        }
    }
    free(mt);
}


size_t old_boring_bruteforce(const char * program, uint32_t * seed){
    eprintf("Trying old boring bruteforce...\n");
    RandomObject r;
    size_t mx = 0;
    const size_t target_len = strlen(program);
    if (target_len == 0){
        eprintf("Wtf man...\n");
        return 0;
    }
    // we dont even need to end it with '\0'
    char program_index[target_len];
    size_t m;
    for(m=0; m<target_len; m++)
        program_index[m] = map_char_to_index(program[m]);

    seed[0] = 0;
    size_t seed_len = 1;
    while(1){
        init_by_array(&r, seed, seed_len);
        //size_t m = verify_program(&r, program);
        m = verify_program_fast(&r, program_index, target_len);
        //mx = mx > m ? mx : m;
        if(m>mx){
            mx = m;
            eprintf("%zu\n", m);
            // would gcc optimize this?
            if(m==target_len){
                break;
            }
        }
        next_pylong(seed, seed_len);
    }
    return 1;
}

// returns the length of seed
size_t seed_program_generator(const char *program, uint32_t *seed){
    size_t target_len = strlen(program);
    if(target_len < 4){
        eprintf("This is a small program. Consider bruteforcing it with the -b flag.\n");
        //return old_boring_bruteforce(program, seed);
    }

    size_t seed_len = M+target_len*2;
    if(seed_len > N-3){
        fprintf(stderr, "The technology is not there yet.\n");
        fprintf(stderr, "You would have to bruteforce %zu chars.\n", (seed_len - (N-3))/2);
        return 0;
    }    

    RandomObject r0;
    // seed is expected to be of length N
    init_by_array(&r0, seed, seed_len);
    into_zero_generator_M(&r0, seed_len);

    char target[target_len];
    uint32_t random_target[2*target_len];
    uint32_t genrand_target[2*target_len];
    size_t i;
    for(i=0; i<target_len; i++){
        target[i] = map_char_to_index(program[i]);
        //random_target[2*i] = reverse_random_random(ftarget[i]);
        random_target[2*i] = reverse_random_random_96(target[i]);
        genrand_target[2*i] = reverse_genrand_last_step(random_target[2*i]);
        random_target[2*i+1] = 0;
        genrand_target[2*i+1] = 0;
        eprintf("%zu: %d\t %08X\t %08X\n", i, target[i],
                random_target[2*i], genrand_target[2*i]);
    }

    for(i=0; i<target_len; i++){
        r0.state[M+i*2] ^= genrand_target[i*2];
    }

    uint32_t *key3 = generic_get_seed(r0.state, seed_len);
    memcpy(seed, key3, sizeof(seed[0])*seed_len);
    free(key3);

    //byte_array_to_python_number(key3, M+target_len*2);
    RandomObject rr;
    init_by_array(&rr, seed, seed_len);
    if(verify_program(&rr, program) == target_len){
        eprintf("The program should work!\n");
    }else{
        fprintf(stderr, "The program will not work :(\n");
        return 0;
    }

    return seed_len;
}

// Returns the number of correctly output characters
size_t verify_program(RandomObject *self, const char *program){
    size_t i;
    for(i = 0; program[i] != '\0'; i++){
        
        double m = random_random(self);
        int n = m*96;
        //eprintf("%d == %d\n", n, map_char_to_index(program[nnn]));
        if(n != map_char_to_index(program[i])){
            return i;
        }
        /*
        uint32_t a,b;
        eprintf("%08X\t", a=genrand_int32(&r3));
        eprintf("%08X\t", b=genrand_int32(&r3));
        eprintf(" == %.16g", test_random_random(a,b));
        eprintf(" == %.16g\n", ftarget[nnn]);
        */
        //eprintf("target: %08X set mt[%d] = %08X\n", 
         //       genrand_target[nnn*2], M+2*nnn, random_target[nnn*2] );
        //eprintf("%.16g\n", random_random(&r3));
    }
    return i;
}

// program_index[i] = map_char_to_index(program[i]);
// consumes self, changing its state and making it unusable
size_t verify_program_fast(RandomObject *self, const char *program_index, size_t target_len){
    pregenerate_genrand(self, target_len*2);
    size_t i;
    for(i=0; i<target_len; i++){
        // how about a custom random_random that doesnt generate N
        // words the first time its called
        // and only generates 'target_len'
        // "premature optimization"? nah
        // (done)
        double m = random_random(self);
        int n = m*96;
        if(n != program_index[i]){
            break;
        }
    }

    return i;
}


// returns the length of the seed
// input: program (null terminated), seed[N] (here we write the seed), number of chars to bruteforce
size_t seed_program_generator_bruteforce(const char *program, uint32_t *seed, uint16_t bruteforce){
    const size_t strlen_program = strlen(program);
    size_t target_len = strlen_program;
    //eprintf("Target_len == %zu\n", target_len);
    if(target_len <= bruteforce){
        //eprintf("This is a small program. Just bruteforce it mate.\n");
        return old_boring_bruteforce(program, seed);
    }

    target_len -= bruteforce;

    size_t seed_len = M+target_len*2;
    if(seed_len > N-3){
        fprintf(stderr, "The technology is not there yet.\n");        
        fprintf(stderr, "You would have to bruteforce %zu chars more.\n", (seed_len - (N-3))/2);
        return 0;
    }
  
    char target[strlen_program];
    char program_index[strlen_program];
    uint32_t random_target[2*strlen_program];
    uint32_t genrand_target[2*strlen_program];
    size_t i;
    for(i=0; i<strlen_program; i++){
        target[i] = map_char_to_index(program[i]);
        program_index[i] = map_char_to_index(program[i]);
        //random_target[2*i] = reverse_random_random(ftarget[i]);
        random_target[2*i] = reverse_random_random_96(target[i]);
        genrand_target[2*i] = reverse_genrand_last_step(random_target[2*i]);
        random_target[2*i+1] = 0;
        genrand_target[2*i+1] = 0;
        eprintf("%zu: %d\t %08X\t %08X\n", i, target[i],
                random_target[2*i], genrand_target[2*i]);
    }

    // seed is expected to be of length N
    
    RandomObject r0, r1;
    do {
        next_pylong(seed, seed_len);
        init_by_array(&r0, seed, seed_len);
        into_zero_generator_M(&r0, seed_len);

        for(i=0; i<target_len; i++){
            r0.state[M+i*2] ^= genrand_target[i*2];
        }

        r1 = r0;
    } while(verify_program_fast(&r1, program_index, strlen_program) < strlen_program);

    uint32_t *key3 = generic_get_seed(r0.state, seed_len);
    memcpy(seed, key3, sizeof(seed[0])*seed_len);
    free(key3);
    //byte_array_to_python_number(key3, M+target_len*2);

    return seed_len;
}

void high_quality_bruteforce(const char *program){
    RandomObject r;
    uint64_t i = 0;
    size_t mx = 0;
    const size_t target_len = strlen(program);
    if (target_len == 0){
        eprintf("Wtf man...\n");
        return;
    }
    // we dont even need to end it with '\0'
    char program_index[target_len];
    size_t m;
    for(m=0; m<target_len; m++)
        program_index[m] = map_char_to_index(program[m]);

    while(1){
        seed_random_object(&r,i);
        //size_t m = verify_program(&r, program);
        m = verify_program_fast(&r, program_index, target_len);
        //mx = mx > m ? mx : m;
        if(m>mx){
            mx = m;
            eprintf("%zu\n", m);
            // would gcc optimize this?
            if(m==target_len){
                break;
            }
        }
        i++;
    }
    eprintf("Lol, found seed:\n%llu\n",(unsigned long long)i);

}
