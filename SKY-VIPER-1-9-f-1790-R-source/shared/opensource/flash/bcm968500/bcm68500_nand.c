/*
    Copyright 2000-2012 Broadcom Corporation

    <:label-BRCM:2012:DUAL/GPL:standard 
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2, as published by
    the Free Software Foundation (the "GPL").
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    
    A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
    writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
    
    :>

*/                       

/***************************************************************************
 * File Name  : nandflash.c
 *
 * Description: This file implements Broadcom DSL defined flash api for
 *              for NAND flash parts.
 ***************************************************************************/

/** Includes. **/
#include "common.h"
#include "nand.h"
#include "lib_printf.h"
#include "lib_string.h"
#include "bcmtypes.h"
#include "bcm_hwdefs.h"
#include "flash_api.h"
#include "jffs2.h"

#undef DEBUG_NAND
#if defined(DEBUG_NAND) && defined(CFG_RAMAPP)
#define DBG_PRINTF printf
#else
#define DBG_PRINTF(...)
#endif

#define JFFS2_CLEANMARKER   {JFFS2_MAGIC_BITMASK, JFFS2_NODETYPE_CLEANMARKER, 0x0000, 0x0008}

/** Prototypes for CFE RAM. **/
int nand_flash_init(flash_device_info_t **flash_info);
static int nand_is_blk_cleanmarker(uint32_t start_addr, int write_if_not);
static int nand_write_cleanmarker(uint32_t start_addr);
static int nand_is_blk_bad(uint32_t blockNum);
static int nand_initialize_spare_area(int eraseBadBlocks);
static void nand_mark_bad_blk(uint32_t page_addr);
static int nandflash_read_spare_area(uint32_t page_addr, uint8_t *buffer, int len);
static int nandflash_read_page(uint32_t start_addr, uint8_t *buffer, int len);
static int nandflash_write_page(uint32_t start_addr, uint8_t *buffer, int len);
static int nandflash_block_erase(uint32_t blk_addr);

static int nand_flash_sector_erase_int(uint16_t blk);
static int nand_flash_read_buf(uint16_t blk, int offset, uint8_t *buffer, int len);
static int nand_flash_write_buf(uint16_t blk, int offset, uint8_t *buffer, int numbytes);
static int nand_flash_get_numsectors(void);
static int nand_flash_get_sector_size(uint16_t sector);
static uint8_t *nand_flash_get_memptr(uint16_t sector);
static int nand_flash_get_blk(int addr);
static int nand_flash_get_total_size(void);

static nand_info_t *p_nand_info;

static flash_device_info_t flash_nand_dev =
{
    0xffff,
    FLASH_IFC_NAND,
    "",
    nand_flash_sector_erase_int,
    nand_flash_read_buf,
    nand_flash_write_buf,
    nand_flash_get_numsectors,
    nand_flash_get_sector_size,
    nand_flash_get_memptr,
    nand_flash_get_blk,
    nand_flash_get_total_size
};

extern int Starter;

/***************************************************************************
 * Function Name: nand_flash_init
 * Description  : Initialize flash part.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
int nand_flash_init(flash_device_info_t **flash_info)
{
    static int initialized = 0;
    int ret = FLASH_API_OK;
    uint16_t blk;

    if( initialized == 0 )
    {
        DBG_PRINTF(">> nand_flash_init - entry\n");

        printf("NAND: ");
        nand_init();

        p_nand_info = &nand_info[nand_curr_device];

        strcpy(flash_nand_dev.flash_device_name, p_nand_info->name);

        *flash_info = &flash_nand_dev;

        /* 
         * First eight blocks are reserved for Starter.
         * If the spare area of the first good block after block 8 is not a 
         * JFFS2 cleanmarker, initialize all block's spare area to a cleanmarker.
         */

        blk = 8;

        while (p_nand_info->block_isbad(p_nand_info, blk * p_nand_info->erasesize))
            blk++;

        if( !nand_is_blk_cleanmarker((blk * p_nand_info->erasesize), 0) )
            ret = nand_initialize_spare_area(0);

        DBG_PRINTF(">> nand_flash_init - return %d\n", ret);

        initialized = 1;
    }
    else
        *flash_info = &flash_nand_dev;

    return( ret );
} /* nand_flash_init */

/***************************************************************************
 * Function Name: nand_is_blk_cleanmarker
 * Description  : Compares a buffer to see if it a JFFS2 cleanmarker.
 * Returns      : 1 - is cleanmarker, 0 - is not cleanmarker
 ***************************************************************************/
static int nand_is_blk_cleanmarker(uint32_t start_addr, int write_if_not)
{
    int ret = 0;
    struct mtd_oob_ops ops;
    uint16_t cleanmarker[] = JFFS2_CLEANMARKER;
    uint8_t oob[sizeof(cleanmarker)];

    start_addr &= ~(p_nand_info->erasesize - 1);

    if ( !p_nand_info->block_isbad(p_nand_info, start_addr) )
    {
        /* First eight blocks are reserved for Starter. Always return 1 */
        if (start_addr >= (8 * p_nand_info->erasesize))
        {
            memset(&ops, 0, sizeof(ops));
            ops.oobbuf = oob;
            ops.ooblen = sizeof(cleanmarker);
            ops.mode = MTD_OOB_AUTO;

            p_nand_info->read_oob(p_nand_info, start_addr, &ops);

            if ( memcmp(oob, cleanmarker, sizeof(cleanmarker)) )
            {
                if ( !p_nand_info->block_isbad(p_nand_info, start_addr) )
                {
                    if (write_if_not)
                    {
                        /* The spare area is not a clean marker but the block is not bad.
                         * Write a clean marker to this block. (Assumes the block is erased.)
                         */
                        if( nand_write_cleanmarker (start_addr) == FLASH_API_OK)
                        {
                            ret = nand_is_blk_cleanmarker(start_addr, 0);
                        }
                    }
                }
            }
            else
                ret = 1;
        }
        else
            ret = 1;
    }

    return( ret );
} /* nand_is_blk_cleanmarker */

/***************************************************************************
 * Function Name: nand_write_cleanmarker
 * Description  : Writes JFFS2 cleanmarker.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int nand_write_cleanmarker(uint32_t start_addr)
{
    int ret = FLASH_API_OK;
    struct mtd_oob_ops ops;
    uint16_t cleanmarker[] = JFFS2_CLEANMARKER;

    /* First eight blocks are reserved for Starter. Do not write clean marker there */
    if (start_addr >= (8 * p_nand_info->erasesize))
    {
        memset(&ops, 0, sizeof(ops));
        ops.oobbuf = (uint8_t*)cleanmarker;
        ops.ooblen = sizeof(cleanmarker);
        ops.mode = MTD_OOB_AUTO;

        if( p_nand_info->write_oob(p_nand_info, start_addr, &ops) )
        {
            nand_mark_bad_blk(start_addr);
            ret = FLASH_API_ERROR;
        }
    }

    return( ret );
} /* nand_write_cleanmarker */

/***************************************************************************
 * Function Name: nand_is_blk_bad
 * Description  : Checks if the block is good or bad.
 * Returns      : If bad returns 1, if good returns 0
 ***************************************************************************/
static int nand_is_blk_bad(uint32_t blockNum)
{
    uint32_t page_addr = (blockNum * p_nand_info->erasesize) &  ~(p_nand_info->writesize - 1);

    return (p_nand_info->block_isbad(p_nand_info, page_addr));
} /* nand_is_blk_bad */

/*****************************************************************************/
static int nand_block_bad_scrub(struct mtd_info *mtd, loff_t ofs, int getchip)
{
    return 0;
}

/***************************************************************************
 * Function Name: nand_initialize_spare_area
 * Description  : Initializes the spare area of the first page of each block 
 *                to a cleanmarker.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int nand_initialize_spare_area(int eraseBadBlocks)
{
    int ret = FLASH_API_OK;
    uint32_t page_addr;
    int (*nand_block_bad_old)(struct mtd_info *, loff_t, int) = NULL;

    DBG_PRINTF(">> nand_initialize_spare_area - entry\n");

    if (eraseBadBlocks)
    {
        struct nand_chip *priv_nand = p_nand_info->priv;

        nand_block_bad_old = priv_nand->block_bad;
        priv_nand->block_bad = nand_block_bad_scrub;
    }

    for( page_addr = 0; page_addr < (uint32_t)p_nand_info->size; page_addr += p_nand_info->erasesize )
    {
        if ( !p_nand_info->block_isbad(p_nand_info, page_addr) )
        {
            if( (nandflash_block_erase(page_addr) == FLASH_API_OK) )
            {
                if( nand_write_cleanmarker (page_addr) == FLASH_API_ERROR )
                {
                    DBG_PRINTF("\nInitialization error writing flash block, blk=%d\n", page_addr);
                }
            }
            else
            {
                DBG_PRINTF("\nInitialization error erasing flash block, blk=%d\n", page_addr);
            }
        }
    }

    if (nand_block_bad_old)
    {
        struct nand_chip *priv_nand = p_nand_info->priv;
        priv_nand->block_bad = nand_block_bad_old;
    }

    return( ret );
} /* nand_initialize_spare_area */

/***************************************************************************
 * Function Name: nand_mark_bad_blk
 * Description  : Marks the specified block as bad by writing 0xFFs to the
 *                spare area and updating the in memory bad block table.
 * Returns      : None.
 ***************************************************************************/
static void nand_mark_bad_blk(uint32_t page_addr)
{
    int blk = page_addr/p_nand_info->erasesize;
    
    if (!nand_is_blk_bad(blk))
        p_nand_info->block_markbad(p_nand_info, page_addr);

} /* nand_mark_bad_blk */

/***************************************************************************
 * Function Name: nand_flash_sector_erase_int
 * Description  : Erase the specfied flash sector.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int nand_flash_sector_erase_int(uint16_t blk)
{
    int ret = FLASH_API_OK;

    if( ( blk == NAND_REINIT_FLASH ) || (blk == NAND_REINIT_FLASH_BAD ))
        nand_initialize_spare_area (blk == NAND_REINIT_FLASH_BAD);
    else
    {
        uint32_t page_addr = blk * p_nand_info->erasesize;

        if( (ret = nandflash_block_erase(page_addr)) == FLASH_API_OK )
        {
            if( nand_write_cleanmarker (page_addr) == FLASH_API_ERROR )
                DBG_PRINTF("\nError writing clean marker, blk=%d\n", blk);
        }
        else
            DBG_PRINTF("\nError erasing flash block, blk=%d\n", blk);
    }

    return( ret );
} /* nand_flash_sector_erase_int */

/***************************************************************************
 * Function Name: nand_flash_read_buf
 * Description  : Reads from flash memory.
 * Returns      : number of bytes read or FLASH_API_ERROR
 ***************************************************************************/
static int nand_flash_read_buf(uint16_t blk, int offset, uint8_t *buffer, int len)
{
    int ret = len;
    uint32_t start_addr;
    uint32_t blk_addr;
    uint32_t blk_offset;
    uint32_t size;
    uint32_t numBlocksInChip = (uint32_t)p_nand_info->size/p_nand_info->erasesize;    

    DBG_PRINTF(">> nand_flash_read_buf - 1 blk=0x%8.8lx, offset=%d, len=%lu\n",
        blk, offset, len);

    start_addr = (blk * p_nand_info->erasesize) + offset;
    blk_addr = start_addr & ~(p_nand_info->erasesize - 1);
    blk_offset = start_addr - blk_addr;
    size = p_nand_info->erasesize - blk_offset;

    if(size > len)
        size = len;

    do
    {
    
        if (blk >= numBlocksInChip)
        {
            DBG_PRINTF("Attempt to read block number(%d) beyond the nand max blk(%d) \n", blk, numBlocksInChip-1);
            return FLASH_API_ERROR;
        }
        
        if (nand_is_blk_bad(blk))
        {
            DBG_PRINTF("nand_flash_read_buf(): Attempt to read bad nand block %d\n", blk);
            ret = FLASH_API_ERROR;
            break;
        }
         
        if(nandflash_read_page(start_addr,buffer,size) != FLASH_API_OK)
        {
            ret = FLASH_API_ERROR;
            break;
        }

        len -= size;
        if( len )
        {
            blk++;

            DBG_PRINTF(">> nand_flash_read_buf - 2 blk=0x%8.8lx, len=%lu\n", blk, len);

            start_addr = blk * p_nand_info->erasesize;
            buffer += size;
            if(len > p_nand_info->erasesize)
                size = p_nand_info->erasesize;
            else
                size = len;
        }
    } while(len);

    DBG_PRINTF(">> nand_flash_read_buf - ret=%d\n", ret);

    return( ret ) ;
} /* nand_flash_read_buf */

/***************************************************************************
 * Function Name: nand_flash_write_buf
 * Description  : Writes to flash memory.
 * Returns      : number of bytes written or FLASH_API_ERROR
 ***************************************************************************/
static int nand_flash_write_buf(uint16_t blk, int offset, uint8_t *buffer, int len)
{
    int ret = len;
    uint32_t start_addr;
    uint32_t blk_addr;
    uint32_t blk_offset;
    uint32_t size;
    uint32_t numBlocksInChip = (uint32_t)p_nand_info->size/p_nand_info->erasesize;

    DBG_PRINTF(">> nand_flash_write_buf - 1 blk=0x%8.8lx, offset=%d, len=%d\n", blk, offset, len);

    if (blk < 8)
        Starter = 1;
    else
        Starter = 0;

    start_addr = (blk * p_nand_info->erasesize) + offset;
    blk_addr = start_addr & ~(p_nand_info->erasesize - 1);
    blk_offset = start_addr - blk_addr;
    size = p_nand_info->erasesize - blk_offset;

    if(size > len)
        size = len;

    do
    {
        if (blk >= numBlocksInChip)
        {
            DBG_PRINTF("Attempt to write block number(%d) beyond the nand max blk(%d) \n", blk, numBlocksInChip-1);
            return ret-len;
        }

        if (nand_is_blk_bad(blk))
        {
            DBG_PRINTF("nand_flash_write_buf(): Attempt to write bad nand block %d\n", blk);
            ret = ret - len;
            break;
        }

        if(nandflash_write_page(start_addr,buffer,size) != FLASH_API_OK)
        {
            ret = ret - len;
            break;
        }
        else
        {
            len -= size;
            if( len )
            {
                blk++;

                DBG_PRINTF(">> nand_flash_write_buf- 2 blk=0x%8.8lx, len=%d\n", blk, len);

                offset = 0;
                start_addr = blk * p_nand_info->erasesize;
                buffer += size;
                if(len > p_nand_info->erasesize)
                    size = p_nand_info->erasesize;
                else
                    size = len;
            }
        }
    } while(len);

    DBG_PRINTF(">> nand_flash_write_buf - ret=%d\n", ret);

    return( ret ) ;
} /* nand_flash_write_buf */

/***************************************************************************
 * Function Name: nand_flash_get_memptr
 * Description  : Returns the base MIPS memory address for the specfied flash
 *                sector.
 * Returns      : Base MIPS memory address for the specfied flash sector.
 ***************************************************************************/
static uint8_t *nand_flash_get_memptr(uint16_t sector)
{
    /* Bad things will happen if this pointer is referenced.  But it can
     * be used for pointer arithmetic to deterine sizes.
     */
    return((uint8_t *) (FLASH_BASE + (sector * p_nand_info->erasesize)));
} /* nand_flash_get_memptr */

/***************************************************************************
 * Function Name: nand_flash_get_blk
 * Description  : Returns the flash sector for the specfied MIPS address.
 * Returns      : Flash sector for the specfied MIPS address.
 ***************************************************************************/
static int nand_flash_get_blk(int addr)
{
    return((int) ((uint32_t) addr - FLASH_BASE) / p_nand_info->erasesize);
} /* nand_flash_get_blk */

/***************************************************************************
 * Function Name: nand_flash_get_total_size
 * Description  : Returns the number of bytes in the "CFE Linux code"
 *                partition.
 * Returns      : Number of bytes
 ***************************************************************************/
static int nand_flash_get_total_size(void)
{
    return((uint32_t)p_nand_info->size);
} /* nand_flash_get_total_size */

/***************************************************************************
 * Function Name: nand_flash_get_sector_size
 * Description  : Returns the number of bytes in the specfied flash sector.
 * Returns      : Number of bytes in the specfied flash sector.
 ***************************************************************************/
static int nand_flash_get_sector_size(uint16_t sector)
{
    return(p_nand_info->erasesize);
} /* nand_flash_get_sector_size */

/***************************************************************************
 * Function Name: nand_flash_get_numsectors
 * Description  : Returns the number of blocks in the "CFE Linux code"
 *                partition.
 * Returns      : Number of blocks
 ***************************************************************************/
static int nand_flash_get_numsectors(void)
{
    return((uint32_t)p_nand_info->size / p_nand_info->erasesize);
} /* nand_flash_get_numsectors */

/***************************************************************************
 * Function Name: nandflash_read_spare_area
 * Description  : Reads the spare area for the specified page.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int nandflash_read_spare_area(uint32_t page_addr, uint8_t *buffer, int len)
{
    int ret = FLASH_API_OK;

    struct mtd_oob_ops ops;

    memset(&ops, 0, sizeof(ops));
    ops.oobbuf = buffer;
    ops.ooblen = len;
    ops.mode = MTD_OOB_RAW;

    if (p_nand_info->read_oob(p_nand_info, page_addr, &ops))
        ret = FLASH_API_ERROR;

    return ret;
} /* nandflash_read_spare_area */

/***************************************************************************
 * Function Name: nandflash_read_page
 * Description  : Reads up to a NAND block of pages into the specified buffer.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int nandflash_read_page(uint32_t start_addr, uint8_t *buffer, int len)
{
    int ret = FLASH_API_ERROR;
    size_t retlen;
    uint32_t page_addr = start_addr & ~(p_nand_info->writesize - 1);

    if( len <= p_nand_info->erasesize )
    {
        if ( !p_nand_info->read(p_nand_info, start_addr, len, &retlen, buffer) )
        {
            ret = FLASH_API_OK;

            /* Verify that the spare area contains a JFFS2 cleanmarker. */
            if( !nand_is_blk_cleanmarker(page_addr, 0) )
            {
                ret = FLASH_API_ERROR;
                DBG_PRINTF("nandflash_read_page: cleanmarker not found at 0x%8.8lx\n", page_addr);
            }
        }
        else
            nand_mark_bad_blk(page_addr);
    }

    return( ret ) ;
} /* nandflash_read_page */

/***************************************************************************
 * Function Name: nandflash_write_page
 * Description  : Writes up to a NAND block of pages from the specified buffer.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int nandflash_write_page(uint32_t start_addr, uint8_t *buffer, int len)
{
    int ret = FLASH_API_ERROR;
    size_t retlen;
    uint32_t page_addr = start_addr & ~(p_nand_info->writesize - 1);

    if( len <= p_nand_info->erasesize )
    {
        /* Verify that the spare area contains a JFFS2 cleanmarker. */
        if( nand_is_blk_cleanmarker(page_addr, 1) )
        {
            if ( !p_nand_info->write(p_nand_info, start_addr, len, &retlen, buffer) )
                ret = FLASH_API_OK;
            else
                nand_mark_bad_blk(page_addr);
        }
        else
            DBG_PRINTF("nandflash_write_page: cleanmarker not found at 0x%8.8lx\n", page_addr);
    }

    return( ret );
} /* nandflash_write_page */

/***************************************************************************
 * Function Name: nandflash_block_erase
 * Description  : Erases a block.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int nandflash_block_erase(uint32_t blk_addr)
{
    int ret = FLASH_API_OK;
    int blk = blk_addr/p_nand_info->erasesize;
    struct erase_info erase;

    if (nand_is_blk_bad(blk))
    {
        DBG_PRINTF("nandflash_block_erase(): Attempt to erase bad nand block %d\n", blk);
        return  FLASH_API_ERROR;            
    }

    memset(&erase, 0, sizeof(erase));
    erase.mtd = p_nand_info;
    erase.len  = p_nand_info->erasesize;
    erase.addr = blk_addr;

    if (p_nand_info->erase(p_nand_info, &erase))
    {
        nand_mark_bad_blk(blk_addr);
        ret = FLASH_API_ERROR;
    }

    DBG_PRINTF(">> nandflash_block_erase - addr=0x%8.8lx, ret=%d\n", blk_addr,
        ret);

    return( ret );
} /* nandflash_block_erase */

void dump_spare(void);
void dump_spare(void)
{
    uint8_t spare[p_nand_info->oobsize];
    uint32_t i;

    for( i = 0; i < (uint32_t)p_nand_info->size; i += p_nand_info->erasesize )
    {
        if( nandflash_read_spare_area(i, spare, p_nand_info->oobsize) == FLASH_API_OK )
        {
            printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                *(uint32_t *) &spare[0], *(uint32_t *) &spare[4],
                *(uint32_t *) &spare[8], *(uint32_t *) &spare[12]);
            if( p_nand_info->oobsize > 16 )
            {
                printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                    *(uint32_t *)&spare[16],*(uint32_t *)&spare[20],
                    *(uint32_t *)&spare[24],*(uint32_t *)&spare[28]);
                printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                    *(uint32_t *)&spare[32],*(uint32_t *)&spare[36],
                    *(uint32_t *)&spare[40],*(uint32_t *)&spare[44]);
                printf("%8.8lx: %8.8lx %8.8lx %8.8lx %8.8lx\n", i,
                    *(uint32_t *)&spare[48],*(uint32_t *)&spare[52],
                    *(uint32_t *)&spare[56],*(uint32_t *)&spare[60]);
            }
        }
        else
            printf("Error reading spare 0x%8.8lx\n", i);
    }
}

int read_spare_data(int blk, uint8_t *buf, int bufsize);
int read_spare_data(int blk, uint8_t *buf, int bufsize)
{
    uint8_t spare[p_nand_info->oobsize];
    uint32_t page_addr = blk * p_nand_info->erasesize;
    int ret;

    if( (ret = nandflash_read_spare_area( page_addr, spare, p_nand_info->oobsize)) == FLASH_API_OK )
    {
        memset(buf, 0x00, bufsize);
        memcpy(buf, spare, (p_nand_info->oobsize > bufsize)
            ? bufsize : p_nand_info->oobsize);
    }

    return(ret);
}

int dump_spare_pages(int blk);
int dump_spare_pages(int blk)
{
    uint32_t spare[p_nand_info->oobsize];
    uint32_t page_addr = blk * p_nand_info->erasesize;
    uint32_t i;
    int ret = 0;

    for( i = 0; i < 6; i++ )
    {
        if( (ret = nandflash_read_spare_area( page_addr + (i * p_nand_info->writesize), (uint8_t *) spare,
            p_nand_info->oobsize)) == FLASH_API_OK )
        {
            printf("%8.8lx %8.8lx %8.8lx %8.8lx\n", spare[0], spare[1],
                spare[2], spare[3]);
            if( p_nand_info->oobsize > 16 )
            {
                printf("%8.8lx %8.8lx %8.8lx %8.8lx\n", spare[4], spare[5],
                    spare[6], spare[7]);
                printf("%8.8lx %8.8lx %8.8lx %8.8lx\n", spare[8], spare[9],
                    spare[10], spare[11]);
                printf("%8.8lx %8.8lx %8.8lx %8.8lx\n\n", spare[12], spare[13],
                    spare[14], spare[15]);
            }
        }
    }

    return(ret);
}
