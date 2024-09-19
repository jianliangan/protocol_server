//
// Created by ajl on 2022/1/11.
//

#ifndef HG_RTP_APP_HG_TCPABSTRACT_H
#define HG_RTP_APP_HG_TCPABSTRACT_H

#include "net/rtp/common/rtp_packet.h"
#include "common/tools.h"
#include "common/hg_buf.h"
#include "net/rtp/common/huage_connect.h"
#include "net/rtp/common/huage_stream.h"
#include "threads/hg_worker.h"
#include "net/rtp/common/hgfdevent.h"
#include "net/rtp/common/hg_netcommon.h"
#include "net/rtp/common/hg_iocp.h"


#define HG_RECV_BUFFER_LEN 512
#define HG_RECV_MAX_BUF_SIZE 512
typedef struct t_twoAddr {
    void *param1;
    void *param2;
} t_twoAddr;

//typedef void (*handleRecv_def)(void *ctx,char*,int);
class HuageTcpManage {

public:
    HgIocp *iocp= nullptr;
    HgFdEvent *fdev= nullptr;
    bool isclient=false;
    uint64_t lastclotim=0;
/*

bool isconnected;

 t_chainNodeData tcpbuf;  64k,不用就删掉
 */
    struct sockaddr_in serveraddr;

    HuageTcpManage();

    bool finished=false;

    int Connect();

    void SetServer(const char *serverip, uint32_t port);


    tf_serverRecv recvCallback=nullptr;
    //tcp
    HgFdEvent *InitEvent(HgIocp *iocp, int confd);

    HgFdEvent *InitLisEvent(HgIocp *iocp, int confd);

    static void tcpAccept(void *ioctx, void *ctx, void *ptr);

    static void tcpRecvmsg(void *ioctx, void *ctx, void *ptr);

    static void tcpSendMsg(void *ioctx, void *ctx, void *ptr);

    static void sendChain(void *pth, void *ctx, void *params, int psize);

    static void preSendDataChain(void *ioctx, void *ctx, HgFdEvent *hfe, uint32_t ssrc);

    static void threadPOOLRecv(void *pth, void *ctx, void *params, int size);

    static void writePipeFree(void *ctx, void *data);

    static void freeMemeryLocal(void *pth, void *ctx, void *params, int psize);

    static void clearSndChain(t_chainList *hct, HuageConnect *hgConnect, bool all);//new
    static void destroyHeads(void *ctx, void *data);

    int PreSendArr(int efd, t_chainList *hct, HuageConnect *hgConnect, HgFdEvent *hfe);

    t_chainNodeData *CreateBuffer(uint8_t pltype, bool isfirst, uint16_t time,
                               HuageConnect *hgConnect, int size,int comptype);

    void PushFramCach(t_mediaFrameChain *mfcsour, HuageConnect *hgConnect, t_chainList *hgConnhct,
                      t_chainNodeData *head);

    int SendDatapre(int efd, t_chainList *hct, t_chainNodeData **hbgtarr, int num, HuageConnect *hgConnect,
                    HgFdEvent *hfe);

    int SendData(void *pth, enumt_PLTYPE pltype,bool isfirst,  t_mediaFrameChain *mfcsour,
                 uint32_t ssrc, HuageConnect *hgConnect, HgFdEvent *hfe,int comptype);

    int RecvData(void *ptr, t_mediaFrameChain *mfc);

    int SendData0(int confd, t_sendArr *sendarr, int size);

    void Init();

    void Listen(int port, const char *ip);
    int GetSocket();

    //函数指针
    ~HuageTcpManage();
};

#endif //HG_RTP_APP_HG_TCPABSTRACT_H
