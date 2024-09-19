//
// Created by ajl on 2021/12/22.
//

#ifndef HG_RTP_APP_HG_CHANNEL_H
#define HG_RTP_APP_HG_CHANNEL_H
#include "threads/hg_tevent.h"
#include "common/tools.h"
#include "common/hg_buf_comm.h"
#include "common/hg_buf.h"
class HgChannel {
public:
  uint64_t ttt=0;
  uint64_t ttt1=0;

    t_mermoryPool *fragCache=nullptr;
    t_chainNode *freen= nullptr;
    t_chainList hct;
    t_chainNodeData tcpbuf;
    int lock=0;
    sem_t eventSem;

    HgChannel();
    void Handlemsg(void *data,t_chainNodeData *tcpb);
    void Drive(void *data,int wait);
    int WriteChan(void *data, int size,int wait);

};


#endif //_HG_RTP_APP_HG_CHANNEL_
