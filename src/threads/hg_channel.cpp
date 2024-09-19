//
// Created by ajl on 2021/12/22.
//

#include "threads/hg_channel.h"
#include "net/rtp/common/hg_channel_stru.h"
#include <sys/syscall.h>

#define HG_CHANNEL_LEN_MAX 4 * 1024
#define HG_HG_MAX_BUF_BLOSIZE 512

HgChannel::HgChannel() {
    fragCache = memoryCreate(HG_CHANNEL_LEN_MAX);

    tcpbuf.data = new char[HG_CHANNEL_LEN_MAX];
    tcpbuf.cap = HG_CHANNEL_LEN_MAX;

    sem_init(&eventSem, 0, 0);
    _spin_init(&lock, 0);

    lock = 0;

}

int HgChannel::WriteChan(void *data, int size, int wait) {
    //the same to pipe
    int canwrsize = 0;
    uint8_t *recvLine;
    int rest = size;
    int realcp = 0;
    if (size <= 0) {
        return 0;
    }
    ttt += size;
    t_chainNode *hcn = nullptr;
    t_chainNodeData *hbgt = nullptr;

    t_huageEvent *head = (t_huageEvent *) data;


    _spin_lock(&lock);


    char fname[255]={0};
    FILE *f2;
    /*sprintf(fname,HG_APP_ROOT"0_%ld.data",(uint64_t)this);
    f2 = fopen(fname, "a+");
    fwrite(data, 1, size, f2);
    fclose(f2);
    */
/*
       char bocontent[500]={0};
       sprintf(bocontent,"%d-psize-%d-handle-%s-ctx-%ld \n", (int)(size - sizeof(t_huageEvent)), head->psize,
       eventstr[head->i], (long)head->ctx);
       sprintf(fname,HG_APP_ROOT"w_%ld.data",(long)this);
       f2 = fopen(fname, "a+");
       fwrite(bocontent, 1, strlen(bocontent), f2);
       fclose(f2);
*/
    while (1) {
        t_chainNodeData *hbbbb = nullptr;
        if (hct.right != nullptr) {
            hbbbb = (t_chainNodeData *) hct.right->data;
        }
        int starttmp = (hbbbb == nullptr ? -1 : hbbbb->start);
        int lentmp = (hbbbb == nullptr ? -1 : hbbbb->len);
        int endtmp = (hbbbb == nullptr ? -1 : hbbbb->end);
        recvLine = (uint8_t *) Hg_GetSingleOrIdleBuf(fragCache, &hct,
                &freen, &canwrsize,
                HG_HG_MAX_BUF_BLOSIZE, 0, 1);
        realcp = canwrsize <= rest ? canwrsize : rest;

        memcpy(recvLine, (char *) data + (size - rest), realcp);

        hcn = hct.right;
        hbgt = (t_chainNodeData *) hcn->data;

        hbgt->len += realcp;
        hbgt->end += realcp;
        if (hbgt->end >= hbgt->cap) {
            hbgt->end -= hbgt->cap;
        }
        rest -= realcp;
        if (rest == 0) {
            break;
        }
    }
    _spin_unlock(&lock);
    //the same to pipe
    if (wait == 1) {
        sem_post(&eventSem);
    }
    return 0;
}

void HgChannel::Drive(void *data, int wait) {

    int lentmp = 0;

    if (wait == 1) {
        while (sem_wait(&eventSem) == -1) {
        };
    }
    _spin_lock(&lock);

    t_chainNode *hcnleft = hct.left;
    t_chainNodeData *hbgt = nullptr;
    int total = 0;




    while (hcnleft != nullptr) {

        hbgt = (t_chainNodeData *) hcnleft->data;

        total = Hg_Buf_WriteN((uint8_t *) hbgt->data + hbgt->start, &tcpbuf, hbgt->len,this);
        if (total != 0) {
/*
            char fname[255]={0};
            sprintf(fname,HG_APP_ROOT"1_%ld.data",(uint64_t)this);
            FILE *f2 = fopen(fname, "a+");
            fwrite((uint8_t *) hbgt->data + hbgt->start, 1, total, f2);
            fclose(f2);
*/
        }


        if (total < hbgt->len) {
            hbgt->start += total;
            if (hbgt->start >= hbgt->cap) {
                hbgt->start -= hbgt->cap;
            }
            hbgt->len -= total;
            break;
        } else if (total == hbgt->len) {
            hcnleft = hcnleft->next;
            Hg_SingleDelchainL(&hct, &freen);
        }
    }

    _spin_unlock(&lock);
    Handlemsg(data, &tcpbuf);
}

void HgChannel::Handlemsg(void *data, t_chainNodeData *tcpb) {
    uint8_t body[HG_CHANNEL_LEN_MAX];
    uint8_t *tmp = nullptr;
    int headlen = sizeof(t_huageEvent);
    t_huageEvent head;
    int willgets = headlen;
    int skip = 0;

    int gethead = 0;

    while (true) {
        if (gethead == 0) {
            tmp = (unsigned char *) &head;
            willgets = headlen;
            skip = headlen;

        } else {
            tmp = body;
            willgets = head.psize;
            skip = head.psize;

        }
        if (Hg_Buf_ReadN(tmp, tcpb, willgets, skip) == 0) {

            if (gethead == 0) {
                gethead = 1;
            } else {
                //void *pth,void *ctx,void *params,int psize

                char fname[255]={0};
                FILE *f2;
/*                sprintf(fname,HG_APP_ROOT"2_%ld.data",(uint64_t)this);
                f2 = fopen(fname, "a+");
                fwrite(&head, 1, sizeof(head), f2);
                fwrite(tmp, 1, willgets, f2);
                fclose(f2);
*/

/*
                   char bocontent[500]={0};
                   sprintf(bocontent,"%d-psize-%d-handle-%s-ctx-%ld \n", willgets, head.psize,
                   eventstr[head.i], (long)head.ctx);
                   sprintf(fname,HG_APP_ROOT"r_%ld.data",(long)this);
                   f2 = fopen(fname, "a+");
                   fwrite(bocontent, 1, strlen(bocontent), f2);
                   fclose(f2);
*/
                head.handle(data, head.ctx, tmp, head.psize);

                gethead = 0;
            }//free

        } else {
            if (gethead == 1) {
                if (tcpb->start >= headlen) {
                    tcpb->start -= headlen;
                } else {
                    tcpb->start = tcpb->cap - (headlen - tcpb->start);
                    if (tcpb->start >= tcpb->cap) {
                        tcpb->start -= tcpb->cap;
                    }
                }
                tcpb->len += headlen;
            }
            break;
        }

    }


}
