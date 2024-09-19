//
// Created by ajl on 2021/11/8.
//
#include "stdint.h"
#ifndef HG_RTP_APP_HG_CIRCLEBUF_H
#define HG_RTP_APP_HG_CIRCLEBUF_H

#include "hg_buf_comm.h"
#include "hg_pool.h"

int Hg_Buf_WriteN(uint8_t *src, t_chainNodeData *dist, int n,void *thiss);
    //only one buffer
    int Hg_Buf_ReadN(uint8_t *dist, t_chainNodeData *src, int n, int skip);
    //big buffer chain 固定长度的buf
    void Hg_SingleDelchainL(t_chainList *chain, t_chainNode **free);
    //每次都能返回一个可用的point和可用的size，length是bufer长度,
    void Hg_SinglePushR(t_chainList *chain, t_chainNode *newnode);
    uint8_t *Hg_GetSingleOrIdleBuf(t_mermoryPool *fragCache, t_chainList *chain, t_chainNode **free, int *size,
            int cap, int nextnode,int single);
    //level 1 big buffer chain 不固定长度的buf
    void Hg_PushChainDataR(t_mermoryPool *fragCache, t_chainList *chain, void *data);

    void Hg_ChainDelNode(t_mermoryPool *fragCache,t_chainNode *node);
    void Hg_ChainDelChain(t_mermoryPool *fragCache, t_chainNode *node);

#endif //HG_RTP_APP_HG_CIRCLEBUF_H
