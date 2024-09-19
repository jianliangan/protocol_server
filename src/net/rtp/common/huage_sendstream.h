//
// Created by ajl on 2020/12/27.
//

#ifndef HUAGE_PROTOCOL_SENDSTREAM_H
#define HUAGE_PROTOCOL_SENDSTREAM_H

#include "net/rtp/common/hg_channel_stru.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/rtp_packet.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/huage_stream.h"


class HuageSendStream {
    public:
        void *ctx= nullptr;
        bool finished=false;

        HuageSendStream();

        void ClearPacketWrBufer(HuageConnect *hgConnect,uint32_t seq, uint8_t *udpLine,int size,void *alloc, t_huageCacheStruct *sendcache,uint16_t *maxseqs,t_rtpHeader *rh);
        void ClearPacket2(HuageConnect *hgConnect,void *alloc);
        int
            SendData(void *pth,enumt_PLTYPE pltype,bool isfirst,t_mediaFrameChain *mfc, uint32_t ssrc,
                    HuageConnect *hgConn,int comptype);

        void
            (*p_sendCallback)(void *ctx, t_sendArr *sendarr,int size, HuageConnect *hgConn)= nullptr;

        void directSendMsg(void *pth,enumt_CMDTYPE cmdtype, uint8_t *rtpdata, int udpsize,t_rtpHeader *rh,
                int ssrc,
                HuageConnect *hgConn);
        ~HuageSendStream();

    private:

        uint8_t hg_event0[HG_EV_HG_PTR_EVENTLEN]={0};
        static void threadPOOLSend(void *pth,void *ctx,void *params,t_rtpHeader *rh,void *allocaddi);
        //发送的时候可能没有connect，所以ssrc得有
        void InnerPushEvent(void *pth,enumt_CMDTYPE cmdtype, uint8_t *data, uint32_t size,t_rtpHeader *rh,
                uint32_t ssrc, HuageConnect *hgConn,void *alloc);


};

#endif //HUAGE_PROTOCOL_SENDSTREAM_H
