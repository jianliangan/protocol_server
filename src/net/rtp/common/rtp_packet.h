//
// Created by ajl on 2020/12/27.
//
//
#ifndef HUAGE_PROTOCOL_PACKET_H
#define HUAGE_PROTOCOL_PACKET_H

#include "common/tools.h"
#include "net/rtp/common/extern.h"

#define HG_TCP_PACKET_HEAD_SIZE 4
#define HG_MAX_RTP_PACKET_SIZE 1000  //保证每个包可以整除4，512-12=500

#define HG_RTP_PACKET_HEAD_SIZE 10


//标识做什么的消息，即消息类型，鉴权的两类消息是低级的，不需要验证权限，每一种类型代表一种处理流程
enum enumt_CMDTYPE {
    enum_CMD_DEFAULT, enum_RTP_CMD_POST, enum_RTP_CMD_RES_REPOST, enum_RTP_CMD_REQ_REPOST, enum_RTP_CMD_TCP
};


typedef struct t_rtpHeader {
    /* byte 0 */
    //uint8_t version;//2 bit 0是第一个版本
    //uint8_t padding;//2 bit没实际用,0代表不扩展头部，1代表扩展4位，2代表扩展16位
    uint8_t first;//1 bit标记open
    uint8_t comp;//1 bit 标记是复杂体
    //6 bit 未用
   // uint8_t channel;//3 bit 子channel 适合几个子通道的需求，不够用就尾部增加扩展头
    /* byte 1 */
    uint8_t cmd;//3 bit 1，投递信息，2，重发投递，3，重发失败回执，4，tcp
    uint8_t tail;//2 bit后两位标志包开始结尾结束
    uint8_t payloadType;// 3 bit 1,文本，2,音频,3，视频,4，心跳，后三位用来标志 8种内容，前面还剩余4位可用
    /* bytes 2,3 */
    uint16_t seq;// 16 bit
    /* bytes 4-5 */
    uint16_t timestamp;// 16 bit
    /* bytes 6-9 */
    uint32_t ssrc;// 32 bit
    /* data */
    uint8_t *payload;/*
 comp为1时支持256种其他复杂体格式，复杂体第一个字节代表协议id，第二个字节代表长度，单位依据协议id确定，一般为32位
（byte1= 0）：代表携带（byte2=n）个16位，目前在音频流里用来携带视频pts
 */
    //8字节 扩展头部，4音频pts+4用户数
    t_rtpHeader():first(0),comp(0),cmd(0),tail(0),payloadType(0),seq(0)
                ,timestamp(0),ssrc(0),payload(nullptr)
    {}
    void init(){

        this->first=0;
        this->comp=0;
        this->cmd=0;
        this->tail=0;
        this->payloadType=0;
        this->seq=0;
        this->timestamp=0;
        this->ssrc=0;
        this->payload= nullptr;
    }
} t_rtpHeader;

uint8_t *rtpPacketHead(uint8_t *dst, t_rtpHeader *rh);

uint32_t pktSsrc(uint8_t *data, uint32_t size);
int pktFlag(uint8_t *data,uint32_t size);
uint16_t pktSeq(uint8_t *data, uint32_t size);
void pktSetSeq(uint8_t *data, uint16_t value);
uint16_t pktTimestamp(uint8_t *data, uint32_t size);

int pktChann(uint8_t *data, uint32_t size);

enumt_PLTYPE pktPLoadType(uint8_t *data, uint32_t size);

uint8_t pktFirst(uint8_t *data, uint32_t size);

uint8_t pktPadding(uint8_t *data, uint32_t size);

uint8_t pktCmd(uint8_t *data, uint32_t size);
uint8_t pktComp(uint8_t *data,uint32_t size);
void pktSetCmd(uint8_t *data, int value);

uint8_t *pktBody(uint8_t *data, uint32_t size, uint32_t max);

#endif //HUAGE_PROTOCOL_PACKET_H
