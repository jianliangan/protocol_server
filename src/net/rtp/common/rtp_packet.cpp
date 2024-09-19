//
// Created by ajl on 2020/12/27.
//

#include "net/rtp/common/rtp_packet.h"
uint8_t * rtpPacketHead(uint8_t *dst,t_rtpHeader *rh) {

        *(dst++) = ((0x01&rh->first)<<7)|((0x01&rh->comp)<<6);//
        *(dst++) = ((0xFF&rh->cmd)<<5)|((rh->tail&0x03)<<3)|(rh->payloadType&0x07);//marker,payloadType
        toolsIntBigEndian(dst, rh->seq, 16);

        dst+=2;
        toolsIntBigEndian(dst, rh->timestamp, 16);
        dst+=2;
        toolsIntBigEndian(dst, rh->ssrc, 32);
        dst+=4;
        return dst;
}

/*************************rtp.cpp***/


uint32_t pktSetSsrc(uint8_t *data,uint32_t size,uint32_t value){
    assert(size<HG_RTP_PACKET_HEAD_SIZE);
        toolsIntBigEndian(data+6, value, 32);
        return 0;
}
uint32_t pktSsrc(uint8_t *data,uint32_t size){

    if (size < HG_RTP_PACKET_HEAD_SIZE)
        return 0;

    else {
        return ((data[6] & 0xff) << 24) | ((data[7] & 0xff) << 16) |
               ((data[8] & 0xff) << 8) |
               ((data[9] & 0xff));
    }


}
int pktFlag(uint8_t *data,uint32_t size){

    assert(size >= HG_RTP_PACKET_HEAD_SIZE);


        return  (((uint8_t)(data[1])>>3)&0x03);


}
enumt_PLTYPE pktPLoadType(uint8_t *data,uint32_t size){

    if (size < HG_RTP_PACKET_HEAD_SIZE)
        return (enumt_PLTYPE)0;
    else {

            return (enumt_PLTYPE)(data[1] & 0x07);

    }

}
uint8_t pktComp(uint8_t *data,uint32_t size){
    if (size < HG_RTP_PACKET_HEAD_SIZE)
        return 0;
    else {
        return data[0] >> 6 & 0x01;
    }
}
void pktSetComp(uint8_t *data,int value){
    data[0]=((value&0x01)<<6)|(data[0]&0xBF);
}
uint8_t pktFirst(uint8_t *data,uint32_t size){

    if (size < HG_RTP_PACKET_HEAD_SIZE)
        return 0;
    else {

            return data[0] >> 7 & 0x01;

    }

}
uint16_t pktTimestamp(uint8_t *data,uint32_t size){

    if (size < HG_RTP_PACKET_HEAD_SIZE)
        return 0;
    else {

            return ((data[4] & 0xff) << 8) |
                (data[5] & 0xff);

    }

}
uint16_t pktSeq(uint8_t *data,uint32_t size){
    if(size<HG_RTP_PACKET_HEAD_SIZE)
        return 0;
    else{

            return ((data[2] & 0xff) << 8) | (data[3] & 0xff);

    }
}

void pktSetSeq(uint8_t *data,uint16_t value){

        *((uint16_t *)(data+2))=value;
        data[2]=*((unsigned char *)(&value));
        data[3]=*((unsigned char *)(&value)+1);
        //return ((data[1]&0xE0)>>5);


}

uint8_t pktCmd(uint8_t *data,uint32_t size){
    if(size<HG_RTP_PACKET_HEAD_SIZE)
        return 0;
    else{

            return ((data[1]&0xE0)>>5);

    }

}
void pktSetCmd(uint8_t *data,int value){


        data[1]=((value&0x07)<<5)|(data[1]&0x1F);
        //return ((data[1]&0xE0)>>5);


}
//size是data的总长度，max是本次取指针最大范围
uint8_t *pktBody(uint8_t *data,uint32_t size,uint32_t max){
    if(size<HG_RTP_PACKET_HEAD_SIZE+max)
        return nullptr;

        return data + HG_RTP_PACKET_HEAD_SIZE;

}
