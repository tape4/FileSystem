#include <stdlib.h>
#include <stdio.h>
#include "buf_utils.h"
#include "disk.h"

void BufInsertToHead(Buf* pBuf, int blkno, BufStateList listNum) {
    int index = BufGetIndex(blkno);
    CIRCLEQ_INSERT_HEAD(&bufList[index], pBuf, blist);
    CIRCLEQ_INSERT_TAIL(&stateList[listNum], pBuf, slist);    
}

void BufInsertToTail(Buf* pBuf, int blkno, BufStateList listNum) {
    int index = BufGetIndex(blkno);
    CIRCLEQ_INSERT_TAIL(&bufList[index], pBuf, blist);
    CIRCLEQ_INSERT_TAIL(&stateList[listNum], pBuf, slist);  
}

Buf* BufGetNewBuffer(void) {
    if (CIRCLEQ_LAST(&freeListHead) != (void *)&freeListHead) {
        Buf *last = CIRCLEQ_LAST(&freeListHead); 
        CIRCLEQ_REMOVE(&freeListHead, last, flist);
        return last;
    } else {
        return BufGetVictim();
    }
}

void BufSyncImpl(Buf *target) {
    if (target->state == BUF_STATE_DIRTY) {
        DevWriteBlock(target -> blkno, target ->pMem);
        target -> state = BUF_STATE_CLEAN;
        CIRCLEQ_REMOVE(&stateList[BUF_DIRTY_LIST], target, slist);
        CIRCLEQ_INSERT_TAIL(&stateList[BUF_CLEAN_LIST], target, slist);
    }
}

BOOL BufDelete(Buf* pBuf) {
    BOOL result;
    result = BufDeleteByBlkno(pBuf->blkno);
    return result;
}

BOOL BufDeleteByBlkno(int blkno) {
    Buf *target;
    BOOL result = FALSE;
    int index = BufGetIndex(blkno);
    CIRCLEQ_FOREACH(target, &bufList[index], blist) {
        if (target -> blkno == blkno) {
            if (target->state == BUF_STATE_DIRTY) BufSyncImpl(target);
            CIRCLEQ_REMOVE(&lruListHead, target, llist);
            CIRCLEQ_REMOVE(&bufList[index], target, blist);
            CIRCLEQ_REMOVE(&stateList[target -> state], target, slist);
            CIRCLEQ_INSERT_TAIL(&freeListHead, target, flist);
            target -> blkno = BLKNO_INVALID;
            result = TRUE;
            break;
        }
    }
    return result;
}

int BufGetIndex(int blkno) {
    return blkno % MAX_BUFLIST_NUM;
}

void BufLruUpdate(Buf *target) {
    Buf *iter;
    CIRCLEQ_FOREACH(iter, &lruListHead, llist) {
        if (iter == target) {
            CIRCLEQ_REMOVE(&lruListHead, iter, llist);
            break;
        }
    }
    CIRCLEQ_INSERT_TAIL(&lruListHead, target, llist);
}

Buf * BufGetVictim() {
    Buf *victim = CIRCLEQ_FIRST(&lruListHead);
    BufDelete(victim);
    return BufGetNewBuffer();
}