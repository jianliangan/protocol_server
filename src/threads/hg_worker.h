//
// Created by ajl on 2020/12/27.
//


#ifndef HG_WORKER_H
#define HG_WORKER_H
#include <semaphore.h>
#include "threads/hg_tevent.h"
#include <pthread.h>
#include <unistd.h>
#include "threads/hg_channel.h"
#define HG_WORKER_NUM 2
class HgWorker {
public:
    
    HgWorker();
    static HgWorker *getWorker(uint32_t index);
    void WriteChanWor(void *data,int size);
static    void init();
    ~HgWorker();

public:
    pthread_t streamth;
    HgChannel hg_chan;
    static void *doWork(void *ctx);
};
extern HgWorker *hgworkers[HG_WORKER_NUM];
#endif //HG_WORKER_H
