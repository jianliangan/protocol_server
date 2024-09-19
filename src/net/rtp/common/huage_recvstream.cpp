//
// Created by ajl on 2020/12/27.
//

#include "net/rtp/common/huage_recvstream.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/huage_udp_manage.h"
#include "net/rtp/common/huage_net_manage.h"
using namespace std;

HuageRecvStream::HuageRecvStream() {


    seq = 0;
    finished = false;
    uint32_t err;

    finished = true;


}


void HuageRecvStream::threadPOOLRecv(void *pth, void *ctx, void *params, int psize) {
    t_clientPacketRecvParams *uprp = (t_clientPacketRecvParams *) params;

    //HG_ALOGI(0,"recv udp threadPOOLRecv psize %d",psize);
    HuageRecvStream *hgRecvStream = (HuageRecvStream *) ctx;

    t_chainNodeData *hgevdata = (t_chainNodeData *) uprp->data;
    int size = uprp->size;
    void *allocptr = hgevdata;
    //接收流，流=自定义包头+udp包
    HgWorker *hworker = (HgWorker *) pth;
    HuageNetManage *huageNetManage=HuageNetManage::huageNetManage;
    HgConnectBucket *hgConnectBucket = huageNetManage->hgConnBucket;
    t_mediaFrameChain *bigframebuf = nullptr;//此处是单层chain，代表一个帧的chain，未来需要的话可以弄成两层多个帧的chain
    t_huageCacheStruct *recvcache = nullptr;

    uint8_t *udpLine = nullptr;
    uint8_t *rtpBodyLine = nullptr;
    uint32_t udpLength = 0;
    uint32_t rtpBodyLength = 0;
    uint8_t flag = 0;
    uint16_t tmpseq = 0;
    uint16_t tmptimestamp = 0;
    //uint32_t reallength = 0;
    uint32_t bodylength = 0;

    uint32_t ssrc = 0;
    enumt_PLTYPE pltype = enum_PLTYPENONE;
    HuageConnect *hgConnPtr = nullptr;
    HuageConnect *tmphgConn = nullptr;
    sockaddr_in *tmpsa = nullptr;
    int tmpfd = 0;
    uint8_t hg_event[HG_EV_HG_PTR_EVENTLEN];

    hgEvent hgev;

    int isfirst = false;
    int hgevcmd = 0;
    uint16_t *CurPacketSeqs = nullptr;
    uint16_t *MaxPacketSeqs = nullptr;
    uint32_t *bigframeOffset = nullptr;


    if (hgevdata == nullptr) {
        return;
    }
    tmpsa = nullptr;
    tmpsa = &(uprp->sa);
    tmpfd = 0;
    udpLine = (unsigned char *) hgevdata->data;//hgev.data;//udp包位置
    udpLength = hgevdata->cap;//udp包长度
    rtpBodyLine = udpLine + HG_RTP_PACKET_HEAD_SIZE;
    rtpBodyLength = udpLength - HG_RTP_PACKET_HEAD_SIZE;

    tmpseq = pktSeq(udpLine, udpLength);
    tmptimestamp = pktTimestamp(udpLine, udpLength);
    ssrc = pktSsrc(udpLine, udpLength);
    flag = pktFlag(udpLine,udpLength);
    pltype = pktPLoadType(udpLine, udpLength);
    isfirst = pktFirst(udpLine, udpLength);
    hgevcmd = pktCmd(udpLine, udpLength);

    {
        char contents[1000];
        sprintf(contents, "%d %d %d %d\n", hgevcmd, tmpseq,tmptimestamp,flag);
        FILE *f2 = fopen(HG_APP_ROOT"recv2.data", "a+");
        fwrite(contents, 1, strlen(contents), f2);
        //fwrite(recvLine + HG_RTP_PACKET_HEAD_SIZE, 1, recvLength - HG_RTP_PACKET_HEAD_SIZE, f2);
        fclose(f2);
    }
    //Does that peer have permission to write data to me,fetch access;

    hgConnPtr = HuageConnect::hgConnGetConnInBuck(hgConnectBucket, ssrc);
    HuageConnect::hgConnRecvPtrStreamUdp(hgConnPtr, pltype,
                                     &bigframebuf, &recvcache,
                                     &CurPacketSeqs, &bigframeOffset, &MaxPacketSeqs);
    HuageConnect::hgConnAttachAddr(hgConnectBucket, hgConnPtr, tmpsa, sizeof(sockaddr_in));
    bigframebuf->pts=tmptimestamp;
    if (hgevcmd <= enum_RTP_CMD_RES_REPOST) {//如果是投递事件



        if (*CurPacketSeqs == 0xff) {
            *CurPacketSeqs = tmpseq;
        }
if(uint16Sub(*CurPacketSeqs, tmpseq)<=0 ){
    hgRecvStream->WriteTocache(tmpseq, MaxPacketSeqs, hgevdata,
                               recvcache);//先写入缓存，并且清过期数据,修改最大seq
}


        hgRecvStream->FindUseful(pth, pltype, false, bigframebuf, recvcache,
                                 hgConnPtr, tmpsa, *MaxPacketSeqs, CurPacketSeqs,
                                 bigframeOffset);//处理无间断的数据，重发丢包,同时清理过期数据



    } else if (hgevcmd == enum_RTP_CMD_REQ_REPOST) {
        t_rtpHeader rh;
        rh.ssrc = ssrc;
        rh.payloadType = pltype;
        rh.cmd = enum_RTP_CMD_RES_REPOST;
        rh.seq = tmpseq;//到这里了

        //rtpPacketHead(hgRecvStream->rtpHeadBuf, &rh);
        hgRecvStream->directSendMsg(pth, enum_RTP_CMD_RES_REPOST, hgRecvStream->ctx,
                                    nullptr,
                                    0, &rh, ssrc, hgConnPtr);
        hgRecvStream->ClearPacket2(hgevdata);
    }

    //}
}


//检查哪些包需要重发，哪些包可以用了
void
HuageRecvStream::FindUseful(void *pth, enumt_PLTYPE pltype, bool allclear, t_mediaFrameChain *bigframebuf,
                            t_huageCacheStruct *recvcache,
                            HuageConnect *hgConnPtr,
                            sockaddr_in *tmpsa, uint16_t MaxPacketSeqs, uint16_t *CurPacketSeqs,
                            uint32_t *bigframeOffset) {
    int index;
    uint16_t seq = *CurPacketSeqs;
    bool isfull = true;//用来查找从第一个开始连续的包
    uint32_t ttseq = 0;
    uint64_t curtime = 0;
    int8_t tttype = 0;
    uint8_t *udpptr = nullptr;
    uint8_t tailflag = 0;
    t_huageCacheStruct *cachestru = nullptr;
    int maxnovalue = 100;//起到保护作用，防止死循环
    int novalue = 0;

    while (1) {
        if (uint16Sub(MaxPacketSeqs, seq) < 0) {

            break;
        }

        index = seq % HG_CONN_CACHE_LEN;
        cachestru = &recvcache[index];
        udpptr = cachestru->data;

        if (udpptr != nullptr) {
            ttseq = pktSeq(udpptr, 100);
            tttype = pktCmd(udpptr, 100);
            t_chainNodeData *hbgttmp1 = (t_chainNodeData *) cachestru->alloc;
            unsigned char *datatmp1 = (uint8_t *) hbgttmp1->data;
            int seqtmp = pktSeq(datatmp1, 100);

            tailflag = pktFlag(udpptr,100);
            if (isfull) {

                   /*char contents[1000];
                   sprintf(contents,
                   "++ rescvcall type %d tailflag %d max %d seq %d alloc %d curseq %d index %d\n",
                   pltype, tailflag, MaxPacketSeqs, ttseq, seqtmp, *CurPacketSeqs, index);
                   FILE *f2 = fopen(HG_APP_ROOT"recv00.data", "a+");
                   fwrite(contents, 1, strlen(contents), f2);
                   fclose(f2);
*/

                MergeData(pltype, allclear, tttype, bigframebuf, cachestru,
                          seq, tailflag, hgConnPtr, tmpsa, bigframeOffset, seqtmp);
                cachestru->reset();

                *CurPacketSeqs = uint16Add(seq, 1);
            }
            seq = uint16Add(seq, 1);
        } else {
            if (isfull)
                isfull = false;
            //重发,
            if (udpptr == nullptr) {
                if (novalue > maxnovalue) {
                    break;
                }

                curtime = (hgetSysTimeMicros() / 1000);
                if(cachestru->time ==0){
                    ReSend(pth, hgConnPtr, pltype, seq);
                    cachestru->time=curtime;
                }else{
                    if (curtime - cachestru->time > 50) {
                        MergeDataClear( bigframebuf,hgConnPtr,bigframeOffset);
                        clearPacket(cachestru);
                        *CurPacketSeqs = uint16Add(seq, 1);
                        cachestru->reset();
                    }

                }
                /*
               if (cachestru->time == 0) {
                    ReSend(pth, hgConnPtr, pltype, seq);
                    cachestru->time = curtime;
                    cachestru->count = 1;
                } else {
                    if (cachestru->count < 2) {
                        if (curtime - cachestru->time > 20) {
                            ReSend(pth, hgConnPtr, pltype, seq);
                            cachestru->time = curtime;
                            cachestru->count += 1;
                        }
                    } else {
                        MergeDataClear( bigframebuf,hgConnPtr,bigframeOffset);
                        clearPacket(cachestru);
                        *CurPacketSeqs = uint16Add(seq, 1);
                        cachestru->reset();
                    }
               }*/
                novalue++;
            }
            seq = uint16Add(seq, 1);
        }


    }

}


void HuageRecvStream::ReSend(void *pth, HuageConnect *hgConnPtr, enumt_PLTYPE pltype, uint16_t seq) {
    t_rtpHeader rh;
    rh.ssrc = hgConnPtr->ssrc;
    rh.payloadType = pltype;

    rh.cmd = enum_RTP_CMD_REQ_REPOST;
    rh.timestamp = 0;
    rh.seq = seq;//到这里了
    rh.tail = 0;


    //rtpPacketHead(rtpHeadBuf, &rh);
    directSendMsg(pth, enum_RTP_CMD_REQ_REPOST, ctx, nullptr,
                  0, &rh,
                  hgConnPtr->ssrc, hgConnPtr);

}
void
HuageRecvStream::MergeDataClear( t_mediaFrameChain *bigframebuf,HuageConnect *hgConnPtr, uint32_t *bigframeOffset) {
    *bigframeOffset = 0;
    freeChain(bigframebuf->hct.left);
    Hg_ChainDelChain(hgConnPtr->fragCache, bigframebuf->hct.left);
    bigframebuf->hct.left = nullptr;
    bigframebuf->hct.right = nullptr;
    bigframebuf->size = 0;
    bigframebuf->pts=0;

}
int
HuageRecvStream::MergeData(enumt_PLTYPE pltype, bool allclear, int8_t evtype,
                           t_mediaFrameChain *bigframebuf,
                           t_huageCacheStruct *udpdata,
                           uint32_t tmpseq,
                           uint8_t tailflag, HuageConnect *hgConnPtr,
                           sockaddr_in *tmpsa, uint32_t *bigframeOffset, uint16_t seqtmp) {
    /*
     *
     前面 尽量正序一下，下面如果遇到乱序的问题就直接忽略了
     */

    t_chainNodeData *hbgt = (t_chainNodeData *) udpdata->alloc;
    uint8_t *udpLine = (unsigned char *) hbgt->data;
    int udpLength = hbgt->len + hbgt->start;;
    uint32_t rtpBodyLength = (unsigned int) (udpLength - HG_RTP_PACKET_HEAD_SIZE);
    uint8_t *rtpBodyLine = udpLine + HG_RTP_PACKET_HEAD_SIZE;
    //此处默认前面保证顺

    if (tailflag == 0) {//音频大部分是这个
        //hgConnPtr->test->clear();
        // hgConnPtr->test->append(std::to_string(tmpseq));
        if (pltype == enum_PLTYPEVIDEO) {
            char out[4096];

            //uint8_t result[16];
            //md5(udpLine+12,udpLength-12,result);
            //uint64_t *l1;
            // uint64_t *l2;
            // l1=(uint64_t *)result;
            // l2=(uint64_t *)(result+8);

            // FILE *f2 = fopen("/data/data/org.libhuagertp.app/3.data", "a");
            // sprintf(out,"%d %s %lu %lu\n",udpLength-12,hgConnPtr->test.c_str(),0,0);
            // fwrite(out, 1, strlen(out), f2);
            //  fclose(f2);
        }
        // 独立的音视频包直接拿去解码
        MergeDataClear( bigframebuf,hgConnPtr,bigframeOffset);
        t_chainList *bigframetmp = &bigframebuf->hct;
        Hg_PushChainDataR(hgConnPtr->fragCache, bigframetmp, hbgt);
        bigframebuf->size = udpLength;
        *bigframeOffset = udpLength;

        bigframebuf->sfree.params = nullptr;
        bigframebuf->sfree.freehandle = nullptr;

        t_chainNode *hcnR = bigframebuf->hct.right;
        t_chainNodeData *hbgtR = (t_chainNodeData *) hcnR->data;
        hbgtR->sfree.ctx = hgConnPtr;
        hbgtR->sfree.params = bigframebuf->hct.left;
        hbgtR->sfree.freehandle = HuageRecvStream::writePipeFree;
        //HG_ALOGI(0,"recv udp threadPOOLRecv audio udpLength %d",udpLength);
        p_recvCallback(pltype, bigframebuf,
                     hgConnPtr);

        *bigframeOffset = 0;
        //Hg_ChainDelChain(hgConnPtr->fragCache,bigframetmp->left);
        bigframebuf->hct.left = nullptr;
        bigframebuf->hct.right = nullptr;
        bigframebuf->size = 0;

    } else {
        if (tailflag == 2) {//大包的开始
            if (udpLength > HG_MAX_FRAME_LEN) {

                clearPacket(udpdata);
                return 0;
            }

            MergeDataClear( bigframebuf, hgConnPtr,bigframeOffset);
            Hg_PushChainDataR(hgConnPtr->fragCache, &bigframebuf->hct, hbgt);


            *bigframeOffset = udpLength;
        } else if (tailflag == 1 || tailflag == 3) {//大包过程中
            if (tmpseq == seqtmp) {
                if (*bigframeOffset + rtpBodyLength > HG_MAX_FRAME_LEN) {
                    MergeDataClear( bigframebuf, hgConnPtr,bigframeOffset);
                    clearPacket(udpdata);
                    return 0;
                }
                Hg_PushChainDataR(hgConnPtr->fragCache, &bigframebuf->hct, hbgt);

                *bigframeOffset += rtpBodyLength;

                if (tailflag == 3) {
                    bigframebuf->size = *bigframeOffset;
                    //bigframebuf->sfree.ctx=hgConnPtr;
                    bigframebuf->sfree.params = nullptr;
                    bigframebuf->sfree.freehandle = nullptr;

                    t_chainNode *hcnR = bigframebuf->hct.right;
                    t_chainNodeData *hbgtR = (t_chainNodeData *) hcnR->data;
                    hbgtR->sfree.ctx = hgConnPtr;
                    hbgtR->sfree.params = bigframebuf->hct.left;
                    hbgtR->sfree.freehandle = HuageRecvStream::writePipeFree;
                    //HG_ALOGI(0,"recv udp threadPOOLRecv udpLength %d",udpLength);
                    p_recvCallback(pltype, bigframebuf, hgConnPtr);

                    *bigframeOffset = 0;
                    //Hg_ChainDelChain(hgConnPtr->fragCache,bigframebuf->hct.left);
                    bigframebuf->hct.left = nullptr;
                    bigframebuf->hct.right = nullptr;
                    bigframebuf->size = 0;
                }
            } else {
                MergeDataClear( bigframebuf,hgConnPtr,bigframeOffset);
                clearPacket(udpdata);
                return 0;
            }

        } else {//非法包
            MergeDataClear( bigframebuf,hgConnPtr,bigframeOffset);
            clearPacket(udpdata);
            return 0;//不能参与逻辑直接忽略
        }
    }
    return 0;
}

void
HuageRecvStream::WriteTocache(uint16_t seq, uint16_t *MaxPacketSeqs,
                              t_chainNodeData *hbgt,
                              t_huageCacheStruct *recvcache) {
    uint16_t maxseq = *MaxPacketSeqs;
    void *allocptr = hbgt;
    int diffnew = uint16Sub(seq, maxseq);
    uint8_t *data = nullptr;
    void *alloc = nullptr;
    t_huageCacheStruct *cachestru = nullptr;
    /*
    //清理数据，新来的seq和已收到最大的seq之间的空洞要清理置空
    for (int i = 1; i < diffnew; i++) {
    uint32_t index = uint16Add(maxseq, i) % HG_CONN_CACHE_LEN;
    cachestru = &recvcache[index];
    data = cachestru->data;
    alloc = cachestru->alloc;
    if (data != nullptr) {
    t_chainNodeData *hgbt=(t_chainNodeData *)alloc;
    HuageUdpManage::writePipeFree(HuageUdpManage::huageUdpManage,hbgt);
    cachestru->data = nullptr;
    cachestru->alloc = nullptr;
    }
    if (i == HG_CONN_CACHE_LEN) {//空洞的长度不会超过缓冲长度
    break;
    }
    }*/
    //cache存入新的数据
    uint32_t index = seq % HG_CONN_CACHE_LEN;
    cachestru = &recvcache[index];
    if (cachestru->data != nullptr) {
        return;//如果缓冲中有数据不能覆盖
        /*
           t_chainNodeData *hgbt = (t_chainNodeData *) cachestru->alloc;
           HuageUdpManage::writePipeFree(HuageUdpManage::huageUdpManage, hbgt);
           cachestru->data = nullptr;
           cachestru->alloc = nullptr;
           */
    }


    char contents[1000];
      sprintf(contents, "222222222222222222222222333 rescvcall seq %d maxseq %d\n", seq, maxseq);
      FILE *f2 = fopen(HG_APP_ROOT"recv000.data", "a+");
      fwrite(contents, 1, strlen(contents), f2);
      fclose(f2);


    cachestru->data = (unsigned char *) hbgt->data;//所有修改recvcache，sendcacke的地方都要先释放
    cachestru->alloc = allocptr;

    if (diffnew > 0) {
        *MaxPacketSeqs = seq;
    }

}

void HuageRecvStream::ClearPacket2(void *allocptr) {
    //HG_ALOGI(0, "send ClearPacket2 packet ");
    if (allocptr != nullptr) {
        t_chainNodeData *hbgt = (t_chainNodeData *) allocptr;
	HuageNetManage *huageNetManage=HuageNetManage::huageNetManage;
        HuageUdpManage::writePipeFree(huageNetManage->huageUdpManage, hbgt);
        // if(hgbt->sfree.freehandle!=null)
        //   hgbt->sfree.freehandle(hgbt->sfree.ctx,hgbt->sfree.params);
    }
}

void HuageRecvStream::clearPacket(t_huageCacheStruct *cachestru) {
    if (cachestru->data != nullptr) {
        t_chainNodeData *hbgt = (t_chainNodeData *) cachestru->alloc;
	HuageNetManage *huageNetManage=HuageNetManage::huageNetManage;
        HuageUdpManage::writePipeFree(huageNetManage->huageUdpManage, hbgt);
    }
}

void HuageRecvStream::freeByChan(void *pth, void *ctx, void *params, int psize) {
    t_chainNode **ptmp = (t_chainNode **) params;
    t_chainNode *hcn = *ptmp;
    HuageConnect *hgConnPtr = (HuageConnect *) ctx;
    t_chainNode *hcntmp = hcn;
    t_chainNodeData *hbgttmp = (t_chainNodeData *) hcntmp->data;
    hbgttmp->freenum--;
    if (hbgttmp->freenum <= 0) {
        freeChain(hcntmp);
        Hg_ChainDelChain(hgConnPtr->fragCache, hcn);
    }
}

void HuageRecvStream::freeChain(t_chainNode *hcntmp) {
    t_chainNodeData *hbgttmp = nullptr;
    int seq = 0;
    HuageNetManage *huageNetManage=HuageNetManage::huageNetManage;
    while (hcntmp != nullptr) {
        hbgttmp = (t_chainNodeData *) hcntmp->data;
        seq = pktSeq((unsigned char *) hbgttmp->data, HG_RTP_PACKET_HEAD_SIZE);

        HuageUdpManage::writePipeFree(huageNetManage->huageUdpManage, hbgttmp);
        hcntmp = hcntmp->next;
    }
}

void HuageRecvStream::writePipeFree(void *ctx, void *data) {
    HuageConnect *hgConn = (HuageConnect *) ctx;

    int lens = sizeof(t_huageEvent) + sizeof(void *);
    char tmp[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) tmp;
    hgtevent->handle = HuageRecvStream::freeByChan;
    hgtevent->i = 8;
    hgtevent->ctx = ctx;
    hgtevent->psize = lens - sizeof(t_huageEvent);
    *((void **) ((char *) tmp + sizeof(t_huageEvent))) = data;

    HgWorker *hgworker = HgWorker::getWorker(hgConn->ssrc);
    hgworker->WriteChanWor(tmp, lens);
}

HuageRecvStream::~HuageRecvStream() {
}
