//
// Created by ajl on 2020/12/27.
//


#ifndef HUAGE_PROTOCOL_STREAM_H
#define HUAGE_PROTOCOL_STREAM_H

#include "net/rtp/common/rtp_packet.h"
#include "net/rtp/common/huage_connect.h"



#define HG_EV_HG_PTR_SIZE(data) ((uint32_t *) (data))
#define HG_EV_HG_PTR_CMD(data) ((uint8_t *) ((data) + sizeof(uint32_t)))
#define HG_EV_HG_PTR_TIME(data) ((uint32_t * )((data) + sizeof(uint32_t) + 1))
#define HG_EV_HG_PTR_SSRC(data) ((uint32_t * )((data) + sizeof(uint32_t) + 1+sizeof(uint32_t)))
#define HG_EV_HG_PTR_SESS(data) ((HuageConnect ** )((data) + sizeof(uint32_t) + 1+sizeof(uint32_t)*2))
#define HG_EV_HG_PTR_DATA(data) ((uint8_t ** )((data) + sizeof(uint32_t) + 1+sizeof(uint32_t)*2+sizeof(void *)))
#define HG_EV_HG_PTR_ADDR(data) ((sockaddr_in *) ((data) + sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2))
#define HG_EV_HG_PTR_FROM(data) ((uint8_t *) ((data) + sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2+sizeof(sockaddr_in)))
#define HG_EV_HG_PTR_FD(data) ((int *) ((data) + sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2+sizeof(sockaddr_in)+1))
#define HG_EV_HG_PTR_EVENTLEN ( sizeof(uint32_t)+1+sizeof(uint32_t)*2+sizeof(void *)*2+sizeof(sockaddr_in)+1+4)
typedef struct t_sendArr{
    void *data=NULL;
    int size=0;
}t_sendArr;
typedef struct hgEvent { //每帧的头
    uint32_t size=0;//前4个字节标识长度
    int8_t cmd=0;
    int8_t first=0;
    uint16_t time=0;//时间戳
    uint32_t ssrc=0;
    HuageConnect *hgConn=NULL;
    uint8_t *data=NULL;
    sockaddr_in sa={0};//only recv need assign
    int8_t from=0;
    int fd=0;
} hgEvent;

#endif //HUAGE_PROTOCOL_STREAM_H
