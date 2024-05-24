/**
 * Copyright (c) 2022 murilo
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */


#include "include/rdrand.h"
#include <intrin.h>
#include <immintrin.h>

#include <stdio.h>
//#include <unistd.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
} CPUIDinfo;

void get_cpuid_windows(int leaf, CPUIDinfo *info) {
    uint32_t arr[4];

    __cpuid(arr, 4);

    info->EAX = arr[0];
    info->EBX = arr[1];
    info->ECX = arr[2];
    info->EDX = arr[3];
}


/* Trying GAS format to make clang happy * /
void get_cpuid_linux(CPUIDinfo *info, const uint32_t func, const uint32_t subfunc) {

    asm(".intel_syntax noprefix;\n\
    mov r8, rdi;\n\
    mov r9, rsi;\n\
    mov r10, rdx;\n\
    push rax;\n\
    push rbx;\n\
    push rcx;\n\
    push rdx;\n\
    mov eax, r9d;\n\
    mov ecx, r10d;\n\
    cpuid;\n\
    mov DWORD PTR [r8], eax;\n\
    mov DWORD PTR [r8+4], ebx;\n\
    mov DWORD PTR [r8+8], ecx;\n\
    mov DWORD PTR [r8+12], edx;\n\
    pop rdx;\n\
    pop rcx;\n\
    pop rbx;\n\
    pop rax;\n\
    .att_syntax prefix\n");
}
*/
#ifdef __i386__
int _have_cpuid() {

	/* cpuid availability is determined by setting and clearing the 
	 * ID flag (bit 21) in the EFLAGS register. If we can do that, we
	 * have cpuid. This is only necessary on 32-bit processors.
	 */
    uint32_t fbefore, fafter;

	asm(" 					;\
		pushf				;\
		pushf				;\
		pop %0				;\
		mov %0,%1			;\
		xor $0x40000,%1		;\
		push %1				;\
		popf				;\
		pushf				;\
		pop %1				;\
		popf				"
	: "=&r" (fbefore), "=&r" (fafter)
	);

	return (0x40000 & (fbefore^fafter));
}
#endif


void get_cpuid(CPUIDinfo *info, const uint32_t func, const uint32_t subfunc) {
    #if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
        get_cpuid_windows(func, info);
    #else
        get_cpuid_linux(info, func, subfunc);
    #endif
}

typedef uint32_t DWORD;

int check_is_intel() {
    CPUIDinfo info;
   
    get_cpuid(&info,0,0);
	if(memcmp((char *)(&info.EBX), "Genu", 4) == 0 &&
		memcmp((char *)(&info.EDX), "ineI", 4) == 0 &&
		memcmp((char *)(&info.ECX), "ntel", 4) == 0) {
			return 1;
	}
    
    return 0;
}

int check_is_amd() {
    CPUIDinfo info;
   
    get_cpuid(&info,0,0);

    if( memcmp((char *)(&info.EBX), "Auth", 4) == 0 &&
		memcmp((char *)(&info.EDX), "enti", 4) == 0 &&
		memcmp((char *)(&info.ECX), "cAMD", 4) == 0) {
			return 1;
	}
    return 0;
}

int check_rdrand() {
    CPUIDinfo info;
   
    get_cpuid(&info,1,0);
   
    if ((info.ECX & 0x40000000)==0x40000000) return 1;
    return 0;
}

int check_rdseed() {
    CPUIDinfo info;
   
    get_cpuid(&info,7,0);
   
   if ((info.EBX & 0x00040000)==0x00040000) return 1;
   return 0;
}

int rdrand_check_support() {
    return 1;
#ifdef __i386___
    if (!_have_cpuid()) return 0;
#endif
	if ((check_is_intel()==1) || (check_is_amd()==1)){
        if (check_rdrand()==1) return 1;
	}
	return 0;
}

int rdseed_check_support() {
    return 1;
#ifdef __i386___
    if (!_have_cpuid()) return 0;
#endif
	if ((check_is_intel()==1) || (check_is_amd()==1)){
        if (check_rdseed()==1) return 1;
	}
	return 0;
}

/* Gathers 16 bits of entropy through RDRAND      
   The 16 bit result is zero extended to 32 bits 
   Writes that entropy to *therand.              
   Returns 1 on success, or 0 on underflow
*/
int rdrand16_step(uint16_t *therand) {
    
    uint16_t foo;
    int cf_error_status;

    /*asm("\n\
            rdrand %%ax;\n\
            mov $1,%%edx;\n\
            cmovae %%ax,%%dx;\n\
            mov %%edx,%1;\n\
            mov %%ax, %0;":"=r"(foo),"=r"(cf_error_status)::"%ax","%dx");*/
    _rdrand16_step(therand);

    return cf_error_status;
}

int rdseed16_step(uint16_t *therand)
{
    uint16_t foo;
    int cf_error_status;

    _rdseed16_step(therand);

    return cf_error_status;
}

/* Gathers 32 bits of entropy through RDRAND
   Writes that entropy to *therand.        
   Returns 1 on success, or 0 on undeerflow
*/
int rdrand32_step(uint32_t *therand) {

    int foo;
    int cf_error_status;

    _rdrand32_step(therand);

    return cf_error_status;
}

int rdseed32_step(uint32_t *therand) {

    int foo;
    int cf_error_status;

    _rdseed32_step(therand);

    return cf_error_status;
}

/* Gathers 64 bits of entropy through RDRAND
   Writes that entropy to *therand.         
   Returns 1 on success, or 0 on underflow
*/
int rdrand64_step(uint64_t *therand) {
        
    uint64_t foo;
    int cf_error_status = 1;

    while (!_rdrand64_step(therand));

    return cf_error_status;
}

int rdseed64_step(uint64_t *therand) {

    uint64_t foo;
    int cf_error_status = 1;

    while(!_rdseed64_step(therand));

    return cf_error_status;
}

/* Uses RdRand to acquire a 32 bit random number 
   Writes that entropy to (uint32_t *)dest.
   Will not attempt retry on underflow
   Returns 1 on success, or 0 on underflow
*/
int rdrand_get_uint32(uint32_t *dest) {

	uint32_t therand;
    
	if (rdrand32_step(&therand)) {
		*dest = therand;
		return 1;
	} else return 0;
}

int rdseed_get_uint32(uint32_t *dest) {

	uint32_t therand;

	if (rdseed32_step(&therand)) {
		*dest = therand;
		return 1;
	} else return 0;
}

int rdrand_get_uint64(uint64_t *dest)
{
	uint64_t therand;

	if (rdrand64_step(&therand)) {
		*dest = (uint64_t)therand;
		return 1;
	} else return 0;
}

int rdseed_get_uint64(uint64_t *dest)
{
	uint64_t therand;

	if (rdseed64_step(&therand)) {
		*dest = (uint64_t)therand;
		return 1;
	} else return 0;
}

/* Uses RdRand to acquire a 32 bit random number  
   Writes that entropy to (uint32_t *)dest. 
   Will retry up to retry_limit times           
   Returns 1 on success, or 0 on underflow
*/
int rdrand_get_uint32_retry(uint32_t retry_limit, uint32_t *dest) {

    int success;
    uint32_t count;
    uint32_t therand;

    count = 0;

    do {
        success=rdrand32_step(&therand);
    } while((success == 0) || (count++ < retry_limit));
  
    if (success == 1) {
        *dest = therand;
        return 1;
    } else {
        return 0;
    }
}

int rdrand_get_uint64_retry(uint32_t retry_limit, uint64_t *dest)
{
    int success;
    uint32_t count;
    uint64_t therand;

    count = 0;

    do {
        success = rdrand64_step(&therand);
    } while((success == 0) || (count++ < retry_limit));
    
    if(success == 1) {
        *dest = therand;
        return 1;
    } else {
        return 0;
    }
}

int rdseed_get_uint32_retry(uint32_t retry_limit, uint32_t *dest) {

    int success;
    uint32_t count;
    uint32_t therand;

    count = 0;

    do {
        success=rdseed32_step(&therand);
    } while((success == 0) || (count++ < retry_limit));
    
    if (success == 1) {
        *dest = therand;
        return 1;
    } else {
        return 0;
    }
}

int rdseed_get_uint64_retry(uint32_t retry_limit, uint64_t *dest) {

    int success;
    uint32_t count;
    uint64_t therand;

    count = 0;

    do {
	    success=rdseed64_step(&therand);
    } while((success == 0) || (count++ < retry_limit));
  
    if (success == 1) {
        *dest = therand;
        return 1;
    }
    else {
	    return 0;
    }
}

/* Uses RdRand to acquire a block of n 32 bit random numbers   
   Writes that entropy to (unsigned long long int *)dest[0+]. 
   Will retry up to retry_limit times                         
   Returns 1 on success, or 0 on underflow
*/
int rdrand_get_n_uint32_retry(uint32_t n, uint32_t retry_limit, uint32_t *dest) {
    
    int success=0;
    uint32_t count=0;
    uint32_t i=0;

    for (i=0; i<n; i++) {
        count = 0;
        do
        {
            success=rdrand32_step(dest);
        } while((success == 0) && (count++ < retry_limit));

        if (success == 0) return 0;
        dest=&(dest[1]);
    }
    return 1; 
}

int rdseed_get_n_uint32_retry(uint32_t n, uint32_t retry_limit, uint32_t *dest) {
    
    int success=0;
    uint32_t count=0;
    uint32_t i=0;

    for (i=0; i<n; i++)
    {
        count = 0;
        do
        {
                success=rdseed32_step(dest);
        } while((success == 0) && (count++ < retry_limit));

        if (success == 0) return 0;
        dest=&(dest[1]);
    }

    return 1; 
}

/* Uses RdRand to acquire a block of n 64 bit random numbers   
   Writes that entropy to (unsigned long long int *)dest[0+].
   Will retry up to retry_limit times                        
   Returns 1 on success, or 0 on underflow
*/                  
int rdrand_get_n_uint64_retry(uint32_t n, uint32_t retry_limit, uint64_t *dest) {

    int success=0;
    uint32_t count=0;
    uint32_t i=0;

    for (i=0; i<n; i++)
    {
        count = 0;
        do
        {
                success=rdrand64_step(dest);
        } while((success == 0) && (count++ < retry_limit));
        if (success == 0) return 0;
        dest=&(dest[1]);
    }
    return 1; 
}

int rdseed_get_n_uint64_retry(uint32_t n, uint32_t retry_limit, uint64_t *dest) {

    int success;
    uint32_t count;
    unsigned int i;

    for (i=0; i<n; i++)
    {
        count = 0;
        do
        {
                success=rdseed64_step(dest);
        } while((success == 0) && (count++ < retry_limit));
        if (success == 0) return 0;
        dest=&(dest[1]);
    }
    return 1; 
}
