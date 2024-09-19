//
// Created by ajl on 2021/12/7.
//

#ifndef HG_RTP_APP_HG_BUF_COMM_H
#define HG_RTP_APP_HG_BUF_COMM_H
#include <stddef.h>
typedef void (*tf_paramHandle)(void *ctx,void *data);

typedef struct t_selfFree {
    void *ctx;
    void *params;
    tf_paramHandle freehandle;
    t_selfFree():ctx(nullptr),params(nullptr),freehandle(nullptr){}
    void init(){
        this->ctx=nullptr;
        this->params=nullptr;
        this->freehandle=nullptr;
    }
} t_selfFree;

typedef struct t_chainNodeData {
    void *data;
    int start;
    int end;//虚拟位置，最大能到cap
    int len;
    int cap;
    t_selfFree sfree;
    int freenum;
    t_chainNodeData():data(nullptr),start(0),end(0),len(0),cap(0),sfree(),freenum(0){}
    void init(){
        this->data=nullptr;
        this->start=0;
        this->end=0;
        this->len=0;
        this->cap=0;
        this->sfree.params= nullptr;
        this->sfree.freehandle= nullptr;
        this->sfree.ctx=nullptr;
        this->freenum=0;
    }
    void reset(){
        this->start=0;
        this->end=0;
        this->len=0;

        this->freenum=0;

    }
} t_chainNodeData;

//可以实现无数层级的chain
typedef struct t_chainNode {
    void *data;
    struct t_chainNode *next;
    struct t_chainNode *pre;
    t_chainNode():data(nullptr),next(nullptr),pre(nullptr){}
    void init(){
        this->data= nullptr;
        this->next= nullptr;
        this->pre= nullptr;
    }
    void reset(){
        t_chainNodeData *hbgt=(t_chainNodeData *)data;
        hbgt->reset();
    }
} t_chainNode;

typedef struct t_chainList {
    t_chainNode *left;
    t_chainNode *right;
    t_chainList():left(nullptr),right(nullptr){}
    void init(){
        this->left= nullptr;
        this->right= nullptr;
    }
    void reset(){
        t_chainNode *tmp=left;
        while(tmp!=nullptr){
            tmp->reset();
            tmp=tmp->next;
        }
    }
} t_chainList;

#endif //HG_RTP_APP_HG_BUF_COMM_H
