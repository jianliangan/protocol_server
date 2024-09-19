#include "app/timer.h"
bool HgTimer::isbusy;
timer_t HgTimer::timerid;
uintptr_t HgTimer::hgCurrentTime;
std::multimap<uintptr_t,struct t_hgTimerEvent> HgTimer::events;
HgTimer *ht=nullptr;
HgTimer::HgTimer(){
    pthread_create(&threadIn, NULL,timeHandle, this);
    sem_init(&semtimer, 0, 0);
}
void *HgTimer::timeHandle(void *ctx){
    HgTimer *ht=(HgTimer *)ctx;
    struct timespec ts;
    while(true){
        //clock_gettime(CLOCK_REALTIME,&ts);
        //ts.tv_nsec += 50000000;
        //ht->handle();
        //printf("dddd\n");
        usleep(50000);
        // sem_timedwait(&ht->semtimer,&ts);
    }
}
void HgTimer::Handle(){
    if(HgTimer::isbusy)
        return;
    HgTimer::isbusy=true;
    std::multimap<uintptr_t,struct t_hgTimerEvent>::iterator it;
    hgCurrentTime=hgetSysTimeMicros()/1000;
    tf_timerHandle handle2;
    void * param2;
    //    HG_ALOGI(0,"heartbeat\n");
    do{
        it=events.begin();
        if(it!=events.end()){
            if(it->first<hgCurrentTime){
                handle2=it->second.handle;
                param2=it->second.param;
                events.erase(it);
                handle2(param2);
            }else{
                break;
            }
        }else{

        }

    }while(it!=events.end());
    isbusy=false;
}
void HgTimer::DelEvent(std::multimap<uintptr_t,t_hgTimerEvent>::iterator it){
    events.erase(it);

}
std::multimap<uintptr_t,t_hgTimerEvent>::iterator HgTimer::AddEvent(t_hgTimerEvent &hgev,uintptr_t expire){
    std::multimap<uintptr_t,t_hgTimerEvent>::iterator it;
    it=events.insert(std::pair<uintptr_t,t_hgTimerEvent>(expire,hgev));
    return it;
}

