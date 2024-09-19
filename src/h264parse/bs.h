#ifndef H264_BS
#define H264_BS
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
typedef struct
{
    uint8_t* start;
    uint8_t* p;
    uint8_t* end;
    int bits_left;
} bs_t;
bs_t* bs_new(uint8_t* buf, size_t size);
uint32_t bs_read_u(bs_t* b, int n);
uint32_t bs_read_u1(bs_t* b);
int bs_eof(bs_t* b) ;
uint32_t bs_read_u8(bs_t* b);
uint32_t bs_read_u1(bs_t* b);
void bs_skip_u(bs_t* b, int n);
void bs_skip_u1(bs_t* b);
int32_t bs_read_se(bs_t* b);
uint32_t bs_read_ue(bs_t* b);
#endif
