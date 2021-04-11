/*!****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: sky_hashtable.c
*
* PROJECT: SkyHub
*
* MODULE: cms_util
*
* Date Created: 
*
* Description:  This file includes hash table implementation
*
* Notes:
*
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "sky_hashtable.h"
#include "cms_log.h"
#include "cms_mem.h"
#include "cms_strconv.h"

/*!*************************************************************************
* NAME: createHashTable 
*
* Description: This function creates a new hash table of the given size
*	
*
* INPUT:  
*	UINT32 size: Size (Number of entries) of the hash table
*
* OUTPUT:
*	pHashTbl - Updates with pointer of the newly created hash table
*
* RETURN:
*       Status of hash table creation
*       (CMSRET_SUCCESS) - Successful creation
*       (CMSRET_INVALID_ARGUMENTS) - Invalid input arguments
*       (CMSRET_RESOURCE_EXCEEDED) - Memory allocation failure
*
* ADDITIONAL NOTES:
*
**************************************************************************/
CmsRet createHashTable (UINT32 size, hashTable_t **pHashTbl)
{
    hashTable_t *pTable = NULL;

    if ((size == 0) || (NULL == pHashTbl))
    {
        cmsLog_error ("Invalid input arguments");
        return CMSRET_INVALID_ARGUMENTS;
    }

    /* Allocate memory for the hash table structure */
    if (NULL == (pTable = cmsMem_alloc (sizeof (hashTable_t), ALLOC_ZEROIZE)))
    {
        cmsLog_error ("Memory allocation for the hash table structure failed");
        return CMSRET_RESOURCE_EXCEEDED;
    }

    /* Allocate memory for the hash table */
    if (NULL ==
        (pTable->pTable = cmsMem_alloc (size * sizeof(hashEntry_t *),
                                        ALLOC_ZEROIZE)))
    {
        cmsLog_error ("Memory allocation for the hash table failed");
        return CMSRET_RESOURCE_EXCEEDED;
    }

    pTable->size = size;
 
    /* Update the output  variable */
    *pHashTbl = pTable;

    return CMSRET_SUCCESS;
} /* End of createHashTable() */ 

/*!*************************************************************************
* NAME: createHashEntry
*
* Description: This function creates a new entry that can be added to the
*              hash table
*       
*
* INPUT:  
*        pKey - Hash key used to uniquely identify the entry in hash table
*
* OUTPUT:
*        pEntry - Pointer to the newly created hash entry.
*
* RETURN:
*        CmsRet - Status of the hash entry creation
*        (TRUE) - Successful creation
*        (FALSE)- Memory allocation failure, Invalid arguments
*
* ADDITIONAL NOTES:
*        The newly created hash entry does not allocate memory for the "pData"
*        parameter. It is user's responsibility to create, use and release
*        memory for "pData" member of the hash entry.
*
**************************************************************************/
CmsRet createHashEntry (const char* pKey, hashEntry_t **pEntry)
{
    hashEntry_t *pHashEntry = NULL;

    cmsLog_debug ("Enter %s", __FUNCTION__);
 
    if ((NULL == pKey) || (NULL == pEntry))
    {
        cmsLog_error ("Invalid input arguments");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (NULL == (pHashEntry = cmsMem_alloc (sizeof (hashEntry_t), ALLOC_ZEROIZE)))
    {
        cmsLog_error ("Cannot allocate memory for hash table entry");
        return CMSRET_RESOURCE_EXCEEDED;
    }
 
    if (NULL == (pHashEntry->pKey = cmsMem_strndup (pKey, BUFLEN_128 - 1)))
    {
        cmsLog_error ("Cannot create hash key");
        return CMSRET_RESOURCE_EXCEEDED;
    }
 
    pHashEntry->pNext = NULL;

    *pEntry = pHashEntry;

    cmsLog_debug ("Exit %s", __FUNCTION__);
 
    return CMSRET_SUCCESS;
} /* End of createHashEntry() */

/*!*************************************************************************
* NAME: releaseHashEntry
*
* Description: This function releases all the memory associated with the hash
*              entry.
*       
* INPUT:  
*        pEntry - Address of the hash entry to be released
*
* OUTPUT:
*        None
*
* RETURN:
*        CmsRet - Status of the hash entry release
*        (TRUE) - Successful release of memory
*        (FALSE)- Invalid arguments
*
* ADDITIONAL NOTES:
*        "pData" parameter is expected to be freed by the user/caller
*         before calling this function.
*
**************************************************************************/
CmsRet releaseHashEntry (hashEntry_t *pEntry)
{
    cmsLog_debug ("Enter %s", __FUNCTION__);

    if (NULL == pEntry)
    {
        cmsLog_error ("Invalid input argument");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (NULL != pEntry->pData)
    {
        cmsLog_error ("pData of hash entry is not released. Memory leak");
        return CMSRET_INTERNAL_ERROR;
    }

    CMSMEM_FREE_BUF_AND_NULL_PTR (pEntry->pKey);

    CMSMEM_FREE_BUF_AND_NULL_PTR (pEntry);

    cmsLog_debug ("Exit %s", __FUNCTION__);

    return CMSRET_SUCCESS;
} /* End of releaseHashEntry() */

/*!*************************************************************************
* NAME: clearHashTable 
*
* Description: This function clears all the contents of the given hash table
*	
*
* INPUT:  
*       None
*
* OUTPUT:
*	pHashTbl - Pointer to the hash table to be cleared
*
* RETURN:
*       Status of the clear operation
*       (CMSRET_SUCCESS) - Success in clearing the hash table
*
* ADDITIONAL NOTES:
*
**************************************************************************/
CmsRet clearHashTable (hashTable_t *pHashTbl, UBOOL8 isClearData)
{
    hashEntry_t *pEntry;
    hashEntry_t *pRelease;
    int index = 0;
    CmsRet retVal = CMSRET_SUCCESS;
    CmsRet ret;

    if (NULL == pHashTbl)
    {
        cmsLog_error ("Invalid input arguments");
        return CMSRET_INVALID_ARGUMENTS;
    }

    if (isClearData)
    {
        for (index = 0; index < (pHashTbl->size); index++)
        {
            pEntry = pHashTbl->pTable[index];

            while (NULL != pEntry)
            {
                pRelease = pEntry;
                pEntry = pEntry->pNext;
                if (CMSRET_SUCCESS != (ret = releaseHashEntry (pRelease)))
                {
                    retVal = ret;
                }
            } /* End of while() - end of bucket list */
        } /* End of hash table */
    } /* End of if() - data is also to be cleared */

    CMSMEM_FREE_BUF_AND_NULL_PTR (pHashTbl->pTable);
    CMSMEM_FREE_BUF_AND_NULL_PTR (pHashTbl);

    return retVal;
} /* End of clearHashTable() */

CmsRet generateHash (const hashTable_t *pHashTable, const char *pHashKey, UINT32 *pHashVal)
{
    CmsRet ret = CMSRET_SUCCESS;
    UINT32 hash = 0;
    UINT32 keyLen = 0;
    UINT32 keyIter;

    cmsLog_debug ("Enter %s", __FUNCTION__);

    if ((NULL == pHashTable) || (NULL == pHashKey) || (NULL == pHashVal))
    {
        cmsLog_error ("Invalid input arguments");
        cmsLog_debug ("Exit %s", __FUNCTION__);
        return CMSRET_INVALID_ARGUMENTS;
    }

    keyLen = cmsUtl_strlen (pHashKey);
    for (keyIter = 0; keyIter < keyLen; keyIter++)
    {
        hash += pHashKey[keyIter];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }

    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    *pHashVal = (hash % pHashTable->size);

    cmsLog_debug ("Hash key: %s, val: %u", pHashKey, *pHashVal);

    cmsLog_debug ("Exit %s", __FUNCTION__);

    return ret;
} /* End of generateHash() */

/*!*************************************************************************
* NAME: findHashEntry
*
* Description: This function detects if the given hash entry is present in
*              the given hash table.
*       
*
* INPUT:
*        index - Position of the hash entry within the table
*        pKey - Hash key used to uniquely identify the entry in hash table
*
* OUTPUT:
*       None 
*
* RETURN:
*        hashEntry_t* - Pointer to the desired hash entry
*        (NULL) - Desired hash entry not present in table
*
* ADDITIONAL NOTES:
*
**************************************************************************/
hashEntry_t* findHashEntry (const hashTable_t *pHashTable, UINT32 index, const char* pKey)
{
    hashEntry_t *pEntry = NULL;
 
    if ((NULL == pHashTable) || (NULL == pKey) || (index > pHashTable->size))
    {
        cmsLog_error ("Invalid input arguments");
        return NULL;
    }

    pEntry = pHashTable->pTable[index];
 
    /* In case of collision, there will be more than one entry pertaining to
     * the same hash value. A linked list is used for maintaining such entries.
     * Traverse the linked list to find the desired hash entry, by validating
     * against the key.
     */
    while ((NULL != pEntry) &&
           ((NULL == pEntry->pKey) || (0 != cmsUtl_strcmp (pKey, pEntry->pKey))))
    {
        pEntry = pEntry->pNext;
    } /* Desired Hash Entry found or End of hash Entry Bucket list */
 
    if ((NULL != pEntry) && (NULL != pEntry->pKey) && (0 == cmsUtl_strcmp (pKey, pEntry->pKey)))
    {
        cmsLog_debug ("Hash entry present in hash table"); 
    }

    return pEntry;

} /* End of findHashEntry() */

/*!*************************************************************************
* NAME: addHashEntry
*
* Description: This function validates if the given hash entry is present in
*              the given hash table. If present, it updates the data of the
*              corresponding hash entry. If not present, it adds the entry
*              to the hash table. 
*       
*
* INPUT:
*        index - Position of the hash entry within the table
*        pKey - Hash key used to uniquely identify the entry in hash table
*
* OUTPUT:
*        pTable - Pointer to the hash table that is updated
*
* RETURN:
*        CmsRet - Pointer to the added/updated hash entry
*        (NULL) - Invalid input argument, Failed to add hash entry in table
*
* ADDITIONAL NOTES:
*
**************************************************************************/
hashEntry_t* addHashEntry (hashTable_t *pHashTable, UINT32 index, const char* pKey)
{
    CmsRet ret;
    hashEntry_t *pNewEntry = NULL;
    hashEntry_t *pEntry = NULL;
 
    if ((NULL == pHashTable) || (NULL == pKey) || (index > pHashTable->size))
    {
        cmsLog_error ("Invalid input arguments");
        return NULL;
    }

    cmsLog_debug ("Adding a new Hash entry with key [%s] at index [%d]",
                   pKey, index);
    if (CMSRET_SUCCESS != (ret = createHashEntry (pKey, &pNewEntry)))
    {
        cmsLog_error ("Failed to create a new hash entry, ret: %d", ret);
    }

    /* Insert the new entry into the hash table */
    pEntry = pHashTable->pTable[index];

    if (NULL != pEntry)
    {
        cmsLog_debug ("Bucket list present");
        /* There is already a hash entry bucket list. Insert new entry at head of the list */
        pNewEntry->pNext = pEntry;
    }
    pHashTable->pTable[index] = pNewEntry;

    cmsLog_debug ("Exit %s", __FUNCTION__);

    return pNewEntry;
} /* End of addHashEntry() */
