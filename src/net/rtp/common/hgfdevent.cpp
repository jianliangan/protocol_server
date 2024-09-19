//
// Created by ajl on 2021/12/24.
//

#include "net/rtp/common/hgfdevent.h"
void HgFdEvent::fdEvChainInit(HgFdEvent **efreechain,int eNums) {

    *efreechain = (HgFdEvent *) malloc(sizeof(HgFdEvent) * eNums);
    //hgConnFree->fnext=NULL;
    for (int i = 0; i < eNums - 1; i++) {
        (*efreechain + i)->fnext = *efreechain + i + 1;
    }
    (*efreechain + eNums - 1)->fnext = NULL;

}

HgFdEvent *HgFdEvent::getFreeFdEv(HgFdEvent **efreechain) {
    HgFdEvent *re = *efreechain;
    *efreechain = re->fnext;
    re->isconnected = false;
    re->fd=0;
    memset(&re->tcpbuf,'0',sizeof(re->tcpbuf));
    re->hgConn= nullptr;
    re->ctx= nullptr;
    re->write= nullptr;
    re->read= nullptr;
    re->status=0;

    return re;
}
int HgFdEvent::setFreeFdEv(HgFdEvent **efreechain,HgFdEvent *fdevent) {
    fdevent->fnext = *efreechain;
    *efreechain = fdevent;

    memoryDestroy(fdevent->fragCache);
    return 0;
}

void
HgFdEvent::fdEvRecvPtrStream1(HgFdEvent *hfe, t_mediaFrameChain **recvMergebuf) {
            *recvMergebuf = &hfe->chainbufsR;
}
