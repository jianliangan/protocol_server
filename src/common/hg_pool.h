#ifndef HG_CORE_MYPOOL_H
#define HG_CORE_MYPOOL_H
#include "tools.h"
/**
 * queue+buffe
 * */
#define HG_ADDR_HEAD 8
typedef struct t_memoryPoolNode {
    void *d;
    uint32_t s;
    uint32_t roffset;//最大撑起范围，用来计算剩余可用位置
    struct t_memoryPoolNode *l;
    struct t_memoryPoolNode *r;
} t_memoryPoolNode ;

typedef struct t_mermoryPool {
    t_memoryPoolNode *left;
    t_memoryPoolNode *right;
    uint32_t tLen;//节点可用存储区最大长度
    uint32_t size;
    struct t_mermoryPool *free;
} t_mermoryPool ;

#ifdef __cplusplus
extern "C"{
#endif
t_mermoryPool *memoryCreate(uint32_t size);
uint8_t *memoryAlloc(t_mermoryPool *mypool,uint32_t size);
uint32_t memorySizeof(uint8_t *data);
void memoryFree(t_mermoryPool *mypool,void *data);
void memoryDestroy(t_mermoryPool *mypool);
#ifdef __cplusplus
}
#endif
t_memoryPoolNode *memoryNewNode(t_mermoryPool *mypool,uint32_t *ret);
t_memoryPoolNode *memoryLeftPopNode(t_mermoryPool *mypool);
void memoryMidlePop(t_mermoryPool *mypool, t_memoryPoolNode *node);
uint32_t memoryRightPush(t_mermoryPool *mypool, t_memoryPoolNode *newNode);
void memoryLeftPopEndFree(t_mermoryPool *mypool, t_memoryPoolNode *node);
#endif
