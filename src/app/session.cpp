#include "app/session.h"
#include <unordered_map>
#include <utility>
SessionC *SessionC::sessFreePtr;
int SessionC::lock;
void SessionC::sessInit(int sessNum) {

    SessionC *sessFree = (SessionC *) malloc(sizeof(SessionC) * sessNum);

    for (int i = 0; i < sessNum; i++) {
        (sessFree + i)->bySubscList = nullptr;
        (sessFree + i)->meSubList = nullptr;
        if(i==sessNum-1){
            (sessFree + i)->next=NULL;       
	}else{
            (sessFree + i)->next=sessFree + i+1;
	}

    }
    sessFreePtr=sessFree;
}


void SessionC::sessFree(void *sess) {
    if(sess==NULL)
      return;
    SessionC *re=(SessionC *)sess;
    re->status = enum_SESSCLOSE;

    re->aconfigt.init();
    re->cpts=0;
    re->spts=0;
    memset(re->name,'\0',sizeof(re->name));
    memset(re->framecache,'\0',sizeof(re->framecache)*sizeof(int16_t));
    memset(re->framesize,'\0',sizeof(re->framesize));
    memset(re->vptsmsgca,'\0',sizeof(re->vptsmsgca));
    memset(re->views,'\0',sizeof(re->views)*sizeof(SessionC *));
    re->viewsn=0;
    re->rroom=nullptr;
    re->roomid=0;
    re->RooEntered=false;
    re->flags=0;
    if(re->bySubscList==nullptr){
        re->bySubscList=new std::unordered_map<uint32_t, SessSubInfo>();
    }
    if(re->meSubList==nullptr){
        re->meSubList=new std::unordered_map<uint32_t, SessSubInfo>();
    }

    re->bySubscList->clear();
    re->meSubList->clear();
    re->ssrc=0;
    
    _spin_lock(&lock);
    re->next=sessFreePtr;
    sessFreePtr=re;
    _spin_unlock(&lock);
}

void SessionC::sessAddSubApi(SessionC *sess, SessionC *bysess,uint32_t ssrc, uint8_t flag) {
    std::unordered_map <uint32_t, SessSubInfo> *mesubptr;
    SessSubInfo ssi;
    ssi.flag = flag;
    ssi.sess = bysess;
    mesubptr = sess->meSubList;
    mesubptr->insert(std::make_pair(ssrc, ssi));
}

void SessionC::sessDelSubApi(SessionC *sess, SessionC *bysess) {
    std::unordered_map <uint32_t, SessSubInfo> *mesubptr;
    mesubptr = sess->meSubList;
    mesubptr->erase(bysess->ssrc);
}

void SessionC::sessAddBySubApi(SessionC *sess,uint32_t ssrc, SessionC *bysess, uint8_t flag) {
    std::unordered_map <uint32_t, SessSubInfo> *mebysubptr;
    SessSubInfo ssi;
    ssi.flag = flag;
    ssi.sess = sess;

    mebysubptr = bysess->bySubscList;
    mebysubptr->insert(std::make_pair(ssrc, ssi));

}

void SessionC::sessDelBySubApi(SessionC *sess, SessionC *bysess) {
    std::unordered_map <uint32_t, SessSubInfo> *mebysubptr;
    mebysubptr = bysess->bySubscList;
    mebysubptr->erase(sess->ssrc);

}

void SessionC::sessSetStatus(SessionC *sess, enumt_sessStatus status) {
    sess->status = status;
}
void SessionC::sessSetAudioConf(SessionC *sess,int chan,int deepn,int samrate,int frlen,int codec){
    sess->aconfigt.channels=chan;
    sess->aconfigt.deep=deepn;
    sess->aconfigt.samplesrate=samrate;
    sess->aconfigt.frameperbuf=frlen;
    sess->aconfigt.codec=(enumt_audioCodecs)codec;
}
SessionC *SessionC::sessGetFree() {
    SessionC *re= nullptr;
    _spin_lock(&lock);
    SessionC *freePtr = sessFreePtr;
    re = freePtr;
    sessFreePtr = re->next;
    _spin_unlock(&lock);
    sessFree(re);
    return re;

}


