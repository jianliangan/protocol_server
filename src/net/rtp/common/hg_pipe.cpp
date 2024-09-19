//
// Created by ajl <429200247@qq.com> on 2020/12/20.
//

#include <string>

#include "net/rtp/common/hg_pipe.h"
#include "app/app.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/hgfdevent.h"

#define HG_MAX_PIPE_SIZE 64*1024
#define HG_HG_MAX_BUF_BLOSIZE 512

HgPipe::HgPipe() {
    fragCache = memoryCreate(HG_MAX_PIPE_SIZE);
    _spin_init(&lock, 0);
    lock = 0;
    uint8_t datachars[HG_MAX_PIPE_SIZE];
    tcpbuf.data = datachars;
    tcpbuf.cap = HG_MAX_PIPE_SIZE;
    int re = pipe(pipe_fd);
    HG_MY_NOBLOCK_F(pipe_fd[0]);
    HG_MY_NOBLOCK_F(pipe_fd[1]);
    if (re == -1) {
        HG_ALOGI(0, "create pipe invalid");
    }
}

void HgPipe::pipeRecvmsg(void *ioctx, void *ctx, void *ptr) {
    HgFdEvent *hfde = (HgFdEvent *) ptr;
    int fd = hfde->fd;
    HgPipe *context = (HgPipe *) ctx;
    struct sockaddr_in clientAddr;

    uint32_t ret = 0;
    socklen_t len = sizeof(struct sockaddr_in);

    context->Handlemsg(fd, &context->tcpbuf);

}

void HgPipe::pipeSendmsg(void *ioctx, void *ctx, void *ptr) {
    HgFdEvent *hfe = (HgFdEvent *) ptr;
    HuageConnect *hgConn = hfe->hgConn;
    HgPipe *hp = (HgPipe *) ctx;
    int n = 0;
    _spin_lock(&hp->lock);

    t_chainNode *hcnleft = hp->hct.left;
    t_chainNode *hcnleftnxt = nullptr;
    t_chainNodeData *hbgt = nullptr;
    while (hcnleft != nullptr) {
        hbgt = (t_chainNodeData *) hcnleft->data;
        n = writePipe0(hp, (char *)hbgt->data + hbgt->start, hbgt->len);
        hbgt->start += n;
        if(hbgt->start>=hbgt->cap){
            hbgt->start-=hbgt->cap;
        }
        hbgt->len -=n;
        if (n == hbgt->len) {
            hcnleft=hcnleft->next;
            Hg_SingleDelchainL(&hp->hct, &hp->freen);

        } else {
            break;
        }
    }

    _spin_unlock(&hp->lock);
}

int HgPipe::Handlemsg(int pfd, t_chainNodeData *tcpb) {
    int n = 0;
    uint8_t *recvLine = nullptr;
    int error = 0;

    uint8_t body[HG_MAX_PIPE_SIZE];
    uint8_t *tmp = nullptr;
    int headlen = sizeof(t_huageEvent);
    t_huageEvent head;
    int willgets = headlen;
    int skip=0;
    int gethead = 0;
    while (1) {
        n = 0;

        if(tcpb->end>=tcpb->cap){
            tcpb->end-=tcpb->cap;
        }
        if (tcpb->start > tcpb->end) {
            n = recv(pfd, (char *) tcpb->data + tcpb->end, tcpb->start - tcpb->end, 0);
            if (n >= 0) {
                tcpb->end += n;
                if(tcpb->end>=tcpb->cap){
                    tcpb->end-=tcpb->cap;
                }

                tcpb->len += n;
            }
        } else {
            n = recv(pfd, (char *) tcpb->data + tcpb->end, tcpb->cap - tcpb->end, 0);
            if (n >= 0) {
                tcpb->end += n;
                if(tcpb->end>=tcpb->cap){
                    tcpb->end-=tcpb->cap;
                }
                tcpb->len += n;
            }
        }
        if (n <= 0) {
            error = errno;
            if (error == EAGAIN || error == EINTR) {
                return 0;
            } else {
                //io错误，去重连
                return -2;
            }
        }

        ///检测可用的数据
        while (true) {
            if (gethead == 0) {
                tmp = (unsigned char *) &head;
                willgets = headlen;
                skip=headlen;
            } else {
                tmp = body;
                willgets = head.psize;
                skip=head.psize;
            }
            if (Hg_Buf_ReadN(tmp, tcpb, willgets, skip) == 0) {

                if (gethead == 0) {
                    gethead = 1;
                } else {
                    //void *pth,void *ctx,void *params,int psize
                    head.handle(this, head.ctx, tmp, head.psize);
                    gethead = 0;
                }//free

            } else {
                break;
            }

        }
    }

}

int HgPipe::writePipe(HgPipe *hp, void *data, int size) {
    int canwrsize = 0;
    uint8_t *recvLine;
    int rest = size;
    int realcp = 0;
    if (size <= 0) {
        return 0;
    }
    t_chainNode *hcn= nullptr;
    t_chainNodeData *hbgt= nullptr;
    _spin_lock(&hp->lock);
    while (1) {
        recvLine = (uint8_t *) Hg_GetSingleOrIdleBuf(hp->fragCache, &hp->hct,
                &hp->freen, &canwrsize,
                HG_HG_MAX_BUF_BLOSIZE, 0, 1);
        realcp = canwrsize <= rest ? canwrsize : rest;
        memcpy(recvLine, (char *)data+(size-rest), realcp);

        hcn = hp->hct.right;
        hbgt = (t_chainNodeData *) hcn->data;
        hbgt->len+=realcp;
        hbgt->end+=realcp;
        if(hbgt->end>=hbgt->cap){
            hbgt->end-=hbgt->cap;
        }
        rest -= realcp;
        if (rest == 0) {
            break;
        }
    }
    _spin_unlock(&hp->lock);
    return size;
}

int HgPipe::writePipe0(HgPipe *hp, void *data, int size) {
    int n = 0;

    n = write(hp->pipe_fd[1], (char *) data, size);
    if (n >= 0) {
        return n;
        //return n;
    }

    int error = errno;
    if (error == EAGAIN || error == EINTR) {
        sched_yield();
        return HG_EP_AGAIN;
        //这里需要延迟发送，nginx是放到延时里了  src/os/unix/ngx_send.c 44行  以及src/http/ngx_http_upstream.c 2594行
    } else {
        return HG_EP_ERROR;
    }


}

HgPipe::~HgPipe() {

}
