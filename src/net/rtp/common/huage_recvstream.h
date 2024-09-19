//
// Created by ajl on 2020/12/27.
//


#ifndef HUAGE_PROTOCOL_RECVSTREAM_H
#define HUAGE_PROTOCOL_RECVSTREAM_H

#include "net/rtp/common/hg_channel_stru.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/rtp_packet.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/huage_stream.h"
#include "common/hg_buf.h"
#include "net/rtp/common/type_comm.h"
class HuageRecvStream {
public:
    void *ctx;
    HuageRecvStream();
    void ClearPacket2(void *udpLine);
    void clearPacket( t_huageCacheStruct *recvcache);
    int
    RecvData(uint8_t *fragdata, uint32_t size,sockaddr_in *sa,int fd);
    bool allowothers;
    tf_serverRecv p_recvCallback= nullptr;

    void (*directSendMsg)(void *pth,enumt_CMDTYPE cmdtype, void *ctx,uint8_t *rtpdata, uint32_t udpsize,t_rtpHeader *rh, uint32_t ssrc,
                             HuageConnect *hgConn)= nullptr;
    static void threadPOOLRecv(void *pth,void *ctx,void *params,int psize);
    tf_paramHandle udpfreehandle;
    void *freectx= nullptr;
    bool finished=false;

    ~HuageRecvStream();

private:
    uint32_t seq=0;
    pthread_t streamth;
    uint32_t headlen=0;
    uint8_t rtpHeadBuf[HG_RTP_PACKET_HEAD_SIZE];

    uint8_t hg_event0[HG_EV_HG_PTR_EVENTLEN];
    hgEvent hgev0;

    uint8_t *streamMaxFrame=nullptr;
    //t_mermoryPool *

    sem_t eventSem;
    static void freeByChan(void *pth, void *ctx, void *params, int psize);
    static void freeChain(t_chainNode *hcntmp);
    static void writePipeFree(void *ctx, void *data);
    void WriteTocache(uint16_t seq,uint16_t *maxseqs, t_chainNodeData *hbgt, t_huageCacheStruct *recvcache);
    void FindUseful(void *pth,enumt_PLTYPE pltype,bool allclear,  t_mediaFrameChain *bigframebuf, t_huageCacheStruct *recvcache,
                    HuageConnect *hgConnPtr,
                    sockaddr_in *tmpsa, uint16_t maxseqs, uint16_t *CurPacketSeqs,
                    uint32_t *bigframeOffset);
    void MergeDataClear( t_mediaFrameChain *bigframebuf, HuageConnect *hgConnPtr, uint32_t *bigframeOffset);
    int MergeData(enumt_PLTYPE pltype,bool allclear,int8_t evtype, t_mediaFrameChain *bigframebuf,
                  t_huageCacheStruct *udpdata, uint32_t tmpseq,
                  uint8_t flag, HuageConnect *hgConnPtr,
                  sockaddr_in *tmpsa,uint32_t *bigframeOffset,uint16_t CurPacketSeqs);
    void ReSend(void *pth,HuageConnect *hgConnPtr,enumt_PLTYPE pltype,uint16_t seq);

};

#endif //HUAGE_PROTOCOL_STREAM_H
