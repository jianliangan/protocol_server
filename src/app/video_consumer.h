#ifndef HG_APP_VIDEOCONSUMER_H
#define HG_APP_VIDEOCONSUMER_H
#include "app/session.h"
#include "common/tools.h"
#include "net/rtp/huage_server.h"
#include "net/rtp/common/huage_recvstream.h"
#include "net/rtp/common/huage_sendstream.h"
#include "threads/hg_channel.h"
#include <unordered_map>
//音频，视频暂时不加session,因为不需要合包处理
class VideoConsumer {
    public:
        //bysubscrip,被订阅
        VideoConsumer();
        void OutConsumeQueue(t_mediaFrameChain *mfc,SessionC *sess);
        static void recvCallback(void *ctx,t_mediaFrameChain *mfc,HuageConnect *sess);
        void RemoveUidBroad(uint32_t ssrc);
        void DelBySubsTa(uint32_t bysub);//只能是comm执行
};
#endif
