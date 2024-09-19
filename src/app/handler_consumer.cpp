#include "app/handler_consumer.h"
#include "app/session.h"
MsgObj0::MsgObj0(){
    this->vpts=0;
    this->apts=0;
    this->ssrc=0;
}
int MsgObj0::getdelen(uint8_t *source,int size){
    assert(size>2);
    if(source[0]==0){
        return source[1];
    }
    return 0;
}
int MsgObj0::getenlen(MsgObj0 *source,int size){
    return sizeof(MsgObjHead)+size*sizeof(MsgObj0);
}
void MsgObj0::encode(uint8_t *dist,MsgObj0 *source,int ssize){
    dist[0]=0;
    dist[1]=ssize;
    int offset=sizeof(MsgObjHead);
    int vsize=sizeof(MsgObj0);
    for(int i=0;i<ssize;i++){
        offset=offset+i*vsize;
        toolsIntBigEndian(dist+offset, (uint32_t)(source+i)->ssrc, 32);
        toolsIntBigEndian(dist+offset+4, (uint32_t)(source+i)->apts, 16);
        toolsIntBigEndian(dist+offset+4+2, (uint32_t)(source+i)->vpts, 16);
    }
}
void MsgObj0::decodeone(uint8_t *source,int ssize,MsgObj0 *dist){
    assert(ssize==HG_SINGLE_MSG0_LEN);
    toolsIntBigEndianV(source, (uint8_t *)&((dist)->ssrc), 32);
    toolsIntBigEndianV(source+4, (uint8_t *)&((dist)->apts), 16);
    toolsIntBigEndianV(source+4+2, (uint8_t *)&((dist)->vpts), 16);
}
void MsgObj0::decode(uint8_t *source,int ssize,MsgObj0 *dist,int dsize){
    assert(source[0]==0);
    int offset=sizeof(MsgObjHead);
    int vsize=sizeof(MsgObj0);
    for(int i=0;i<dsize;i++){
        offset=offset+i*vsize;
        toolsIntBigEndianV(source+offset, (uint8_t *)&((dist+i)->ssrc), 32);
        toolsIntBigEndianV(source+offset+4, (uint8_t *)&((dist+i)->apts), 16);
        toolsIntBigEndianV(source+offset+4+2, (uint8_t *)&((dist+i)->vpts), 16);
    }
}
