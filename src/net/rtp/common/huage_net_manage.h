//
// Created by ajl on 2022/1/12.
//

#ifndef HG_RTP_APP_HG_NETABSTRACT_H
#define HG_RTP_APP_HG_NETABSTRACT_H

#include "net/rtp/common/extern.h"
#include <stdint.h>
#include "net/rtp/common/hg_iocp.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/hg_pipe.h"
#include "net/rtp/common/huage_udp_manage.h"
#include "net/rtp/common/huage_tcp_manage.h"

class HuageNetManage {
    public:
        bool isclient ;
        HuageTcpManage *huageTcpManage= nullptr;
        HuageUdpManage *huageUdpManage= nullptr;
        t_selfCallBack selfcb;
        HuageNetManage();
	HgConnectBucket *hgConnBucket;
        static HuageNetManage *huageNetManage;
        static int
            createFrameStr(t_mermoryPool *fragCache, t_chainList *hct, uint32_t ssrc, int pltype,
                    bool isfirst, int cmdtype, uint16_t time,uint8_t *data,int size,int ssize,int comptype);
        void CaBaChan(int id,int code);
        void ClearReqFram(t_mediaFrameChain *mfc);
        static t_chainNodeData *allocMemNodeData(t_mermoryPool *fragCache, int size);

        //client
        static void sendDataQueue(void *pth, void *ctx, void *params, int psize);
        //tohgConn 发送给某个socket，如果链接的是远端，就发给远端
        static void preSendData(void *ctx, t_mediaFrameChain *mfc, uint32_t ssrc,
                int pltype, bool isfirst, HuageConnect *tohgConn,
                bool direct, void *pth,int comptype);
	void SetConfig(tf_serverRecv recvCallback,void *ctx);
	void Listen(const char *ip, int port);
};


#endif //HG_RTP_APP_HG_NETABSTRACT_H
