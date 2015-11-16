#ifndef __MWNL_GF_H
#define __MWNL_GF_H
#include <stdint.h>

uint16_t gmul(uint16_t a, uint16_t b);
uint16_t gmul_inverse(uint16_t in);
uint16_t gsub(uint16_t a, uint16_t b);
uint16_t gadd(uint16_t a, uint16_t b);
uint16_t gdiv(uint16_t a, uint16_t b);

#endif
