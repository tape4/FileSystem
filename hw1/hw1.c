#include <stdio.h>
#include "hw1.h"

int GetHash(int objNum);
int EnumberateObjectsByHashIndex(int index, Object* ppObject[], int count);
BOOL DeleteObjectFromList(Object *pObj, List listNum);
BOOL DeleteObjectFromHash(Object *pObj);
// Object*     FindObjectByNumLIST3(int objnum);

struct hashTable ppHashTable[HASH_TBL_SIZE];
struct list ppListHead[MAX_LIST_NUM];

/**
* Initialize ppHashTable and ppListHead.
*/
void        Init(void)
{
    for (int i = 0; i < HASH_TBL_SIZE; i++) CIRCLEQ_INIT(&ppHashTable[i]);
    for (int i = 0; i < MAX_LIST_NUM; i++) CIRCLEQ_INIT(&ppListHead[i]);
}

/** 
* Insert Object to HashTable's Tail and List's Tail.
*
* @Param pObj    : The Object to be insert into HashTable and List.
* @Param objNum  : pObj's objNum (member variable).
* @Param listNum : The List that pObj to be inserted.
*/
void        InsertObjectToTail(Object* pObj, int objNum, List listNum)
{
    pObj -> objnum = objNum;
    if (listNum != LIST3) {
        int index = GetHash(objNum);
        CIRCLEQ_INSERT_TAIL(&ppHashTable[index], pObj, hash);
    }

    CIRCLEQ_INSERT_TAIL(&ppListHead[listNum], pObj, link);
}

/** 
* Insert Object to HashTable's Head and List's Tail.
* if listNum is LIST3, Insert Head of LIST3.
*
* @Param pObj    : The Object to be insert into HashTable and List.
* @Param objNum  : pObj's objNum (member variable).
* @Param listNum : The List that pObj to be inserted.
*/
void        InsertObjectToHead(Object* pObj, int objNum, List listNum)
{
    pObj -> objnum = objNum;
    if (listNum != LIST3) {
        int index = GetHash(objNum);
        CIRCLEQ_INSERT_HEAD(&ppHashTable[index], pObj, hash);
        CIRCLEQ_INSERT_TAIL(&ppListHead[listNum], pObj, link);
    } else { // LIST3
        CIRCLEQ_INSERT_HEAD(&ppListHead[listNum], pObj, link);
    }
}

/** 
* Find Object by given objnum on HashTable? -> List
*
* @Param objNum  : objNum of the Object that User looking for.
* @Return : Pointer if find Object else NULL.
*/
Object*     FindObjectByNum(int objnum)
{
    Object * iter;

    for (int i = 0; i<MAX_LIST_NUM; i++) {
        CIRCLEQ_FOREACH(iter, &ppListHead[i], link) {
            if (iter->objnum == objnum) return iter;
        }
    }
    // for (int i = 0; i<HASH_TBL_SIZE; i++) {
    //     CIRCLEQ_FOREACH(iter, &ppHashTable[i], hash) {
    //         if (iter->objnum == objnum) return iter;
    //     }
    // }
    return NULL;
}

// 해쉬테이블과, 모든 리스트에서 호출가능
/** 
* Delete Object on HashTable and List.
*
* @Param pObj : The Object that to be removed.
* @Return : TRUE if delete success else FALSE.
*/
BOOL        DeleteObject(Object* pObj)
{
    if (DeleteObjectFromList(pObj, LIST3)) return TRUE;

    for (int i = 0; i<MAX_LIST_NUM - 1; i++) {
        if (DeleteObjectFromList(pObj, i)) break;
    }

    BOOL deleteFlag = DeleteObjectFromHash(pObj);
    return deleteFlag;
}

/** 
* Find Object by given objNum and delete found Object in HashTable and List.
*
* @Param objNum : The Object that to be found and removed.
* @Return : TRUE if find & delete success else FALSE.
*/
BOOL        DeleteObjectByNum(int objNum)
{
    Object *target = FindObjectByNum(objNum);
    // if (target == NULL) target = FindObjectByNumLIST3(objNum);
    if (target == NULL) return FALSE;
    return DeleteObject(target);
}


/** 
* Get all objects in a specific list.
*
* @Param listnum  : Indicates a list number.
* @Param ppObject : Indicates a buffer into which to store the pointers of the Objects.
* @Param count    : Buffer's size.
* @Return : Specifies the number of objects in the list.
*/
int         EnumberateObjectsByListNum(List listnum, Object* ppObject[], int count)
{
    int size = 0;
    Object *iter;

    CIRCLEQ_FOREACH(iter, &ppListHead[listnum], link) {
        if (size + 1 <= count) ppObject[size++] = iter;
        else break;
    }
    return size;
}

int EnumberateObjectsByHashIndex(int index, Object* ppObject[], int count)
{
    int size = 0;
    Object *iter;

    CIRCLEQ_FOREACH(iter, &ppHashTable[index], hash) {
        if (size + 1 <= count) ppObject[size++] = iter;
        else break;
    }
    return size;
}

/** 
* @Return Remainder objNum by 'HASH_TBL_SIZE' 
*/
int GetHash(int objNum) {
    return objNum % HASH_TBL_SIZE;
}

/** 
* Delete Object from HashTable
*
* @Param pObj  : The Object that to be removed.
* @Return : TRUE if delete success else FALSE.
*/
BOOL DeleteObjectFromHash(Object *pObj) {
    int hash_num = GetHash(pObj->objnum);
    BOOL deleteFlag = FALSE;
    Object *iter;

    CIRCLEQ_FOREACH(iter, &ppHashTable[hash_num], hash) {
        if (iter == pObj) {
            deleteFlag = TRUE;
            CIRCLEQ_REMOVE(&ppHashTable[hash_num], pObj, hash);
            break;
        }
    }
    return deleteFlag;
}

/** 
* Delete Object from List
*
* @Param pObj    : The Object that to be removed.
* @Param listnum : The Listnum that pObj to be removed.
* @Return : TRUE if delete success else FALSE.
*/
BOOL DeleteObjectFromList(Object *pObj, List listnum) {
    BOOL deleteFlag = FALSE;
    Object *iter;

    CIRCLEQ_FOREACH(iter, &ppListHead[listnum], link) {
        if (iter == pObj) {
            deleteFlag = TRUE;
            CIRCLEQ_REMOVE(&ppListHead[listnum], pObj, link);
            break;
        }
    }

    // if (deleteFlag) InsertObjectToHead(pObj, pObj->objnum, LIST3);
    return deleteFlag;
}


/** 
* Find Object on LIST3 using given objnum.
*
* @Param objnum  : The Object that to be found.
* @Return : Pointer if find else NULL.
*/
// Object*     FindObjectByNumLIST3(int objnum) {
//     Object *iter;

//     CIRCLEQ_FOREACH(iter, &ppListHead[LIST3], link) {
//         if (iter->objnum == objnum) return iter;
//     }
//     return NULL;
// }