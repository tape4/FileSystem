#ifndef __UTILS_H__
#define __UTILS_H__

#include "queue.h"
#include "buf.h"

extern void BufInsertToHead(Buf* pBuf, int blkno, BufStateList listNum);
extern void BufInsertToTail(Buf* pBuf, int blkno, BufStateList listNum);
extern Buf* BufGetNewBuffer(void);
extern BOOL BufDelete(Buf* pBuf);
extern BOOL BufDeleteByBlkno(int blokno);
extern int BufGetIndex(int blkno);
extern void BufSyncImpl(Buf *target);
extern void BufLruUpdate(Buf *target);
extern Buf* BufGetVictim();

#endif