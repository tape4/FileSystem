#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs_utils.h"
#include "buf.h"
#include "fs.h"

FileDesc pFileDesc[MAX_FD_ENTRY_MAX];
FileSysInfo* pFileSysInfo;

int      GetFileStatus(const char* szPathName, FileStatus* pStatus) {
    char **paths = malloc(sizeof(char *) * sizeof(szPathName));
    int counter = ParsingPath(szPathName, paths);
    int parentInodeNo = 0;
    
    for (int i = 0; i < counter - 1; i++) {
        parentInodeNo = FindNextInodeNo(parentInodeNo, paths[i]);
        if (parentInodeNo == -1) return -1; // 중간 경로를 못찾았을 때
    }

    memset(pStatus, 0, sizeof(FileStatus));
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(parentInodeNo, pInode);
    for (int i = 0; i<pInode->allocBlocks; i++) {
        int nextBlkno = pInode -> dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(nextBlkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, paths[counter - 1]) == 0) {

                Inode *resultInode = malloc(sizeof(Inode));
                GetInode(dirent[j].inodeNum, resultInode);

                pStatus->allocBlocks = resultInode->allocBlocks;
                pStatus->size = resultInode->size;
                pStatus->type = resultInode->type;
                for (int i = 0; i < resultInode->allocBlocks; i++)
                    pStatus->dirBlockPtr[i] = resultInode->dirBlockPtr[i];
                return 0;
            }
        }
    }

    return -1;
}

int     CreateFile(const char* szFileName)
{
    int inodeno = GetFreeInodeNum();

    char **paths = malloc(sizeof(char *) * sizeof(szFileName));
    int counter = ParsingPath(szFileName, paths);
    int parentInodeNo = 0;

    if (inodeno == -1) return -1;
    if (strcmp(paths[counter - 1], ".") == 0 || strcmp(paths[counter - 1], "..") == 0 ) return -1; 

    for (int i = 0; i < counter - 1; i++) {
        parentInodeNo = FindNextInodeNo(parentInodeNo, paths[i]);
        if (parentInodeNo == -1)  return -1; // 중간 경로를 못찾았을 때
    }

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(parentInodeNo, pInode);
    for (int i = 0; i< pInode->allocBlocks; i++) {
        int nextBlkno = pInode -> dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(nextBlkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, "") == 0) { // dirent 비어있는거 조건 바뀔수도
                strncpy(dirent[j].name, paths[counter - 1], MAX_NAME_LEN - 1);
                dirent[j].inodeNum = inodeno;
                BufWrite(nextBlkno, (char *)dirent);
                InitNewFile(inodeno);
                UpdateFSI(0, 1);
                return InsertFileDesc(pFileDesc, inodeno);
            }
        }
    }

    // 새 블럭 할당할 자리가 없는경우 return -1
    if (pInode->allocBlocks == NUM_OF_DIRECT_BLOCK_PTR) return -1;

    // 새 블럭 할당할 자리가 있는경우 새로운 direntry block을 만든다.
    InitNewFile(inodeno);

    int newDirentBlockNo = GetFreeBlockNum();
    if (newDirentBlockNo == -1) { // RollBack No Available Block
        Inode *pInode = (Inode *)malloc(sizeof(Inode));
        GetInode(inodeno, pInode);
        memset(pInode, 0, sizeof(Inode));
        PutInode(inodeno, pInode);

        ResetInodeBitmap(inodeno);
    }

    DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
    BufRead(newDirentBlockNo, (char *)dirent);
    strncpy(dirent[0].name, paths[counter -1], MAX_NAME_LEN -1);
    dirent[0].inodeNum = inodeno;

    pInode->dirBlockPtr[pInode->allocBlocks] = newDirentBlockNo;
    pInode->allocBlocks++;
    pInode->size = pInode->allocBlocks * BLOCK_SIZE;
    PutInode(parentInodeNo, pInode);
    BufWrite(newDirentBlockNo, (char *)dirent);
    SetBlockBitmap(newDirentBlockNo);
    UpdateFSI(1, 1);
    return InsertFileDesc(pFileDesc, inodeno);
}

int     OpenFile(const char* szFileName)
{
    char **paths = malloc(sizeof(char *) * sizeof(szFileName));
    int counter = ParsingPath(szFileName, paths);
    int parentInodeNo = 0;

    for (int i = 0; i < counter - 1; i++) {
        parentInodeNo = FindNextInodeNo(parentInodeNo, paths[i]);
        if (parentInodeNo == -1) return -1; // 중간 경로를 못찾았을 때
    }

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(parentInodeNo, pInode);
    for (int i = 0; i< pInode->allocBlocks; i++) {
        int nextBlkno = pInode -> dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(nextBlkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, paths[counter - 1]) == 0) {
                return InsertFileDesc(pFileDesc, dirent[j].inodeNum);
            }
        }
    }

    return -1; // 찾기 실패
}


int     WriteFile(int fileDesc, char* pBuffer, int length)
{
    int blkno, allocBlkno;
    File *open_file = pFileDesc[fileDesc].pOpenFile;
    int inodeno = open_file->inodeNum;
    allocBlkno = open_file->fileOffset / BLOCK_SIZE;

    if (allocBlkno >= 5 || pFileDesc[fileDesc].bUsed == 0) return -1;
    
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);

    if (pInode -> dirBlockPtr[allocBlkno] == 0) { // 새로운 블럭할당
        blkno = GetFreeBlockNum();
        if (blkno == - 1) return -1;

        pInode->dirBlockPtr[allocBlkno] = blkno;
        pInode->allocBlocks = allocBlkno + 1;
        pInode->size = pInode->allocBlocks * BLOCK_SIZE;
        open_file->fileOffset += length;
        SetBlockBitmap(blkno);
        UpdateFSI(1, 0);
        PutInode(inodeno, pInode);
    } else { // 이미 데이터 블럭이 존재할때 (덮어쓰기)
        blkno = pInode->dirBlockPtr[allocBlkno];        
        open_file->fileOffset += length;
    }

    void *block = malloc(BLOCK_SIZE);
    BufRead(blkno, (char *)block);
    memcpy(block, pBuffer, length);
    BufWrite(blkno, (char *)block);

    return length;
}

int     ReadFile(int fileDesc, char* pBuffer, int length)
{
    File *open_file = pFileDesc[fileDesc].pOpenFile;
    int inodeno = open_file->inodeNum;
    if (pFileDesc[fileDesc].bUsed == 0) return -1;
    
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);

    int targetBlockInx = open_file->fileOffset / BLOCK_SIZE;
    if (targetBlockInx == 5) return -1; // 파일을 이미 다 읽은 fd

    open_file->fileOffset += length;
    int blkno = pInode->dirBlockPtr[targetBlockInx];
    void *block = malloc(BLOCK_SIZE);

    BufRead(blkno, (char *)block);
    memcpy(pBuffer, block, length);

    return length;
}


int     CloseFile(int fileDesc)
{
    if (pFileDesc[fileDesc].bUsed == 0) return -1;

    pFileDesc[fileDesc].bUsed = 0;
    free(pFileDesc[fileDesc].pOpenFile);

    return 0;
}

int     RemoveFile(const char* szFileName)
{
    char **paths = malloc(sizeof(char *) * sizeof(szFileName));
    int counter = ParsingPath(szFileName, paths);
    int blkno, parentInodeNo = 0;

    if (strcmp(paths[counter - 1], ".") == 0 || strcmp(paths[counter - 1], "..") == 0 ) return -1; 

    for (int i = 0; i < counter - 1; i++) {
        parentInodeNo = FindNextInodeNo(parentInodeNo, paths[i]);
        if (parentInodeNo == -1) return -1; // 중간 경로를 못찾았을 때
    }

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(parentInodeNo, pInode);
    for (int i = 0; i< pInode->allocBlocks; i++) {
        blkno = pInode->dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(blkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, paths[counter - 1]) == 0 && // 제거 대상 발견
                IsRegularFile(dirent[j].inodeNum) == 0) {
                
                int blkcount = RemoveFileImpl(dirent[j].inodeNum);
                memset(&dirent[j], 0, sizeof(DirEntry));
                BufWrite(blkno, (char *)dirent);
                UpdateFSI(-blkcount, -1);
                
                if (i != 0 && IsEmptyDirentBlk(blkno) == 0) {
                    ResetBlockBitmap(blkno);
                    UpdateFSI(-1, 0);
                    pInode -> allocBlocks--;
                    pInode -> size = pInode -> allocBlocks * BLOCK_SIZE;
                    pInode -> dirBlockPtr[i] = 0;
                    PutInode(parentInodeNo, pInode);
                }

                return 0;
            }
        }
    }


    return -1;
}


int     MakeDir(const char* szDirName)
{
    char **paths = malloc(sizeof(char *) * sizeof(szDirName));
    int counter = ParsingPath(szDirName, paths);
    int inodeno = GetFreeInodeNum();
    int blkno = GetFreeBlockNum();
    int parentInodeNo = 0;


    if (inodeno == -1 || blkno == -1) return -1;

    for (int i = 0; i < counter - 1; i++) {
        parentInodeNo = FindNextInodeNo(parentInodeNo, paths[i]);
        if (parentInodeNo == -1) return -1; // 중간 경로를 못찾았을 때
    }
    
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(parentInodeNo, pInode);
    for (int i = 0; i< pInode->allocBlocks; i++) {
        int nextBlkno = pInode -> dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(nextBlkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, "") == 0) { // dirent 비어있는거 조건 바뀔수도
                strncpy(dirent[j].name, paths[counter - 1], MAX_NAME_LEN - 1);
                dirent[j].inodeNum = inodeno;
                BufWrite(nextBlkno, (char *)dirent);

                InitNewDir(blkno, inodeno, parentInodeNo);
                UpdateFSI(1, 1);
                return 0;
            }
        }
    }
 
    // 새 블럭 할당할 자리가 없는경우 return -1
    if (pInode->allocBlocks == NUM_OF_DIRECT_BLOCK_PTR) return -1;

    // 새 블럭 할당할 자리가 있는경우 새로운 direntry block을 만든다.
    InitNewDir(blkno, inodeno, parentInodeNo);
    int newDirentBlockNo = GetFreeBlockNum();

    // Rollback
    if (newDirentBlockNo == -1) {
        DirEntry *block = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(blkno, (char *)block);
        memset(block, 0, BLOCK_SIZE);
        BufWrite(blkno, (char *)block);

        Inode *pInode = (Inode *)malloc(sizeof(Inode));
        GetInode(inodeno, pInode);
        memset(pInode, 0, sizeof(Inode));
        PutInode(inodeno, pInode);
        
        ResetBlockBitmap(blkno);
        ResetInodeBitmap(inodeno);
        return -1;
    }

    DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
    BufRead(newDirentBlockNo, (char *)dirent);
    strncpy(dirent[0].name, paths[counter-1], MAX_NAME_LEN - 1);
    dirent[0].inodeNum = inodeno;

    pInode->dirBlockPtr[pInode->allocBlocks] = newDirentBlockNo;
    pInode->allocBlocks++;
    pInode->size = pInode->allocBlocks * BLOCK_SIZE;
    PutInode(parentInodeNo, pInode);
    BufWrite(newDirentBlockNo, (char *)dirent);
    SetBlockBitmap(newDirentBlockNo);
    UpdateFSI(2, 1);
    return 0;
}


int     RemoveDir(const char* szDirName)
{
    char **paths = malloc(sizeof(char *) * sizeof(szDirName));
    int counter = ParsingPath(szDirName, paths);
    int blkno, parentInodeNo = 0;

    for (int i = 0; i < counter - 1; i++) {
        parentInodeNo = FindNextInodeNo(parentInodeNo, paths[i]);
        if (parentInodeNo == -1) return -1; // 중간 경로를 못찾았을 때
    }

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(parentInodeNo, pInode);
    for (int i = 0; i< pInode->allocBlocks; i++) {
        blkno = pInode -> dirBlockPtr[i];
        DirEntry *dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(blkno, (char *)dirent);
        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, paths[counter - 1]) == 0 &&
                IsEmptyDirectory(dirent[j].inodeNum) == 0) {

                int blkcount = RemoveDirImpl(dirent[j].inodeNum);
                memset(&dirent[j], 0, sizeof(DirEntry));
                BufWrite(blkno, (char *)dirent);
                UpdateFSI(-blkcount, -1);

                if (i != 0 && IsEmptyDirentBlk(blkno) == 0) {
                    ResetBlockBitmap(blkno);
                    UpdateFSI(-1, 0);
                    pInode -> allocBlocks--;
                    pInode -> size = pInode -> allocBlocks * BLOCK_SIZE;
                    pInode -> dirBlockPtr[i] = 0;
                    PutInode(parentInodeNo, pInode);
                }

                return 0;
            }
        }
    }

    return -1;
}

int   EnumerateDirStatus(const char* szDirName, DirEntryInfo* pDirEntry, int dirEntrys)
{
    char **paths = malloc(sizeof(char *) * sizeof(szDirName));
    int counter = ParsingPath(szDirName, paths);
    int blkno, inodeno = 0, size = 0;
    DirEntry *dirent;

    for (int i = 0; i < counter; i++) {
        inodeno = FindNextInodeNo(inodeno, paths[i]);
        if (inodeno == -1) return -1; // 중간 경로를 못찾았을 때
    }

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);

    if (pInode->type != FILE_TYPE_DIR) return -1;

    for (int i = 0; i<pInode->allocBlocks; i++) {
        blkno = pInode -> dirBlockPtr[i];
        dirent = (DirEntry *)malloc(BLOCK_SIZE);
        BufRead(blkno, (char *)dirent);
        for (int j = 0; j< NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (strcmp(dirent[j].name, "") == 0) continue;
            else if (size + 1 <= dirEntrys) 
                pDirEntry[size++] = *MakeDirEntryInfo(&dirent[j]);
        }
    }

    return size;
}


void    CreateFileSystem()
{
    DevCreateDisk();
    FileSysInit();
    int blkno = GetFreeBlockNum();
    int inodeno = GetFreeInodeNum();

    DirEntry *block = (DirEntry *)malloc(BLOCK_SIZE);
    BufRead(blkno, (char *)block);
    strncpy(block[0].name, ".", MAX_NAME_LEN - 1);
    block[0].inodeNum = inodeno;
    BufWrite(blkno, (char *)block);

    pFileSysInfo = (FileSysInfo *)malloc(BLOCK_SIZE);
    pFileSysInfo->blocks = 512;
    pFileSysInfo->rootInodeNum = inodeno;
    pFileSysInfo->diskCapacity = FS_DISK_CAPACITY;
    pFileSysInfo->numAllocBlocks = 0;
    pFileSysInfo->numFreeBlocks = 512 - 7;
    pFileSysInfo->numAllocInodes = 0;
    pFileSysInfo->blockBitmapBlock = BLOCK_BITMAP_BLOCK_NUM;
    pFileSysInfo->inodeBitmapBlock = INODE_BITMAP_BLOCK_NUM;
    pFileSysInfo->inodeListBlock = INODELIST_BLOCK_FIRST;
    pFileSysInfo->dataRegionBlock = INODELIST_BLOCK_FIRST + INODELIST_BLOCKS;
    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    pFileSysInfo->numAllocInodes++;
    BufWrite(FILESYS_INFO_BLOCK, (char *)pFileSysInfo);

    SetBlockBitmap(blkno);
    SetInodeBitmap(inodeno);

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);
    pInode -> allocBlocks = 1;
    pInode -> size = pInode -> allocBlocks * BLOCK_SIZE;
    pInode -> type = FILE_TYPE_DIR;
    pInode -> dirBlockPtr[0] = blkno;

    PutInode(inodeno, pInode);
    Sync();
}


void    FileSysInit(void) 
{
    char *pData = (char *)malloc(BLOCK_SIZE);
    memset(pData, 0, BLOCK_SIZE);
    BufInit();

    for(int i = 0; i < 512; i++) {
        BufWrite(i, pData);
    }

    for(int i = 0; i <= 6; i++)  {
        SetBlockBitmap(i);
    }
}

void    OpenFileSystem()
{
    BufInit();
    DevOpenDisk();
}

void     CloseFileSystem()
{
    DevCloseDisk();
}

void	Sync()
{
    BufSync();
}