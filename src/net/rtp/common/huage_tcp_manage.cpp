//
// Created by ajl on 2022/1/11.
//

#include "net/rtp/common/huage_tcp_manage.h"
#include "app/app.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/huage_net_manage.h"
#include "net/rtp/common//hgfdevent.h"

HuageTcpManage::HuageTcpManage() {
    finished = false;

    isclient = false;
    lastclotim = 0;
    fdev = nullptr;
    iocp = nullptr;
    // 使用socket()，生成套接字文件描述符；
}

int HuageTcpManage::GetSocket() {
    int confd = 0;
    if ((confd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        HG_ALOGE(0, "tcp client socket error %s", strerror(errno));
    }

    return confd;
}

int HuageTcpManage::Connect() {
    if (!isclient) {
        return -1;
    }
    int confd = 0;
    confd = GetSocket();
    if (connect(confd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr)) !=
            0) {
        close(confd);
        return errno;
    }
    HG_MY_NOBLOCK_F(confd);
    fdev = InitEvent(iocp, confd);
    return 0;
}

HgFdEvent *HuageTcpManage::InitEvent(HgIocp *iocp, int confd) {
    HgFdEvent *fdevtcp = HgFdEvent::getFreeFdEv(&iocp->fdchainfree);
    fdevtcp->ctx = this;
    fdevtcp->read = HuageTcpManage::tcpRecvmsg;
    fdevtcp->write = HuageTcpManage::tcpSendMsg;
    fdevtcp->fd = confd;
    fdevtcp->fragCache = memoryCreate(1024 * 2);
    fdevtcp->tcpbuf.data = memoryAlloc(fdevtcp->fragCache, HG_RECV_BUFFER_LEN);
    fdevtcp->tcpbuf.start = 0;
    fdevtcp->tcpbuf.end = 0;
    fdevtcp->tcpbuf.cap = HG_RECV_BUFFER_LEN;
    fdevtcp->tcpbuf.len = 0;
    iocp->AddFdEvt(confd, EPOLL_CTL_ADD, fdevtcp,0);
    fdevtcp->isconnected = true;
    return fdevtcp;
}

HgFdEvent *HuageTcpManage::InitLisEvent(HgIocp *iocp, int confd) {
    HgFdEvent *fdevtcp = HgFdEvent::getFreeFdEv(&iocp->fdchainfree);
    fdevtcp->ctx = this;
    fdevtcp->read = HuageTcpManage::tcpAccept;
    fdevtcp->write = nullptr;
    fdevtcp->fd = confd;
    iocp->AddFdEvt(confd, EPOLL_CTL_ADD, fdevtcp,1);
    fdevtcp->isconnected = true;
    return fdevtcp;
}

void HuageTcpManage::SetServer(const char *serverip, uint32_t port) {
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(serverip);
    serveraddr.sin_port = htons(port);
    bzero(&(serveraddr.sin_zero), 8);

}

HuageTcpManage::~HuageTcpManage() {

}

void HuageTcpManage::tcpAccept(void *ioctx, void *ctx, void *ptr) {
    HgIocp *iocp = (HgIocp *) ioctx;
    HgFdEvent *fdev = (HgFdEvent *) ptr;
    struct sockaddr_in clientAddr;
    socklen_t socklen;
    HuageTcpManage *htb = (HuageTcpManage *) ctx;
    int connfd = accept(fdev->fd, (struct sockaddr *) &clientAddr, &socklen);
    if (connfd == -1) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    HG_MY_NOBLOCK_F(connfd);
    htb->InitEvent(htb->iocp, connfd);
}

void HuageTcpManage::tcpRecvmsg(void *ioctx, void *ctx, void *ptr) {
    int n = 0;
    bool hashead;
    int framerest = 0, bodysize = 0;
    HgFdEvent *hfde = (HgFdEvent *) ptr;
    int pfd = hfde->fd;

    t_mermoryPool *mpool = hfde->fragCache;
    HuageTcpManage *context = (HuageTcpManage *) ctx;
    t_chainNodeData *tcpb = &hfde->tcpbuf;

    uint8_t *recvLine = nullptr;
    enumt_PLTYPE pltype = enum_PLTYPENONE;
    uint32_t ssrc = 0;
    int seq = 0;
    enumt_CMDTYPE cmdtype;
    int error = 0;
    uint8_t tmp[16];
    uint8_t isfirst;
    int canwrsize = 0;
    int canresize = 0;
    int needsize = 0;
    int len=0;
    t_mediaFrameChain *hctrecvframe = nullptr;
    while (1) {
        n = 0;


        if (tcpb->start > tcpb->end) {
            len=tcpb->start - tcpb->end;
            n = recv(pfd, (char *) tcpb->data + tcpb->end, len, 0);
            if (n >= 0) {
                tcpb->end = (tcpb->end + n);
                if(tcpb->end>=tcpb->cap){
                    tcpb->end-=tcpb->cap;
                }
                tcpb->len += n;
            }
        } else {
            len=tcpb->cap - tcpb->end;
            n = recv(pfd, (char *) tcpb->data + tcpb->end, len, 0);
            if (n >= 0) {
                tcpb->end = (tcpb->end + n);
                if(tcpb->end>=tcpb->cap){
                    tcpb->end-=tcpb->cap;
                }
                tcpb->len += n;
            }

        }

        if (n <= 0) {
            error = errno;
            if (error == EAGAIN || error == EINTR) {
                break;
            } else {
                //io错误，去重连
                break;
            }
        }
        hashead=(hfde->status>>1)&0x01;
        ///检测可用的数据
        if (!hashead) {
            if (Hg_Buf_ReadN(tmp, tcpb, HG_TCP_PACKET_HEAD_SIZE + HG_RTP_PACKET_HEAD_SIZE,
                        HG_TCP_PACKET_HEAD_SIZE) == 0) {
                bodysize = *((int *) tmp);
                if (bodysize <= 0 || bodysize > HG_MAX_FRAME_LEN) {
                    return;
                }
                framerest = bodysize;
                hashead = true;
                hfde->status=hfde->status|0x02;
                int pltype = 0;
                pltype = pktPLoadType(tmp + HG_TCP_PACKET_HEAD_SIZE, HG_RTP_PACKET_HEAD_SIZE);
                HgFdEvent::fdEvRecvPtrStream1(hfde, &hctrecvframe);
                hctrecvframe->hct.left= nullptr;
                hctrecvframe->hct.right= nullptr;
                hctrecvframe->size=0;
            } else {
                continue;
            }
        }
        if (hashead && hctrecvframe != nullptr) {
            int nextframe;
            while (true) {
                nextframe=((hfde->status&0x01)==0?1:0);
                recvLine = (uint8_t *) Hg_GetSingleOrIdleBuf(mpool, &hctrecvframe->hct,
                        nullptr, &canwrsize,
                        HG_RECV_MAX_BUF_SIZE, nextframe, 0);
                if (nextframe == 1) {
                    t_chainNode *hcn = hctrecvframe->hct.left;
                    t_chainNodeData *hbgt = (t_chainNodeData *) hcn->data;
                }
                nextframe=0;
                hfde->status=hfde->status|0x01;
                canresize = tcpb->len;
                needsize = canresize > canwrsize ? canwrsize : canresize;
                needsize = framerest > needsize ? needsize : framerest;

                if (Hg_Buf_ReadN(recvLine, tcpb, needsize, needsize) != -1) {
                    framerest -= needsize;
                    t_chainNode *hcn = hctrecvframe->hct.right;
                    t_chainNodeData *hbgt = (t_chainNodeData *) hcn->data;
                    hbgt->len+=needsize;
                    hbgt->end+=needsize;
                    if(hbgt->end>=hbgt->cap){
                        hbgt->end -=hbgt->cap;
                    }
                } else {
                    break;//继续去拿
                }

                if (framerest == 0) {

                    //hctrecvframe->sfree.ctx = ptr;
                    hctrecvframe->sfree.params = nullptr;
                    hctrecvframe->sfree.freehandle = nullptr;
                    hctrecvframe->size = bodysize;


                    char jsonblock[4096]={0};
                    int total=0;
                    t_chainNodeData *hbgt=nullptr;
                    t_chainNode *hcn=hctrecvframe->hct.left;
                    hbgt=(t_chainNodeData *)hcn->data;
                    hbgt->start=HG_RTP_PACKET_HEAD_SIZE;

                    if(hbgt->end>=hbgt->cap){
                        hbgt->end-=hbgt->cap;
                    }

                    hbgt->len=hbgt->len-HG_RTP_PACKET_HEAD_SIZE;
                    while (hcn!=nullptr) {
                        hbgt=(t_chainNodeData *)hcn->data;
                        memcpy((char *)jsonblock+total,(char *)hbgt->data+HG_RTP_PACKET_HEAD_SIZE,hbgt->len);
                        total+=hbgt->len;
                        hcn=hcn->next;
                    }





                    context->RecvData(ptr, hctrecvframe);
                    //hctrecvframe->hct.right = nullptr;
                    //hctrecvframe->hct.left = nullptr;
                    //hctrecvframe->size = 0;

                    hashead = false;
                    nextframe=1;
                    hfde->status=hfde->status&0xfc;
                    break;
                } else if (tcpb->len > 0) {
                    continue;
                }
            }
            //////////////////
        }
        if(n<len){
            return;
        }
    }
}

void HuageTcpManage::tcpSendMsg(void *ioctx, void *ctx, void *ptr) {
    HgFdEvent *hfe = (HgFdEvent *) ptr;
    HuageConnect *hgConn = hfe->hgConn;
    preSendDataChain(ioctx, ctx, hfe, hgConn->ssrc);
}

void HuageTcpManage::sendChain(void *pth, void *ctx, void *params, int psize) {

    t_twoAddr *paramptr = (t_twoAddr *) params;
    HgIocp *ioctx = (HgIocp *) paramptr->param1;
    HuageTcpManage *htcl = (HuageTcpManage *) ctx;
    int efd = ioctx->efd;
    HgFdEvent *hfe = (HgFdEvent *) paramptr->param2;
    HuageConnect *hgConnect = hfe->hgConn;
    t_chainList *hct = nullptr;
    HuageConnect::hgConnSendPtrStreamTCP(hgConnect, &hct);
    if (!hfe->isconnected) {
        HuageTcpManage::clearSndChain(hct, hgConnect, true);
        return;
    }


    if (htcl->PreSendArr(efd, hct, hgConnect, hfe) == HG_EP_ERROR) {
        HuageTcpManage::clearSndChain(hct, hgConnect, true);
    }
    HuageTcpManage::clearSndChain(hct, hgConnect, false);


}

void HuageTcpManage::Init() {
    if (isclient) {
        int error = Connect();
    }

}

void HuageTcpManage::Listen(int port, const char *ip) {
    int confd = 0;
    confd = GetSocket();

    struct sockaddr_in serverAddr, clientAddr;
    int serverIp = 0, err = 0;
    struct epoll_event tep;
    // 通过struct sockaddr_in 结构设置服务器地址和监听端口；
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, (void *) &serverIp);
    serverAddr.sin_addr.s_addr = serverIp;

    //socklen_t reelen = sizeof(uint32_t);
    // int32_t ree;
    // int32_t rees;
    // err = getsockopt(confd, SOL_SOCKET, SO_RCVBUF, (void *) &ree, &reelen);
    // if (err == -1) {
    //     HG_ALOGI(0, "err getsockopt");
    // }
    //  err = getsockopt(confd, SOL_SOCKET, SO_SNDBUF, (void *) &rees, &reelen);
    //  if (err == -1) {
    //      HG_ALOGI(0, "err getsockopt");
    //  }
    //HG_ALOGI(0, "start recv %s:%d,rcvbuf:%d,sendbuf:%d", ip, port, ree, rees);
    int ret = 0;
    int one = 1;
    if (setsockopt(confd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        close(confd);
        return;
    }

    ret = bind(confd, (const struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (ret == -1) {
        HG_ALOGI(0, "bind error %s", strerror(errno));

        return;
    }
    listen(confd, 20);
    InitLisEvent(iocp, confd);
}

int HuageTcpManage::PreSendArr(int efd, t_chainList *hct, HuageConnect *hgConnect, HgFdEvent *hfe) {
    int total = 4;
    int hasnum = 0;
    int ret = 0;
    t_chainNode *hcnleft = nullptr;
    t_chainNodeData *hbgtleft = nullptr;
    t_chainNodeData *hbgttmp[total];

    for (int i = 0; i < total; i++) {
        if (hcnleft == nullptr) {
            hcnleft = hct->left;
        } else {
            hcnleft = hcnleft->next;
        }
        if (hcnleft == nullptr)
            break;
        hasnum = i + 1;
        hbgttmp[i] = (t_chainNodeData *) hcnleft->data;
    }
    ret = SendDatapre(efd, hct, hbgttmp, hasnum, hgConnect, hfe);

    return ret;
}

void HuageTcpManage::preSendDataChain(void *ioctx, void *ctx, HgFdEvent *hfe, uint32_t ssrc) {
    t_twoAddr addrs = {ioctx, hfe};

    int lens = sizeof(t_huageEvent) + sizeof(void *);
    char hgteventptr[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) hgteventptr;

    hgtevent->handle = HuageTcpManage::sendChain;
    hgtevent->i=3;
    hgtevent->psize = lens - sizeof(t_huageEvent);
    hgtevent->ctx = ctx;
    memcpy((char *) hgteventptr + sizeof(t_huageEvent), &addrs, sizeof(t_twoAddr));
    HgWorker *hgworker = HgWorker::getWorker(ssrc);
    hgworker->WriteChanWor(hgteventptr, lens);

}

void HuageTcpManage::threadPOOLRecv(void *pth, void *ctx, void *params, int size) {

    t_clientListRecvParams *clrp = (t_clientListRecvParams *) params;

    t_mediaFrameChain *mfc = &clrp->mfc;
    t_chainNode *hcn = mfc->hct.left;
    HgFdEvent *ptr = (HgFdEvent *) clrp->ev_ptr;
    HuageTcpManage *context = (HuageTcpManage *) clrp->ctx;

    t_chainNodeData *hbgt = (t_chainNodeData *) hcn->data;
    uint8_t *rtpdata = (uint8_t *) hbgt->data;

    HgWorker *hworker = (HgWorker *) pth;
    HuageNetManage *huageNetManage=HuageNetManage::huageNetManage;
    HgConnectBucket *hgConnectBucket = huageNetManage->hgConnBucket;
    int udpLength = mfc->size;
    enumt_PLTYPE pltype = pktPLoadType(rtpdata, HG_RTP_PACKET_HEAD_SIZE);
    int isfirst = pktFirst(rtpdata, HG_RTP_PACKET_HEAD_SIZE);
    int ssrc = pktSsrc(rtpdata, HG_RTP_PACKET_HEAD_SIZE);
    HuageConnect *hgConnPtr= ptr->hgConn;
    if(hgConnPtr== nullptr) {
        hgConnPtr = HuageConnect::hgConnGetFreeConn(hgConnectBucket);
        ptr->hgConn = hgConnPtr;
	hgConnPtr->ssrc=ssrc;
        hgConnPtr->tcpfd = ptr;
    }
    context->recvCallback(pltype,  mfc, hgConnPtr);
}

void HuageTcpManage::clearSndChain(t_chainList *hct, HuageConnect *hgConnect, bool all) {
    t_chainNode *hcnleft = hct->left;
    t_chainNodeData *hbgtleft = nullptr;
    if (all) {
        while (hcnleft != nullptr) {
            hbgtleft = (t_chainNodeData *) hcnleft->data;
            Hg_ChainDelNode(hgConnect->fragCache, hcnleft);
            if (hbgtleft->sfree.freehandle != nullptr)
                hbgtleft->sfree.freehandle(hbgtleft->sfree.ctx, hbgtleft->sfree.params);
            hct->left=hcnleft = hcnleft->next;
        }
    } else {
        while (hcnleft != nullptr) {
            hbgtleft = (t_chainNodeData *) hcnleft->data;
            if (hbgtleft->len == 0) {

                Hg_ChainDelNode(hgConnect->fragCache, hcnleft);
                if (hbgtleft->sfree.freehandle != nullptr)
                    hbgtleft->sfree.freehandle(hbgtleft->sfree.ctx, hbgtleft->sfree.params);
            }
            hct->left=hcnleft = hcnleft->next;
        }
    }
    if (hct->left == NULL || hct->right == NULL) {
        hct->left = hct->right = NULL;
    }
}

int HuageTcpManage::SendDatapre(int efd, t_chainList *hct, t_chainNodeData **hbgtarr, int num,
        HuageConnect *hgConnect,
        HgFdEvent *hfe) {

    t_sendArr sendarr[num];
    for (int i = 0; i < num; i++) {
        t_chainNodeData *hbgtleft = *(hbgtarr + i);
        sendarr[i].data = (char *) hbgtleft->data + hbgtleft->start;
        sendarr[i].size = hbgtleft->len;
    }


    int n = 0, ret = 0, fd = 0;
    fd = hgConnect->tcpfd->fd;
    n = SendData0(fd, sendarr, num);
    if (n >= 0) {
        int rest = n;
        for (int i = 0; i < num; i++) {
            t_chainNodeData *hbgtleft = *(hbgtarr + i);
            if (hbgtleft->len <= rest) {
                hbgtleft->len = 0;
                hbgtleft->start += n;

                if(hbgtleft->start>=hbgtleft->cap){
                    hbgtleft->start-=hbgtleft->cap;
                }

                rest -= hbgtleft->len;
            } else {
                hbgtleft->len -= rest;
                hbgtleft->start += rest;
                if(hbgtleft->start>=hbgtleft->cap){
                    hbgtleft->start-=hbgtleft->cap;
                }
                rest = 0;
            }
            if (rest == 0) {
                break;
            }
        }


    } else if (n == HG_EP_AGAIN) {
        /*struct epoll_event tep;
          tep.events = EPOLLIN | EPOLLOUT | EPOLLET;
          tep.data.ptr = hfe;
          ret = epoll_ctl(efd, EPOLL_CTL_MOD, hfe->fd, &tep);*/
        ret = HG_EP_AGAIN;
        return ret;
    } else {
        epoll_ctl(efd, EPOLL_CTL_DEL, hfe->fd, NULL);
        close(hfe->fd);
        hfe->hgConn->tcpfd= nullptr;
        hfe->hgConn= nullptr;

        HgFdEvent::setFreeFdEv(&iocp->fdchainfree, hfe);
        this->lastclotim = hgetSysTimeMicros() / 1000;
        this->fdev = nullptr;
        return HG_EP_ERROR;
    }
    return n;
}

int HuageTcpManage::SendData(void *pth, enumt_PLTYPE pltype,bool isfirst, t_mediaFrameChain *mfcsour,
        uint32_t ssrc, HuageConnect *hgConnect, HgFdEvent *hfe,int comptype) {

    if (!hfe->isconnected) {
        return HG_EP_ERROR;
    }

uint16_t time=mfcsour->pts;
    HgIocp *hgio = (HgIocp *) (this->iocp);
    int efd = hgio->efd;

    t_chainList *hgConnhct = nullptr;

    int size = mfcsour->size;

    //pktSetCmd((uint8_t *) hbgt->data, (int) enum_RTP_CMD_TCP);
    //pktSetSeq((uint8_t *) hbgt->data, 0);

    uint16_t *seq = nullptr;
    t_sendArr sendarr[2];
    HuageConnect::hgConnSendPtrStreamTCP(hgConnect, &hgConnhct);

    HuageConnect::hgConnSendPtrStreamSeq(hgConnect, pltype, &seq);

    int ret = 0;

    ///////////
    t_chainNodeData *head = CreateBuffer(pltype, isfirst, time,
            hgConnect, size,comptype);
    //////////////////

    //if (hcnleft != nullptr) {
    PushFramCach(mfcsour, hgConnect, hgConnhct, head);
    if (PreSendArr(efd, hgConnhct, hgConnect, hfe) == HG_EP_ERROR) {
        HuageTcpManage::clearSndChain(hgConnhct, hgConnect, true);
        ret = HG_EP_ERROR;
    } else {
        HuageTcpManage::clearSndChain(hgConnhct, hgConnect, false);
    }
    ////////////
    return ret;
}

t_chainNodeData *HuageTcpManage::CreateBuffer(uint8_t pltype, bool isfirst, uint16_t time,
        HuageConnect *hgConnect, int size,int comptype) {
    t_rtpHeader rh;
    rh.ssrc = hgConnect->ssrc;
    rh.payloadType = (uint8_t) pltype;
rh.comp=comptype;
    rh.first = 0;
    if (isfirst) {
        rh.first = 1;
    }

    rh.cmd = enum_RTP_CMD_TCP;
    rh.timestamp = time;
    rh.seq = 0;
    rh.tail = 0;

    t_chainNodeData *rhtmp = (t_chainNodeData *) memoryAlloc(hgConnect->fragCache,
            sizeof(t_chainNodeData) + HG_RTP_PACKET_HEAD_SIZE +
            HG_TCP_PACKET_HEAD_SIZE);
    rhtmp->init();


    int *tmphead = (int *) ((char *) rhtmp + sizeof(t_chainNodeData));
    *tmphead = size;
    rtpPacketHead((unsigned char *) rhtmp + sizeof(t_chainNodeData) + HG_TCP_PACKET_HEAD_SIZE, &rh);
    rhtmp->data = (char *) rhtmp + sizeof(t_chainNodeData);
    rhtmp->start = 0;
    rhtmp->len = HG_RTP_PACKET_HEAD_SIZE + HG_TCP_PACKET_HEAD_SIZE;
    rhtmp->end = 0;
    rhtmp->cap = HG_RTP_PACKET_HEAD_SIZE + HG_TCP_PACKET_HEAD_SIZE;
    rhtmp->sfree.ctx = hgConnect;
    rhtmp->sfree.params = rhtmp;
    rhtmp->sfree.freehandle = HuageTcpManage::destroyHeads;
    return rhtmp;
}

void
HuageTcpManage::PushFramCach(t_mediaFrameChain *mfcsour, HuageConnect *hgConnect, t_chainList *hgConnhct,
        t_chainNodeData *head) {
    t_chainNode *hcnlefttmp = mfcsour->hct.left;
    t_chainNodeData *hbgttmp = nullptr;
    Hg_PushChainDataR(hgConnect->fragCache, hgConnhct, head);
    while (hcnlefttmp != nullptr) {
        hbgttmp = (t_chainNodeData *) hcnlefttmp->data;
        Hg_PushChainDataR(hgConnect->fragCache, hgConnhct, hbgttmp);
        hcnlefttmp = hcnlefttmp->next;
    }
}

///////////////////////
int
HuageTcpManage::RecvData(void *ptr, t_mediaFrameChain *mfc) {
    uint8_t *rtpdata = nullptr;
    if (mfc != nullptr) {
        t_chainNodeData *hbgt = nullptr;
        t_chainNode *hcn = (t_chainNode *) mfc->hct.left;
        hbgt = (t_chainNodeData *) hcn->data;

        t_chainNode *hcnR = (t_chainNode *) mfc->hct.right;
        t_chainNodeData *hbgtR;
        hbgtR = (t_chainNodeData *) hcnR->data;
        hbgtR->freenum = 0;
        hbgtR->sfree.ctx = ptr;
        hbgtR->sfree.params = mfc->hct.left;
        hbgtR->sfree.freehandle = HuageTcpManage::writePipeFree;
        rtpdata = (uint8_t *) hbgt->data;

    }

    uint32_t ssrc = pktSsrc(rtpdata, mfc->size);

    int lens = sizeof(t_huageEvent) + sizeof(t_clientListRecvParams);
    char hgteventptr[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) hgteventptr;
    t_clientListRecvParams *clrp = (t_clientListRecvParams *) (hgteventptr + sizeof(t_huageEvent));

    clrp->mfc = *mfc;
    clrp->ctx = this;
    clrp->ev_ptr = ptr;

    hgtevent->handle = HuageTcpManage::threadPOOLRecv;
    hgtevent->i=4;
    hgtevent->ctx = this;
    hgtevent->psize = sizeof(t_clientListRecvParams);

    HgWorker *hgworker = HgWorker::getWorker(ssrc);
    hgworker->WriteChanWor(hgteventptr, lens);

    return 0;
}

int HuageTcpManage::SendData0(int confd, t_sendArr *sendarr, int size) {

    struct msghdr snd_msg;
    struct iovec snd_iov[size];

    for (int i = 0; i < size; i++) {
        snd_iov[i].iov_base = sendarr[i].data;
        snd_iov[i].iov_len = sendarr[i].size;
    }


    snd_msg.msg_name = nullptr; // Socket is connected
    snd_msg.msg_namelen = 0;
    snd_msg.msg_iov = snd_iov;
    snd_msg.msg_iovlen = size;
    snd_msg.msg_control = 0;
    snd_msg.msg_controllen = 0;

    int n = 0;
    int error = 0;
    n = sendmsg(confd, &snd_msg, 0);

    //////////
    if (n >= 0) {
        return n;
    }
    error = errno;
    if (error == EAGAIN || error == EINTR) {
        return HG_EP_AGAIN;
        //这里需要延迟发送，nginx是放到延时里了  src/os/unix/ngx_send.c 44行  以及src/http/ngx_http_upstream.c 2594行
    } else {
        return HG_EP_ERROR;
    }
}

void HuageTcpManage::writePipeFree(void *ctx, void *data) {
    HgFdEvent *hfe = (HgFdEvent *) ctx;
    HuageTcpManage *hta = (HuageTcpManage *) hfe->ctx;
    HgIocp *hicp = (HgIocp *) hta->iocp;
    HgPipe *hp = hicp->aPipeClient;

    t_chainNode *hcn = (t_chainNode *) data;

    int lens = sizeof(t_huageEvent) + sizeof(void *) * 2;
    char tmp[lens];
    t_twoAddr params = {hfe, hcn};
    t_huageEvent *hgtevent = (t_huageEvent *) tmp;

    hgtevent->handle = HuageTcpManage::freeMemeryLocal;
    hgtevent->i=5;
    hgtevent->ctx = hp;
    hgtevent->psize = sizeof(void *) * 2;

    memcpy((char *) tmp + sizeof(t_huageEvent), &params, sizeof(params));
    HgPipe::writePipe(hp, tmp, lens);
}


void HuageTcpManage::freeMemeryLocal(void *pth, void *ctx, void *params, int psize) {
    t_twoAddr *twoparams = (t_twoAddr *) params;

    HgFdEvent *hfe = (HgFdEvent *) twoparams->param1;
    t_chainNode *hcn = (t_chainNode *) twoparams->param2;
    if (hcn->data != nullptr) {
        t_chainNodeData *hbgt = (t_chainNodeData *) hcn->data;
        hbgt->freenum--;
        if (hbgt->freenum <= 0) {
            while (hcn != nullptr) {
                memoryFree(hfe->fragCache, (uint8_t *) hcn->data);
                memoryFree(hfe->fragCache, (uint8_t *) hcn);
                hcn = hcn->next;
            }
        }

    }


}

void HuageTcpManage::destroyHeads(void *ctx, void *data) {
    HuageConnect *hgConnect = (HuageConnect *) ctx;
    memoryFree(hgConnect->fragCache, (uint8_t *) data);
}
