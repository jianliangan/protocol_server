//
// Created by ajl on 2022/1/12.
//

#include "net/rtp/common/huage_net_manage.h"
HuageNetManage *HuageNetManage::huageNetManage;
HuageNetManage::HuageNetManage() {
    huageUdpManage=new HuageUdpManage();
    huageTcpManage=new HuageTcpManage();
    huageNetManage=this;
    isclient = false;
}

int
HuageNetManage::createFrameStr(t_mermoryPool *fragCache, t_chainList *hct, uint32_t ssrc,
        int pltype, bool isfirst, int cmdtype, uint16_t time,
        uint8_t *data, int size, int ssize,int comptype) {

    bool first = true;
    int scap = 0;
    t_chainNodeData *hbgt = nullptr,*hbgt_re = nullptr;
    int tosize = size;
    int tmpsize = 0;
    int start=0;
    int pad=0;
    while (1) {
        if (tosize == 0) {
            break;
        }
        tmpsize = tosize;
        if (tosize > ssize) {
            tmpsize = ssize;

        }

        if (first) {
            start=HG_RTP_PACKET_HEAD_SIZE;
            scap = HG_RTP_PACKET_HEAD_SIZE + tmpsize;
            //if(padding==1){
           //     pad=ADD_PACKET_HEAD1_SIZE;
           // }
        } else {
            start=0;
           // pad=0;
            scap = tmpsize;
        }
        hbgt = allocMemNodeData(fragCache, sizeof(t_chainNodeData) + scap);

        Hg_PushChainDataR(fragCache, hct, hbgt);
        if (first) {
            hbgt_re=hbgt;
            t_rtpHeader rh;
            rh.ssrc = ssrc;
            rh.payloadType = pltype;
                rh.comp=comptype;
            rh.first = 0;
            if (isfirst) {
                rh.first = 1;
            }

            rh.timestamp = time;
            rh.seq = 0;//到这里了
            rh.tail = 0;
            rh.cmd = cmdtype;
            rh.payloadType = pltype;

            rtpPacketHead((uint8_t *) hbgt + sizeof(t_chainNodeData), &rh);
        }

        hbgt->data = (char *) hbgt + sizeof(t_chainNodeData);
        hbgt->cap = scap;
        hbgt->len = tmpsize;
        hbgt->start = start;
        if(hbgt->start>=hbgt->cap){
            hbgt->start-=hbgt->cap;
        }
        hbgt->end = 0;
        if(data!= nullptr)
            memcpy((char *)hbgt->data+start+pad,data+(size-tosize),tmpsize-pad);
        tosize -= (tmpsize-pad);
        first = false;
    }


    return size+HG_RTP_PACKET_HEAD_SIZE;
}

void HuageNetManage::preSendData(void *ctx, t_mediaFrameChain *mfc, uint32_t ssrc,
        int pltype, bool isfirst, HuageConnect *hgConn,
        bool direct, void *pth,int comptype) {
    HuageNetManage *hclnt = (HuageNetManage *) ctx;

    int lens = sizeof(t_huageEvent) + sizeof(t_clientFrameSendParams);
    char hgteventptr[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) hgteventptr;
    t_clientFrameSendParams *cfsp = (t_clientFrameSendParams *) (hgteventptr + sizeof(t_huageEvent));
    cfsp->mfc = *mfc;
cfsp->comptype=comptype;


    cfsp->ssrc = ssrc;
    cfsp->pltype = pltype;//;
    cfsp->isfirst = isfirst ? 1 : 0;
    cfsp->hgConn = hgConn;
    if (direct) {
        HuageNetManage::sendDataQueue(pth, ctx, cfsp, lens - sizeof(t_huageEvent));
    } else {
        hgtevent->handle = HuageNetManage::sendDataQueue;
        hgtevent->i=1;
        hgtevent->psize = lens - sizeof(t_huageEvent);
        hgtevent->ctx = ctx;
        HgWorker *hgworker = HgWorker::getWorker(ssrc);
        hgworker->WriteChanWor(hgteventptr, lens);

    }

}

//client
void HuageNetManage::sendDataQueue(void *pth, void *ctx, void *params, int psize) {
    t_clientFrameSendParams *cfsp = (t_clientFrameSendParams *) params;
    t_mediaFrameChain *mfc1 = &cfsp->mfc;
    int comptype=cfsp->comptype;
    HuageNetManage *hgcl = (HuageNetManage *) ctx;
    HuageTcpManage *huageTcpManage = hgcl->huageTcpManage;
    HuageUdpManage *huageUdpManage = hgcl->huageUdpManage;
    HgWorker *hworker = (HgWorker *) pth;
    HuageNetManage *huageNetManage=HuageNetManage::huageNetManage;
    HgConnectBucket *hgConnectBucket = huageNetManage->hgConnBucket;

    if (cfsp->hgConn == nullptr) {
           return;
    }

    enumt_PLTYPE pltype = (enumt_PLTYPE) cfsp->pltype;
    bool isfirst = cfsp->isfirst ? true : false;
    uint16_t time = cfsp->mfc.pts;

    uint32_t ssrc = cfsp->ssrc;
    HuageConnect *hgConn = (HuageConnect *) cfsp->hgConn;
    HgFdEvent *fdptr = hgConn->tcpfd;
    int usetcp = 0;
    char sendbuf[1024];
    enumt_CMDTYPE cmdtype;
    if (pltype == enum_PLTYPETEXT) {
        if (hgConn->tproto == HG_USETCP) {
            usetcp = 1;
        }
    } else {
        if (hgConn->avproto == HG_USETCP) {
            usetcp = 1;
        }
    }


    if (usetcp) {
        //这里负责丢帧

        if (huageTcpManage->SendData(pth, pltype, isfirst, mfc1, ssrc, hgConn, fdptr,comptype) ==
                HG_EP_ERROR) {
            if ((hgetSysTimeMicros() / 1000) - huageTcpManage->lastclotim > 1) {
                huageTcpManage->Connect();
            }
        }
    } else {

        huageUdpManage->SendData(pth, pltype, isfirst, mfc1,ssrc, hgConn,comptype);
    }
    if (mfc1->sfree.freehandle != nullptr)
        mfc1->sfree.freehandle(mfc1->sfree.ctx, mfc1->sfree.params);
}

void HuageNetManage::ClearReqFram(t_mediaFrameChain *mfc) {

    if (mfc->sfree.freehandle != nullptr)
        mfc->sfree.freehandle(mfc->sfree.ctx, mfc->sfree.params);

    t_chainNode *hcnleft = mfc->hct.left;
    t_chainNodeData *hbgt = nullptr;

    while (hcnleft != nullptr) {
        hbgt = (t_chainNodeData *) hcnleft->data;
        t_selfFree *ss = &hbgt->sfree;
        if (ss->freehandle)
            ss->freehandle(ss->ctx, ss->params);
        hcnleft = hcnleft->next;
    }

}

void HuageNetManage::CaBaChan(int id, int code) {
    int lens = sizeof(t_huageEvent) + sizeof(t_responseParams);
    char tmp[lens];
    t_responseParams *sps = (t_responseParams *) (tmp + sizeof(t_huageEvent));
    sps->id = id;
    sps->code = code;
    t_huageEvent *hgtevent = (t_huageEvent *) tmp;
    hgtevent->handle = selfcb.cbhandle;
    hgtevent->i=2;
    hgtevent->ctx = this;
    hgtevent->psize = sizeof(t_responseParams);
    selfcb.chan->WriteChan(tmp, lens, 1);
}

t_chainNodeData *HuageNetManage::allocMemNodeData(t_mermoryPool *fragCache, int size) {
    t_chainNodeData *tmp = (t_chainNodeData *) memoryAlloc(fragCache, size);
    tmp->init();
    tmp->cap = size - sizeof(t_chainNodeData);
    tmp->data = (char *) tmp + sizeof(t_chainNodeData);

    return tmp;
}


void HuageNetManage::SetConfig(tf_serverRecv recvCallback,void *ctx){
    huageUdpManage->recvCallback = recvCallback;
    huageTcpManage->recvCallback = recvCallback;
}
void HuageNetManage::Listen(const char *ip, int port){
    /***iocp***/
    HgIocp *iocp = new HgIocp();
    HgFdEvent::fdEvChainInit(&iocp->fdchainfree,100000);

    huageTcpManage->iocp = iocp;
    huageTcpManage->Init();
    huageTcpManage->Listen(port+1,ip);

    /****iocp2****/
    ///////////////////////
    huageUdpManage->iocp=iocp;
    huageUdpManage->Init();
    huageUdpManage->Listen(port,ip);
}
