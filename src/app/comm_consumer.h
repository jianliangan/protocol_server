#ifndef HG_APP_COMMCONSUMER_H
#define HG_APP_COMMCONSUMER_H
#include "common/tools.h"
#include "net/rtp/huage_server.h"
#include "net/rtp/common/huage_recvstream.h"
#include "net/rtp/common/huage_sendstream.h"
#include "threads/hg_channel.h"
#define HG_MAX_TEXT_BUF_SIZE 4096
#define HG_HANDLE_MAX_FRAME_SLICE 2048


class SessionC;
typedef struct t_responseCtx{
    uint32_t uniqId=0;
    HuageConnect *conn=NULL;
}t_responseInfo;
typedef struct t_requestCtx{
    uint32_t uniqId=0;
    HuageConnect *conn=NULL;
}t_requestCtx;
//先确定信令的订阅列表是增量还是全量同步，然后是音频，视频只是只读备份，
class CommConsumer {
    public:

        CommConsumer();
        static void recvCallback(void *param,t_mediaFrameChain *mfc,HuageConnect *sess);
        static void asyncLogin(void *ctx, t_mediaFrameChain *mfc,t_requestCtx *tRequest);
        static void workHanLogin(void *pth,void *ctx,void *params,int psize);
        static void writeFree(void *ctx, void *data);
        static void freeByChan(void *pth, void *ctx, void *params, int psize);

        void SessOut(SessionC *sess);
        void Logout(SessionC *sess);
        void FrameBufSet(t_mediaFrameChain *mfc,uint16_t time,t_chainList *hct,int length,void *ctx);
        void Login(uint32_t ssrc,SessionC *sess);

};
#endif
