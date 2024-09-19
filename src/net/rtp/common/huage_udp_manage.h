//
// Created by ajl on 2022/1/11.
//
#include "net/rtp/common/rtp_packet.h"
#include "common/hg_buf.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/hg_channel_stru.h"
#include "net/rtp/common/huage_sendstream.h"
#include "net/rtp/common/huage_recvstream.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/hg_iocp.h"
#include "net/rtp/common/hg_netcommon.h"
#ifndef HG_RTP_APP_HG_UDPABSTRACT_H
#define HG_RTP_APP_HG_UDPABSTRACT_H



class HuageUdpManage {
    public:
        HgIocp *iocp= nullptr;
        struct sockaddr_in serveraddr;
        uint32_t confd=0;
        HuageRecvStream *recvth= nullptr;
        HuageSendStream *sendth= nullptr;
        bool isclient=false;
        uint32_t maxmtu=0;
        bool haslog=false;
        HuageUdpManage();
        pthread_t recvThread;//接收数据
        void Connect();
        void List();
        bool finished=false;
        void Init();
        int GetSocket();

        void Listen(int port, const char *ip);
        void SetServer(const char *serverip, uint32_t port);

        tf_serverRecv recvCallback=nullptr;
        HgFdEvent *InitEvent(HgIocp *iocp, int confd);
        //udp

        static void udpRecvmsg(void *ioctx, void *ctx, void *ptr);
        static void sendCallback(void *param, t_sendArr *sendarr,int size,HuageConnect *hgConn);
        //函数指针
        static void directSendMsg(void *pth,enumt_CMDTYPE cmdtype,void *ctx, uint8_t *rtpdata, uint32_t udpsize,t_rtpHeader *rh, uint32_t ssrc,
                HuageConnect *hgConn);
        static void *allocMemeryLocal(HuageUdpManage *huc,int size);
        static void freeMemeryLocal(void *pth,void *ctx,void *params,int psize);
        static void writePipeFree(void *ctx,void * data);
        int SendData(void *pth,enumt_PLTYPE pltype,bool isfirst,t_mediaFrameChain *mfc, uint32_t ssrc,
                HuageConnect *hgConn,int comptype);
        int RecvData(uint8_t *fragdata, uint32_t size, sockaddr_in *sa, int fd);
        ~HuageUdpManage();

};


#endif //HG_RTP_APP_HG_UDPABSTRACT_H
