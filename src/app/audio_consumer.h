#ifndef HG_APP_AUDIOCONSUMER_H
#define HG_APP_AUDIOCONSUMER_H
#include "app/session.h"
#include "common/tools.h"
#include "net/rtp/huage_server.h"
#include "net/rtp/common/huage_recvstream.h"
#include "net/rtp/common/huage_sendstream.h"
#include "base/g711.h"
#include "threads/hg_channel.h"
#include "net/rtp/common/hg_channel_stru.h"
#include <unordered_map>
//音频，视频暂时不加session,因为不需要合包处理
class AudioConsumer {
    public:
        t_mermoryPool *fragCache=nullptr;

        //bysubscrip,被订阅
        AudioConsumer();
        static void recvCallback(void *ctx,t_mediaFrameChain *mfc,HuageConnect *sess);
        void PreRoomMergeData(uint32_t pts,int comp,SessionC * hgsess,t_mediaFrameChain *mfc,Room *room);
};
#endif
