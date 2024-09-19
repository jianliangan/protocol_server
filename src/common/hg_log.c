#include "hg_log.h"
#include <sys/file.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <inttypes.h>
#include "tools.h"
#ifdef __this_android__
void my__android_log_print(uint32_t level, uint32_t tag,const char *fmt, ...){
    if(level<0||tag<0){
        return;
    }
    va_list args;
    char out[4096];
    // const uint8_t *args1;
    va_start(args, fmt);
    vsnprintf(out, sizeof(out), fmt, args);
    __android_log_print(ANDROID_LOG_INFO, "__android_log_print","%s", out);
    va_end(args);
}
#else
pthread_mutex_t hg_log_mutex;
int hg_log_init(){
    if (access ( "./log/", F_OK ) == -1 ) {
        return -1;
    }
    pthread_mutex_init(&hg_log_mutex,NULL);
    return 0;
}
void hg_log(int level,const char *fmt, ...){
    //if(level<=0)
    //  return;
    pthread_mutex_lock(&hg_log_mutex);
    va_list args;
    char out[4096];
    // const char *args1;
    va_start(args, fmt);
    // args1 = va_arg(args,const char *);
    //va_list必须用vsnprintf取
    vsnprintf(out, sizeof(out), fmt, args);
    //__android_log_print(level, tag,fmt, args);
    FILE *f2;
    f2=fopen("./log/access.log","a");
    time_t now;
    struct tm *tm_now;
    char    datetime[200];
    time(&now);
    tm_now = localtime(&now);
    strftime(datetime, 200, "%Y-%m-%d %H:%M:%S %Z", tm_now);
    fprintf(f2,"%s,%s",datetime,out);
    va_end(args);
    fclose(f2);
    pthread_mutex_unlock(&hg_log_mutex);
}

#endif


