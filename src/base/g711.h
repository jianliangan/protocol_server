//
// Created by ajl on 2021/9/2.
//

#ifndef HG_RTP_APP_G711_H
#define HG_RTP_APP_G711_H
#include <stdint.h>
#ifdef __cplusplus
extern "C"{
#endif

    int8_t ALaw_Encode(int16_t number);

    int16_t ALaw_Decode(int8_t number);

    int8_t MuLaw_Encode(int16_t number);

    int16_t MuLaw_Decode(int8_t number);

#ifdef __cplusplus
}
#endif
#endif //HG_RTP_APP_G711_H

