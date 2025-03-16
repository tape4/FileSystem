#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "fs.h"
#include "fs_utils.h"

void SetInodeBitmap(int inodeno)
{
    ModifyInodeBitmap(inodeno, 1);    
}


void ResetInodeBitmap(int inodeno)
{
    ModifyInodeBitmap(inodeno, 0);    
}


void SetBlockBitmap(int blkno)
{
    ModifyBlockBitmap(blkno, 1);
}


void ResetBlockBitmap(int blkno)
{
    ModifyBlockBitmap(blkno, 0);
}


void PutInode(int inodeno, Inode* pInode)
{
    int blkno = GetBlockNoByInodeNo(inodeno);
    char *block = (char *)malloc(BLOCK_SIZE);

    BufRead(blkno, block);
    memcpy(&((Inode *)block)[inodeno % NUM_OF_INODE_PER_BLOCK], pInode, sizeof(Inode));
    BufWrite(blkno, block);
}


void GetInode(int inodeno, Inode* pInode)
{
    int blkno = GetBlockNoByInodeNo(inodeno);
    char *block = (char *)malloc(BLOCK_SIZE);

    BufRead(blkno, block);
    memcpy(pInode, &((Inode *)block)[inodeno % NUM_OF_INODE_PER_BLOCK], sizeof(Inode));
}


int GetFreeInodeNum(void)
{
    int count = 0;
    char *byte;
    char *block = (char *)malloc(BLOCK_SIZE);

    BufRead(INODE_BITMAP_BLOCK_NUM, block);
    for(int i = 0; i < (NUM_OF_INODE_PER_BLOCK * INODELIST_BLOCKS) / 8; i++) { // 16 * 4 = 64개 -> 8byte로 표현가능
        byte = block + i;
        for (int j =0; j<8; j++) {
            if (*byte % 2 == 0) return count;
            *byte = *byte >> 1;
            count ++;
        }
    }

    return -1;
}


int GetFreeBlockNum(void)
{
    int count = 0;
    char *byte;
    char *block = (char *)malloc(BLOCK_SIZE);

    BufRead(BLOCK_BITMAP_BLOCK_NUM, block);
    for(int i = 0; i < 512 / 8; i++) {
        byte = block + i;
        for (int j = 0; j < 8; j++) {
            if (*byte % 2 == 0) return count;

            *byte = *byte >> 1;
            count ++;
        }
    }

    return -1;
}