#include "app/audio_consumer.h"
#include "app/app.h"
#include <atomic>
#include "net/rtp/common/hg_channel_stru.h"
using namespace std;
AudioConsumer::AudioConsumer(){
    int err;
}

void AudioConsumer::recvCallback(void *ctx,t_mediaFrameChain *mfc,HuageConnect *hgConn) {
    SessionC *sess=(SessionC *)hgConn->data;
    if(sess==nullptr || sess->status != enum_SESSLOGIN){
      return;
    }
    t_chainNodeData *hbgt=nullptr;

    t_chainNode *hcn=mfc->hct.left;
    int size=mfc->size;
    uint8_t *leftdata=nullptr;


    if(hcn!=nullptr){
        hbgt=(t_chainNodeData *)hcn->data;
        leftdata=(uint8_t *)hbgt->data;
    }

    AudioConsumer * context=(AudioConsumer *)ctx;
    uint16_t packetPts=pktTimestamp(leftdata,size);
    int comp=pktComp(leftdata,size);
    uint32_t seq=pktSeq(leftdata,size);
    //1 calculate the spts and cpts
    Room * room=sess->rroom;
    if(room->id!=sess->roomid){//防止异步产生不同步问题
        return;
    }


    t_chainNode *tmp11=mfc->hct.left;
    while(tmp11!=nullptr){
        t_chainNodeData *bttmp= (t_chainNodeData *)tmp11->data;
        uint32_t seq=pktSeq((unsigned char *)bttmp->data,12);
        tmp11=tmp11->next;
    }


    context->PreRoomMergeData(packetPts,comp,sess,mfc,room);

}
void AudioConsumer::PreRoomMergeData(uint32_t pts,int comp,SessionC *sess,t_mediaFrameChain *mfc,Room *room){

    ////////////
    int lens = sizeof(t_huageEvent) + sizeof(t_serverPacketRoomMergeParams);
    char hgteventptr[lens];
    t_huageEvent *hgtevent = (t_huageEvent *) hgteventptr;
    t_serverPacketRoomMergeParams *uprp = (t_serverPacketRoomMergeParams *) (hgteventptr + sizeof(t_huageEvent));
    uprp->sess=sess;
    uprp->pts=pts;
    uprp->comp=comp;
    uprp->mfc=*mfc;
    uprp->ssrc=sess->ssrc;
    uprp->room=room;
    hgtevent->handle = Room::RoomMergeData;
    hgtevent->psize = lens-sizeof(t_huageEvent);
    hgtevent->ctx = this;
    hgtevent->i=12;
    HgWorker *hgworker = HgWorker::getWorker(room->id);
    hgworker->WriteChanWor(hgteventptr, lens);
}


