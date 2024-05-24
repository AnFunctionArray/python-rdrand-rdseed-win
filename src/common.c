/**
 * Copyright (c) 2022 murilo
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#define __CHAR_BIT__ 8

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

#include "include/common.h"
#include "include/rdrand.h"


unsigned int absolute(int value) {

    unsigned int result;
    int const mask = value >> sizeof(int) * (CHAR_BIT - 1);
    result = (value + mask) ^ mask;

    return result;
}

/*char generate_rdrand64_ia32(float* randf, float min, float max) {
    int retries = 10;
    unsigned long long rand64;

    while(retries--) {
        if(__builtin_ia32_rdrand64_step(&rand64)) {
            *randf = (float) rand64 / ULONG_MAX * (max - min) + min;
            return 1;
        }
    }
    return 0;
}*/

char generate_rdrand64(uint64_t *number) {

    uint64_t retn;

    if (rdrand_check_support() == 1) {
        rdrand_get_uint64_retry(10, &retn);
        *number = retn;
    } else {
        fprintf(stderr, "RDRAND instruction not supported.\n");
        return -1;
    }
    return 0;
}

char generate_rdrand64_below(int *number, int below) {
    
    uint64_t retn;
    int m, v;

    if (rdrand_check_support() == 1) {
        rdrand_get_uint64_retry(10, &retn);
        
        v = (int) retn % (below - 1);
        m = v >> sizeof(int) * (__CHAR_BIT__ - 1);
        *number = (int) (v + m) ^ m;
    } else {
        fprintf(stderr, "RDRAND instruction not supported.\n");
        return -1;
    }

    return 0;
}

char generate_rdrand64_boundary(int *number, int boundary) {
    
    uint64_t retn;
    int m, v;

    if(rdrand_check_support() == 1) {
        
        rdrand_get_uint64_retry(10, &retn);

        v = (int) retn % (boundary + 1);
        m = v >> sizeof(int) * (__CHAR_BIT__ - 1);
        *number = (int) (v + m) ^ m;
    
    } else {
        fprintf(stderr, "RDRAND instruction not supported.\n");
        return -1;
    }
    
    return 0;
}

char generate_rdseed(uint64_t *number) {
    
    if (rdseed_check_support() == 1) {
        rdseed_get_uint64_retry(10, number);
        return 0;
    }

    fprintf(stderr, "RDSEED instruction isn't supported.");
    return -1;

}
