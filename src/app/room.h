//
// Created by ajl on 2021/9/14.
//

#ifndef HG_RTP_APP_ROOM_H
#define HG_RTP_APP_ROOM_H
#include <stdint.h>
#include "base/json.hpp"
#include "app/common.h"
#include "app/handler_consumer.h"
#include "app/comm_consumer.h"
#include "common/tools.h"
#include "common/hg_pool.h"
#include "base/g711.h"
#include "common/hg_buf_comm.h"
#include "net/rtp/common/hg_channel_stru.h"
#include <atomic>
#include <pthread.h>
#define HG_AUDIO__CHANNEL 1
#define HG_AUDIO__DEEP 16
#define HG_AUDIO__FRAMERPER 160
#define HG_AUDIO__SAMPLERATE 8000
#define HG_AUDIO__MERGEBUF (HG_AUDIO__SAMPLERATE/HG_AUDIO__FRAMERPER)

/**
 * 1\ouput fixed 24bit deep
 * 2\client and server use the same framelen,samplerate  160,8000
 */
class SessionC;
typedef struct t_roomBufInfo{
    char *size=NULL;//每个位置帧总数
    int *samptota=NULL;//每个采样加起来的总和
    //uint8_t *rawcache;//音频原始数据

}t_roomBufInfo;

class FreeFrameChain
{
    public:
        FreeFrameChain *next;
        t_mediaFrameChain mfc;
        FreeFrameChain();
        void init();
        void reset();
        static int getlen();
        static FreeFrameChain *instan(int relays);
        static void revert(FreeFrameChain *ffca);
};

class RoomUserInfo {
    public:
        SessionC *sess=NULL;
        uint8_t flag=0;
};

class Room{//
    public:
        uint32_t id=0;
        uint64_t lasttime=0;
        uint64_t startplay=0;
        uint32_t nextptsplay=0;
        FreeFrameChain *framefreea=nullptr;//音频cache
        int frameusea=0;
        int audioFramRawByte=0;
        int audioCaNum=0;
        int audioFrameOutByte=0;
        t_mermoryPool *fragCache=nullptr;
        t_mediaFrameChain mfcULst;
        t_roomBufInfo rooBufinfo;
        static t_audioConfig audioconfig;
        std::unordered_map <uint32_t, RoomUserInfo> *userList;
        static Room *CreateRoom(uint32_t uniqid,int roomid,SessionC *sess,bool create);
        static int CreEnterRoom(unsigned uniqid,unsigned int roomid,SessionC *sess,bool create);
        static void HandleCreEnterRoo(void *pth,void *ctx,void *params,int psize);
        FreeFrameChain *GetFreeFrame();
        int EnterRoom(SessionC *sess);
        void WriteHeader(uint8_t *data,uint32_t ssrc,int pltype,int padding,bool isfirst,int channel,uint32_t time,int cmdtype,int comptype);
        void OutConsumeQueue(char rsize,uint32_t pts,int fromi,SessionC *sess);
        Room(uint32_t roomid);
        int MergeAudioData(int index,t_mediaFrameChain *mfc,SessionC *sess);
        static void RoomMergeData(void *pth,void *ctx,void *params,int psize);
        int subIndex(int newi,int oldi);
        int addIndex(int add1,int add2);
        static void writeFree2(void *ctx, void *data);
        static void freeByChan2(void *pth, void *ctx, void *params, int psize);

        static void writeFree(void *ctx, void *data);
        static void freeByChan(void *pth, void *ctx, void *params, int psize);
};
#endif //_HG_RTP_APP_ROOM_

