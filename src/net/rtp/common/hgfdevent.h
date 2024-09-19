//
// Created by ajl on 2021/12/24.
//

#ifndef HG_RTP_APP_HGTCPCONN_H
#define HG_RTP_APP_HGTCPCONN_H

#include "common/hg_pool.h"
#include "net/rtp/common/hg_channel_stru.h"
#define HG_MY_NOBLOCK_F(s) fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
class HuageConnect;
class HgFdEvent {
public:
    t_mermoryPool *fragCache=nullptr;
    t_mediaFrameChain chainbufsR;

    HuageConnect *hgConn= nullptr;
    HgFdEvent *fnext= nullptr;
    void *ctx= nullptr;
    IocpHandle write= nullptr;
    IocpHandle read= nullptr;
    int fd=0;

    t_chainNodeData tcpbuf;
    bool isconnected=false;
    uint8_t status;//8 bit 1代表必须新建cache，0代表自己检查是否需新建，7 bit代表是否有头0标识没有，1标识有
    static int setFreeFdEv(HgFdEvent **efreechain, HgFdEvent *fdevent);

    static HgFdEvent *getFreeFdEv(HgFdEvent **efreechain);

    static void fdEvChainInit(HgFdEvent **efreechain, int eNums);
    static void fdEvRecvPtrStream1(HgFdEvent *hgConn, t_mediaFrameChain **recvMergebuf);

};


#endif //HG_RTP_APP_HGTCPCONN_H
