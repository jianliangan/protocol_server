#include "hg_pool.h"
#include <assert.h>

#include <inttypes.h>
//代替malloc的
//size 应该是512或者1024，固定的这个buf就是存小内存的
t_mermoryPool *memoryCreate(uint32_t size) {
    uint32_t ret;
    t_mermoryPool *mypool, *mypoolfree;
    mypool=(t_mermoryPool *)malloc(sizeof(t_mermoryPool)*2);
    mypool->tLen = size+HG_ADDR_HEAD;
    mypool->left = NULL;
    mypool->right = NULL;
    mypool->size = 0;
    mypoolfree=mypool+1;

    mypoolfree->tLen = mypool->tLen;
    mypoolfree->left = NULL;
    mypoolfree->right = NULL;
    mypoolfree->size = 0;
    mypool->free = mypoolfree;
    return mypool;
}

//size直接写0就行了，这个是为了校验的
t_memoryPoolNode *memoryNewNode(t_mermoryPool *mypool, uint32_t *re) {
    void *p=NULL;
    t_memoryPoolNode *newNode=NULL;
    //first get a node，先考虑链表，然后是首位
    uint32_t tsize = mypool->tLen;

    if (mypool->free!=NULL&&mypool->free->left != NULL) {
        p = memoryLeftPopNode(mypool->free);
    } else {
        p = malloc(tsize +sizeof(t_memoryPoolNode));
    }
    if (p == NULL) {
        *re = 2;
        assert(0);
    }

    newNode = (t_memoryPoolNode *) p;
    newNode->d = (uint8_t *) p + sizeof(t_memoryPoolNode);
    newNode->l = NULL;
    newNode->r = NULL;
    newNode->s = 0;
    newNode->roffset=0;
    *re = 0;
    return newNode;
}

t_memoryPoolNode *memoryLeftPopNode(t_mermoryPool *mypool) {
    t_memoryPoolNode *leftNode=NULL;
    leftNode = mypool->left;
    if (leftNode == NULL) {
        return NULL;
    }
    mypool->left = mypool->left->r;
    leftNode->l = NULL;
    leftNode->r = NULL;

    if (mypool->left != NULL)
        mypool->left->l = NULL;

    mypool->size--;
    if (mypool->size == 0) {
        mypool->right = NULL;
    }
    return leftNode;
}

uint32_t memoryRightPush(t_mermoryPool *mypool, t_memoryPoolNode *newNode) {
    newNode->l = NULL;
    newNode->r = NULL;

    if (mypool->right != NULL) {
        newNode->l = mypool->right;
        mypool->right->r = newNode;

    }
    mypool->right = newNode;
    mypool->size++;
    if (mypool->size == 1) {
        mypool->left = mypool->right;
    }
    return mypool->size;
}


void memoryLeftPopEndFree(t_mermoryPool *mypool, t_memoryPoolNode *node) {
    memoryRightPush(mypool->free, node);
}
void memoryMidlePop(t_mermoryPool *mypool, t_memoryPoolNode *node){
    mypool->size-=1;
    if(node->l!=NULL){
        node->l->r=node->r;
    }else{
        mypool->left=node->r;
    }
    if(node->r!=NULL){
        node->r->l=node->l;
    }else{
        mypool->right=node->l;
    }

    node->l=NULL;
    node->r=NULL;
    memoryLeftPopEndFree(mypool,node);
}
uint8_t *memoryAlloc(t_mermoryPool *mypool,uint32_t size){

    t_memoryPoolNode *newnode=NULL;
    uint32_t re = 0;
    uint32_t *offset=NULL;
    uint32_t *mysize=NULL;
    uint32_t reals=size+HG_ADDR_HEAD;
    uint8_t *alloc=NULL;
    if(mypool->tLen<reals){
        assert(0);
    }

    if (mypool->right==NULL||(mypool->tLen-mypool->right->roffset<reals)) {
        newnode = memoryNewNode(mypool, &re);
        memoryRightPush(mypool, newnode);
        // *page=mypool->right->d;
    }

    alloc=mypool->right->d+mypool->right->roffset;
    offset=(uint32_t *)alloc;
    *offset=mypool->right->roffset;
    mysize=(uint32_t *)(alloc+HG_ADDR_HEAD/2);
    *mysize=size;

    mypool->right->s+=reals;
    mypool->right->roffset+=reals;

    return alloc+HG_ADDR_HEAD;

}
uint32_t memorySizeof(uint8_t *data){
   return *(uint32_t *)(data-HG_ADDR_HEAD/2);
}
//不要重复free,
void memoryFree(t_mermoryPool *mypool,void *data0){
char *data=data0;
    if(data==NULL)
        return;
    uint32_t offset=*(uint32_t *)(data-HG_ADDR_HEAD);
    uint32_t size=*(uint32_t *)(data-HG_ADDR_HEAD/2);

    //int offs=offsetof(t_memoryPoolNode);
    t_memoryPoolNode *mypooln=(t_memoryPoolNode *)(data-HG_ADDR_HEAD-offset-sizeof(t_memoryPoolNode));
        mypooln->s-=(size+HG_ADDR_HEAD);

    if(mypooln->s==0){
        memoryMidlePop(mypool,mypooln);
    }
}

void memoryDestroy(t_mermoryPool *mypool){
    if(mypool==NULL)
        return ;
    t_memoryPoolNode *mypooln=NULL;

    while(1){
        mypooln=memoryLeftPopNode(mypool);
        if(mypooln!=NULL){
            free(mypooln);
        }else{
            break;
        }
    }
    while(1){
        mypooln=memoryLeftPopNode(mypool->free);
        if(mypooln!=NULL){
            free(mypooln);
        }else{
            break;
        }
    }
    free(mypool);
}
