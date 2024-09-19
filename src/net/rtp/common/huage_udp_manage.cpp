//
// Created by ajl on 2022/1/11.
//

#include "net/rtp/common/huage_udp_manage.h"
#include "app/app.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/huage_net_manage.h"
HuageUdpManage::HuageUdpManage() {
    finished = false;
    maxmtu = HG_MAX_RTP_PACKET_SIZE;
    haslog = false;
    uint32_t recvbuff = 8 * 1024 * 1024;
    // 使用socket()，生成套接字文件描述符；
    isclient = false;
}

void HuageUdpManage::Init() {
    uint32_t err=0;

    recvth = new HuageRecvStream();
    recvth->p_recvCallback = recvCallback;
    recvth->directSendMsg = directSendMsg;
    recvth->allowothers = true;
    recvth->ctx = this;

    sendth = new HuageSendStream();
    sendth->p_sendCallback = sendCallback;
    sendth->ctx = this;

    finished = true;
    if (isclient)
        Connect();
}

void HuageUdpManage::SetServer(const char *serverip, uint32_t port) {
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(serverip);
    serveraddr.sin_port = htons(port);
}

int HuageUdpManage::GetSocket() {
    int confd=0;
    int recvbuff=8*1024*1024;
    if ((confd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        HG_ALOGE(0, "rtp client socket error %s", strerror(errno));
        finished = false;
    } else
        finished = true;
    if (setsockopt(confd, SOL_SOCKET, SO_RCVBUF, (const uint8_t *) &recvbuff, sizeof(uint32_t)) ==
            -1) {
    }
    if (setsockopt(confd, SOL_SOCKET, SO_SNDBUF, (const uint8_t *) &recvbuff, sizeof(uint32_t)) ==
            -1) {
    }
    socklen_t reelen = sizeof(uint32_t);
    uint32_t ree = 0;
    int err = getsockopt(confd, SOL_SOCKET, SO_SNDBUF, (uint8_t * ) & ree, &reelen);
    if (err != 0) {
        HG_ALOGI(1, "error getsockopt \n");
    }
    return confd;
}

void HuageUdpManage::Connect() {
    int confd0=0;
    confd0 = GetSocket();
    HG_MY_NOBLOCK_F(confd0);
    InitEvent(iocp, confd0);
    confd = confd0;
}

HgFdEvent *HuageUdpManage::InitEvent(HgIocp *iocp, int confd) {
    HgFdEvent *fdevudp = HgFdEvent::getFreeFdEv(&iocp->fdchainfree);
    fdevudp->ctx = this;
    fdevudp->read = HuageUdpManage::udpRecvmsg;
    fdevudp->write = nullptr;
    fdevudp->fd = confd;
    iocp->AddFdEvt(confd, EPOLL_CTL_ADD, fdevudp,1);
    return fdevudp;
}

HuageUdpManage::~HuageUdpManage() {
    close(confd);
}

void HuageUdpManage::Listen(int port, const char *ip) {
    int confd0=0;
    confd0 = GetSocket();
    HG_MY_NOBLOCK_F(confd0);
    int one=1;
    if (setsockopt(confd0, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        close(confd0);
        return ;
    }
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    serveraddr.sin_port = htons(port);

    if (bind(confd0, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
        HG_ALOGI(0, "bind error %s", strerror(errno));

        return;
    }
    InitEvent(iocp, confd0);
    confd = confd0;
}

int
HuageUdpManage::SendData(void *pth, enumt_PLTYPE pltype, bool isfirst, t_mediaFrameChain *mfc,
                uint32_t ssrc,
        HuageConnect *hgConn,int comptype) {
/*
    t_chainNode *hcntmp=mfc->hct.left;
    t_chainNodeData *hbgttmp= nullptr;
    FILE *f2 = fopen(HG_APP_ROOT"send0.data", "a+");
    while(hcntmp!= nullptr){
        hbgttmp=(t_chainNodeData *)hcntmp->data;
        fwrite((char *)hbgttmp->data + hbgttmp->start, 1, hbgttmp->len, f2);
        hcntmp=hcntmp->next;
    }
    fclose(f2);
*/

    return sendth->SendData(pth, pltype, isfirst, mfc, ssrc, hgConn,comptype);
}

void HuageUdpManage::udpRecvmsg(void *ioctx, void *ctx, void *ptr) {
    HgFdEvent *hfde = (HgFdEvent *) ptr;
    int fd = hfde->fd;
    int recvLength = 0;
    HuageUdpManage *context = (HuageUdpManage *) ctx;

    uint32_t bufferlen = HG_MAX_RTP_PACKET_SIZE;
    uint8_t recvLine[bufferlen];
    struct sockaddr_in clientAddr;
    bool isdrop;
    uint32_t ret = 0;
    socklen_t len = sizeof(struct sockaddr_in);
    int error;

    enumt_PLTYPE type = enum_PLTYPENONE;

    uint32_t ssrc = 0;
    int seq=0;
    enumt_CMDTYPE cmdtype;
    //FILE *f2 = fopen("/data/data/org.libhuagertp.app/1.data", "a");
    while (true) {
        isdrop = false;
        recvLength = recvfrom(fd, recvLine, sizeof(recvLine), 0,
                (struct sockaddr *) &clientAddr, &len);
        if (recvLength <= 0) {
            //error = errno;
            //if (error == EAGAIN || error == EINTR) {
            return;
            //}else{
            //error
            //}

        }
        int abc=pktFlag(recvLine,13);

        if(recvLength>=HG_RTP_PACKET_HEAD_SIZE) {
            char contents[512] = {0};
            int seq = pktSeq(recvLine, 13);
            int cmd = pktCmd(recvLine, 13);
            int time=pktTimestamp(recvLine, 13);
            int flag=pktFlag(recvLine,13);
            int comptype=pktComp(recvLine,13);
          //  if (cmd <= enum_RTP_CMD_POST) {
            sprintf(contents, "seq %d cmd %d time %d flag %d comp %d\n", seq,cmd,time,flag,comptype);
                FILE *f2 = fopen(HG_APP_ROOT"recv0.data", "a+");
                fwrite(contents, 1, strlen(contents), f2);
                //fwrite(recvLine + HG_RTP_PACKET_HEAD_SIZE, 1, recvLength - HG_RTP_PACKET_HEAD_SIZE, f2);
                fclose(f2);

        }
        type = pktPLoadType(recvLine, recvLength);
        ssrc = pktSsrc(recvLine, recvLength);
        seq = pktSeq(recvLine, recvLength);
        cmdtype = (enumt_CMDTYPE) pktCmd(recvLine, recvLength);


        if (type != enum_PLTYPEVIDEO && type != enum_PLTYPEAUDIO && type != enum_PLTYPETEXT) {
            continue;
        }
        {
            char contents[512] = {0};
            int seq = pktSeq(recvLine, 13);
            int cmd = pktCmd(recvLine, 13);
            int time=pktTimestamp(recvLine, 13);
            int flat=pktFlag(recvLine,13);
            sprintf(contents, "%d %d %d %d\n", cmd, seq,time,flat);
            FILE *f2 = fopen(HG_APP_ROOT"recv1.data", "a+");
            fwrite(contents, 1, strlen(contents), f2);
            //fwrite(recvLine + HG_RTP_PACKET_HEAD_SIZE, 1, recvLength - HG_RTP_PACKET_HEAD_SIZE, f2);
            fclose(f2);
        }

        context->RecvData(recvLine, recvLength, &clientAddr, 0);
    }


}

int
HuageUdpManage::RecvData(uint8_t *fragdata, uint32_t size, sockaddr_in *sa, int fd) {
    uint32_t ssrc = pktSsrc(fragdata, size);

    t_chainNodeData *hbgt= nullptr;

    //uint8_t *Hg_Buf_GetInfinite2(t_chainList *rnode,t_chainList **free, int *size,int length);
    //

    hbgt = (t_chainNodeData *) allocMemeryLocal(this, sizeof(t_chainNodeData) + size);
    hbgt->init();
    hbgt->data = (char *) hbgt + sizeof(t_chainNodeData);
    hbgt->end = 0;
    hbgt->cap = size;
    hbgt->len = size - HG_RTP_PACKET_HEAD_SIZE;
    hbgt->start = HG_RTP_PACKET_HEAD_SIZE;


    memcpy(hbgt->data, fragdata, size);

    //hbgt->sfree.ctx = this;
    hbgt->sfree.params = nullptr;
    hbgt->sfree.freehandle = nullptr;
    //////
    int lens = sizeof(t_huageEvent) + sizeof(t_clientPacketRecvParams);
    char hgteventptr[lens];
    t_clientPacketRecvParams *uprp = (t_clientPacketRecvParams *) (hgteventptr + sizeof(t_huageEvent));
    uprp->sa = *sa;
    uprp->data = hbgt;
    uprp->ctx = recvth;
    uprp->size = size;

    t_huageEvent *hgtevent = (t_huageEvent *) hgteventptr;
    hgtevent->handle = HuageRecvStream::threadPOOLRecv;
    hgtevent->i=6;
    hgtevent->ctx = recvth;
    hgtevent->psize = sizeof(t_clientPacketRecvParams);

    HgWorker *hgworker = HgWorker::getWorker(ssrc);

    hgworker->WriteChanWor(hgteventptr, lens);
    /////
    return 0;
}


void HuageUdpManage::sendCallback(void *param, t_sendArr *sendarr, int size, HuageConnect *hgConn) {
    struct msghdr snd_msg;
    struct iovec snd_iov[size];

    for (int i = 0; i < size; i++) {
        snd_iov[i].iov_base = sendarr[i].data;
        snd_iov[i].iov_len = sendarr[i].size;

    }
    if(size>0){
        char contents[512]={0};
        int seq=pktSeq((uint8_t *)sendarr[0].data, 13);
        int cmd=pktCmd((uint8_t *)sendarr[0].data, 13);
        int time=pktTimestamp((uint8_t *)sendarr[0].data, 13);
        int flag=pktFlag((uint8_t *)sendarr[0].data, 13);
       // if(cmd<=enum_RTP_CMD_POST) {


               sprintf(contents, "seq %d cmd %d time %d flag %d\n", seq,cmd,time,flag);
               FILE *f2 = fopen(HG_APP_ROOT"send.data", "a+");
               fwrite(contents, 1, strlen(contents), f2);
              // fwrite((uint8_t *)sendarr[1].data, 1, sendarr[1].size, f2);
               fclose(f2);


      //  }else{
          //   return;
       // }
    }


    HuageUdpManage *uc = (HuageUdpManage *) param;
    struct sockaddr_in *sin= nullptr;
    if(uc->isclient){
        sin=&uc->serveraddr;
    }else{
        sin=hgConn->sa;
    }

    snd_msg.msg_name =sin ; // Socket is connected
    snd_msg.msg_namelen = sizeof(struct sockaddr);
    snd_msg.msg_iov = snd_iov;
    snd_msg.msg_iovlen = size;
    snd_msg.msg_control = 0;
    snd_msg.msg_controllen = 0;

    // int abc=ADEC_HEADER_FLAG(*(recvLine + 1));

    sendmsg(uc->confd, &snd_msg, 0);
}

void HuageUdpManage::directSendMsg(void *pth, enumt_CMDTYPE cmdtype, void *ctx, uint8_t *rtpdata,
        uint32_t udpsize, t_rtpHeader *rh, uint32_t ssrc,
        HuageConnect *hgConn) {
    HuageUdpManage *context = (HuageUdpManage *) ctx;
    context->sendth->directSendMsg(pth, cmdtype, rtpdata, udpsize, rh, ssrc,
            hgConn);
}

void *HuageUdpManage::allocMemeryLocal(HuageUdpManage *ctx, int size) {
    return memoryAlloc(ctx->iocp->fragCache, size);
}

void HuageUdpManage::freeMemeryLocal(void *pth, void *ctx, void *params, int psize) {
    HuageUdpManage *huc = (HuageUdpManage *) ctx;
    unsigned char **ptmp = (unsigned char **) params;
    unsigned char *hcn=*ptmp;
    memoryFree(huc->iocp->fragCache, (unsigned char *) hcn);
}

void HuageUdpManage::writePipeFree(void *ctx, void *data) {
    HgPipe *hp= nullptr;
    HuageUdpManage *huageUdpManage = (HuageUdpManage *) ctx;
    hp = huageUdpManage->iocp->aPipeClient;

    int lens = sizeof(t_huageEvent) + sizeof(void *);
    char tmp[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) tmp;
    // HG_ALOGI(0,"HuageUdpManage::freeMemeryLocal %ld,,,,,",HuageUdpManage::freeMemeryLocal);
    hgtevent->handle = HuageUdpManage::freeMemeryLocal;
    hgtevent->i=7;
    hgtevent->ctx = huageUdpManage;
    hgtevent->psize = sizeof(data);
    *((void **) ((char *) tmp + sizeof(t_huageEvent))) = data;
    HgPipe::writePipe(hp, tmp, lens);
}
