//
// Created by ajl <429200247@qq.com> on 2020/12/20.
//contain audiostream vediostream comstream
//

#ifndef ANDROID_PROJECT_HGPIPE_H
#define ANDROID_PROJECT_HGPIPE_H
#include "net/rtp/common/rtp_packet.h"
#include "common/tools.h"
#include "common/hg_buf.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/hg_netcommon.h"

//typedef void (*handleRecv_def)(void *ctx,char*,int);
class HgPipe {
public:

    t_mermoryPool *fragCache=nullptr;
    t_chainNode *freen= nullptr;
    t_chainList hct;

    int lock=0;
    int pipe_fd[2];
    t_chainNodeData tcpbuf;
    void *data= nullptr;
    HgPipe();
    int Handlemsg(int pfd, t_chainNodeData *tcpb);
    static void pipeRecvmsg(void *ioctx, void *ctx, void *ptr);
    static void pipeSendmsg(void *ioctx, void *ctx, void *ptr);
    static int writePipe(HgPipe *hp,void *data, int size);
    static int writePipe0(HgPipe *hp,void *data, int size);

    ~HgPipe();

};

#endif //ANDROID_PROJECT_HGPIPE_H
