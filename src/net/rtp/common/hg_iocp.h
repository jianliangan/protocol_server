//
// Created by ajl <429200247@qq.com> on 2020/12/20.
//contain audiostream vediostream comstream
//

#ifndef HG_ANDROID_IOCP_CLIENT_H
#define HG_ANDROID_IOCP_CLIENT_H



#include "net/rtp/common/rtp_packet.h"
#include "net/rtp/common/huage_sendstream.h"
#include "net/rtp/common/huage_recvstream.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/hg_channel_stru.h"
#include "net/rtp/common/hgfdevent.h"
#include "net/rtp/common/hg_pipe.h"
#include "net/rtp/common/hg_netcommon.h"
#include <sys/epoll.h>
#define HG_MAX_OPEN_FD 20
class HgPipe;
class HgIocp {
public:
    uint32_t confd=0;
    HgPipe *aPipeClient= nullptr;
    t_mermoryPool *fragCache= nullptr;
    struct epoll_event tep,ep[20];
    int efd=0;
    HgFdEvent *fdchainfree= nullptr;
    pthread_t runThread;//接收数据
    HgIocp();
    void StartRun();
    static int writePipe(HgPipe *hp,char *data,int size);
    int AddFdEvt(int fd,int op,void *ctx,int opa);
    static void *loop(void *ctx);
    ~HgIocp();

};


#endif //HG_ANDROID_IOCP_CLIENT_H
