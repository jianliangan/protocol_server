#ifndef _HG_APP_HEAD_
#define _HG_APP_HEAD_ 
#include <string>
#include "common/tools.h"
#include "net/rtp/common/rtp_packet.h"
#include "app/session.h"
#include "app/room.h"
#include "threads/hg_channel.h"
typedef struct t_serverPacketRoomMergeParams {
    SessionC *sess=NULL;
    uint32_t pts=0;
    int comp=0;
    t_mediaFrameChain mfc; 
    unsigned int ssrc=0;
    void *room=NULL;
} t_serverPacketRoomMergeParams;

typedef struct t_serverPacketLoginParams{
    uint32_t ssrc=0;
    void *sess=NULL;
    void *bysess=NULL;
}t_serverPacketLoginParams;

typedef struct t_textRoomEnter{
    SessionC *sess=NULL;
    unsigned int roomid=0;
    unsigned int uniqid=0;
    bool create=false;
} t_textRoomEnter;

class HuageServer;
class App{
    public:
            void *audioctx=nullptr;
        void *videoctx=nullptr;
        void *textctx=nullptr;
        HgChannel AppChannel;
        int threadNum=0;
        int workSessNum=0;
        HuageServer *hgser=nullptr;
        std::unordered_map<std::string,Room *> *rooms=nullptr;
        App(void);
        int CreateRoom(int roomid,SessionC *sess);
};
extern App *g_app;
#endif
