#ifndef HG_TIMER_H
#define HG_TIMER_H
#include <map>
#include <pthread.h>
#include "common/tools.h"
typedef void(*tf_timerHandle)(void *);
typedef struct t_hgTimerEvent{
    tf_timerHandle handle=NULL;
    void * param=NULL;
}t_hgTimerEvent;
class HgTimer{
    public:
        HgTimer();
        static bool isbusy;
        pthread_t threadIn;
        static timer_t timerid;
        sem_t semtimer;
        static uintptr_t hgCurrentTime;
        static std::multimap<uintptr_t,struct t_hgTimerEvent> events;
        void Handle();
        static void *timeHandle(void *);
        std::multimap<uintptr_t,t_hgTimerEvent>::iterator  AddEvent(t_hgTimerEvent &,uintptr_t);
        void DelEvent(std::multimap<uintptr_t,t_hgTimerEvent>::iterator);
};
extern HgTimer *ht;
#endif
