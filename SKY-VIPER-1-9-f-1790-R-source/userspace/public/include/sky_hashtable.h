#ifndef SKY_HASHTABLE_H
#define SKY_HASHTABLE_H
/*!****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: sky_hashtable.h
*
* PROJECT: 
*
* MODULE: cms_util
*
* Date Created: 
*
* Description:  This file includes definitions related to Hash Table implementation
*  
*
* Notes:
*
*****************************************************************************/
#include "os_defs.h"

typedef struct hashEntry_s
{
    char *pKey;
    char *pData;
    struct hashEntry_s *pNext;
}hashEntry_t;
 
typedef struct
{
    UINT32 size;
    hashEntry_t **pTable;
}hashTable_t;
 

#endif //SKY_HASHTABLE_H
