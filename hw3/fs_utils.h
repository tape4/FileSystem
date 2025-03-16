#ifndef __UTILS_H__
#define __UTILS_H__

#include "fs.h"

extern void ModifyInodeBitmap(int inodeno, int value);
extern void ModifyBlockBitmap(int blkno, int value);
extern int GetBlockNoByInodeNo(int inodeno);
extern int ParsingPath(const char* szDirName, char **paths);
extern int FindNextInodeNo(int sourceInodeNo, char *path);
extern void InitNewDir(int blkno, int inodeno, int parentInodeno);
extern void InitNewFile(int inodeno);
extern void UpdateFSI(int allocBlockCount, int allocInodeCount);
// extern void UpdateFSI();
extern int InsertFileDesc(FileDesc *fileDesc, int inodeno);
extern int IsEmptyDirectory(int inodeno);
extern int RemoveDirImpl(int inodeno);
extern int IsRegularFile(int inodeno);
extern int RemoveFileImpl(int inodeno);
extern DirEntryInfo *MakeDirEntryInfo(DirEntry *de);
extern int IsEmptyDirentBlk(int blkno);

#endif