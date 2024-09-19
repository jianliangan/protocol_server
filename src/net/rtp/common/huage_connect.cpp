#include "net/rtp/common/huage_connect.h"
#include <unordered_map>
#include <utility>
#include "net/rtp/common/rtp_packet.h"

uint8_t HgConnectBucket::videoproto = HG_UDP_TCP;
uint8_t HgConnectBucket::audioproto = HG_UDP_TCP;
uint8_t HgConnectBucket::textproto = HG_FIX_TCP;//HG_FIX_TCP;
void HuageConnect::hgConnInit(int hgConnNum, HgConnectBucket **hgConnectBucket) {

    HuageConnect *hgConnFree = (HuageConnect *) malloc(sizeof(HuageConnect) * hgConnNum);
    //hgConnFree->fnext=NULL;
    for (int i = 0; i < hgConnNum; i++) {
        if(i==hgConnNum-1){
           (hgConnFree + i)->fnext = NULL;
	}else{
           (hgConnFree + i)->fnext = hgConnFree + i + 1;
	}
        (hgConnFree + i)->tproto = HG_USEUDP;
        (hgConnFree + i)->avproto = HG_USETCP;
        if (HgConnectBucket::textproto == HG_FIX_TCP || HgConnectBucket::textproto == HG_TCP_UDP)
            (hgConnFree + i)->tproto = HG_USETCP;
        if (HgConnectBucket::audioproto == HG_FIX_UDP || HgConnectBucket::audioproto == HG_UDP_TCP) {
            (hgConnFree + i)->avproto = HG_USEUDP;
        }
        (hgConnFree + i)->fragCache = memoryCreate(2046);
        _spin_init(&(hgConnFree + i)->lock, 0);
        //(hgConnFree+i)->test=new std::string();
    }


    *hgConnectBucket = (HgConnectBucket *) malloc(sizeof(HgConnectBucket));


    (*hgConnectBucket)->sockaddrPool = memoryCreate(512);
    (*hgConnectBucket)->hgConnFreePtr = hgConnFree;
    (*hgConnectBucket)->bucksmap = new std::unordered_map<uint32_t, HuageConnect *>();

}

HuageConnect *HuageConnect::hgConnGetFreeConn(HgConnectBucket *hgConnectBucket) {
    
    _spin_lock(&hgConnectBucket->lock);
    HuageConnect *re= nullptr;
    HuageConnect *freePtr = hgConnectBucket->hgConnFreePtr;
    re = freePtr;
    hgConnectBucket->hgConnFreePtr = re->fnext;
    _spin_unlock(&hgConnectBucket->lock);

    re->bigVideoframeOffset = 0;
    re->curVideoPacketSeqs = 0xff;


    re->bigAudioframeOffset = 0;
    re->curAudioPacketSeqs = 0xff;


    re->bigTextframeOffset = 0;
    re->curTextPacketSeqs = 0xff;

    re->tcpfd = nullptr;
    re->chainbufsR[0].init();
    re->chainbufsR[1].init();
    re->chainbufsR[2].init();
    re->chainbufsS.init();
    re->fnext= nullptr;

    re->sa= nullptr;

    memset(re->sendTextCache,'\0',sizeof(t_huageCacheStruct)*HG_CONN_CACHE_LEN);
    memset(re->recvTextCache,'\0',sizeof(t_huageCacheStruct)*HG_CONN_CACHE_LEN);
    memset(re->sendAudioCache,'\0',sizeof(t_huageCacheStruct)*HG_CONN_CACHE_LEN);
    memset(re->recvAudioCache,'\0',sizeof(t_huageCacheStruct)*HG_CONN_CACHE_LEN);
    memset(re->sendVideoCache,'\0',sizeof(t_huageCacheStruct)*HG_CONN_CACHE_LEN);
    memset(re->recvVideoCache,'\0',sizeof(t_huageCacheStruct)*HG_CONN_CACHE_LEN);
    return re;

}

int HuageConnect::hgConnSetFreeConn(HgConnectBucket *hgConnectBucket, HuageConnect *hgConn) {
    hgConn->fnext = hgConnectBucket->hgConnFreePtr;
    hgConnectBucket->hgConnFreePtr = hgConn;
    _spin_unlock(&hgConnectBucket->lock);
    return 0;
}

int HuageConnect::hgConnFree(HgConnectBucket *hgConnectBucket, HuageConnect *hgConn) {

    hgConnSetFreeConn(hgConnectBucket, hgConn);


    Hg_ChainDelChain(hgConn->fragCache, hgConn->chainbufsR->hct.left);
    Hg_ChainDelChain(hgConn->fragCache, (hgConn->chainbufsR + 1)->hct.left);
    Hg_ChainDelChain(hgConn->fragCache, (hgConn->chainbufsR + 2)->hct.left);
    hgConn->chainbufsR->hct.left= nullptr;
    hgConn->chainbufsR->hct.left= nullptr;
    (hgConn->chainbufsR + 1)->hct.left= nullptr;
    (hgConn->chainbufsR + 1)->hct.left= nullptr;

    (hgConn->chainbufsR + 2)->hct.left= nullptr;
    (hgConn->chainbufsR + 2)->hct.left= nullptr;
    hgConn->bigVideoframeOffset = 0;


    hgConn->curVideoMaxSeq = 0;

    hgConn->bigAudioframeOffset = 0;


    hgConn->curAudioMaxSeq = 0;

    hgConn->bigTextframeOffset = 0;


    hgConn->curTextMaxSeq = 0;

    //hgConn->test->clear();
    hgConnDelBucket(hgConnectBucket, hgConn->ssrc);
    hgConn->ssrc = 0;

    return 0;

}

HuageConnect *HuageConnect::hgConnGetConnInBuck(HgConnectBucket *hgConnectBucket, uint32_t ssrc) {

    HuageConnect *hgConn = nullptr;
    std::unordered_map<uint32_t, HuageConnect *>::iterator bmapiter;

    std::unordered_map < uint32_t, HuageConnect * > *bmap= nullptr;
    bmap = hgConnectBucket->bucksmap;
    bmapiter = bmap->find(ssrc);
    if (bmapiter != bmap->end()) {
        hgConn = bmapiter->second;
    }

    return hgConn;
}


void HuageConnect::hgConnDelBucket(HgConnectBucket *hgConnectBucket, uint32_t ssrc) {
    _spin_lock(&hgConnectBucket->lock);
    HuageConnect *hgConn = nullptr;
    std::unordered_map<uint32_t, HuageConnect *>::iterator bmapiter;

    std::unordered_map < uint32_t, HuageConnect * > *bmap;

    bmap = hgConnectBucket->bucksmap;

    bmapiter = bmap->find(ssrc);
    if (bmapiter != bmap->end()) {
        hgConn = bmapiter->second;
    } else {
        _spin_unlock(&hgConnectBucket->lock);
        assert(false);
        return;
    }

    memoryFree(hgConnectBucket->sockaddrPool, (uint8_t *) hgConn->sa);
    bmap->erase(bmapiter);
    _spin_unlock(&hgConnectBucket->lock);

}

void HuageConnect::hgConnAttachAddr(HgConnectBucket *hgConnectBucket, HuageConnect *hgConn, void *sa, int size) {

    void *sockaddr = memoryAlloc(hgConnectBucket->sockaddrPool, size);
    memcpy(sockaddr, sa, size);
    hgConn->sa = (sockaddr_in *) sockaddr;

}

void HuageConnect::hgConnSetStatus(HuageConnect *hgConn, uint8_t status) {
    hgConn->status = status;
}
void HuageConnect::hgConnSetCurSeq(uint16_t *seq,int val,int path){
    *seq=val;
}
void
HuageConnect::hgConnRecvPtrStreamUdp(HuageConnect *hgConn, enumt_PLTYPE st, t_mediaFrameChain **recvMergebuf,
        t_huageCacheStruct **recvcache, uint16_t **seq,
        uint32_t **offset, uint16_t **maxseq) {
    switch (st) {
        case enum_PLTYPETEXT:

            *seq = &hgConn->curTextPacketSeqs;
            *offset = &hgConn->bigTextframeOffset;
            *recvMergebuf = hgConn->chainbufsR;

            *recvcache = hgConn->recvTextCache;
            *maxseq = &hgConn->curTextMaxSeq;
            break;
        case enum_PLTYPEAUDIO:

            *seq = &hgConn->curAudioPacketSeqs;
            *offset = &hgConn->bigAudioframeOffset;
            *recvMergebuf = hgConn->chainbufsR + 1;

            *recvcache = hgConn->recvAudioCache;
            *maxseq = &hgConn->curAudioMaxSeq;
            break;
        case enum_PLTYPEVIDEO:

            *seq = &hgConn->curVideoPacketSeqs;
            *offset = &hgConn->bigVideoframeOffset;
            *recvMergebuf = hgConn->chainbufsR + 2;

            *recvcache = hgConn->recvVideoCache;
            *maxseq = &hgConn->curVideoMaxSeq;
            break;
        case enum_PLTYPENONE:
            ;
        default:
            ;
    }

}

void
HuageConnect::hgConnSendPtrStreamUdp(HuageConnect *hgConn, enumt_PLTYPE st, t_huageCacheStruct **sendcache,
        uint16_t **maxseq) {
    switch (st) {
        case enum_PLTYPETEXT:
            *sendcache = hgConn->sendTextCache;
            *maxseq = &hgConn->curSendTextMaxSeq;
            break;
        case enum_PLTYPEAUDIO:
            *sendcache = hgConn->sendAudioCache;
            *maxseq = &hgConn->curSendAudioMaxSeq;
            break;
        case enum_PLTYPEVIDEO:
            *sendcache = hgConn->sendVideoCache;
            *maxseq = &hgConn->curSendVideoMaxSeq;
            break;
        default:
            ;
    }

}

void
HuageConnect::hgConnSendPtrStreamTCP(HuageConnect *hgConn, t_chainList **sendMergebuf) {
    *sendMergebuf = &hgConn->chainbufsS;
}
void
HuageConnect::hgConnSendPtrStreamSeq(HuageConnect *hgConn, enumt_PLTYPE st, uint16_t **seq) {
    switch (st) {
        case enum_PLTYPETEXT:
            *seq=&hgConn->curTextSndPacketSeqs;
            break;
        case enum_PLTYPEAUDIO:
            *seq=&hgConn->curAudioSndPacketSeqs;
            break;
        case enum_PLTYPEVIDEO:
            *seq=&hgConn->curVideoSndPacketSeqs;
            break;
        default:
            ;
    }

}
void *HuageConnect::allocMemeryLocal(HuageConnect *hgConn, int size) {
    return memoryAlloc(hgConn->fragCache, size);
}
void HuageConnect::freeMemeryLocal(HuageConnect *hgConn,void *data) {
    memoryFree(hgConn->fragCache, (uint8_t *)data);
}
void HgConnectBucket::Accept(HuageConnect *hgConn){
    ConnInsertBucket(hgConn);
}
void HgConnectBucket::ConnInsertBucket(HuageConnect *hgConn) {
    uint32_t ssrc=hgConn->ssrc;
    _spin_lock(&lock);
    bucksmap->insert(std::unordered_map<uint32_t, HuageConnect *>::value_type(ssrc, hgConn));
    _spin_unlock(&lock);
}
