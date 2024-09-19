//
// Created by ajl on 2021/11/8.
//
#include "hg_buf.h"
#include <string.h>
#include <stdlib.h>

//circle small buffer


static t_chainNode *
Hg_SingleNewNode(t_mermoryPool *fragCache, t_chainList *chain, t_chainNode **free, int cap,int single);

static void Hg_PushChainNodeR(t_chainList *hct, t_chainNode *node);

int Hg_Buf_WriteN(uint8_t *src, t_chainNodeData *dist, int n,void *thiss) {
    int total=0;
int cpy=0;
        //if(rest>=n){
        if(dist->len==dist->cap){
            return 0;
        }
            if(dist->start<=dist->end){
                if(dist->cap-dist->end>=n){
                    memcpy((char *)dist->data+dist->end,src,n);

/*
                    char fname[255]={0};
                    sprintf(fname,HG_APP_ROOT"1-1_%ld.data",(uint64_t)thiss);
                    FILE *f2 = fopen(fname, "a+");
                    fwrite(src, 1, n, f2);
                    fclose(f2);
*/


                    total=n;
                    //dist->end=dist->end+n;
                    if (dist->end+n>=dist->cap) {
                        dist->end=dist->end+n-dist->cap;
                    } else {
                        dist->end=dist->end+n;
                    }
                }else{
                    int cpy=dist->cap-dist->end;
                    memcpy((char *)dist->data+dist->end,src,cpy);
/*
                    char fname[255]={0};
                    sprintf(fname,HG_APP_ROOT"1-1_%ld.data",(uint64_t)thiss);
                    FILE *f2 = fopen(fname, "a+");
                    fwrite(src, 1, cpy, f2);
                    fclose(f2);
*/
                    total=cpy;
                    cpy=dist->start>=(n-cpy)?(n-cpy):dist->start;
                    memcpy((char *)dist->data,src+total,cpy);
/*
                    f2 = fopen(fname, "a+");
                    fwrite(src+total, 1, cpy, f2);
                    fclose(f2);
*/

                    total+=cpy;
                    dist->end=cpy;
                }
            }else if(dist->start>dist->end){
                cpy=(dist->start-dist->end)>n?n:(dist->start-dist->end);
                memcpy((char *)dist->data+dist->end,src,cpy);
/*
                char fname[255]={0};
                sprintf(fname,HG_APP_ROOT"1-1_%ld.data",(uint64_t)thiss);
                FILE *f2 = fopen(fname, "a+");
                fwrite(src, 1, cpy, f2);
                fclose(f2);
*/
                total=cpy;
                dist->end=dist->end+cpy;
                if(dist->end>=dist->cap){
                    dist->end-=dist->cap;
                }
            }
            dist->len+=total;

    return total;
}
//单个buffer
int Hg_Buf_ReadN(uint8_t *dist, t_chainNodeData *src, int n, int skip) {
    if (src->len < n||src->len==0)
        return -1;
    if (src->start >= src->end) {
        if (src->cap - src->start >= n) {
            if (dist != NULL)
                memcpy(dist, ((char *)src->data + src->start), n);
        } else {
            if (dist != NULL) {
                memcpy(dist, (char *)src->data + src->start, src->cap - src->start);
                memcpy(dist + (src->cap - src->start), src->data, n - (src->cap - src->start));
            }
        }
        if (src->start+skip>=src->cap) {
            src->start = src->start+skip-src->cap;
        } else {
            src->start = src->start+skip;
        }
    } else {
        if (dist != NULL)
            memcpy(dist, (char *)src->data + src->start, n);
        src->start = src->start+skip;
        if(src->start>=src->cap){
            src->start-=src->cap;
        }
    }
    src->len -= skip;
    return 0;
}

//
void Hg_SingleDelchainL(t_chainList *chain, t_chainNode **free) {

    if (chain->left != NULL) {

        t_chainNode *hcn = chain->left->next;
        chain->left->next = (*free);
        (*free) = chain->left;
        chain->left = hcn;
        if (chain->left == NULL) {
            chain->right = NULL;
        }

    }

}

void Hg_SinglePushR(t_chainList *chain, t_chainNode *newnode) {
    if (chain->right == NULL) {
        chain->right = newnode;
        chain->left = newnode;
    } else {
        chain->right->next = newnode;
        chain->right = newnode;
    }
}

t_chainNode *
Hg_SingleNewNode(t_mermoryPool *fragCache, t_chainList *chain,t_chainNode **free, int cap,int single) {
    if (free == NULL || *free == NULL) {

        t_chainNode *hcn = (t_chainNode *) memoryAlloc(fragCache, sizeof(t_chainNode) );
        t_chainNodeData *hbgt = (t_chainNodeData *) memoryAlloc(fragCache, sizeof(t_chainNodeData) + cap);
        hbgt->init();
        hbgt->data = (char *) hbgt + sizeof(t_chainNodeData);
        hbgt->cap = cap;
        hcn->init();
        hcn->data = hbgt;

        if(single==1){
            Hg_SinglePushR(chain, hcn);
        }else{
            Hg_PushChainNodeR( chain, hcn);
        }

        return hcn;
    } else {
        t_chainNode *hcn = *free;
            (*free) = (*free)->next;
        hcn->next = NULL;
        hcn->pre=NULL;

        t_chainNodeData *hbgt;

        hbgt = (t_chainNodeData *)hcn->data;
        void *tmp=hbgt->data;
        hbgt->init();
        hbgt->cap = cap;
        hbgt->data=tmp;
        if(single==1){
            Hg_SinglePushR(chain, hcn);
        }else{
            Hg_PushChainNodeR( chain, hcn);
        }

        return hcn;
    }
}
//can't use for circle buf
uint8_t *
Hg_GetSingleOrIdleBuf(t_mermoryPool *fragCache, t_chainList *chain, t_chainNode **free, int *size,
                      int cap, int next,int single) {//无限右侧可写
    int tmpsize;
    t_chainNode *rnode=chain->right;
    if (rnode != NULL && next != 1) {
        t_chainNodeData *hbgt;

        hbgt = (t_chainNodeData *)(rnode)->data;
        tmpsize = hbgt->cap - hbgt->len-hbgt->start;

        if (tmpsize>0) {
            *size = tmpsize;
            return (unsigned char *)hbgt->data + hbgt->end;
        }
    }

    t_chainNode *hcn = Hg_SingleNewNode(fragCache,chain,free, cap,single);
    t_chainNodeData *hbgttmp = (t_chainNodeData *) hcn->data;
    tmpsize = hbgttmp->cap;
    if (size != NULL) {
        *size = tmpsize;
    }
    return (uint8_t *)hbgttmp->data;
}


//以下都是按数据报接收，支持pre，next链
t_chainNodeData *Hg_New_Buf(t_mermoryPool *fragCache, int size) {
    t_chainNodeData *hbgt = (t_chainNodeData *) memoryAlloc(fragCache, sizeof(t_chainNodeData) + size);
  hbgt->init();
    hbgt->data = (char *) hbgt + sizeof(t_chainNodeData);
    hbgt->start = 0;
    hbgt->len = size;
    hbgt->cap = size;
    hbgt->end = 0;
    return hbgt;
}

void Hg_PushChainNodeR(t_chainList *hct, t_chainNode *node) {
    if (hct == NULL) {
        return;
    }
    if (hct->right == NULL) {
        hct->right = node;
        hct->left = node;
        node->pre = NULL;
        node->next = NULL;
    } else {
        t_chainNode *oldr = hct->right;
        oldr->next = node;
        node->pre = oldr;
        hct->right = node;
        node->next=NULL;
    }
}

void Hg_PushChainDataR(t_mermoryPool *fragCache, t_chainList *chain, void *data) {

    t_chainNode *hcn=NULL;

    if (chain == NULL) {

    }
    hcn = (t_chainNode *) memoryAlloc(fragCache, sizeof(t_chainNode));
    hcn->init();
    hcn->data = data;
    Hg_PushChainNodeR( chain, hcn);
}

void Hg_ChainDelNode(t_mermoryPool *fragCache, t_chainNode *node) {

    if (node == NULL) {
        return;
    } else {
        if (node->pre == NULL) {

            if (node->next != NULL) {
                node->next->pre = NULL;
            }

        } else {
            if (node->next == NULL) {
                    node->pre->next = NULL;
            } else {
                node->pre->next = node->next;
                node->next->pre = node->pre;
            }
        }
    }
    memoryFree(fragCache, (void *) node);


}
void Hg_ChainDelChain(t_mermoryPool *fragCache, t_chainNode *hcn) {
    while (hcn!=NULL) {
        t_chainNode *tmp=hcn->next;
        Hg_ChainDelNode(fragCache, hcn);
        hcn = tmp;
    }
}
//////////
//不确定带下的bufferchain结束
