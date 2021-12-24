#ifndef COMMON_H
#define COMMON_H

#define FIXED_SIZE 512

char generate_rdrand64_ia32(float *randf, float min, float max);
// char* generate_range(unsigned int length);
char generate_rdrand64_90(int*);
char generate_rdrand64(int *number, int max);

#endif