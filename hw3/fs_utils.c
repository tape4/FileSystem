#include <stdlib.h>
#include <string.h>
#include "fs_utils.h"
#include "buf.h"

void ModifyInodeBitmap(int inodeno, int value) {
    char *InodeBlock = (char *)malloc(BLOCK_SIZE);
    int bytenum = inodeno / 8;
    int bitnum = inodeno % 8;
    char target_bit = 1 << bitnum;
    char *byte;

    BufRead(INODE_BITMAP_BLOCK_NUM, InodeBlock);
    byte = InodeBlock + bytenum;

    if (value == 1 &&
        ((*byte & target_bit) != target_bit)) // 1을 넣어줘야함.
        *byte += target_bit;
    
    else if (value == 0 &&
            ((*byte & target_bit) == target_bit)) // 0을 넣어줘야함
        *byte -= target_bit;

    BufWrite(INODE_BITMAP_BLOCK_NUM, InodeBlock);
}


void ModifyBlockBitmap(int blkno, int value) {
    char *BBlock = (char *)malloc(BLOCK_SIZE);
    int bytenum = blkno / 8;
    int bitnum = blkno % 8;
    char target_bit = 1 << bitnum;
    char *byte;

    BufRead(BLOCK_BITMAP_BLOCK_NUM, BBlock);
    byte = BBlock + bytenum;

    if (value == 1 &&
        ((*byte & target_bit) != target_bit)) // 1을 넣어줘야함.
        *byte += target_bit;
    
    else if (value == 0 &&
            ((*byte & target_bit) == target_bit)) // 0을 넣어줘야함
        *byte -= target_bit;

    BufWrite(BLOCK_BITMAP_BLOCK_NUM, BBlock);
}

int GetBlockNoByInodeNo(int inodeno) {
    return inodeno / NUM_OF_INODE_PER_BLOCK + INODELIST_BLOCK_FIRST;
}

int ParsingPath(const char* szDirName, char **paths) {
    int counter = 0;
    char *path = strdup(szDirName);
    char *token = strtok(path, "/");

    while (token != NULL) {
        // paths[counter]  = token;
        paths[counter] = strndup(token, MAX_NAME_LEN - 1);
        counter++;
        token = strtok(NULL, "/");
    }

    return counter;
}

int FindNextInodeNo(int sourceInodeNo, char *path) {
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(sourceInodeNo, pInode);

    for (int i = 0; i < pInode->allocBlocks; i++) {
        int nextBlkno = pInode->dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(nextBlkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, path) == 0) {
                return dirent[j].inodeNum;
            }
        }
    }
    return -1;
}

void UpdateFSI(int allocBlockCount, int allocInodeCount) {
    pFileSysInfo = (FileSysInfo *)malloc(BLOCK_SIZE);

    BufRead(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
    pFileSysInfo -> numAllocBlocks+=allocBlockCount;
    pFileSysInfo -> numFreeBlocks-=allocBlockCount;
    pFileSysInfo -> numAllocInodes+=allocInodeCount;
    BufWrite(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);
}

void InitNewDir(int blkno, int inodeno, int parentInodeno) {
    DirEntry *block = (DirEntry *)malloc(BLOCK_SIZE);

    BufRead(blkno, (char *)block);
    strncpy(block[0].name, ".", MAX_NAME_LEN - 1);
    strncpy(block[1].name, "..", MAX_NAME_LEN - 1);
    block[0].inodeNum = inodeno;
    block[1].inodeNum = parentInodeno;
    BufWrite(blkno, (char *)block);

    Inode *pInode = (Inode *)malloc(sizeof(Inode));

    GetInode(inodeno, pInode);
    pInode->allocBlocks = 1;
    pInode->size = pInode->allocBlocks * BLOCK_SIZE;
    pInode->type = FILE_TYPE_DIR;
    pInode->dirBlockPtr[0] = blkno;
    PutInode(inodeno, pInode);
    SetBlockBitmap(blkno);
    SetInodeBitmap(inodeno);
}

void InitNewFile(int inodeno) {
    Inode *pInode = malloc(sizeof(Inode));

    GetInode(inodeno, pInode);
    pInode->allocBlocks = 0;
    pInode->size = 0;
    pInode->type = FILE_TYPE_FILE;
    PutInode(inodeno, pInode);
    SetInodeBitmap(inodeno);
}

int InsertFileDesc(FileDesc *fileDesc, int inodeno) {
    for (int i = 0; i < MAX_FD_ENTRY_MAX; i++) {
        if (fileDesc[i].bUsed == 0) { // false
            File *f = malloc(sizeof(File));
            f->fileOffset = 0;
            f->inodeNum = inodeno;

            fileDesc[i].bUsed = 1; // true
            fileDesc[i].pOpenFile = f;
            return i;
        }
    }

    return -1;
}

int IsEmptyDirectory(int inodeno) {
    Inode *pInode = malloc(sizeof(Inode));

    GetInode(inodeno, pInode);
    if (pInode -> type != FILE_TYPE_DIR) return -1;

    for (int i = 0; i < pInode->allocBlocks; i++) {
        int blkno = pInode->dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(blkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, "") != 0 &&
                strcmp(dirent[j].name, ".") != 0 &&
                strcmp(dirent[j].name, "..") != 0)
            return -1;
        }
    }

    return 0;
}

int RemoveDirImpl(int inodeno) {
    int blkno, counter = 0;
    void *blk = malloc(BLOCK_SIZE);
    Inode *pInode = malloc(sizeof(Inode));

    GetInode(inodeno, pInode);

    for (int i =0; i <pInode->allocBlocks; i++) {
        counter++;
        blkno = pInode->dirBlockPtr[i];

        BufRead(blkno, blk);
        memset(blk, 0, BLOCK_SIZE);
        BufWrite(blkno, blk);
        ResetBlockBitmap(blkno);
    }

    memset(pInode, 0, sizeof(Inode));
    ResetInodeBitmap(inodeno);
    PutInode(inodeno, pInode);

    return counter;
}

int IsRegularFile(int inodeno) {
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);

    return pInode->type == FILE_TYPE_FILE ? 0 : -1;
}

int RemoveFileImpl(int inodeno) {
    int blkno, counter = 0;
    void *blk = malloc(BLOCK_SIZE);
    Inode *pInode = malloc(sizeof(Inode));

    GetInode(inodeno, pInode);
    for (int i = 0; i < pInode->allocBlocks; i++) {
        counter++;
        blkno = pInode->dirBlockPtr[i];

        BufRead(blkno, blk);
        memset(blk, 0, BLOCK_SIZE);
        BufWrite(blkno, blk);
        ResetBlockBitmap(blkno);
    }

    memset(pInode, 0, sizeof(Inode));
    ResetInodeBitmap(inodeno);
    PutInode(inodeno, pInode);

    for (int i = 0; i < MAX_FD_ENTRY_MAX; i++) {
        if (pFileDesc[i].bUsed == 1 &&
            pFileDesc[i].pOpenFile->inodeNum == inodeno) {
            
            free(pFileDesc[i].pOpenFile);
            pFileDesc[i].bUsed = 0;
            break;
        }
    }

    return counter;
}

DirEntryInfo *MakeDirEntryInfo(DirEntry *de) {
    DirEntryInfo *buf = malloc(sizeof(DirEntryInfo));

    memcpy(buf->name, de->name, MAX_NAME_LEN);
    buf->inodeNum = de->inodeNum;

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(de->inodeNum, pInode);
    buf->type = pInode->type;

    return buf;
}

int IsEmptyDirentBlk(int blkno) {
    DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
    
    BufRead(blkno, (char *)dirent);
    for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
        if (strcmp(dirent[i].name, "") != 0) return -1;
    }

    return 0;
}