#ifndef HG_CORE_TOP_H
#define HG_CORE_TOP_H

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <semaphore.h>
#include <assert.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include<sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include <unistd.h>
#include "hg_log.h"
#define HG_min(x, y) (((x) < (y)) ? (x) : (y))
#define HG_max(x, y) (((x) > (y)) ? (x) : (y))

typedef char * t_yuvchar;
#ifdef __cplusplus
extern "C"{
#endif

void md5(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest);
void
YUV_420_888toNV21(t_yuvchar data, t_yuvchar planeY, t_yuvchar planeU, t_yuvchar planeV, int imWidth,
                  int imHeight,
                  int pitchY, int pitchU, int pitchV, int Ulenth, int pixPitchY, int pixPitchUV,
                  int rotate);
void
YUV_420_888toIyuv(t_yuvchar data, t_yuvchar planeY, t_yuvchar planeU, t_yuvchar planeV, int imWidth,
                  int imHeight,
                  int pitchY, int pitchU, int pitchV, int Ulenth, int pixPitchY, int pixPitchUV,
                  int rotate);
void toolsIntBigEndianV(uint8_t *d, uint8_t *val, uint32_t digits);
void toolsIntBigEndian(uint8_t *d, uint32_t val, uint32_t digits);
void toolsIntLitEndian(uint8_t *d, uint32_t val, uint32_t digits);
int uint16Sub(uint16_t newd,uint16_t oldd);
uint16_t uint16Add(uint16_t add1,uint16_t add2);
void init16Num();
int hgSemWait(sem_t * sem);
uint64_t hgetSysTimeMicros();
void  hg_nsleep(unsigned int miliseconds);
int checkLitEndian();
void PrintBuffer(void* pBuff,uint32_t f1, uint32_t nLen);
uint32_t uint32Add(uint32_t add1,uint32_t add2);
int uint32Sub(uint32_t newd,uint32_t oldd);

static inline int _spin_init(int *lock, int pshared) {
    __asm__ __volatile__ ("" ::: "memory");
    *lock = 0;
    return 0;
}

static inline int _spin_destroy(int *lock) {
    return 0;
}

static inline int _spin_lock(int *lock) {
    while (1) {
        int i;
        for (i=0; i < 10000; i++) {
            if (__sync_bool_compare_and_swap(lock, 0, 1)) {
                return 0;
            }
        }
        sched_yield();
    }
}

static inline int _spin_trylock(int *lock) {
    if (__sync_bool_compare_and_swap(lock, 0, 1)) {
        return 0;
    }
    return -1;
}

static inline int _spin_unlock(int *lock) {
    __asm__ __volatile__ ("" ::: "memory");
    *lock = 0;
    return 0;
}
#ifdef __cplusplus
}
#endif
#endif
