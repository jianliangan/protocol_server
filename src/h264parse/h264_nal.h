#ifndef H264_NAL
#define H264_NAL
#include <stdint.h>

int nal_to_rbsp(const uint8_t* nal_buf, int* nal_size, uint8_t* rbsp_buf, int* rbsp_size);
#endif

