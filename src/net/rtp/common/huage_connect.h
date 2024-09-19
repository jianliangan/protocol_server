#ifndef HG_PROTOCOL_SESSION_h

#define HG_PROTOCOL_SESSION_h
#include "net/rtp/common/extern.h"
#include "net/rtp/common/hg_channel_stru.h"
#include "common/tools.h"
#include "common/hg_pool.h"
#include "net/rtp/common/rtp_packet.h"
#include <cassert>
#include <string>
#include <unordered_map>
#include "common/hg_buf.h"
#define HG_CONN_CACHE_LEN 200
#define HG_USETCP 1
#define HG_USEUDP 2

#define HG_FIX_TCP 1
#define HG_FIX_UDP 2
#define HG_UDP_TCP 3
#define HG_TCP_UDP 4

#define HG_MAX_FRAME_LEN   2097152

typedef struct t_huageCacheStruct {
    uint8_t *data;
    void *alloc;

    uint64_t time;
    int size;
    uint8_t count;
    unsigned char sndhead[HG_RTP_PACKET_HEAD_SIZE];
    void reset(){
        data= nullptr;
        alloc= nullptr;
        time=0;
        size=0;
        count=0;
    }
} t_huageCacheStruct;

class HuageConnect;
class HgFdEvent;
class HgConnectBucket {
    public:
        int lock=0;        

        static uint8_t videoproto ;
        static uint8_t audioproto;
        static uint8_t textproto;
        std::unordered_map<uint32_t, HuageConnect *> *bucksmap=nullptr;
        t_mermoryPool *sockaddrPool=nullptr;//先放这，暂时不知道放哪合适
        HuageConnect *hgConnFreePtr=nullptr;
	void Accept(HuageConnect *hgConn);
        void ConnInsertBucket(HuageConnect *hgConn);
};

class HuageConnect {
    public:
        HuageConnect *fnext=nullptr;
        int lock=0;        
        HgFdEvent *tcpfd=nullptr;//tcp use
        void *data=nullptr;
        t_mermoryPool *fragCache=nullptr;
        sockaddr_in *sa=nullptr;
        t_chainList chainbufsS;//tcp send
        t_mediaFrameChain chainbufsR[3];//udp recv
        //下面这6个cache需要优化，暂时不动，2022-01-19
        t_huageCacheStruct sendTextCache[HG_CONN_CACHE_LEN];//以后优化，占空间太大
        t_huageCacheStruct recvTextCache[HG_CONN_CACHE_LEN];

        t_huageCacheStruct sendAudioCache[HG_CONN_CACHE_LEN];//以后优化，占空间太大
        t_huageCacheStruct recvAudioCache[HG_CONN_CACHE_LEN];

        t_huageCacheStruct sendVideoCache[HG_CONN_CACHE_LEN];//以后优化，占空间太大
        t_huageCacheStruct recvVideoCache[HG_CONN_CACHE_LEN];

        uint32_t ssrc=0;
        uint32_t bigVideoframeOffset=0;
        uint32_t bigAudioframeOffset=0;
        uint32_t bigTextframeOffset=0;

        uint16_t curVideoSndPacketSeqs=0;
        uint16_t curVideoPacketSeqs=0;
        uint16_t curVideoMaxSeq=0;

        uint16_t curAudioSndPacketSeqs=0;
        uint16_t curAudioPacketSeqs=0;
        uint16_t curAudioMaxSeq=0;

        uint16_t curTextSndPacketSeqs=0;
        uint16_t curTextPacketSeqs=0;
        uint16_t curTextMaxSeq=0;

        uint16_t curSendTextMaxSeq=0;
        uint16_t curSendAudioMaxSeq=0;
        uint16_t curSendVideoMaxSeq=0;
        uint8_t status=0;
        uint8_t tproto=0;
        uint8_t avproto=0;



        static HuageConnect *hgConnGetFreeConn(HgConnectBucket *hgConnectBucket);
        static int hgConnSetFreeConn(HgConnectBucket *hgConnectBucket,HuageConnect *hgConn);
        static int hgConnFree(HgConnectBucket *hgConnectBucket,HuageConnect *hgConn);

        static HuageConnect *hgConnGetConnInBuck(HgConnectBucket *hgConnectBucket,uint32_t ssrc);

        static void hgConnDelBucket(HgConnectBucket *hgConnectBucket, uint32_t ssrc);

        static void hgConnInit(int hgConnNum,HgConnectBucket **hgConnectBucket);

        static void hgConnAttachAddr(HgConnectBucket *hgConnectBucket,HuageConnect *hgConn, void *sa, int size);

        static void hgConnSetStatus(HuageConnect *hgConn, uint8_t status);
    static void hgConnSetCurSeq(uint16_t *seq,int val,int path);
        static void
            hgConnRecvPtrStreamUdp(HuageConnect *hgConn, enumt_PLTYPE st, t_mediaFrameChain **recvMergebuf,
                    t_huageCacheStruct **recvcache, uint16_t **seq,
                    uint32_t **offset, uint16_t **maxseq);

        static void
            hgConnSendPtrStreamUdp(HuageConnect *hgConn, enumt_PLTYPE st, t_huageCacheStruct **sendcache,
                    uint16_t **maxseq);
        static void
            hgConnSendPtrStreamTCP(HuageConnect *hgConn, t_chainList **recvMergebuf);
        void
            static hgConnSendPtrStreamSeq(HuageConnect *hgConn, enumt_PLTYPE st, uint16_t **seq);
        static void *allocMemeryLocal(HuageConnect *hgConn, int size);
        static void freeMemeryLocal(HuageConnect *hgConn,void *data);
};

#endif

