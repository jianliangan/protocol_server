//
// Created by ajl on 2021/11/24.
//

#ifndef HG_RTP_APP_HG_CHANNEL_STRU_H
#define HG_RTP_APP_HG_CHANNEL_STRU_H

#include <netinet/in.h>
#include "common/hg_buf_comm.h"
#include "threads/hg_tevent.h"
#include "threads/hg_channel.h"
//iocp handle
class HuageConnect;
class HgFdEvent;
typedef void(*IocpHandle)(void *ioctx, void *ctx, void *ptr);

//
typedef struct t_selfCallBack {
    HgChannel *chan=NULL;
    tf_evenHandle cbhandle;
} t_selfCallBack;
#define HG_NET_IO_SESS_ERROR -3
#define HG_NET_IO_ERROR -1
#define HG_NET_IO_CLOSED -2

#define HG_NET_IO_UN 0
#define HG_NET_IO_TCP 1
#define HG_NET_IO_UDP 2
inline const char *errorms(int l){
    switch (l) {
        case  HG_NET_IO_CLOSED:
            return "net fd close";
        case  HG_NET_IO_ERROR:
            return "net io error";
        case  HG_NET_IO_SESS_ERROR:
            return "connect error";
        case  HG_NET_IO_UN:
            return "unknow";
        case HG_NET_IO_TCP:
            return "tcp";
        case HG_NET_IO_UDP:
            return "udp";
        default:
            return "unknow";
    }
}
typedef struct t_responseParams {
    int id=0;
    int code=0;
} t_responseParams;
//udp packet raw handle params
typedef struct t_clientPacketRecvParams {
    struct sockaddr_in sa={0};
    struct t_chainNodeData *data=NULL;
    int size=0;
    void *ctx=NULL;
    t_selfFree sfree;
} t_clientPacketRecvParams;

typedef struct t_mediaFrameChain{
    t_chainList hct;
    t_selfFree sfree;
    int size=0;
    uint16_t pts=0;
    t_mediaFrameChain():hct(),sfree(),size(0),pts(0){}
    void init(){
        this->hct.init();
        this->sfree.init();
        this->size=0;
        this->pts=0;
    }
    void reset(){
        this->hct.reset();
        this->size=0;
    }
}t_mediaFrameChain;

typedef struct t_clientListRecvParams {
    t_mediaFrameChain mfc;
    void *ctx=NULL;

    void *ev_ptr=NULL;
} t_clientListRecvParams;
///////////////////////////////////
typedef struct t_clientPacketSendParams {
    unsigned char *data=NULL;
    int size=0;
    unsigned int time=0;
    void *ctx=NULL;
    HuageConnect *hgConn=NULL;
    t_selfFree sfree;
} t_clientPacketSendParams;
typedef struct t_clientPacketSendFreeParams {
    void *data=NULL;
    void *ctx=NULL;
} t_clientPacketSendFreeParams;

typedef struct t_clientFrameSendParams {
    t_mediaFrameChain mfc;
int comptype=0;
    uint32_t ssrc=0;
    int pltype=0;
    int isfirst=0;
    HuageConnect *hgConn=NULL;
} t_clientFrameSendParams;



#endif //HG_RTP_APP_HG_CHANNEL_STRU_H
