
#ifndef HG_HANDLER_CONSUMER_H
#include <cstdint>
#include <cassert>
#include "common/tools.h"
#define HG_HANDLER_CONSUMER_H
class MsgObjHead{
    uint8_t type=0;
    uint8_t count=0;
};
class MsgObj0{
    public:
        uint32_t ssrc=0;
        uint16_t vpts=0;
        uint16_t apts=0;
        MsgObj0();
        static int getdelen(uint8_t *source,int size);
        static int getenlen(MsgObj0 *source,int size);
        static void decodeone(uint8_t *source,int ssize,MsgObj0 *dist);
        static void encode(uint8_t *dist,MsgObj0 *source,int ssize);
        static void decode(uint8_t *source,int ssize,MsgObj0 *dist,int dsize);
};
#endif
