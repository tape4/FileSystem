#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "buf.h"
#include "disk.h"
#include "queue.h"
#include "buf_utils.h"


struct bufList      bufList[MAX_BUFLIST_NUM];
struct stateList    stateList[MAX_BUF_STATE_NUM];
struct freeList     freeListHead;
struct lruList      lruListHead;


void BufInit(void)
{
    CIRCLEQ_INIT(&freeListHead);
    CIRCLEQ_INIT(&lruListHead);
    for(int i = 0; i < MAX_BUFLIST_NUM; i++) CIRCLEQ_INIT(&bufList[i]);
    for(int i = 0; i < MAX_BUF_STATE_NUM; i++) CIRCLEQ_INIT(&stateList[i]);
    for(int i = 0; i < MAX_BUF_NUM; i++) {
        Buf *newBuf = (Buf *)malloc(sizeof(Buf));
        newBuf->blkno = BLKNO_INVALID;
        CIRCLEQ_INSERT_TAIL(&freeListHead, newBuf, flist);
    }
}

Buf* BufFind(int blkno)
{
    int index = BufGetIndex(blkno);
    Buf *iter;
    CIRCLEQ_FOREACH(iter, &bufList[index], blist) {
        if (iter -> blkno == blkno) {
            return iter;
        }
    }
    return NULL;
}


void BufRead(int blkno, char* pData)
{
    Buf *findBuffer = BufFind(blkno);
    if (findBuffer == NULL) {// 만약 buffer cache내에 없다면
        Buf *buf = BufGetNewBuffer();
        // 디스크에서 해당 블럭을 불러와 저장
        buf -> blkno = blkno;
        buf -> state = BUF_STATE_CLEAN;

        char temp[BLOCK_SIZE] = {0,};
        DevReadBlock(blkno, temp); // 디스크에서 버퍼로 저장
        buf -> pMem = malloc(BLOCK_SIZE);
        memcpy(buf -> pMem, temp, BLOCK_SIZE);
        memcpy(pData, buf -> pMem, BLOCK_SIZE);
        BufInsertToHead(buf, buf ->blkno, BUF_CLEAN_LIST);
        BufLruUpdate(buf);
    } else { // 만약 buffer cache 내에 있다면
        memcpy(pData, findBuffer -> pMem, BLOCK_SIZE);
        BufLruUpdate(findBuffer);
    }
}


void BufWrite(int blkno, char* pData)
{
    Buf *findBuffer = BufFind(blkno);
    if (findBuffer == NULL) { // 만약 buffer cache 내에 없다면
        Buf *buf = BufGetNewBuffer();

        // 새로운 버퍼에 해당 블럭을 할당
        buf -> blkno = blkno;
        buf -> state = BUF_STATE_DIRTY;
        buf -> pMem = malloc(BLOCK_SIZE);
        memcpy(buf->pMem, pData, BLOCK_SIZE);

        BufInsertToHead(buf, buf->blkno, BUF_DIRTY_LIST);
        BufLruUpdate(buf);
    } else { // 만약 buffer cache 내에 있다면
        findBuffer -> pMem = malloc(BLOCK_SIZE);
        memcpy(findBuffer->pMem, pData, BLOCK_SIZE);
        BufLruUpdate(findBuffer);
        if (findBuffer -> state == BUF_STATE_CLEAN) {
            findBuffer -> state = BUF_STATE_DIRTY;
            CIRCLEQ_REMOVE(&stateList[BUF_CLEAN_LIST], findBuffer, slist);
            CIRCLEQ_INSERT_TAIL(&stateList[BUF_DIRTY_LIST], findBuffer, slist);
        }
    }
}

void BufSync(void)
{
    Buf *target;
    while(!CIRCLEQ_EMPTY(&stateList[BUF_DIRTY_LIST])) {
        target = CIRCLEQ_FIRST(&stateList[BUF_DIRTY_LIST]);
        BufSyncImpl(target);
    }
}

void BufSyncBlock(int blkno)
{
    Buf *buf = BufFind(blkno);
    BufSyncImpl(buf);
}

/* 
 * dirty 또는 clean list의 번호(listnum)을 입력받아서 해당 list의 head부터 tail까지
 * Buf의 포인터를 ppBufInfo의 배열에 저장함.
 * 배열의 크기는 numBuf로 지정함. 획득한 buf의 포인터의 개수는 함수의 반환 값으로 리턴됨.
 */
int GetBufInfoInStateList(BufStateList listnum, Buf* ppBufInfo[], int numBuf)
{
    int size = 0;
    Buf *iter;
    
    CIRCLEQ_FOREACH(iter, &stateList[listnum], slist) {
        if (size + 1 <= numBuf) ppBufInfo[size++] = iter;
        else break;
    }
    return size;
}

/*
 * LRU list에 있는 Buf의 포인터를 ppBufInfo가 지정하는 배열에 저정함.
 * 배열의 크기는 numBuf로 지정함
 * 획득한 buf의 포인터의 개수는 함수의 반환 값으로 리턴됨.
 */
int GetBufInfoInLruList(Buf* ppBufInfo[], int numBuf)
{
    int size = 0;
    Buf *iter;
    
    CIRCLEQ_FOREACH(iter, &lruListHead, llist) {
        if (size + 1 <= numBuf) ppBufInfo[size++] = iter;
        else break;
    }
    return size;
}


/*
 * index가 지정하는 Buffer list에 있는 Buf의 포인터를 ppBufInfo가 지정하는 배열에 저장한다.
 * 배열의 크기는 numBuf로 지정함.
 * 획득한 buf의 포인터의 개수는 함수의 반환 값으로 리턴됨.
 */
int GetBufInfoInBufferList(int index, Buf* ppBufInfo[], int numBuf)
{
    int size = 0;
    Buf *iter;
    
    CIRCLEQ_FOREACH(iter, &bufList[index], blist) {
        if (size + 1 <= numBuf) ppBufInfo[size++] = iter;
        else break;
    }
    return size;
}