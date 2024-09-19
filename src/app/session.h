#ifndef HG_APP_SESSION_H

#define HG_APP_SESSION_H
#define HG_SESS_SUB_AUDIO 0x01
#define HG_SESS_SUB_VIDEO 0x02
#include "common/tools.h"
#include "common/hg_pool.h"
#include "common/hg_buf.h"
#include "app/common.h"
#include "app/room.h"
#include <cassert>
#include <string>
#include <unordered_map>
#define HG_VIEWS_NUM_MAX 25
#define HG_SINGLE_MSG0_LEN 8
enum enumt_sessStatus {
    enum_SESSCLOSE,enum_SESSSTREAM, enum_SESSLOGIN
};
class SessionC;
class SessSubInfo {
    public:
        SessionC *sess=NULL;
        uint8_t flag=0;
};

class SessionC {
    public:
	static int lock;
	static SessionC *sessFreePtr;
        enum enumt_sessStatus status=enum_SESSCLOSE;
        HuageConnect *data=NULL;
        t_audioConfig aconfigt;
        char flags=0;//1bit标识是否收到房间全量信息,2bit 性别
        uint32_t spts=0;//标识最后的值
        uint32_t cpts=0;//标识最后的值
        char name[10]={0};
        //uint32_t cptscache[HG_AUDIO__MERGEBUF];
        int16_t framecache[HG_AUDIO__CHANNEL*HG_AUDIO__SAMPLERATE]={0};
        char framesize[HG_AUDIO__MERGEBUF]={0};
        uint8_t vptsmsgca[HG_SINGLE_MSG0_LEN]={0};
        bool lastvptsmsg=false;
        uint8_t viewsn=0;
        SessionC *views[HG_VIEWS_NUM_MAX]={0};
        Room *rroom=NULL;
        uint32_t roomid=0;
        bool RooEntered=false;
        //void *data;//may pointer protocol sess
        std::unordered_map <uint32_t, SessSubInfo> *bySubscList;//这里改成地址了，原来不是，其他地方还没动呢
        std::unordered_map <uint32_t, SessSubInfo> *meSubList;
        SessionC *next=NULL;

        uint32_t ssrc=0;


        static void sessAttachRecv(SessionC *sess, int vframesize, int aframesize, int tframesize);


        static void sessFree(void *sess);

        static void sessAddSubApi(SessionC *sess, SessionC *bysess,uint32_t ssrc, uint8_t);

        static void sessAddBySubApi(SessionC *sess,uint32_t ssrc, SessionC *bysess, uint8_t);

        static void sessDelSubApi(SessionC *sess, SessionC *bysess);

        static void sessDelBySubApi(SessionC *sess, SessionC *bysess);



        static void sessSetAudioConf(SessionC *sess,int chan,int deepn,int samrate,int frlen,int codec);

        static void sessInit(int sessNum);


        static void sessSetStatus(SessionC *sess, enumt_sessStatus status);
	static SessionC *sessGetFree();
};

#endif

