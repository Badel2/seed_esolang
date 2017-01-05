#include "pylong.h"

void next_pylong(uint32_t * x, size_t key_len){
    size_t i = 0;
    while(i < key_len && ++x[i++] == 0);
}

/*
void next_pylong(uint32_t * x, size_t * key_len){
    next_pylong_(x,*key_len);
    return;
    size_t i = 0;
    do {
        x[i]++;
        if(x[i] == 0){
            if(*key_len == i+1){
                (*key_len)++;
                x[i+1] = 0;
                break;
            } else {
                x[i+1]++;
            }
        }else{
            break;
        }
        i++;
    } while(i < *key_len && x[i] == 0);
}
*/


/* Paste this to random.seed() in python */
void byte_array_to_python_number(uint32_t* x, size_t len){
    if(len){
        --len;
        eprintf("0x%X", x[len]);
        for(;len;){
            --len;
            eprintf("%08X", x[len]);
        }
        eprintf("\n");
    }
}


/* converts a pylong to decimal using an external command, stores the result in "out".
 * out should be declared by the caller similarly to this:
 *
 *     char out[seed_len*10];
 * 
 * returns 0 on success and non zero on failure
 */
int byte_array_to_decimal_number(uint32_t* the_seed, size_t seed_len, char * out){
    char cmd[seed_len*8 + 30];
    sprintf(cmd, "python -c 'print(0x");
    size_t len = seed_len;
    char * cmd_end = cmd + strlen(cmd);
    for(;len;){
        cmd_end += sprintf(cmd_end, "%08X", the_seed[--len]);
    }
    cmd_end += sprintf(cmd_end, ")'");
    
    if(cmd_end - cmd > sizeof(cmd)){
        fprintf(stderr, "Buffer overflow in function byte_array_to_decimal_number\n");
        return 3;
    }

    eprintf("Executing: %s\n", cmd);

    FILE *fp;
    if ((fp = popen(cmd, "r")) == NULL) {
        eprintf("Error opening pipe!\n");
        return 1;
    }

    // this was a while loop but we only needed one line anyways, so...
    if (fgets(out, seed_len*10, fp) != NULL) {
        //sprintf(out, "%s", buf);
    }

    if(pclose(fp)) {
        fprintf(stderr, "Command not found or exited with error status\n");
        return 2;
    }

    remove_useless_newline(out);
    return 0;
}


