//
// Created by ajl on 2021/11/22.
//

#ifndef HG_RTP_APP_HG_TEVENT_H
#define HG_RTP_APP_HG_TEVENT_H
#include <cstddef>
class HgFdEvent;
typedef void (*tf_evenHandle)(void *pth,void *ctx,void *params,int psize);
typedef struct t_huageEvent { //每帧的头
    char i=0;
    int psize=0;
    void *ctx;
    tf_evenHandle handle=NULL;
} t_huageEvent;
#endif //HG_RTP_APP_HG_TEVENT_H
