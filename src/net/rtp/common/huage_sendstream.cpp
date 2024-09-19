//
// Created by ajl on 2020/12/27.
//

#include "net/rtp/common/huage_sendstream.h"

#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/hg_channel_stru.h"

using namespace std;

HuageSendStream::HuageSendStream() {
    finished = true;
}

void
HuageSendStream::threadPOOLSend(void *pth, void *ctx, void *params, t_rtpHeader *rh, void *allocaddi) {
    t_clientPacketSendParams *upsp = (t_clientPacketSendParams *) params;
    HuageSendStream *hgStream = (HuageSendStream *) ctx;
    HgWorker *hworker = (HgWorker *) pth;
    uint8_t *hgevdata = upsp->data;
    int hgevcmd;
    uint16_t time = upsp->time;
    int hgevsize = upsp->size;
    HuageConnect *hgConnPtr = upsp->hgConn;
    //HuageSendStream *hgStream = (HuageSendStream *) ctx;
    //直接发送需要的变量
    uint32_t totalLength = HG_EV_HG_PTR_EVENTLEN;
    uint32_t reallength = 0;
    uint8_t hg_event1[HG_EV_HG_PTR_EVENTLEN];

    uint16_t *MaxPacketSeqs = nullptr;

    uint32_t rtpseq = 0;

    t_huageCacheStruct *sendcache;
    enumt_PLTYPE pltype;

    //do {

    rtpseq = rh->seq;
    pltype = (enumt_PLTYPE) rh->payloadType;
    hgevcmd = rh->cmd;

    HuageConnect::hgConnSendPtrStreamUdp(hgConnPtr, pltype,
            &sendcache,
            &MaxPacketSeqs);

    if (*MaxPacketSeqs - (HG_CONN_CACHE_LEN-1) >= 0) {
        t_huageCacheStruct css = sendcache[(*MaxPacketSeqs - (HG_CONN_CACHE_LEN-1)) % HG_CONN_CACHE_LEN];
        t_huageCacheStruct css2 = sendcache[*MaxPacketSeqs % HG_CONN_CACHE_LEN];
        if (css.data != nullptr && css2.data != nullptr) {
            //    pktTimestamp(css.data, 100), pktTimestamp(css2.data, 100));
        }

    }

    //先加单播和广播，然后再优化速度看一下广播的锁是否和接收一直冲突
    if (hgevcmd <= enum_RTP_CMD_POST) {
        uint8_t rhtmp[HG_RTP_PACKET_HEAD_SIZE];
        rtpPacketHead(rhtmp, rh);
        t_sendArr sendarr[2];
        sendarr[0].size = HG_RTP_PACKET_HEAD_SIZE;
        sendarr[0].data = rhtmp;
        sendarr[1].size = hgevsize;
        sendarr[1].data = hgevdata;

        /*FILE *f2 = fopen(HG_APP_ROOT"0-1.data", "a");
          fwrite(hgevdata, 1, hgevsize, f2);
          fclose(f2);
          */

        hgStream->p_sendCallback(hgStream->ctx, sendarr, 2,
                hgConnPtr);

        hgStream->ClearPacketWrBufer(hgConnPtr, rtpseq, hgevdata, hgevsize, allocaddi, sendcache,
                MaxPacketSeqs, rh);
    } else if (hgevcmd == enum_RTP_CMD_REQ_REPOST) {
        unsigned char rhtmp[HG_RTP_PACKET_HEAD_SIZE];
        rtpPacketHead(rhtmp, rh);
        t_sendArr sendarr[1];
        sendarr[0].size = HG_RTP_PACKET_HEAD_SIZE;
        sendarr[0].data = rhtmp;
        hgStream->p_sendCallback(hgStream->ctx, sendarr, 1,
                hgConnPtr);
        //hgStream->ClearPacket2(hgConnPtr, upsp);

    } else if (hgevcmd == enum_RTP_CMD_RES_REPOST) {

        int difftmp = uint16Sub(*MaxPacketSeqs, rtpseq);
        if (difftmp <= HG_CONN_CACHE_LEN && difftmp >= 0) {//判断缓存区内有没有，

            uint32_t index = rtpseq % HG_CONN_CACHE_LEN;
            t_huageCacheStruct *cachestru = &sendcache[index];
            uint8_t *sendc = cachestru->data;
            uint32_t sendclen = cachestru->size;//根据pool的接口直接拿到要发送的长度
            void *rhtmp = cachestru->sndhead;
            pktSetCmd((uint8_t *) rhtmp, (int) hgevcmd);
            //rhtmp.cmd=(int)hgevcmd;
            if (sendc != nullptr) {
                t_sendArr sendarr[2];
                sendarr[0].size = HG_RTP_PACKET_HEAD_SIZE;
                sendarr[0].data = rhtmp;
                sendarr[1].size = sendclen;
                sendarr[1].data = sendc;

                hgStream->p_sendCallback(hgStream->ctx,
                        sendarr, 2,
                        hgConnPtr);
            }
        } else {
            HG_ALOGI(0, "resend packet error do not found %u", rtpseq);
        }
    }
}

void
HuageSendStream::directSendMsg(void *pth, enumt_CMDTYPE cmdtype, uint8_t *rtpdata, int udpsize,
        t_rtpHeader *rh,
        int ssrc,
        HuageConnect *hgConn) {
    InnerPushEvent(pth, cmdtype, rtpdata, udpsize, rh, ssrc, hgConn, nullptr);
}

void HuageSendStream::InnerPushEvent(void *pth, enumt_CMDTYPE cmdtype, uint8_t *data, uint32_t size,
        t_rtpHeader *rh,
        uint32_t ssrc, HuageConnect *hgConn, void *alloc) {
    //t_clientPacketSendParams *upsp = (t_clientPacketSendParams *) HuageConnect::allocMemeryLocal(hgConn,
    //                                                                                       sizeof(t_clientPacketSendParams));
    t_clientPacketSendParams upsp;
    upsp.data = data;
    upsp.size = size;
    upsp.time = 0;
    upsp.ctx = this;
    upsp.hgConn = hgConn;

    HuageSendStream::threadPOOLSend(pth, this, &upsp, rh, alloc);


    //
}

void HuageSendStream::ClearPacket2(HuageConnect *hgConnect, void *alloc) {
    //HG_ALOGI(0, "send ClearPacket2 packet ");
    if (alloc != nullptr) {
        t_chainNodeData *hbgt = (t_chainNodeData *) alloc;
        t_selfFree *ss = &hbgt->sfree;
        if (ss->freehandle != nullptr)
            ss->freehandle(ss->ctx, ss->params);
    }
}

void
HuageSendStream::ClearPacketWrBufer(HuageConnect *hgConnect, uint32_t seq, uint8_t *udpLine, int size,
        void *alloc,
        t_huageCacheStruct *sendcache,
        uint16_t *maxseqs, t_rtpHeader *rh) {
    uint32_t index = seq % HG_CONN_CACHE_LEN;
    t_huageCacheStruct *cachestru = &sendcache[index];
    //HG_ALOGI(0, "send clear packet %u", seq);
    if (cachestru->alloc != nullptr) {
        ClearPacket2(hgConnect, cachestru->alloc);


    } else {
        // HG_ALOGI(0, "send clear packet");
    }
    cachestru->data = udpLine;
    cachestru->size = size;
    cachestru->alloc = alloc;
    rtpPacketHead(cachestru->sndhead, rh);

    int diffnew = uint16Sub(seq, *maxseqs);
    if (diffnew > 0) {
        *maxseqs = seq;
    }
}

int
HuageSendStream::SendData(void *pth, enumt_PLTYPE pltype, bool isfirst, t_mediaFrameChain *mfc,
        uint32_t ssrc, HuageConnect *hgConn,int comptype) {
    uint32_t maxcopysize = 0;

    uint32_t shouldcopy = 0;
    uint32_t copiedLength = 0;
    int8_t tail = 0;
    void *alloctmp = nullptr;
    enumt_CMDTYPE cmdtype;
    uint16_t *seqtmp = nullptr;
    uint16_t time=mfc->pts;
    maxcopysize = HG_MAX_RTP_PACKET_SIZE - HG_RTP_PACKET_HEAD_SIZE;
    t_rtpHeader rh;
    rh.ssrc = ssrc;
    rh.payloadType = (uint8_t) pltype;

    rh.first = 0;
    if (isfirst) {
        rh.first = 1;
    }


    cmdtype = enum_RTP_CMD_POST;
    rh.cmd = cmdtype;
    HuageConnect::hgConnSendPtrStreamSeq(hgConn, pltype, &seqtmp);

    /*
       t_chainNode *hcntmp1 = mfc->hct.left;
       t_chainNodeData *hbgttmp = nullptr;
       FILE *f2 = fopen(HG_APP_ROOT"send1.data", "a+");
       while (hcntmp1 != nullptr) {
       hbgttmp = (t_chainNodeData *) hcntmp1->data;
       fwrite((char *) hbgttmp->data + hbgttmp->start, 1, hbgttmp->len, f2);
       hcntmp1 = hcntmp1->next;
       }
       fwrite(aaa, 1, strlen(aaa), f2);
       fclose(f2);
       */

    t_chainNode *hcntmp = mfc->hct.left;
    int restsize = 0;
    int framesize = mfc->size - HG_RTP_PACKET_HEAD_SIZE;
    int sendsize = 0;
    while (hcntmp != nullptr) {

        shouldcopy = 0;
        copiedLength = 0;
        tail = 0;
        alloctmp = nullptr;


        t_chainNodeData *hbgt = (t_chainNodeData *) hcntmp->data;
        uint8_t *bigData = (uint8_t *) hbgt->data + hbgt->start;
        uint8_t *cursor = bigData;
        restsize = hbgt->len;


        while (true) {
            tail = 0;
            if (restsize > 0) {
                //recvLineRtp = rtpdata;//00代表独立的，10代表开始，01代表过程中，11代表结束

                shouldcopy = 0;
                if (maxcopysize < restsize) {//和节点剩余数据比较
                    shouldcopy = maxcopysize;
                } else {
                    shouldcopy = restsize;
                }
                cursor = (uint8_t *) bigData + copiedLength;
                if (sendsize == 0) {
                    if (shouldcopy >= framesize) {
                        tail = 0x00;
                    } else {
                        tail = 0x02;
                    }

                } else {
                    if (shouldcopy >= framesize - sendsize) {
                        tail = 0x03;
                    } else {
                        tail = 0x01;
                    }
                }
                /*
                   if (restsize-shouldcopy==0) {
                   if (copiedLength==0) {
                   tail = 0x00;
                // 4-5修改成00 独立的
                } else {
                tail = 0x03;
                // 修改成11 过程结束
                }
                } else {
                if (copiedLength==0) {
                tail = 0x02;
                // 修改成10 过程开始
                } else {
                tail = 0x01;
                // 修改成01 过程中
                }

                }
                */
            } else {
                break;
            }


            if (shouldcopy > 0) {
                rh.timestamp = time;
                *seqtmp = uint16Add(*seqtmp, 1);
                rh.seq = *seqtmp;//到这里了
                //test=(test==""?"":(test+","))+std::to_string(rh.seq);
                rh.tail = tail;
rh.comp=comptype;
                rh.payloadType = pltype;

                //rtpPacketHead(recvLineRtp, &rh);
                if (tail == 0x00 || tail == 0x03) {
                    alloctmp = hbgt;
                } else {
                    alloctmp = nullptr;
                }



                InnerPushEvent(pth, cmdtype, cursor, shouldcopy, &rh,
                        ssrc, hgConn, alloctmp);
                copiedLength += shouldcopy;
                restsize -= shouldcopy;
                sendsize += shouldcopy;
            }
        }
        hcntmp = hcntmp->next;
    }
    return 0;
}

HuageSendStream::~HuageSendStream() {

}
