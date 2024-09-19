//
// Created by ajl on 2020/12/27.
//
#include "threads/hg_worker.h"

HgWorker *hgworkers[HG_WORKER_NUM];
HgWorker::HgWorker() {
    pthread_create(&streamth, NULL, HgWorker::doWork, this);
}

void *HgWorker::doWork(void *ctx) {
    HgWorker *hgworker = (HgWorker *) ctx;

    t_huageEvent *hgtevent;
    while (true) {
        hgworker->hg_chan.Drive(hgworker, 1);
    }

    // }
    pthread_exit(NULL);

}

HgWorker *HgWorker::getWorker(uint32_t index) {
    return hgworkers[index % HG_WORKER_NUM];
}

void HgWorker::WriteChanWor(void *data,int size) {
    hg_chan.WriteChan(data,size, 1);
}
void HgWorker::init(){

 for (int i = 0; i < HG_WORKER_NUM ;
            i++){
        hgworkers[i] = new HgWorker();
  }

}
HgWorker::~HgWorker() {
}
