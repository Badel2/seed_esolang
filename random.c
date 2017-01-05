/* half of this code was copied from cpython's random implementation
      https://github.com/python/cpython/blob/master/Modules/_randommodule.c

   This was on that file:
 ------------------------------------------------------------------
   The code in this module was based on a download from:
      http://www.math.keio.ac.jp/~matumoto/MT2002/emt19937ar.html
   It was modified in 2002 by Raymond Hettinger as follows:
    * the principal computational lines untouched.
    * renamed genrand_res53() to random_random() and wrapped
      in python calling/return code.
    * genrand_int32() and the helper functions, init_genrand()
      and init_by_array(), were declared static, wrapped in
      Python calling/return code.  also, their global data
      references were replaced with structure references.
    * unused functions from the original were deleted.
      new, original C python code was added to implement the
      Random() interface.
   The following are the verbatim comments from the original code:
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.
   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).
   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
     1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
     3. The names of its contributors may not be used to endorse or promote
    products derived from this software without specific prior written
    permission.
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp
*/

/* ---------------------------------------------------------------*/

/* In 2016 I (Badel2) modified this code to allow easier bruteforce, I reversed
 * most of the functions, first to make it possible to obtain the seed from a seeded
 * RandomObject, then to modify the RandomObject to output desired values (zeros) and
 * finally I added a small optimization to precalculate only the needed genrand32 values
 * by random_random. I left some legacy and debug functions for future interest. */


#include "random.h"

/* since init_genrand is only ever used with one argument, we precalculate it
 * and use this cached result in init_by_array and other functions */

RandomObject init_genrand_19650218;

void getstate(RandomObject *self){
    int i;
    for(i=0; i<N; i++){
        eprintf("%08X, ",self->state[i]);
    }
    eprintf("\n");
}

int same_state(RandomObject *r1, RandomObject *r2){
    int i;
    for(i=0; i<N; i++){
        if(r1->state[i] != r2->state[i]){
            return 0;
        }
    }
    return 1;
}

void init_genrand(RandomObject *self, uint32_t s)
{
    int mti;
    uint32_t *mt;

    mt = self->state;
    mt[0]= s;
    for (mti=1; mti<N; mti++) {
        mt[mti] =
        (1812433253U * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                                */
        /* 2002-01-09 modified by Makoto Matsumoto                     */
    }
    self->index = mti;
    return;
}

void init_by_array(RandomObject *self, uint32_t init_key[], size_t key_length)
{
    size_t i, j, k;       /* was signed in the original code. RDH 2002-12-16 */
    uint32_t *mt;

    mt = self->state;
    //init_genrand(self, 19650218U);
    //memcpy(self, &init_genrand_19650218, sizeof(*self));
    self->index = N;
    const uint32_t *omt = init_genrand_19650218.state;
    //getstate(self);
    i=1; j=0;
    // get the first round from the static array omt:
    k = N-1;
    mt[0] = omt[0];
    for (; k; k--) {
        //eprintf("j=%d original mt[%d] == %lld\t", j, i, mt[i]);
        mt[i] = (omt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U))
                 + init_key[j] + (uint32_t)j; /* non linear */
        //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i++; j++;
        if (j>=key_length) j=0;
    }
    mt[0] = mt[N-1]; i=1;
    k = (N>key_length ? 1 : key_length-N+1);
    for (; k; k--) {
        //eprintf("j=%d original mt[%d] == %lld\t", j, i, mt[i]);
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U))
                 + init_key[j] + (uint32_t)j; /* non linear */
        //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }

    //eprintf("After first round:\n");
    //getstate(self);
    // Until here we have complete control of mt[] except mt[1]
    // unless key_length == N+1
    // Well, we actually have complete control of the entire state,
    // because the seed is a PyLong, an infinitely large integer.
    // Nice. Now we only need to find the state.
    for (k=N-1; k; k--) {
        //eprintf("original mt[%d] == %lld\t", i, mt[i]);
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941U))
                 - (uint32_t)i; /* non linear */
        //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000U; /* MSB is 1; assuring non-zero initial array */
    //getstate(self);
}

uint32_t genrand_int32(RandomObject *self)
{
    uint32_t y;
    static const uint32_t mag01[2] = {0x0U, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */
    uint32_t *mt;

    mt = self->state;
    if (self->index >= N) { /* generate N words at one time */
        // y = (mt[kk] & 0x80000000) | (mt[(kk+1)%N] & 0x7FFFFFFF)
        // mt[kk] = mt[(kk+M)%N] ^ (y >> 1) ^ mag01[y & 1]
        // this was signed
        size_t kk;

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1U];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1U];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1U];

        self->index = 0;
    }

    y = mt[self->index++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= (y >> 18);
    return y;
}

// calculates the first x elements used later by random_random
void pregenerate_genrand(RandomObject * self, size_t x){
    uint32_t y;
    static const uint32_t mag01[2] = {0x0U, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */
    uint32_t *mt = self->state;

    // y = (mt[kk] & 0x80000000) | (mt[(kk+1)%N] & 0x7FFFFFFF)
    // mt[kk] = mt[(kk+M)%N] ^ (y >> 1) ^ mag01[y & 1]
    size_t kk;

    if(x <= (N-M)){
        for (kk=0;kk<x;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1U];
        }
    } else {
        for (kk=0;kk<(N-M);kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1U];
        }
    
        if(x <= (N-1)){
            for (;kk<x;kk++) {
                y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
                mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1U];
            }
        } else {
            for (;kk<N-1;kk++) {
                y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
                mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1U];
            }
            if(x == N){
                y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
                mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1U];
            }  
            if(x >= N){
                eprintf("Dude, you can't use this optimization here...");
            }
        }
    }

    self->index = 0;
}


double random_random(RandomObject *self)
{
    uint32_t a=genrand_int32(self)>>5, b=genrand_int32(self)>>6;
    double c = ((a*67108864.0+b)*(1.0/9007199254740992.0)); //PyFloat_FromDouble
    //eprintf("a: %08X b: %08X c: %016llX", a, b, c);
    return c;
}

// This implementation only allows up to 64 bit seeds.
// If you want something longer, just use init_by_array
void seed_random_object(RandomObject *self, uint64_t n){
    //size_t bits, keyused;
    //bits = _PyLong_NumBits(n);
    //keyused = bits == 0 ? 1 : (bits - 1) / 32 + 1;
    //uint32_t *key = NULL;
    //key = (uint32_t *)malloc((size_t)4 * keyused);
    //uint32_t key[2];
    //memcpy(key, &n, sizeof(key[0]) * keyused);
    // no
    //key[0] = n;
    //key[1] = n>>32;
    // still no
    size_t keyused = n < 0x100000000 ? 1 : 2;
    // is this undefined behaviour?
    init_by_array(self, (uint32_t *)&n, keyused);
    //free(key);
}


void reverse_genrand_all(){

    /* ORIGINAL:
            #define UPPER_MASK 0x80000000U 
            #define LOWER_MASK 0x7fffffffU
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1U];
    */
    // We can easyly control kk+M
    // For example, for kk=0 and getting a 0:
    // y = (mt[0]&UPPER_MASK)|(mt[1]&LOWER_MASK);
    // mt[0] = mt[M] ^ (y>>1) ^ mag01[y & 1];
    // we only need to change mt[M]...
}



// reverts the operation y ^= y >> n
uint32_t unBitshiftRightXor(uint32_t y, size_t n){
    // the first n bits are the same:
    size_t bits = n;
    uint32_t partMask = 0xffffffff << (32 - n);

    while(bits < 32){
        uint32_t part = y & partMask;
        y ^= part >> n;
        partMask = partMask >> n;
        bits += n;
    }

    return y;
}

// reverts the operation y ^= (y << n) & mask
uint32_t unBitshiftLeftXor(uint32_t y, size_t n, uint32_t mask){
    // the last n bits are the same:
    size_t bits = n;
    uint32_t partMask = 0xffffffff >> (32 - n);

    while(bits < 32){
        uint32_t part = y & partMask;
        y ^= (part << n) & mask;
        partMask = partMask << n;
        bits += n;
    } 

    return y;
}

// returns the value that needs to be in mt[i] so that genrand_int32 returns target
uint32_t reverse_genrand_last_step(uint32_t target){
/*
    y = mt[self->index++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= (y >> 18);
    return y;
    
    n = y ^ ((y << 15) & 0xefc60000U);
    0000 ^ 0000 & ABCD = 0000
    ffff ^ ff00 & ABCD = ffff ^ AB00 = 5400
        //eprintf("4: %08X\n", y);
    y ^= (y >> 18);
        //eprintf("3: %08X\n", y);
    y ^= ((y << 15) ^ ~0xefc60000U) & 0xefc60000U;
        eprintf("2: %08X\n", y);
    y ^= ((y << 7) ^ ~0x9d2c5680U)& 0x9d2c5680U;
        //eprintf("1: %08X\n", y);
    y ^= (y >> 11);
        //eprintf("0: %08X\n", y);
    return y; // pleaaaase
*/
    uint32_t y = target;
    y = unBitshiftRightXor(y, 18);
    y = unBitshiftLeftXor(y, 15, 0xefc60000U);
    y = unBitshiftLeftXor(y, 7, 0x9d2c5680U);
    y = unBitshiftRightXor(y, 11);
    return y;
}

int test_reverse_genrand_last_step(){
    int mm=1;
    while(mm>0){
        mm += 255;
        uint32_t y=mm;
        //eprintf("0: %08X\n", y);
        y ^= (y >> 11);
        //eprintf("1: %08X\n", y);
        y ^= (y << 7) & 0x9d2c5680U;
        //eprintf("2: %08X\n", y);
        y ^= (y << 15) & 0xefc60000U;
        //eprintf("3: %08X\n", y);
        y ^= (y >> 18);
        //eprintf("4: %08X\n", y);
        int m2 = reverse_genrand_last_step(y);
        if(m2!=mm){
            eprintf("NOOOOOOOOOOOOOOOOO\n");
            int m3 = reverse_genrand_last_step(m2);
            eprintf("org: %08X got: %08X rev: %08X\n", mm, m2, m3);
        }
    }
    return 0;
}


/*
uint32_t reverse_random_random(double target){
    uint32_t a=genrand_int32(self)>>5, b=genrand_int32(self)>>6;
    return ((a*67108864.0+b)*(1.0/9007199254740992.0)); //PyFloat_FromDouble
    a: 27 bits
    b: 26 bits
*//* this was the code
    uint32_t a,b;
    double c=0;
    a=0; b=0;
    a = target*9007199254740992.0/67108864.0 + 0x20;
    */
    // We can leave b=0 because the difference between
    // b=0 and b=ffffffff is 7.45e-09
    //b = target*9007199254740992.0 - a*67108864.0;
    /*
    while(c < target){
        a++;
        c = ((a*67108864.0+b)*(1.0/9007199254740992.0));
        eprintf("%.18g\n", c);
    }*/
    //c = ((a*67108864.0+b)*(1.0/9007199254740992.0));
    //eprintf("REVERSED %.18g into %.18g: a=%08X b=%08X\n", target, c, a, b);
//    return a<<5;
    
//}



double test_random_random(uint32_t a, uint32_t b){

    a>>=5; b>>=6;
    return ((a*67108864.0+b)*(1.0/9007199254740992.0)); //PyFloat_FromDouble
}

/*
uint32_t* anti_state(uint32_t* s){
    int i;
    uint32_t *k = malloc(N*sizeof(uint32_t));
    memset(k,0,sizeof(k));
    k[0] = 0;
    for(i=2; i<N; i++){
        k[i-1] = -s[i]+i;
    }
    return k;
}*/

uint32_t get_seed_if_key_length_1(RandomObject *self){
    uint32_t *smt = self->state;
    uint32_t mt[2];
    int i=N-1;

    // REVERSE LAST STEP FOR N-1 and N-2
    mt[1] = (smt[i] + (uint32_t)i) ^ ((smt[i-1] ^ (smt[i-1]>>30)) * 1566083941U);
    i--;
    mt[0] = (smt[i] + (uint32_t)i) ^ ((smt[i-1] ^ (smt[i-1]>>30)) * 1566083941U);
    i--;
    
    // REVERSE FIRST STEP FOR N-1 AND GET SEED
    i=N-1;
    const uint32_t *original_mt = init_genrand_19650218.state;
    uint32_t xor = ((mt[0] ^ (mt[0] >> 30)) * 1664525U);
    uint32_t seed = (mt[1]) - (original_mt[i]^xor);
    //eprintf("N-2 test: orig: %08X mine: %08X xor: %08X diff: %d\n",
    //        original_mt[i], mt[i], xor, seed);
    return seed;
}

// only works for keysize <= N
void reverse_init_last_step(uint32_t *mt){
    if(mt[0] != 0x80000000U){
        eprintf("You need to call reverse_init_last_step on something");
        eprintf(" that was seeded and not yet used by genrand_int32\n");
        return;
    }
    // REVERSE LAST STEP
    /* If you ever need a key > N,
     * use this line:
        i=(keysize+1)%N;
     */
    size_t i,k;
    i=1;
    for (k=0; k<N-1; k++) { 
        if(i==1){ mt[0] = mt[N-1]; }
        mt[i] = (mt[i] + (uint32_t)i) ^ ((mt[i-1] ^ (mt[i-1]>>30)) * 1566083941U);
        i--;
        if(i==0){ i=N-1; }
    }
    mt[0] = mt[N-1];
    // LAST STEP DONE
    // i returns to its initial value
}

uint32_t* generic_get_seed(const uint32_t *s, size_t key_length){
    uint32_t mt[N];
    memcpy(mt, s, N*sizeof(mt[0]));
    reverse_init_last_step(mt);
    int i,j,k;
    i=1;

    /* this works if you have the seed */
    int kmax = (N>key_length ? N : key_length);
    //j=2; //i==1
    j=(N-1)%key_length;
    const uint32_t *original_mt = init_genrand_19650218.state;
    uint32_t *expected_key = malloc(key_length*sizeof(uint32_t));
    for (k=0; k<kmax; k++) {
        // finds the seed when key_length <= N-3
        if (k>1 && k<kmax-1){
            uint32_t xor = ((mt[i] ^ (mt[i] >> 30)) * 1664525U);
            uint32_t seed = (mt[i+1]^xor) - (original_mt[i+1]^xor);
            //eprintf("i=%03d j=%03d test: orig: %08X mine: %08X xor: %08X diff: %d\n", i, j, original_mt[i+1], mt[i+1], xor, seed);
            if(k-2 > key_length){
                // we have all the parts, verify:
                if(expected_key[(j+1)%key_length] != seed){
                    eprintf("Error getting seed!\n");
                }
            }else{
                if(j+1>=key_length){
                    expected_key[0] = seed;
                }else{
                    expected_key[j+1] = seed;
                }
            }
        }
        //if(i==1){ mt[0] = mt[N-1]; }
        //eprintf("j=%d original mt[%d] == %lld\t", j, i, mt[i]);
        mt[i] = (mt[i] - (uint32_t)j) ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U);
        //         + init_key[j] + (uint32_t)j;
        //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i--; j--;
        if (i==0) { i=N-1; mt[0] = original_mt[0]; }
        if (j<0) { j=key_length-1; }
    }
    //if(i==1){ mt[0] = mt[N-1]; }
    j=2; i=1;

    return expected_key;
}

uint32_t* reverse_init(uint32_t* s, size_t key_length){
    uint32_t* mt = malloc(N*sizeof(uint32_t));
    memcpy(mt, s, N*sizeof(uint32_t));
    int i,j,k;
    // REVERSE LAST STEP
    i=1;
    for (k=0; k<N-1; k++) { 
        if(i==1){ mt[0] = mt[N-1]; }
        mt[i] = (mt[i] + (uint32_t)i) ^ ((mt[i-1] ^ (mt[i-1]>>30)) * 1566083941U);
        //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i--;
        if(i==0){ i=N-1; }
    }
    if(i==1){ mt[0] = mt[N-1]; }
    // LAST STEP DONE
    RandomObject r;
    memcpy(r.state, mt, N*sizeof(uint32_t));
    getstate(&r);
    /*
    uint32_t key[1];
    bruteforce_seed(mt,key,key_length);
    eprintf("KEY FOUND: %lld\n", key[0]);
    */
    /*
    int kmax = (N>key_length ? N : key_length);
    uint32_t init_key[1] = {0};
    j=0; // i==1
    uint32_t *original_mt = init_genrand_19650218.state;
    for (k=0; k<kmax; k++){
        eprintf("genrand mt[%d] == %lld\t", i, original_mt[i]);
        eprintf("original mt[%d] == %lld\t", i, mt[i]);

        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U))
                 + init_key[j] + (uint32_t)j;
        
        {eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i--; j--;
        if (i==0) { i=N-1; mt[0] = original_mt[0]; }
        if (j<0) { j=key_length-1; }
    }*/
    /* encuentra la seed si key_length == 1 
    i = N-1;
    uint32_t *original_mt = init_genrand_19650218.state;
    uint32_t xor = ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U);
    uint32_t seed = (mt[i]) - (original_mt[i]^xor);
    eprintf("N-2 test: orig: %08X mine: %08X xor: %08X diff: %d\n",
            original_mt[i], mt[i], xor, seed);
    */
    // backup
    uint32_t* mtlast = malloc(N*sizeof(uint32_t));
    memcpy(mtlast, mt, N*sizeof(uint32_t));

    /* this works if you have the seed: */
    int kmax = (N>key_length ? N : key_length);
    uint32_t init_key[N] = {0};
    j=key_length-1; //i==1
    const uint32_t original_mt0 = init_genrand_19650218.state[0];
    const uint32_t *original_mt = init_genrand_19650218.state;
    uint32_t *expected_key = malloc(N*sizeof(uint32_t));
    for (k=0; k<kmax; k++) {
        // this works for lenght N
        // off by one? 3 keys missing...
        if (k>1 && k<kmax-1){
            uint32_t xor = ((mt[i] ^ (mt[i] >> 30)) * 1664525U);
            uint32_t seed = (mt[i+1]^xor) - (original_mt[i+1]^xor);
    //        eprintf("i=%03d j=%03d test: orig: %08X mine: %08X xor: %08X diff: %d\n", i, j, original_mt[i+1], mt[i+1], xor, seed);
            expected_key[j+1] = seed;
        }
        //if(i==1){ mt[0] = mt[N-1]; }
        //eprintf("j=%d original mt[%d] == %lld\t", j, i, mt[i]);
        mt[i] = (mt[i] - init_key[j] - (uint32_t)j) ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U);
        //         + init_key[j] + (uint32_t)j;
        //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i--; j--;
        if (i==0) { i=N-1; mt[0] = original_mt0; }
        if (j<0) { j=key_length-1; }
    }
    //if(i==1){ mt[0] = mt[N-1]; }
    j=key_length-1; i=1;
    for (k=0; k<kmax; k++) {
        //if(i==1){ mt[0] = mt[N-1]; }
        //eprintf("j=%d original mt[%d] == %lld\t", j, i, mt[i]);
        mtlast[i] = (mtlast[i] - expected_key[j] - (uint32_t)j) ^ ((mtlast[i-1] ^ (mtlast[i-1] >> 30)) * 1664525U);
        //         + init_key[j] + (uint32_t)j;
        //{eprintf("mt[%d] == %lld\n", i, mt[i]);}
        i--; j--;
        if (i==0) { i=N-1; mtlast[0] = original_mt0; }
        if (j<0) { j=key_length-1; }
    }
    memcpy(r.state, mtlast, N*sizeof(uint32_t));
    getstate(&r);
    // all correct except mt[1], mt[2], 
    // we dont have key[0], key[1], key[N-1]
    // calculate mt[1] ~ key[N-1], 
    // then mt[2]==original ~ key[1], 
    // and  mt[1]==original ~ key[0],
    
    /*
    i=2;
    i=1;
    mt[i] = (mt[i] + (uint32_t)i) ^ ((mt[i-1] ^ (mt[i-1]>>30)) * 1566083941U);
    
    for(i=N-1; i>3; i--){
        mt[i] = (mt[i] + (uint32_t)i) ^ ((mt[i-1] ^ (mt[i-1]>>30)) * 1566083941U);
    }
    mt[0] = mt[N-1];
    */
    /*
    uint32_t mt3;
    do {
        mt3 = (mt[3] ^ ((mt[2] ^ (mt[2]>>30)) * 1566083941U)) 
               - (uint32_t)3;
        mt[3]++;
    } while(mt3 != s[3]);
    mt[3]--;
    */
    //eprintf("mt[3]: %lld\n", mt[3]);
    /*
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941U))
                 - (uint32_t)i;
        //if(i==2){eprintf("mt[i2] == %lld\n", mt[i]);}
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }
    mt[0] = 0x80000000U; 
    */

    return expected_key;
}

// zeros == key_len - M - 1 
void into_zero_generator_M(RandomObject *self, size_t key_len){
    // currently only works if key_len < M + 227
    // so, key_len < N
    size_t i;

    RandomObject r2;

    r2 = *self;

    // It seems we can modify the state in the interval [4, N-5]
    // Probably need a longer key to modify the rest...
    // OPTIMIZATION!
    // We only need to modify [M, M+n] to get n zeros
    RandomObject r = r2;
    genrand_int32(&r);
    //getstate(&r);
    
    // Since the [i] generated by genrand_int32 depends on [i+M]
    // (it's only a simple xor)
    // we revert it to get a 0
    for(i=M; i<key_len-1; i++){
        r2.state[i] ^= r.state[i-M];
    }

    uint32_t *key3 = generic_get_seed(r2.state, key_len);
    byte_array_to_python_number(key3, key_len);
    RandomObject r3;
    init_by_array(&r3, key3, key_len);
    free(key3);
    eprintf("Same state? %d\n", same_state(&r2,&r3));

    *self = r3;

    /*
    eprintf("Zero generator ready:\n");
    int a = genrand_int32(&r3);
    getstate(&r3);
    size_t zeros = 0;
    while(a==0){
        zeros++;
        a = genrand_int32(&r3);
    }
    eprintf("Available zeros: %zu (%zu characters)\n", zeros, (zeros+1)/2);
    */
}