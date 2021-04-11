
/*
<:copyright-BRCM:2013:DUAL/GPL:standard 

   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

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
/*
 ***************************************************************************
 * File Name  : bcm63xx_flash.c
 *
 * Description: This file contains the flash device driver APIs for bcm63xx board. 
 *
 * Created on :  8/10/2002  seanl:  use cfiflash.c, cfliflash.h (AMD specific)
 *
 ***************************************************************************/

/* Includes. */
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/preempt.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/mtd/mtd.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/jffs2.h>
#include <linux/mount.h>
#include <linux/crc32.h>
#include <linux/sched.h>
#include <linux/bcm_assert_locks.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/compiler.h>
#ifdef SKY
#define  BCMTAG_EXE_USE
#endif /* SKY */

#include <bcmtypes.h>
#include <bcm_map_part.h>
#include <board.h>
#include <bcmTag.h>
#include "flash_api.h"
#include "boardparms.h"
#include "boardparms_voice.h"

#if defined(CONFIG_BCM968500)
#include "bl_lilac_mips_blocks.h"
#endif

#include <linux/fs_struct.h>
//#define DEBUG_FLASH

extern int kerSysGetSequenceNumber(int);
#ifdef SKY
#ifdef CONFIG_SKY_FEATURE_WATCHDOG
extern void restartHWWatchDog(void);
#endif //CONFIG_SKY_FEATURE_WATCHDOG
extern void ledToggle_fromFirmwareUpdate(short gpio,unsigned short led_type);
extern PFILE_TAG kerSysSetTagSequenceNumber(int,int);
#endif //SKY
extern PFILE_TAG kerSysUpdateTagSequenceNumber(int);




/*
 * inMemNvramData an in-memory copy of the nvram data that is in the flash.
 * This in-memory copy is used by NAND.  It is also used by NOR flash code
 * because it does not require a mutex or calls to memory allocation functions
 * which may sleep.  It is kept in sync with the flash copy by
 * updateInMemNvramData.
 */
static unsigned char *inMemNvramData_buf;
static NVRAM_DATA inMemNvramData;
static DEFINE_SPINLOCK(inMemNvramData_spinlock);
static void updateInMemNvramData(const unsigned char *data, int len, int offset);
#define UNINITIALIZED_FLASH_DATA_CHAR  0xff
static FLASH_ADDR_INFO fInfo;
static struct semaphore semflash;

// mutex is preferred over semaphore to provide simple mutual exclusion
// spMutex protects scratch pad writes
static DEFINE_MUTEX(spMutex);
extern struct mutex flashImageMutex;
static int bootFromNand = 0;

static int setScratchPad(char *buf, int len);
static char *getScratchPad(int len);
static int nandNvramSet(const char *nvramString );

// Variables not used in the simplified API used for the IKOS target
#if !defined(CONFIG_BRCM_IKOS)
static char bootCfeVersion[CFE_VERSION_MARK_SIZE+CFE_VERSION_SIZE];
#endif



#define ALLOC_TYPE_KMALLOC   0
#define ALLOC_TYPE_VMALLOC   1

#ifdef SKY
#if defined(CONFIG_BCM96362)
#define  PWR_LED_GPIO 27 
#elif defined(CONFIG_BCM963268)
#define  PWR_LED_GPIO 6  
#endif
static unsigned char *inMemCfeNvramData_buf;
static NVRAM_DATA inMemCfeNvramData;
static UINT32 KerSysgetCrc32(byte *pdata, UINT32 size, UINT32 crc);
static unsigned char *inMemSkyScratchPad_buf;
static SKY_IHR_SP_HEADER inMemSkyScratchPad;
static int erasePSIandSP=0;
static int writeNvramToFlash=0;
#define MAX_MAC_STR_LEN     19

static DEFINE_SPINLOCK(inMemCfeNvramData_spinlock);

#ifdef SKY
static unsigned int sky_GetMiddleBoundaryBlock(void);
#endif

/*
 * inMemSerialisationData an in-memory copy of the Serialisation data that is in the flash.
 * similar in purpose to the NVRAM copy
 */
static unsigned char *inMemSerialisationData_buf;
static SERIALISATION_DATA inMemSerialisationData;
static DEFINE_SPINLOCK(inMemSerialisationData_spinlock);
static void updateInMemSerialisationData(const unsigned char *data, int len, int offset);

/*
 * inMemPublicKey an in-memory copy of the Public Key data that is in the flash.
 * similar in purpose to the NVRAM copy
 */
static unsigned char *inMemPublicKey_buf;
static unsigned char inMemPublicKey[PUBLIC_KEY_LENGTH];


int kerSysGetFlashSectorSize( unsigned short sector );
void KerSysErasePSIAndSP(void);
void updateRestoreDefaultFlag(char  value);
void kerSysCfeNvRamGet(char *string, int strLen, int offset);
int kerSysCfeNvRamSet(const char *string, int strLen, int offset);
void KeySysOverRideNvramData(void);
static int parsexdigit(char str);
static int parsehwaddr(char *str,uint8_t *hwaddr);
void KerSysCloneCfeNVRAMData(void);
int kerSysSerialisationSet(const char *string, int strLen, int offset);
void kerSysSerialisationGet(char *string, int strLen, int offset);
void kerSysPublicKeyGet(char *string, int strLen, int offset);
void updateInMemSerialisationData(const unsigned char *data, int len, int offset);
int KerSysEraseAuxfs(void);
#endif /* SKY */


#ifdef SKY
static unsigned int sky_GetMiddleBoundaryBlock() {
	unsigned char *pBase = flash_get_memptr(0);
	return flash_get_blk((int)(pBase + ((flash_get_total_size() - flash_get_reserved_bytes_auxfs()) / 2)));
}
#endif

static void *retriedKmalloc(size_t size)
{
    void *pBuf;
    unsigned char *bufp8 ;

    size += 4 ; /* 4 bytes are used to store the housekeeping information used for freeing */

    // Memory allocation changed from kmalloc() to vmalloc() as the latter is not susceptible to memory fragmentation under low memory conditions
    // We have modified Linux VM to search all pages by default, it is no longer necessary to retry here
    if (!in_interrupt() ) {
        pBuf = vmalloc(size);
        if (pBuf) {
            memset(pBuf, 0, size);
            bufp8 = (unsigned char *) pBuf ;
            *bufp8 = ALLOC_TYPE_VMALLOC ;
            pBuf = bufp8 + 4 ;
        }
    }
    else { // kmalloc is still needed if in interrupt
        printk("retriedKmalloc: someone calling from intrrupt context?!");
        BUG();
        pBuf = kmalloc(size, GFP_ATOMIC);
        if (pBuf) {
            memset(pBuf, 0, size);
            bufp8 = (unsigned char *) pBuf ;
            *bufp8 = ALLOC_TYPE_KMALLOC ;
            pBuf = bufp8 + 4 ;
        }
    }

    return pBuf;
}

void retriedKfree(void *pBuf)
{
    unsigned char *bufp8  = (unsigned char *) pBuf ;
    bufp8 -= 4 ;

    if (*bufp8 == ALLOC_TYPE_KMALLOC)
        kfree(bufp8);
    else
        vfree(bufp8);
}

// get shared blks into *** pTempBuf *** which has to be released bye the caller!
// return: if pTempBuf != NULL, poits to the data with the dataSize of the buffer
// !NULL -- ok
// NULL  -- fail
char *getSharedBlks(int start_blk, int num_blks)
{
    int i = 0;
    int usedBlkSize = 0;
    int sect_size = 0;
    char *pTempBuf = NULL;
    char *pBuf = NULL;

    down(&semflash);

    for (i = start_blk; i < (start_blk + num_blks); i++)
        usedBlkSize += flash_get_sector_size((unsigned short) i);

    if ((pTempBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
        printk("failed to allocate memory with size: %d\n", usedBlkSize);
        up(&semflash);
        return pTempBuf;
    }
    
    pBuf = pTempBuf;
    for (i = start_blk; i < (start_blk + num_blks); i++)
    {
        sect_size = flash_get_sector_size((unsigned short) i);

#if defined(DEBUG_FLASH)
        printk("getSharedBlks: blk=%d, sect_size=%d\n", i, sect_size);
#endif
        flash_read_buf((unsigned short)i, 0, pBuf, sect_size);
        pBuf += sect_size;
    }
    up(&semflash);
    
    return pTempBuf;
}

// Set the pTempBuf to flash from start_blk for num_blks
// return:
// 0 -- ok
// -1 -- fail
int setSharedBlks(int start_blk, int num_blks, char *pTempBuf)
{
    int i = 0;
    int sect_size = 0;
    int sts = 0;
    char *pBuf = pTempBuf;

    down(&semflash);

    for (i = start_blk; i < (start_blk + num_blks); i++)
    {
        sect_size = flash_get_sector_size((unsigned short) i);
        flash_sector_erase_int(i);

        if (flash_write_buf(i, 0, pBuf, sect_size) != sect_size)
        {
            printk("Error writing flash sector %d.", i);
            sts = -1;
            break;
        }

#if defined(DEBUG_FLASH)
        printk("setShareBlks: blk=%d, sect_size=%d\n", i, sect_size);
#endif

        pBuf += sect_size;
    }

    up(&semflash);

    return sts;
}

#if !defined(CONFIG_BRCM_IKOS)
// Initialize the flash and fill out the fInfo structure
void kerSysEarlyFlashInit( void )
{
#ifdef CONFIG_BCM_ASSERTS
    // ASSERTS and bug() may be too unfriendly this early in the bootup
    // sequence, so just check manually
    if (sizeof(NVRAM_DATA) != NVRAM_LENGTH)
        printk("kerSysEarlyFlashInit: nvram size mismatch! "
               "NVRAM_LENGTH=%d sizeof(NVRAM_DATA)=%d\n",
               NVRAM_LENGTH, sizeof(NVRAM_DATA));
#endif
    inMemNvramData_buf = (unsigned char *) &inMemNvramData;
    memset(inMemNvramData_buf, UNINITIALIZED_FLASH_DATA_CHAR, NVRAM_LENGTH);
#ifdef SKY	
    inMemSerialisationData_buf = (unsigned char *) &inMemSerialisationData;
    memset(inMemSerialisationData_buf, UNINITIALIZED_FLASH_DATA_CHAR, sizeof(SERIALISATION_DATA));
	inMemPublicKey_buf = (unsigned char *) &inMemPublicKey;
	memset(inMemPublicKey_buf, UNINITIALIZED_FLASH_DATA_CHAR, PUBLIC_KEY_LENGTH);
#endif /* SKY */
	
    flash_init();

    if (flash_get_flash_type() == FLASH_IFC_NAND)
        bootFromNand = 1;
    else
        bootFromNand = 0;

#if defined(CONFIG_BCM968500)
    bootFromNand = 1;
    kerSysFlashInit();
#endif

#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM96328) || defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96838)
    if( bootFromNand == 1 )
    {
        unsigned int bootCfgSave =  NAND->NandNandBootConfig;
        NAND->NandNandBootConfig = NBC_AUTO_DEV_ID_CFG | 0x101;
        NAND->NandCsNandXor = 1;

        memcpy((unsigned char *)&bootCfeVersion, (unsigned char *)
            FLASH_BASE + CFE_VERSION_OFFSET, sizeof(bootCfeVersion));
        memcpy(inMemNvramData_buf, (unsigned char *)
            FLASH_BASE + NVRAM_DATA_OFFSET, sizeof(NVRAM_DATA));

        NAND->NandNandBootConfig = bootCfgSave;
        NAND->NandCsNandXor = 0;
    }
    else
#elif defined(CONFIG_BCM968500)
    if( bootFromNand == 1 )
    {
        unsigned long save, val = 0;
        MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_DTE cfg;

        BL_WRITE_32(CE_MIPS_BLOCKS_VPB_BRIDGE_ADDRESS + 0x1c8, val);
        BL_MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_READ(cfg);
        cfg.nfmc_bypass_en = 0;
        cfg.nfmc_bypass_boot_strap = 0;
        BL_MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_WRITE(cfg);
        BL_MIPS_BLOCKS_VPB_NAND_FLASH_ACCESS_CONTROL_READ(save);
        BL_MIPS_BLOCKS_VPB_NAND_FLASH_ACCESS_CONTROL_WRITE(val);

        memcpy((unsigned char *)&bootCfeVersion, (unsigned char *)
            FLASH_BASE + CFE_VERSION_OFFSET, sizeof(bootCfeVersion));
        memcpy(inMemNvramData_buf, (unsigned char *)
            FLASH_BASE + NVRAM_DATA_OFFSET, sizeof(NVRAM_DATA));

        cfg.nfmc_bypass_en = 1;
        cfg.nfmc_bypass_boot_strap = 1;
        BL_MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_WRITE(cfg);
        BL_MIPS_BLOCKS_VPB_NAND_FLASH_ACCESS_CONTROL_WRITE(save);
    }
    else
#endif
    {
#ifndef SKY		
        fInfo.flash_rootfs_start_offset = flash_get_sector_size(0);
#if defined (CONFIG_BCM96838)
        if( fInfo.flash_rootfs_start_offset < PSRAM_SIZE )
            fInfo.flash_rootfs_start_offset = PSRAM_SIZE;
#else
        if( fInfo.flash_rootfs_start_offset < FLASH_LENGTH_BOOT_ROM )
            fInfo.flash_rootfs_start_offset = FLASH_LENGTH_BOOT_ROM;
#endif

        flash_read_buf (NVRAM_SECTOR, CFE_VERSION_OFFSET,
            (unsigned char *)&bootCfeVersion, sizeof(bootCfeVersion));

        /* Read the flash contents into NVRAM buffer */
        flash_read_buf (NVRAM_SECTOR, NVRAM_DATA_OFFSET,
                        inMemNvramData_buf, sizeof (NVRAM_DATA)) ;
#else /* SKY */
		/* RootFS is now in sector 2 */
#if (CONFIG_FLASH_CHIP_SIZE==8)
		fInfo.flash_rootfs_start_offset = flash_get_sector_size(0) + flash_get_sector_size(1);
#elif (CONFIG_FLASH_CHIP_SIZE==16)
		fInfo.flash_rootfs_start_offset = flash_get_sector_size(0) + flash_get_sector_size(1) + flash_get_sector_size(2) + flash_get_sector_size(3);
#elif (CONFIG_FLASH_CHIP_SIZE==32)
		fInfo.flash_rootfs_start_offset = 
			flash_get_sector_size(0) + 
			flash_get_sector_size(1) + 
			flash_get_sector_size(2) + 
			flash_get_sector_size(3) + 
			flash_get_sector_size(4) + 
			flash_get_sector_size(5) + 
			flash_get_sector_size(6) + 
			flash_get_sector_size(7);
#else		
#error "Flash size not supported yet."
#endif
    
        flash_read_buf (CFE_SECTOR, CFE_VERSION_OFFSET,
            (unsigned char *)&bootCfeVersion, sizeof(bootCfeVersion));

        /* Read the flash contents into NVRAM buffer */
        flash_read_buf (NVRAM_SECTOR, NVRAM_DATA_OFFSET,
                        inMemNvramData_buf, sizeof (NVRAM_DATA)) ;

        /* Read the flash contents into Serialisation buffer */
        flash_read_buf (SERIALISATION_SECTOR, SERIALISATION_DATA_OFFSET,
                        inMemSerialisationData_buf, sizeof (SERIALISATION_DATA)) ;
                       
        /* Read the flash contents into Public Key buffer */
        flash_read_buf (PUBLIC_KEY_SECTOR, PUBLIC_KEY_DATA_OFFSET,
					    inMemPublicKey_buf,	PUBLIC_KEY_LENGTH);                      

		KerSysCloneCfeNVRAMData();	
#endif /* SKY */
    }


#ifndef SKY
    printk("reading nvram into inMemNvramData\n");
    printk("ulPsiSize 0x%x\n", (unsigned int)inMemNvramData.ulPsiSize);
    printk("backupPsi 0x%x\n", (unsigned int)inMemNvramData.backupPsi);
    printk("ulSyslogSize 0x%x\n", (unsigned int)inMemNvramData.ulSyslogSize);
#else /* SKY */
    printk("macaddr - %x:%x:%x:%x:%x:%x \n",inMemNvramData.ucaBaseMacAddr[0], inMemNvramData.ucaBaseMacAddr[1],
    	inMemNvramData.ucaBaseMacAddr[2],inMemNvramData.ucaBaseMacAddr[3],inMemNvramData.ucaBaseMacAddr[4],
	    inMemNvramData.ucaBaseMacAddr[5]);

    /*
	printk("CFE macaddr - %x:%x:%x:%x:%x:%x \n",inMemCfeNvramData.ucaBaseMacAddr[0], inMemCfeNvramData.ucaBaseMacAddr[1],
	    inMemCfeNvramData.ucaBaseMacAddr[2],inMemCfeNvramData.ucaBaseMacAddr[3],inMemCfeNvramData.ucaBaseMacAddr[4],
	    inMemCfeNvramData.ucaBaseMacAddr[5]);
	 */
	
    printk("bootline = %s \n",inMemNvramData.szBootline); 
    printk("szBoardId 0x%x\n", (unsigned int)inMemNvramData.szBoardId);
    printk("szVoiceBoardId 0x%x\n", (unsigned int)inMemNvramData.szVoiceBoardId);
    printk("ulNumMacAddrs 0x%x\n", (unsigned int)inMemNvramData.ulNumMacAddrs);
    printk("ulPsiSize 0x%x\n", (unsigned int)inMemNvramData.ulPsiSize);
    printk("ulPsiSize 0x%x\n", (unsigned int)inMemNvramData.ulPsiSize);
    printk("backupPsi 0x%x\n", (unsigned int)inMemNvramData.backupPsi);
    printk("ulSyslogSize 0x%x\n", (unsigned int)inMemNvramData.ulSyslogSize);
#endif /* SKY */	


    if ((BpSetBoardId(inMemNvramData.szBoardId) != BP_SUCCESS))
    {
        // 19/11/12 - MRB - ROUTER-2270 (production rev. 2479)- Workaround for
        // subsequent hang if NVRAM is not initialized with readable board ID -
        // recover pre-emptively by rebooting the router!
        printk("\n*** Board is not initialized properly ***\n\n*** REBOOTING !!! ***\n\n");
        kerSysMipsSoftReset();
        
    }
#if defined (CONFIG_BCM_ENDPOINT_MODULE)
    if ((BpSetVoiceBoardId(inMemNvramData.szVoiceBoardId) != BP_SUCCESS))
        printk("\n*** Voice Board id is not initialized properly ***\n\n");

    if(BpGetVoiceDectType(inMemNvramData.szBoardId) != BP_VOICE_NO_DECT) 
     {
       BpSetDectPopulatedData((int)(inMemNvramData.ulBoardStuffOption & VOICE_OPTION_DECT_MASK));
     }
#endif

}


#ifdef CONFIG_SKY_REBOOT_DIAGNOSTICS

void kerSysSkyDiagnosticsGet(SKY_DIAGNOSTICS_DATA *sd) {
	size_t s = 0;
	memset(sd, 0x00, sizeof(SKY_DIAGNOSTICS_DATA));
	mutex_lock(&flashImageMutex);
	s = flash_read_buf(SKY_DIAGNOSTICS_SECTOR,
			SKY_DIAGNOSTICS_OFFSET,
			(unsigned char *)sd,
			sizeof(SKY_DIAGNOSTICS_DATA));
	mutex_unlock(&flashImageMutex);
}


int kerSysSkyDiagnosticsFieldClear(off_t offset, size_t length) {
	char *pBuf = NULL;

	mutex_lock(&flashImageMutex);
	if ((pBuf = getSharedBlks(SKY_DIAGNOSTICS_SECTOR, 1)) == NULL) return 0;

	memset(pBuf + SKY_DIAGNOSTICS_OFFSET + offset, 0xff, length);
	setSharedBlks(SKY_DIAGNOSTICS_SECTOR, 1, pBuf);

	retriedKfree(pBuf);
	mutex_unlock(&flashImageMutex);

	return 0;
}


void kerSysSkyDiagnosticsFieldSet(off_t offset, size_t s, void *value) {
	mutex_lock(&flashImageMutex);
	if (flash_write_buf(SKY_DIAGNOSTICS_SECTOR,
				SKY_DIAGNOSTICS_OFFSET + offset, value, s) != s) {
		printk(KERN_EMERG "\nSKY DIAGNOSTICS RECORD WRITE FAILED\n");
	}
	mutex_unlock(&flashImageMutex);
}


void kerSysSkyDiagnosticsDyingGaspWrite(void *data, size_t len) {
    /*
	 * Using a dedicated flash memory cells to store this information
	 * since, ScrachPad routines are too complex and require too much
	 * time and energy in order to operate.
     */
	const size_t maxlen = sizeof(((SKY_DIAGNOSTICS_DATA *)0x00)->dying_gasp);
	if (len > maxlen) len = maxlen;
	kerSysSkyDiagnosticsFieldSet(offsetof(SKY_DIAGNOSTICS_DATA, dying_gasp), len, data);
}


void kerSysSkyDiagnosticsKpWrite() {
	const size_t s = sizeof(((SKY_DIAGNOSTICS_DATA *)0x00)->kernel_panic);
	int data = 0xdeadbeef;
	kerSysSkyDiagnosticsFieldSet(offsetof(SKY_DIAGNOSTICS_DATA, kernel_panic), s, &data);
}


void kerSysSkyDiagnosticsKdieWrite() {
	const size_t s = sizeof(((SKY_DIAGNOSTICS_DATA *)0x00)->kernel_die);
	int data = 0xdeadbeef;
	kerSysSkyDiagnosticsFieldSet(offsetof(SKY_DIAGNOSTICS_DATA, kernel_die), s, &data);
}

#endif


/***********************************************************************
 * Function Name: kerSysCfeVersionGet
 * Description  : Get CFE Version.
 * Returns      : 1 -- ok, 0 -- fail
 ***********************************************************************/
int kerSysCfeVersionGet(char *string, int stringLength)
{
    memcpy(string, (unsigned char *)&bootCfeVersion, stringLength);
    return(0);
}

/****************************************************************************
 * NVRAM functions
 * NVRAM data could share a sector with the CFE.  So a write to NVRAM
 * data is actually a read/modify/write operation on a sector.  Protected
 * by a higher level mutex, flashImageMutex.
 * Nvram data is cached in memory in a variable called inMemNvramData, so
 * writes will update this variable and reads just read from this variable.
 ****************************************************************************/


/** set nvram data
 * Must be called with flashImageMutex held
 *
 * @return 0 on success, -1 on failure.
 */
int kerSysNvRamSet(const char *string, int strLen, int offset)
{
    int sts = -1;  // initialize to failure
    char *pBuf = NULL;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);
    BCM_ASSERT_R(offset+strLen <= NVRAM_LENGTH, sts);

    if (bootFromNand == 0)
    {
        if ((pBuf = getSharedBlks(NVRAM_SECTOR, 1)) == NULL)
            return sts;

        // set string to the memory buffer
        memcpy((pBuf + NVRAM_DATA_OFFSET + offset), string, strLen);

        sts = setSharedBlks(NVRAM_SECTOR, 1, pBuf);

        retriedKfree(pBuf);       
    }
    else
    {
        sts = nandNvramSet(string);
    }
    
    if (0 == sts)
    {
        // write to flash was OK, now update in-memory copy
        updateInMemNvramData((unsigned char *) string, strLen, offset);
    }

    return sts;
}


/** get nvram data
 *
 * since it reads from in-memory copy of the nvram data, always successful.
 */
void kerSysNvRamGet(char *string, int strLen, int offset)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    memcpy(string, inMemNvramData_buf + offset, strLen);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);

    return;
}


/** Erase entire nvram area.
 *
 * Currently there are no callers of this function.  THe return value is
 * the opposite of kerSysNvramSet.  Kept this way for compatibility.
 *
 * @return 0 on failure, 1 on success.
 */
int kerSysEraseNvRam(void)
{
    int sts = 1;

    BCM_ASSERT_NOT_HAS_MUTEX_C(&flashImageMutex);

    if (bootFromNand == 0)
    {
        char *tempStorage;
        if (NULL == (tempStorage = kmalloc(NVRAM_LENGTH, GFP_KERNEL)))
        {
            sts = 0;
        }
        else
        {
            // just write the whole buf with '0xff' to the flash
            memset(tempStorage, UNINITIALIZED_FLASH_DATA_CHAR, NVRAM_LENGTH);
            mutex_lock(&flashImageMutex);
            if (kerSysNvRamSet(tempStorage, NVRAM_LENGTH, 0) != 0)
                sts = 0;
            mutex_unlock(&flashImageMutex);
            kfree(tempStorage);
        }
    }
    else
    {
        printk("kerSysEraseNvram: not supported when bootFromNand == 1\n");
        sts = 0;
    }

    return sts;
}

#else // CONFIG_BRCM_IKOS
static NVRAM_DATA ikos_nvram_data =
    {
    NVRAM_VERSION_NUMBER,
    "",
    "ikos",
    0,
    DEFAULT_PSI_SIZE,
    11,
    {0x02, 0x10, 0x18, 0x01, 0x00, 0x01},
    0x00, 0x00,
    0x720c9f60
    };

void kerSysEarlyFlashInit( void )
{
    inMemNvramData_buf = (unsigned char *) &inMemNvramData;
    memset(inMemNvramData_buf, UNINITIALIZED_FLASH_DATA_CHAR, NVRAM_LENGTH);

    memcpy(inMemNvramData_buf, (unsigned char *)&ikos_nvram_data,
        sizeof (NVRAM_DATA));
    fInfo.flash_scratch_pad_length = 0;
    fInfo.flash_persistent_start_blk = 0;
}

int kerSysCfeVersionGet(char *string, int stringLength)
{
    *string = '\0';
    return(0);
}


void kerSysNvRamGet(char *string, int strLen, int offset)
{
    memcpy(string, (unsigned char *) &ikos_nvram_data, sizeof(NVRAM_DATA));
}

int kerSysNvRamSet(const char *string, int strLen, int offset)
{
    if( bootFromNand )
        nandNvramSet(string);
    return(0);
}

int kerSysEraseNvRam(void)
{
    return(0);
}

unsigned long kerSysReadFromFlash( void *toaddr, unsigned long fromaddr,
    unsigned long len )
{
    return((unsigned long) memcpy((unsigned char *) toaddr, (unsigned char *) fromaddr, len));
}
#endif  // CONFIG_BRCM_IKOS

/** Update the in-Memory copy of the nvram with the given data.
 *
 * @data: pointer to new nvram data
 * @len: number of valid bytes in nvram data
 * @offset: offset of the given data in the nvram data
 */
void updateInMemNvramData(const unsigned char *data, int len, int offset)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    memcpy(inMemNvramData_buf + offset, data, len);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
}


/** Get the bootline string from the NVRAM data.
 * Assumes the caller has the inMemNvramData locked.
 * Special case: this is called from prom.c without acquiring the
 * spinlock.  It is too early in the bootup sequence for spinlocks.
 *
 * @param bootline (OUT) a buffer of NVRAM_BOOTLINE_LEN bytes for the result
 */
void kerSysNvRamGetBootlineLocked(char *bootline)
{
    memcpy(bootline, inMemNvramData.szBootline,
                     sizeof(inMemNvramData.szBootline));
}
EXPORT_SYMBOL(kerSysNvRamGetBootlineLocked);
#ifdef SKY
EXPORT_SYMBOL(kerSysSerialisationGet);
#endif


/** Get the bootline string from the NVRAM data.
 *
 * @param bootline (OUT) a buffer of NVRAM_BOOTLINE_LEN bytes for the result
 */
void kerSysNvRamGetBootline(char *bootline)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    kerSysNvRamGetBootlineLocked(bootline);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
}
EXPORT_SYMBOL(kerSysNvRamGetBootline);


/** Get the BoardId string from the NVRAM data.
 * Assumes the caller has the inMemNvramData locked.
 * Special case: this is called from prom_init without acquiring the
 * spinlock.  It is too early in the bootup sequence for spinlocks.
 *
 * @param boardId (OUT) a buffer of NVRAM_BOARD_ID_STRING_LEN
 */
void kerSysNvRamGetBoardIdLocked(char *boardId)
{
    memcpy(boardId, inMemNvramData.szBoardId,
                    sizeof(inMemNvramData.szBoardId));
}
EXPORT_SYMBOL(kerSysNvRamGetBoardIdLocked);


/** Get the BoardId string from the NVRAM data.
 *
 * @param boardId (OUT) a buffer of NVRAM_BOARD_ID_STRING_LEN
 */
void kerSysNvRamGetBoardId(char *boardId)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    kerSysNvRamGetBoardIdLocked(boardId);
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
}
EXPORT_SYMBOL(kerSysNvRamGetBoardId);


/** Get the base mac addr from the NVRAM data.  This is 6 bytes, not
 * a string.
 *
 * @param baseMacAddr (OUT) a buffer of NVRAM_MAC_ADDRESS_LEN
 */
void kerSysNvRamGetBaseMacAddr(unsigned char *baseMacAddr)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemNvramData_spinlock, flags);
    memcpy(baseMacAddr, inMemNvramData.ucaBaseMacAddr,
                        sizeof(inMemNvramData.ucaBaseMacAddr));
    spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);
}
EXPORT_SYMBOL(kerSysNvRamGetBaseMacAddr);


/** Get the nvram version from the NVRAM data.
 *
 * @return nvram version number.
 */
unsigned long kerSysNvRamGetVersion(void)
{
    return (inMemNvramData.ulVersion);
}
EXPORT_SYMBOL(kerSysNvRamGetVersion);


void kerSysFlashInit( void )
{
#ifndef SKY
    sema_init(&semflash, 1);

    // too early in bootup sequence to acquire spinlock, not needed anyways
    // only the kernel is running at this point
    flash_init_info(&inMemNvramData, &fInfo);
#else /* SKY */
		char *pBuf = NULL;

    sema_init(&semflash, 1);
	
  //	if(inMemSkyScratchPad.restoreDefault == 1)
  	if(writeNvramToFlash)
	{	
		printk("kerSysFlashInit:: Writing New NVRAM contents to Flash  \n");
		printk("SKY:kerSysFlashInit NVRAM_SECTOR=%d,NVRAM_DATA_OFFSET=%d,NVRAM_SIZE=%d\n",NVRAM_SECTOR,NVRAM_DATA_OFFSET,sizeof(NVRAM_DATA));
		if ((pBuf = getSharedBlks(NVRAM_SECTOR, 1)) == NULL)
		{
			printk("SOMETHING TERRIBALY WRONG, NOT ABLE TO READ NVRAM FROM FLASH \n");
			writeNvramToFlash = 0;
		    return ;
		}
	
		 // set string to the memory buffer
		 memcpy((pBuf + NVRAM_DATA_OFFSET), (char *)&inMemNvramData, sizeof(NVRAM_DATA));
		 if(setSharedBlks(NVRAM_SECTOR, 1, pBuf) != 0)
		 	printk(" Cannot Store new NVRAM contents to Flash  \n");
	 
		 retriedKfree(pBuf);
		 writeNvramToFlash = 0;
	
	}
	 

    // too early in bootup sequence to acquire spinlock, not needed anyways
    // only the kernel is running at this point
    flash_init_info(&inMemNvramData, &fInfo);

  
	if(erasePSIandSP)
	{
		printk("PSI and Scratchpad will also be erased as the NVRAM is reinitialised \n");
		KerSysErasePSIAndSP();
		erasePSIandSP = 0;
		
	}
	if(inMemSkyScratchPad.restoreDefault == 1)
	{
		updateRestoreDefaultFlag(0);
		inMemSkyScratchPad.restoreDefault = 0;
        KerSysEraseAuxfs();
	}
	
#endif /* SKY */
}

/***********************************************************************
 * Function Name: kerSysFlashAddrInfoGet
 * Description  : Fills in a structure with information about the NVRAM
 *                and persistent storage sections of flash memory.  
 *                Fro physmap.c to mount the fs vol.
 * Returns      : None.
 ***********************************************************************/
void kerSysFlashAddrInfoGet(PFLASH_ADDR_INFO pflash_addr_info)
{
    memcpy(pflash_addr_info, &fInfo, sizeof(FLASH_ADDR_INFO));
}

/*******************************************************************************
 * PSI functions
 * PSI is where we store the config file.  There is also a "backup" PSI
 * that stores an extra copy of the PSI.  THe idea is if the power goes out
 * while we are writing the primary PSI, the backup PSI will still have
 * a good copy from the last write.  No additional locking is required at
 * this level.
 *******************************************************************************/
#define PSI_FILE_NAME           "/data/psi"
#define PSI_BACKUP_FILE_NAME    "/data/psibackup"
#define SCRATCH_PAD_FILE_NAME   "/data/scratchpad"


// get psi data
// return:
//  0 - ok
//  -1 - fail
int kerSysPersistentGet(char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read PSI from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;
        int len;

        memset(string, 0x00, strLen);
        fp = filp_open(PSI_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((len = (int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos)) <= 0)
                printk("Failed to read psi from '%s'\n", PSI_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if ((pBuf = getSharedBlks(fInfo.flash_persistent_start_blk,
        fInfo.flash_persistent_number_blk)) == NULL)
        return -1;

    // get string off the memory buffer
    memcpy(string, (pBuf + fInfo.flash_persistent_blk_offset + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}

int kerSysBackupPsiGet(char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read backup PSI from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;
        int len;

        memset(string, 0x00, strLen);
        fp = filp_open(PSI_BACKUP_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((len = (int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos)) <= 0)
                printk("Failed to read psi from '%s'\n", PSI_BACKUP_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if (fInfo.flash_backup_psi_number_blk <= 0)
    {
        printk("No backup psi blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    if ((pBuf = getSharedBlks(fInfo.flash_backup_psi_start_blk,
                              fInfo.flash_backup_psi_number_blk)) == NULL)
        return -1;

    // get string off the memory buffer
    memcpy(string, (pBuf + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}

int kerSysFsFileSet(const char *filename, char *buf, int len)
{
    int ret = -1;
    struct file *fp;
    mm_segment_t fs;

    fs = get_fs();
    set_fs(get_ds());

    fp = filp_open(filename, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);

    if (!IS_ERR(fp))
    {
        if (fp->f_op && fp->f_op->write)
        {
            fp->f_pos = 0;

            if((int) fp->f_op->write(fp, (void *) buf, len, &fp->f_pos) == len)
            {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)                
                vfs_fsync(fp, 0);
#else
                vfs_fsync(fp, fp->f_path.dentry, 0);
#endif
                ret = 0;
            }
        }

        filp_close(fp, NULL);
    }

    set_fs(fs);

    if (ret != 0)
    {
        printk("Failed to write to '%s'.\n", filename);
    }

    return( ret );
}

// set psi 
// return:
//  0 - ok
//  -1 - fail
int kerSysPersistentSet(char *string, int strLen, int offset)
{
    int sts = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write PSI to
         * a file on the NAND flash.
         */
        return kerSysFsFileSet(PSI_FILE_NAME, string, strLen);
    }

    if ((pBuf = getSharedBlks(fInfo.flash_persistent_start_blk,
        fInfo.flash_persistent_number_blk)) == NULL)
        return -1;

    // set string to the memory buffer
    memcpy((pBuf + fInfo.flash_persistent_blk_offset + offset), string, strLen);

    if (setSharedBlks(fInfo.flash_persistent_start_blk, 
        fInfo.flash_persistent_number_blk, pBuf) != 0)
        sts = -1;
    
    retriedKfree(pBuf);

    return sts;
}

int kerSysBackupPsiSet(char *string, int strLen, int offset)
{
    int i;
    int sts = 0;
    int usedBlkSize = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write backup PSI to
         * a file on the NAND flash.
         */
        return kerSysFsFileSet(PSI_BACKUP_FILE_NAME, string, strLen);
    }

    if (fInfo.flash_backup_psi_number_blk <= 0)
    {
        printk("No backup psi blks allocated, change it in CFE\n");
        return -1;
    }

    if (fInfo.flash_persistent_start_blk == 0)
        return -1;

    /*
     * The backup PSI does not share its blocks with anybody else, so I don't have
     * to read the flash first.  But now I have to make sure I allocate a buffer
     * big enough to cover all blocks that the backup PSI spans.
     */
    for (i=fInfo.flash_backup_psi_start_blk;
         i < (fInfo.flash_backup_psi_start_blk + fInfo.flash_backup_psi_number_blk); i++)
    {
       usedBlkSize += flash_get_sector_size((unsigned short) i);
    }

    if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
       printk("failed to allocate memory with size: %d\n", usedBlkSize);
       return -1;
    }

    memset(pBuf, 0, usedBlkSize);

    // set string to the memory buffer
    memcpy((pBuf + offset), string, strLen);

    if (setSharedBlks(fInfo.flash_backup_psi_start_blk, fInfo.flash_backup_psi_number_blk, 
                      pBuf) != 0)
        sts = -1;
    
    retriedKfree(pBuf);

    return sts;
}


/*******************************************************************************
 * "Kernel Syslog" is one or more sectors allocated in the flash
 * so that we can persist crash dump or other system diagnostics info
 * across reboots.  This feature is current not implemented.
 *******************************************************************************/

#define SYSLOG_FILE_NAME        "/etc/syslog"

int kerSysSyslogGet(char *string, int strLen, int offset)
{
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Read syslog from
         * a file on the NAND flash.
         */
        struct file *fp;
        mm_segment_t fs;

        memset(string, 0x00, strLen);
        fp = filp_open(SYSLOG_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((int) fp->f_op->read(fp, (void *) string, strLen,
               &fp->f_pos) <= 0)
                printk("Failed to read psi from '%s'\n", SYSLOG_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }

        return 0;
    }

    if (fInfo.flash_syslog_number_blk <= 0)
    {
        printk("No syslog blks allocated, change it in CFE\n");
        return -1;
    }
    
    if (strLen > fInfo.flash_syslog_length)
        return -1;

    if ((pBuf = getSharedBlks(fInfo.flash_syslog_start_blk,
                              fInfo.flash_syslog_number_blk)) == NULL)
        return -1;

    // get string off the memory buffer
    memcpy(string, (pBuf + offset), strLen);

    retriedKfree(pBuf);

    return 0;
}

int kerSysSyslogSet(char *string, int strLen, int offset)
{
    int i;
    int sts = 0;
    int usedBlkSize = 0;
    char *pBuf = NULL;

    if( bootFromNand )
    {
        /* Root file system is on a writable NAND flash.  Write PSI to
         * a file on the NAND flash.
         */
        return kerSysFsFileSet(PSI_FILE_NAME, string, strLen);
    }

    if (fInfo.flash_syslog_number_blk <= 0)
    {
        printk("No syslog blks allocated, change it in CFE\n");
        return -1;
    }
    
    if (strLen > fInfo.flash_syslog_length)
        return -1;

    /*
     * The syslog does not share its blocks with anybody else, so I don't have
     * to read the flash first.  But now I have to make sure I allocate a buffer
     * big enough to cover all blocks that the syslog spans.
     */
    for (i=fInfo.flash_syslog_start_blk;
         i < (fInfo.flash_syslog_start_blk + fInfo.flash_syslog_number_blk); i++)
    {
        usedBlkSize += flash_get_sector_size((unsigned short) i);
    }

    if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
       printk("failed to allocate memory with size: %d\n", usedBlkSize);
       return -1;
    }

    memset(pBuf, 0, usedBlkSize);

    // set string to the memory buffer
    memcpy((pBuf + offset), string, strLen);

    if (setSharedBlks(fInfo.flash_syslog_start_blk, fInfo.flash_syslog_number_blk, pBuf) != 0)
        sts = -1;

    retriedKfree(pBuf);

    return sts;
}


/*******************************************************************************
 * Writing software image to flash operations
 * This procedure should be serialized.  Look for flashImageMutex.
 *******************************************************************************/


#define je16_to_cpu(x) ((x).v16)
#define je32_to_cpu(x) ((x).v32)

/*
 * nandUpdateSeqNum
 * 
 * Read the sequence number from each rootfs partition.  The sequence number is
 * the extension on the cferam file.  Add one to the highest sequence number
 * and change the extenstion of the cferam in the image to be flashed to that
 * number.
 */
static char *nandUpdateSeqNum(unsigned char *imagePtr, int imageSize, int blkLen)
{
    char fname[] = NAND_CFE_RAM_NAME;
    int fname_actual_len = strlen(fname);
    int fname_cmp_len = strlen(fname) - 3; /* last three are digits */
    int seq = -1;
    int seq2 = -1;
    char *ret = NULL;

    seq = kerSysGetSequenceNumber(1);
    seq2 = kerSysGetSequenceNumber(2);
    seq = (seq >= seq2) ? seq : seq2;

    if( seq != -1 )
    {
        unsigned char *buf, *p;
        struct jffs2_raw_dirent *pdir;
        unsigned long version = 0;
        int done = 0;

        if( *(unsigned short *) imagePtr != JFFS2_MAGIC_BITMASK )
        {
            imagePtr += blkLen;
            imageSize -= blkLen;
        }

        /* Increment the new highest sequence number. Add it to the CFE RAM
         * file name.
         */
        seq++;

        /* Search the image and replace the last three characters of file
         * cferam.000 with the new sequence number.
         */
        for(buf = imagePtr; buf < imagePtr+imageSize && done == 0; buf += blkLen)
        {
            p = buf;
            while( p < buf + blkLen )
            {
                pdir = (struct jffs2_raw_dirent *) p;
                if( je16_to_cpu(pdir->magic) == JFFS2_MAGIC_BITMASK )
                {
                    if( je16_to_cpu(pdir->nodetype) == JFFS2_NODETYPE_DIRENT &&
                        fname_actual_len == pdir->nsize &&
                        !memcmp(fname, pdir->name, fname_cmp_len) &&
                        je32_to_cpu(pdir->version) > version &&
                        je32_to_cpu(pdir->ino) != 0 )
                     {
                        /* File cferam.000 found. Change the extension to the
                         * new sequence number and recalculate file name CRC.
                         */
                        p = pdir->name + fname_cmp_len;
                        p[0] = (seq / 100) + '0';
                        p[1] = ((seq % 100) / 10) + '0';
                        p[2] = ((seq % 100) % 10) + '0';
                        p[3] = '\0';

                        je32_to_cpu(pdir->name_crc) =
                            crc32(0, pdir->name, (unsigned int)
                            fname_actual_len);

                        version = je32_to_cpu(pdir->version);

                        /* Setting 'done = 1' assumes there is only one version
                         * of the directory entry.
                         */
                        done = 1;
                        ret = buf;  /* Pointer to the block containing CFERAM directory entry in the image to be flashed */
                        break;
                    }

                    p += (je32_to_cpu(pdir->totlen) + 0x03) & ~0x03;
                }
                else
                {
                    done = 1;
                    break;
                }
            }
        }
    }

    return(ret);
}

/* Erase the specified NAND flash block. */
static int nandEraseBlk( struct mtd_info *mtd, int blk_addr )
{
    struct erase_info erase;
    int sts;

    /* Erase the flash block. */
    memset(&erase, 0x00, sizeof(erase));
    erase.addr = blk_addr;
    erase.len = mtd->erasesize;
    erase.mtd = mtd;

    if( (sts = mtd_block_isbad(mtd, blk_addr)) == 0 )
    {
        sts = mtd_erase(mtd, &erase);

        /* Function local_bh_disable has been called and this
         * is the only operation that should be occurring.
         * Therefore, spin waiting for erase to complete.
         */
        if( 0 == sts )
        {
            int i;

            for(i = 0; i < 10000 && erase.state != MTD_ERASE_DONE &&
                erase.state != MTD_ERASE_FAILED; i++ )
            {
                udelay(100);
            }

            if( erase.state != MTD_ERASE_DONE )
            {
                sts = -1;
                printk("nandEraseBlk - Block 0x%8.8x. Error erase block timeout.\n", blk_addr);
            }
        }
        else
            printk("nandEraseBlk - Block 0x%8.8x. Error erasing block.\n", blk_addr);
    }
    else
        printk("nandEraseBlk - Block 0x%8.8x. Bad block.\n", blk_addr);

    return (sts);
}

/* Write data with or without JFFS2 clean marker, must pass function an aligned block address */
static int nandWriteBlk(struct mtd_info *mtd, int blk_addr, int data_len, char *data_ptr, bool write_JFFS2_clean_marker)
{
	const unsigned short jffs2_clean_marker[] = {JFFS2_MAGIC_BITMASK, JFFS2_NODETYPE_CLEANMARKER, 0x0000, 0x0008};
    struct mtd_oob_ops ops;
    int sts = 0;
    int page_addr, byte;

    for (page_addr = 0; page_addr < data_len; page_addr += mtd->writesize)
    {
        memset(&ops, 0x00, sizeof(ops));

        // check to see if whole page is FFs
        for (byte = 0; (byte < mtd->writesize) && ((page_addr + byte) < data_len); byte += 4)
        {
            if ( *(uint32 *)(data_ptr + page_addr + byte) != 0xFFFFFFFF )
            {
                ops.len = mtd->writesize < (data_len - page_addr) ? mtd->writesize : data_len - page_addr;
                ops.datbuf = data_ptr + page_addr;
                break;
            }
        }

        if (write_JFFS2_clean_marker)
        {
            ops.mode = MTD_OPS_AUTO_OOB;
            ops.oobbuf = (char *)jffs2_clean_marker;
            ops.ooblen = sizeof(jffs2_clean_marker);
        }

        if (ops.len || ops.ooblen)
        {
            if( (sts = mtd_write_oob(mtd, blk_addr + page_addr, &ops)) != 0 )
            {
                printk("nandWriteBlk - Block 0x%8.8x. Error writing page.\n", blk_addr + page_addr);
                nandEraseBlk(mtd, blk_addr);
                mtd_block_markbad(mtd, blk_addr);
                break;
            }
        }
    }

    return(sts);
}


// NAND flash bcm image 
// return: 
// 0 - ok
// !0 - the sector number fail to be flashed (should not be 0)
int kerSysBcmNandImageSet( char *rootfs_part, char *image_ptr, int img_size,
    int should_yield )
{
    int sts = -1;
    int blk_addr;
    int old_img = 0;
    char *cferam_ptr;
    int rsrvd_for_cferam;
    char *end_ptr = image_ptr + img_size;
    struct mtd_info *mtd0 = get_mtd_device_nm("image");
    WFI_TAG wt = {0};

    /* Reserve room to flash block containing directory entry for CFERAM. */
    rsrvd_for_cferam = 8 * mtd0->erasesize;

    if( !IS_ERR_OR_NULL(mtd0) )
    {
        unsigned int chip_id = kerSysGetChipId();
        int blksize = mtd0->erasesize / 1024;

        memcpy(&wt, end_ptr, sizeof(wt));

#if defined(CHIP_FAMILY_ID_HEX)
        chip_id = CHIP_FAMILY_ID_HEX;
#endif

        if( (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
            wt.wfiChipId != chip_id )
        {
            int id_ok = 0;

            if (id_ok == 0) {
                printk("Chip Id error.  Image Chip Id = %04x, Board Chip Id = "
                    "%04x.\n", wt.wfiChipId, chip_id);
                put_mtd_device(mtd0);
                return -1;
            }
        }
        else if( wt.wfiFlashType == WFI_NOR_FLASH )
        {
            printk("\nERROR: Image does not support a NAND flash device.\n\n");
            put_mtd_device(mtd0);
            return -1;
        }
        else if( (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
            ((blksize == 16 && wt.wfiFlashType != WFI_NAND16_FLASH) ||
             (blksize == 128 && wt.wfiFlashType != WFI_NAND128_FLASH) ||
             (blksize != 16 && blksize !=128) ) )
        {
            printk("\nERROR: NAND flash block size %dKB does not work with an "
                "image built with %dKB block size\n\n", blksize,
                (wt.wfiFlashType == WFI_NAND16_FLASH) ? 16 : 128);
            put_mtd_device(mtd0);
            return -1;
        }
        else
        {
            mtd0 = get_mtd_device_nm(rootfs_part);

            /* If the image version indicates that is uses a 1MB data partition
             * size and the image is intended to be flashed to the second file
             * system partition, change to the flash to the first partition.
             * After new image is flashed, delete the second file system and
             * data partitions (at the bottom of this function).
             */
            if( wt.wfiVersion == WFI_VERSION_NAND_1MB_DATA )
            {
                unsigned long rootfs_ofs;
                kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs);
                
                if(rootfs_ofs == inMemNvramData.ulNandPartOfsKb[NP_ROOTFS_1] &&
                    mtd0)
                {
                    printk("Old image, flashing image to first partition.\n");
                    put_mtd_device(mtd0);
                    mtd0 = NULL;
                    old_img = 1;
                }
            }

            if( IS_ERR_OR_NULL(mtd0) || mtd0->size == 0LL )
            {
                /* Flash device is configured to use only one file system. */
                if( !IS_ERR_OR_NULL(mtd0) )
                    put_mtd_device(mtd0);
                mtd0 = get_mtd_device_nm("image");
            }
        }
    }

    if( !IS_ERR_OR_NULL(mtd0) )
    {
        int ofs;
        unsigned long flags=0;
        int writelen;
        int writing_ubifs;

        if( *(unsigned short *) image_ptr == JFFS2_MAGIC_BITMASK )
        { /* Downloaded image does not contain CFE ROM boot loader */
            ofs = 0;
        }
        else
        {
            /* Downloaded image contains CFE ROM boot loader. */
            PNVRAM_DATA pnd = (PNVRAM_DATA) (image_ptr + NVRAM_DATA_OFFSET);

            ofs = mtd0->erasesize;

            /* Copy NVRAM data to block to be flashed so it is preserved. */
            spin_lock_irqsave(&inMemNvramData_spinlock, flags);
            memcpy((unsigned char *) pnd, inMemNvramData_buf,
                sizeof(NVRAM_DATA));
            spin_unlock_irqrestore(&inMemNvramData_spinlock, flags);

            /* Recalculate the nvramData CRC. */
            pnd->ulCheckSum = 0;
            pnd->ulCheckSum = crc32(CRC32_INIT_VALUE, pnd, sizeof(NVRAM_DATA));
        }

        /* 
         * Scan downloaded image for cferam.000 directory entry and change file externsion 
         * to cfe.YYY where YYY is the current cfe.XXX + 1
         */
        cferam_ptr = nandUpdateSeqNum((unsigned char *) image_ptr, img_size, mtd0->erasesize);

        if( cferam_ptr == NULL )
        {
            printk("\nERROR: Invalid image. cferam.000 not found.\n\n");
            put_mtd_device(mtd0);
            return -1; 
        }

        /*
         * write image to flash memory.
         * In theory, all calls to flash_write_buf() must be done with
         * semflash held, so I added it here.  However, in reality, all
         * flash image writes are protected by flashImageMutex at a higher
         * level.
         */
        down(&semflash);

        // Once we have acquired the flash semaphore, we can
        // disable activity on other processor and also on local processor.
        // Need to disable interrupts so that RCU stall checker will not complain.
        if (!should_yield)
        {
            stopOtherCpu();
            local_irq_save(flags);
        }

        local_bh_disable();

        if( 0 != ofs ) /* Image contains CFE ROM boot loader. */
        {
            /* Flash the CFE ROM boot loader. */
            struct mtd_info *mtd1 = get_mtd_device_nm("nvram");
            if( !IS_ERR_OR_NULL(mtd1) )
            {
#if defined(CONFIG_BCM968500)
                extern int Starter;
                int i;

                /* Erase then write flash blocks, CFEROM replicated 8 times on 68500. */
                Starter = 1;
                for (i = 0; i < 8; i++)
                {
                    if (nandEraseBlk(mtd1, mtd1->erasesize * i) == 0)
                        nandWriteBlk(mtd1, mtd1->erasesize * i, mtd1->erasesize, image_ptr, FALSE); // 68500, write CFEROM without JFFS2 clean markers
                }
                Starter = 0;
#else
                if (nandEraseBlk(mtd1, 0) == 0)
                    nandWriteBlk(mtd1, 0,  mtd1->erasesize, image_ptr,TRUE);
#endif
                put_mtd_device(mtd1);
            }

            image_ptr += ofs;
        }

        /* Erase blocks containing directory entry for CFERAM before flashing the image. */
        for( blk_addr = 0; blk_addr < rsrvd_for_cferam; blk_addr += mtd0->erasesize )
            nandEraseBlk( mtd0, blk_addr );

        /* Flash the image except for CFERAM directory entry. */
        writing_ubifs = 0;
        for( blk_addr = rsrvd_for_cferam; blk_addr < mtd0->size; blk_addr += mtd0->erasesize )
        {
            if (nandEraseBlk( mtd0, blk_addr ) == 0)
            { // block was erased successfully
                if ( image_ptr == cferam_ptr )
                { // skip CFERAM directory entry block
                    image_ptr += mtd0->erasesize;
                }
                else
                { /* Write a block of the image to flash. */
                    if( image_ptr < end_ptr )
                    { // if any data left, prepare to write it out
                        writelen = ((image_ptr + mtd0->erasesize) <= end_ptr)
                            ? mtd0->erasesize : (int) (end_ptr - image_ptr);
                    }
                    else
                        writelen = 0;

                    if (writelen || !writing_ubifs) /* Write data and/or OOB */
                    {
                        /* Write clean markers only to JFFS2 blocks */
                        if (nandWriteBlk(mtd0, blk_addr, writelen, image_ptr, !writing_ubifs) != 0 )
                            printk("Error writing Block 0x%8.8x\n", blk_addr);
                        else
                        { // successful write
                            printk(".");

                            if ( writelen )
                            { // increment counter and check for UBIFS split marker if data was written
                                image_ptr += writelen;

                                if (!strncmp(BCM_BCMFS_TAG, image_ptr - 0x100, strlen(BCM_BCMFS_TAG)))
                                    if (!strncmp(BCM_BCMFS_TYPE_UBIFS, image_ptr - 0x100 + strlen(BCM_BCMFS_TAG), strlen(BCM_BCMFS_TYPE_UBIFS)))
                                    { // check for UBIFS split marker
                                        writing_ubifs = 1;
                                        printk("U");
                                    }
                            }
                        }
                    }

                    if (should_yield)
                    {
                        local_bh_enable();
                        yield();
                        local_bh_disable();
                    }
                }
            }
        }

        /* Flash the block containing directory entry for CFERAM. */
        for( blk_addr = 0; blk_addr < rsrvd_for_cferam; blk_addr += mtd0->erasesize )
        {
            if (mtd_block_isbad(mtd0, blk_addr) == 0)
            {
                /* Write a block of the image to flash. */
                if (nandWriteBlk(mtd0, blk_addr, cferam_ptr ? mtd0->erasesize : 0, cferam_ptr, TRUE) == 0)
                {
                    printk(".");
                    cferam_ptr = NULL;
                }

                if (should_yield)
                {
                    local_bh_enable();
                    yield();
                    local_bh_disable();
                }
            }
        }

        if( cferam_ptr == NULL )
        {
            /* block containing directory entry for CFERAM was written successfully! */
            /* Whole flash image is programmed successfully! */
            sts = 0;
        }

        up(&semflash);

        printk("\n\n");

        if (should_yield)
        {
            local_bh_enable();
        }

        if( sts )
        {
            /*
             * Even though we try to recover here, this is really bad because
             * we have stopped the other CPU and we cannot restart it.  So we
             * really should try hard to make sure flash writes will never fail.
             */
            printk(KERN_ERR "nandWriteBlk: write failed at blk=%d\n", blk_addr);
            sts = (blk_addr > mtd0->erasesize) ? blk_addr / mtd0->erasesize : 1;
            if (!should_yield)
            {
                local_irq_restore(flags);
                local_bh_enable();
            }
        }
    }

    if( !IS_ERR_OR_NULL(mtd0) )
        put_mtd_device(mtd0);

    if( sts == 0 && old_img == 1 )
    {
        printk("\nOld image, deleting data and secondary file system partitions\n");
        mtd0 = get_mtd_device_nm("data");
        for( blk_addr = 0; blk_addr < mtd0->size; blk_addr += mtd0->erasesize )
        { // write JFFS2 clean markers
            if (nandEraseBlk( mtd0, blk_addr ) == 0)
                nandWriteBlk(mtd0, blk_addr, 0, NULL, TRUE);
        }

        mtd0 = get_mtd_device_nm("image_update");
        for( blk_addr = 0; blk_addr < mtd0->size; blk_addr += mtd0->erasesize )
        {
            if (nandEraseBlk( mtd0, blk_addr ) == 0)
                nandWriteBlk(mtd0, blk_addr, 0, NULL, TRUE);
        }
    }

    return sts;
}

    // NAND flash overwrite nvram block.    
    // return: 
    // 0 - ok
    // !0 - the sector number fail to be flashed (should not be 0)
static int nandNvramSet(const char *nvramString )
{
#if !defined(CONFIG_BCM968500)
    /* Image contains CFE ROM boot loader. */
    struct mtd_info *mtd = get_mtd_device_nm("nvram"); 
    char *cferom_ptr = NULL;
    int retlen = 0;
    
    if( IS_ERR_OR_NULL(mtd) )
        return -1;
    
    if ( (cferom_ptr = (char *)retriedKmalloc(mtd->erasesize)) == NULL )
    {
        printk("\n Failed to allocate memory in nandNvramSet();");
        return -1;
    }

    /* Read the whole cfe rom nand block 0 */
    mtd_read(mtd, 0, mtd->erasesize, &retlen, cferom_ptr);

    /* Copy the nvram string into place */
    memcpy(cferom_ptr + NVRAM_DATA_OFFSET, nvramString, sizeof(NVRAM_DATA));
    
    /* Flash the CFE ROM boot loader. */
    if (nandEraseBlk( mtd, 0 ) == 0)
        nandWriteBlk(mtd, 0, mtd->erasesize, cferom_ptr, TRUE);

    retriedKfree(cferom_ptr);
#endif
    return 0;
}

           
#ifdef FLASH_PROCESS_DEBUG
static void dump(char *data, unsigned int len) {

	unsigned int i,l = 0;
	unsigned int total = 0;

	while (len) {
		l = (len < 16) ? len : 16;

		printk("%04x: ", total);

		for (i = 0; i<l; i++) {
			printk("%02x ", (unsigned char)data[total + i]);
		}

		printk("\n");
		total += l;
		len -= l;
	}
}
#endif


// flash bcm image 
// return: 
// 0 - ok
// !0 - the sector number fail to be flashed (should not be 0)
// Must be called with flashImageMutex held.
int kerSysBcmImageSet(unsigned int flash_start_addr, char *string, int size,
    int should_yield)
{
    int sts;
    int sect_size;
    int blk_start;
#ifndef SKY	
    int savedSize = size;
#endif /* SKY */	
    int whole_image = 0;
    unsigned long flags=0;
    int is_cfe_write=0;
    WFI_TAG wt = {0};
#if defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
	int sectorNum = 0;
#endif //defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)

#ifdef SKY
    FLASH_PARTITION_INFO fPartAuxFS;
    int end_blk=0;
	unsigned int middle_blk = 0x00;
#endif //SKY	

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    if (flash_start_addr == FLASH_BASE)
    {
        unsigned int chip_id = kerSysGetChipId();
        whole_image = 1;
        memcpy(&wt, string + size, sizeof(wt));
        if( (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
            wt.wfiChipId != chip_id )
        {
            int id_ok = 0;
#if defined(CHIP_FAMILY_ID_HEX)
            if (wt.wfiChipId == CHIP_FAMILY_ID_HEX) {
                id_ok = 1;
            }
#endif
            if (id_ok == 0) {
                printk("Chip Id error.  Image Chip Id = %04x, Board Chip Id = "
                    "%04x.\n", wt.wfiChipId, chip_id);
                return -1;
            }
        }
    }

    if( whole_image && (wt.wfiVersion & WFI_ANY_VERS_MASK) == WFI_ANY_VERS &&
        wt.wfiFlashType != WFI_NOR_FLASH )
    {
        printk("ERROR: Image does not support a NOR flash device.\n");
        return -1;
    }

    // this info is useful since during the flashing process it gives an idea
    // of the bank affected - enabled permanently
    printk("\n\nkerSysBcmImageSet: flash_start_addr=0x%x string=%p len=%d whole_image=%d\n",
           flash_start_addr, string, size, whole_image);

    blk_start = flash_get_blk(flash_start_addr);
    if( blk_start < 0 )
        return( -1 );

    is_cfe_write = ((NVRAM_SECTOR == blk_start) &&
                    (size <= FLASH_LENGTH_BOOT_ROM));

    /*
     * write image to flash memory.
     * In theory, all calls to flash_write_buf() must be done with
     * semflash held, so I added it here.  However, in reality, all
     * flash image writes are protected by flashImageMutex at a higher
     * level.
     */
    down(&semflash);

    // Once we have acquired the flash semaphore, we can
    // disable activity on other processor and also on local processor.
    // Need to disable interrupts so that RCU stall checker will not complain.
    if (!is_cfe_write && !should_yield)
    {
        stopOtherCpu();
        local_irq_save(flags);
    }

    local_bh_disable();

	// may be called once - all sectors have equal size
	sect_size = flash_get_sector_size(blk_start);

    do 
    {
        flash_sector_erase_int(blk_start);     // erase blk before flash

        if (sect_size > size) 
        {
            if (size & 1) 
                size++;
            sect_size = size;
        }
#ifdef FLASH_PROCESS_DEBUG
		printk("Sector: [%d]\n", blk_start);
		dump(&string[sect_size - 16], 8);
#endif

        if (flash_write_buf(blk_start, 0, string, sect_size) != sect_size) {
            break;
        }

        // check if we just wrote into the sector where the NVRAM is.
        // update our in-memory copy
        if (NVRAM_SECTOR == blk_start)
        {
            updateInMemNvramData(string+NVRAM_DATA_OFFSET, NVRAM_LENGTH, 0);
        }
#if defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
		sectorNum++;
		if(sectorNum >= 5) /* Every 5 sector reset watch dog*/
		{
			restartHWWatchDog();
			sectorNum = 0;

		}
#endif //defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
        printk(".");
        blk_start++;
        string += sect_size;
        size -= sect_size; 

        if (should_yield)
        {
            local_bh_enable();
            yield();
            local_bh_disable();
        }
#ifdef SKY
		ledToggle_fromFirmwareUpdate(PWR_LED_GPIO,0); /* This is to blink power LED since in firmware update stage LED timers will not be running since we are getting hold of current CPU and disabled all the bootm half(Actual timer). */
#endif

    } while (size > 0);


#ifndef SKY
    if (whole_image && savedSize > fInfo.flash_rootfs_start_offset)
    {
        // If flashing a whole image, erase to end of flash.
        int total_blks = flash_get_numsectors();
#else /* SKY */

	// clear the rest of the flash up to auxfs or the beginning of the second bank
	// don't do it if the image is a whole image - it means it contains CFE as well
#ifdef CONFIG_SKY_ETHAN
    if (!whole_image) 
#endif
    {
        kerSysFlashPartInfoGet(&fPartAuxFS);
		middle_blk = sky_GetMiddleBoundaryBlock();
        
		end_blk = (blk_start > middle_blk ? fPartAuxFS.start_blk : middle_blk);
		printk("\nErasing the flash sectors - [%d - %d]\n", blk_start, end_blk);
#endif /* SKY */
		
#if defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
		sectorNum = 0;
#endif //defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)

        while( blk_start < end_blk ) {
			flash_sector_erase_int(blk_start);
			printk(".");
#if defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
		sectorNum++;
		if(sectorNum >= 5) /* Every 5 sector reset watch dog*/
		{
			restartHWWatchDog();
			sectorNum = 0;

		}
#endif //defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
            blk_start++;

            if (should_yield)
            {
                local_bh_enable();
                yield();
                local_bh_disable();
            }
#ifdef SKY
		ledToggle_fromFirmwareUpdate(PWR_LED_GPIO,0); /* This is to blink power LED since in firmware update stage LED timers will not be running since we are getting hold of current CPU and disabled all the bootm half(Actual timer). */
#endif

        }
    }

    up(&semflash);
    printk("\n\n");

#ifdef SKY	
    if (whole_image) {
       ledToggle_fromFirmwareUpdate(PWR_LED_GPIO,0);
       local_irq_restore(flags);
       local_bh_enable(); 
    } 
#endif
    if (is_cfe_write || should_yield)
    {
        local_bh_enable();
    }

    if( size == 0 )
    {
        sts = 0;  // ok
    }
    else
    {
        /*
         * Even though we try to recover here, this is really bad because
         * we have stopped the other CPU and we cannot restart it.  So we
         * really should try hard to make sure flash writes will never fail.
         */
        printk(KERN_ERR "kerSysBcmImageSet: write failed at blk=%d\n",
                        blk_start);
        sts = blk_start;    // failed to flash this sector
        if (!is_cfe_write && !should_yield)
        {
            local_irq_restore(flags);
            local_bh_enable();
        }
    }

    return sts;
}

/*******************************************************************************
 * SP functions
 * SP = ScratchPad, one or more sectors in the flash which user apps can
 * store small bits of data referenced by a small tag at the beginning.
 * kerSysScratchPadSet() and kerSysScratchPadCLearAll() must be protected by
 * a mutex because they do read/modify/writes to the flash sector(s).
 * kerSysScratchPadGet() and KerSysScratchPadList() do not need to acquire
 * the mutex, however, I acquire the mutex anyways just to make this interface
 * symmetrical.  High performance and concurrency is not needed on this path.
 *
 *******************************************************************************/

// get scratch pad data into *** pTempBuf *** which has to be released by the
//      caller!
// return: if pTempBuf != NULL, points to the data with the dataSize of the
//      buffer
// !NULL -- ok
// NULL  -- fail
static char *getScratchPad(int len)
{
    /* Root file system is on a writable NAND flash.  Read scratch pad from
     * a file on the NAND flash.
     */
    char *ret = NULL;

    if( (ret = retriedKmalloc(len)) != NULL )
    {
        struct file *fp;
        mm_segment_t fs;

        memset(ret, 0x00, len);
        fp = filp_open(SCRATCH_PAD_FILE_NAME, O_RDONLY, 0);
        if (!IS_ERR(fp) && fp->f_op && fp->f_op->read)
        {
            fs = get_fs();
            set_fs(get_ds());

            fp->f_pos = 0;

            if((int) fp->f_op->read(fp, (void *) ret, len, &fp->f_pos) <= 0)
                printk("Failed to read scratch pad from '%s'\n",
                    SCRATCH_PAD_FILE_NAME);

            filp_close(fp, NULL);
            set_fs(fs);
        }
    }
    else
        printk("Could not allocate scratch pad memory.\n");

    return( ret );
}

// set scratch pad - write the scratch pad file
// return:
// 0 -- ok
// -1 -- fail
static int setScratchPad(char *buf, int len)
{
    return kerSysFsFileSet(SCRATCH_PAD_FILE_NAME, buf, len);
}

/*
 * get list of all keys/tokenID's in the scratch pad.
 * NOTE: memcpy work here -- not using copy_from/to_user
 *
 * return:
 *         greater than 0 means number of bytes copied to tokBuf,
 *         0 means fail,
 *         negative number means provided buffer is not big enough and the
 *         absolute value of the negative number is the number of bytes needed.
 */
int kerSysScratchPadList(char *tokBuf, int bufLen)
{
    PSP_TOKEN pToken = NULL;
    char *pBuf = NULL;
    char *pShareBuf = NULL;
    char *startPtr = NULL;
    int usedLen;
    int tokenNameLen=0;
    int copiedLen=0;
    int needLen=0;
    int sts = 0;

    BCM_ASSERT_NOT_HAS_MUTEX_R(&spMutex, 0);

    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL) {
            mutex_unlock(&spMutex);
            return sts;
        }

        pBuf = pShareBuf;
    }
    else
    {
        if( (pShareBuf = getSharedBlks(fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            printk("could not getSharedBlks.\n");
            mutex_unlock(&spMutex);
            return sts;
        }

        // pBuf points to SP buf
        pBuf = pShareBuf + fInfo.flash_scratch_pad_blk_offset;  
    }

    if(memcmp(((PSP_HEADER)pBuf)->SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN) != 0) 
    {
        printk("Scratch pad is not initialized.\n");
        retriedKfree(pShareBuf);
        mutex_unlock(&spMutex);
        return sts;
    }

    // Walk through all the tokens
    usedLen = sizeof(SP_HEADER);
    startPtr = pBuf + sizeof(SP_HEADER);
    pToken = (PSP_TOKEN) startPtr;

    while( pToken->tokenName[0] != '\0' && pToken->tokenLen > 0 &&
           ((usedLen + pToken->tokenLen) <= fInfo.flash_scratch_pad_length))
    {
        tokenNameLen = strlen(pToken->tokenName);
        needLen += tokenNameLen + 1;
        if (needLen <= bufLen)
        {
            strcpy(&tokBuf[copiedLen], pToken->tokenName);
            copiedLen += tokenNameLen + 1;
        }

        usedLen += ((pToken->tokenLen + 0x03) & ~0x03);
        startPtr += sizeof(SP_TOKEN) + ((pToken->tokenLen + 0x03) & ~0x03);
        pToken = (PSP_TOKEN) startPtr;
    }

    if ( needLen > bufLen )
    {
        // User may purposely pass in a 0 length buffer just to get
        // the size, so don't log this as an error.
        sts = needLen * (-1);
    }
    else
    {
        sts = copiedLen;
    }

    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

    return sts;
}


/*
 * get sp data.  NOTE: memcpy work here -- not using copy_from/to_user
 * return:
 *         greater than 0 means number of bytes copied to tokBuf,
 *         0 means fail,
 *         negative number means provided buffer is not big enough and the
 *         absolute value of the negative number is the number of bytes needed.
 */
int kerSysScratchPadGet(char *tokenId, char *tokBuf, int bufLen)
{
    PSP_TOKEN pToken = NULL;
    char *pBuf = NULL;
    char *pShareBuf = NULL;
    char *startPtr = NULL;
    int usedLen;
    int sts = 0;

    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL) {
            mutex_unlock(&spMutex);
            return sts;
        }

        pBuf = pShareBuf;
    }
    else
    {
        if( (pShareBuf = getSharedBlks(fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            printk("could not getSharedBlks.\n");
            mutex_unlock(&spMutex);
            return sts;
        }

        // pBuf points to SP buf
        pBuf = pShareBuf + fInfo.flash_scratch_pad_blk_offset;
    }

    if(memcmp(((PSP_HEADER)pBuf)->SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN) != 0) 
    {
        printk("Scratch pad is not initialized.\n");
        retriedKfree(pShareBuf);
        mutex_unlock(&spMutex);
        return sts;
    }

    // search for the token
    usedLen = sizeof(SP_HEADER);
    startPtr = pBuf + sizeof(SP_HEADER);
    pToken = (PSP_TOKEN) startPtr;
    while( pToken->tokenName[0] != '\0' && pToken->tokenLen > 0 &&
        pToken->tokenLen < fInfo.flash_scratch_pad_length &&
        usedLen < fInfo.flash_scratch_pad_length )
    {

        if (strncmp(pToken->tokenName, tokenId, TOKEN_NAME_LEN) == 0)
        {
            if ( pToken->tokenLen > bufLen )
            {
               // User may purposely pass in a 0 length buffer just to get
               // the size, so don't log this as an error.
               // printk("The length %d of token %s is greater than buffer len %d.\n", pToken->tokenLen, pToken->tokenName, bufLen);
                sts = pToken->tokenLen * (-1);
            }
            else
            {
                memcpy(tokBuf, startPtr + sizeof(SP_TOKEN), pToken->tokenLen);
                sts = pToken->tokenLen;
            }
            break;
        }

        usedLen += ((pToken->tokenLen + 0x03) & ~0x03);
        startPtr += sizeof(SP_TOKEN) + ((pToken->tokenLen + 0x03) & ~0x03);
        pToken = (PSP_TOKEN) startPtr;
    }

    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

    return sts;
}

// set sp.  NOTE: memcpy work here -- not using copy_from/to_user
// return:
//  0 - ok
//  -1 - fail
int kerSysScratchPadSet(char *tokenId, char *tokBuf, int bufLen)
{
    PSP_TOKEN pToken = NULL;
    char *pShareBuf = NULL;
    char *pBuf = NULL;
    SP_HEADER SPHead;
    SP_TOKEN SPToken;
    char *curPtr;
    int sts = -1;

    if( bufLen >= fInfo.flash_scratch_pad_length - sizeof(SP_HEADER) -
        sizeof(SP_TOKEN) )
    {
        printk("Scratch pad overflow by %d bytes.  Information not saved.\n",
            bufLen  - fInfo.flash_scratch_pad_length - sizeof(SP_HEADER) -
            sizeof(SP_TOKEN));
        return sts;
    }

    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL)
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        pBuf = pShareBuf;
    }
    else
    {
        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        // pBuf points to SP buf
        pBuf = pShareBuf + fInfo.flash_scratch_pad_blk_offset;  
    }

    // form header info.
    memset((char *)&SPHead, 0, sizeof(SP_HEADER));
    memcpy(SPHead.SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN);
    SPHead.SPVersion = SP_VERSION;

    // form token info.
    memset((char*)&SPToken, 0, sizeof(SP_TOKEN));
    strncpy(SPToken.tokenName, tokenId, TOKEN_NAME_LEN - 1);
    SPToken.tokenLen = bufLen;

    if(memcmp(((PSP_HEADER)pBuf)->SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN) != 0)
    {
        // new sp, so just flash the token
        printk("No scratch pad found.  Initialize scratch pad...\n");
        memcpy(pBuf, (char *)&SPHead, sizeof(SP_HEADER));
        curPtr = pBuf + sizeof(SP_HEADER);
        memcpy(curPtr, (char *)&SPToken, sizeof(SP_TOKEN));
        curPtr += sizeof(SP_TOKEN);
        if( tokBuf )
            memcpy(curPtr, tokBuf, bufLen);
    }
    else  
    {
        int putAtEnd = 1;
        int curLen;
        int usedLen;
        int skipLen;

        /* Calculate the used length. */
        usedLen = sizeof(SP_HEADER);
        curPtr = pBuf + sizeof(SP_HEADER);
        pToken = (PSP_TOKEN) curPtr;
        skipLen = (pToken->tokenLen + 0x03) & ~0x03;
        while( pToken->tokenName[0] >= 'A' && pToken->tokenName[0] <= 'z' &&
            strlen(pToken->tokenName) < TOKEN_NAME_LEN &&
            pToken->tokenLen > 0 &&
            pToken->tokenLen < fInfo.flash_scratch_pad_length &&
            usedLen < fInfo.flash_scratch_pad_length )
        {
            usedLen += sizeof(SP_TOKEN) + skipLen;
            curPtr += sizeof(SP_TOKEN) + skipLen;
            pToken = (PSP_TOKEN) curPtr;
            skipLen = (pToken->tokenLen + 0x03) & ~0x03;
        }

        if( usedLen + SPToken.tokenLen + sizeof(SP_TOKEN) >
            fInfo.flash_scratch_pad_length )
        {
            printk("Scratch pad overflow by %d bytes.  Information not saved.\n",
                (usedLen + SPToken.tokenLen + sizeof(SP_TOKEN)) -
                fInfo.flash_scratch_pad_length);
            mutex_unlock(&spMutex);
            return sts;
        }

        curPtr = pBuf + sizeof(SP_HEADER);
        curLen = sizeof(SP_HEADER);
        while( curLen < usedLen )
        {
            pToken = (PSP_TOKEN) curPtr;
            skipLen = (pToken->tokenLen + 0x03) & ~0x03;
            if (strncmp(pToken->tokenName, tokenId, TOKEN_NAME_LEN) == 0)
            {
                // The token id already exists.
                if( tokBuf && pToken->tokenLen == bufLen )
                {
                    // The length of the new data and the existing data is the
                    // same.  Overwrite the existing data.
                    memcpy((curPtr+sizeof(SP_TOKEN)), tokBuf, bufLen);
                    putAtEnd = 0;
                }
                else
                {
                    // The length of the new data and the existing data is
                    // different.  Shift the rest of the scratch pad to this
                    // token's location and put this token's data at the end.
                    char *nextPtr = curPtr + sizeof(SP_TOKEN) + skipLen;
                    int copyLen = usedLen - (curLen+sizeof(SP_TOKEN) + skipLen);
                    memcpy( curPtr, nextPtr, copyLen );
                    memset( curPtr + copyLen, 0x00, 
                        fInfo.flash_scratch_pad_length - (curLen + copyLen) );
                    usedLen -= sizeof(SP_TOKEN) + skipLen;
                }
                break;
            }

            // get next token
            curPtr += sizeof(SP_TOKEN) + skipLen;
            curLen += sizeof(SP_TOKEN) + skipLen;
        } // end while

        if( putAtEnd )
        {
            if( tokBuf )
            {
                memcpy( pBuf + usedLen, &SPToken, sizeof(SP_TOKEN) );
                memcpy( pBuf + usedLen + sizeof(SP_TOKEN), tokBuf, bufLen );
            }
            memcpy( pBuf, &SPHead, sizeof(SP_HEADER) );
        }

    } // else if not new sp

    if( bootFromNand )
        sts = setScratchPad(pShareBuf, fInfo.flash_scratch_pad_length);
    else
        sts = setSharedBlks(fInfo.flash_scratch_pad_start_blk, 
            fInfo.flash_scratch_pad_number_blk, pShareBuf);
    
    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

    return sts;

    
}

#if defined(CONFIG_BCM968500) || defined (CONFIG_BCM96838)
EXPORT_SYMBOL(kerSysScratchPadSet);
#endif

// wipe out the scratchPad
// return:
//  0 - ok
//  -1 - fail
int kerSysScratchPadClearAll(void)
{ 
    int sts = -1;
    char *pShareBuf = NULL;
    int j ;
    int usedBlkSize = 0;

    // printk ("kerSysScratchPadClearAll.... \n") ;
    mutex_lock(&spMutex);

    if( bootFromNand )
    {
        if((pShareBuf = getScratchPad(fInfo.flash_scratch_pad_length)) == NULL)
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        memset(pShareBuf, 0x00, fInfo.flash_scratch_pad_length);

        setScratchPad(pShareBuf, fInfo.flash_scratch_pad_length);
    }
    else
    {
        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) == NULL )
        {
            mutex_unlock(&spMutex);
            return sts;
        }

        if (fInfo.flash_scratch_pad_number_blk == 1)
            memset(pShareBuf + fInfo.flash_scratch_pad_blk_offset, 0x00, fInfo.flash_scratch_pad_length) ;
        else
        {
            for (j = fInfo.flash_scratch_pad_start_blk;
                j < (fInfo.flash_scratch_pad_start_blk + fInfo.flash_scratch_pad_number_blk);
                j++)
            {
                usedBlkSize += flash_get_sector_size((unsigned short) j);
            }

            memset(pShareBuf, 0x00, usedBlkSize) ;
        }

        sts = setSharedBlks(fInfo.flash_scratch_pad_start_blk,    
            fInfo.flash_scratch_pad_number_blk,  pShareBuf);
    }

    retriedKfree(pShareBuf);

    mutex_unlock(&spMutex);

    //printk ("kerSysScratchPadClearAll Done.... \n") ;
    return sts;
}

int kerSysFlashSizeGet(void)
{
    int ret = 0;

    if( bootFromNand )
    {
        struct mtd_info *mtd;

        if( (mtd = get_mtd_device_nm("image")) != NULL )
        {
            ret = mtd->size;
            put_mtd_device(mtd);
        }
    }
    else
        ret = flash_get_total_size();

    return ret;
}

/***********************************************************************
 * Function Name: writeBootImageState
 * Description  : Persistently sets the state of an image update.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
static int writeBootImageState( int currState, int newState )
{
    int ret = -1;

    if(bootFromNand == 0)
    {
        /* NOR flash */
        char *pShareBuf = NULL;
#if defined(CONFIG_SKY_ETHAN) || !defined(SKY)

        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) != NULL )
        {
            PSP_HEADER pHdr = (PSP_HEADER) pShareBuf;
#else /* SKY */
	    if( (pShareBuf = getSharedBlks( fInfo.flash_sky_scratch_pad_start_blk,
    	    fInfo.flash_sky_scratch_pad_number_blk)) != NULL )
	    {
    	    PSKY_IHR_SP_HEADER pHdr = (PSKY_IHR_SP_HEADER) (pShareBuf + fInfo.flash_sky_scratch_pad_blk_offset);
#endif /* SKY */		
            unsigned long *pBootImgState=(unsigned long *)&pHdr->NvramData2[0];

            /* The boot image state is stored as a word in flash memory where
             * the most significant three bytes are a "magic number" and the
             * least significant byte is the state constant.
             */
            if( (*pBootImgState & 0xffffff00) == (BLPARMS_MAGIC & 0xffffff00) )
            {
                *pBootImgState &= ~0x000000ff;
                *pBootImgState |= newState;
#if defined(CONFIG_SKY_ETHAN) || !defined(SKY)

                ret = setSharedBlks(fInfo.flash_scratch_pad_start_blk,    
                    fInfo.flash_scratch_pad_number_blk,  pShareBuf);
#else /* SKY */
            ret = setSharedBlks(fInfo.flash_sky_scratch_pad_start_blk,    
                fInfo.flash_sky_scratch_pad_number_blk,  pShareBuf);
#endif /* SKY */	
            }

            retriedKfree(pShareBuf);
        }
    }
    else
    {
        mm_segment_t fs;

        /* NAND flash */
        char state_name_old[] = "/data/" NAND_BOOT_STATE_FILE_NAME;
        char state_name_new[] = "/data/" NAND_BOOT_STATE_FILE_NAME;

        fs = get_fs();
        set_fs(get_ds());

        if( currState == -1 )
        {
            /* Create new state file name. */
            struct file *fp;

            state_name_new[strlen(state_name_new) - 1] = newState;
            fp = filp_open(state_name_new, O_RDWR | O_TRUNC | O_CREAT,
                S_IRUSR | S_IWUSR);

            if (!IS_ERR(fp))
            {
                fp->f_pos = 0;
                fp->f_op->write(fp, (void *) "boot state\n",
                    strlen("boot state\n"), &fp->f_pos);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35)
                vfs_fsync(fp, 0);
#else
                vfs_fsync(fp, fp->f_path.dentry, 0);
#endif
                filp_close(fp, NULL);
            }
            else
                printk("Unable to open '%s'.\n", PSI_FILE_NAME);
        }
        else
        {
            if( currState != newState )
            {
                /* Rename old state file name to new state file name. */
                state_name_old[strlen(state_name_old) - 1] = currState;
                state_name_new[strlen(state_name_new) - 1] = newState;
                sys_rename(state_name_old, state_name_new);
            }
        }

        set_fs(fs);
        ret = 0;
    }

    return( ret );
}

/***********************************************************************
 * Function Name: readBootImageState
 * Description  : Reads the current boot image state from flash memory.
 * Returns      : state constant or -1 for failure
 ***********************************************************************/
static int readBootImageState( void )
{
    int ret = -1;

    if(bootFromNand == 0)
    {
        /* NOR flash */
        char *pShareBuf = NULL;

#if defined(CONFIG_SKY_ETHAN) || !defined(SKY)
        if( (pShareBuf = getSharedBlks( fInfo.flash_scratch_pad_start_blk,
            fInfo.flash_scratch_pad_number_blk)) != NULL )
        {
            PSP_HEADER pHdr = (PSP_HEADER) pShareBuf;
#else /* SKY */	
	    if( (pShareBuf = getSharedBlks( fInfo.flash_sky_scratch_pad_start_blk,
    	    fInfo.flash_sky_scratch_pad_number_blk)) != NULL )
	    {
    	    PSKY_IHR_SP_HEADER pHdr = (PSKY_IHR_SP_HEADER) (pShareBuf + fInfo.flash_sky_scratch_pad_blk_offset);
#endif /* SKY */			
            unsigned long *pBootImgState=(unsigned long *)&pHdr->NvramData2[0];

            /* The boot image state is stored as a word in flash memory where
             * the most significant three bytes are a "magic number" and the
             * least significant byte is the state constant.
             */
            if( (*pBootImgState & 0xffffff00) == (BLPARMS_MAGIC & 0xffffff00) )
            {
                ret = *pBootImgState & 0x000000ff;
            }

            retriedKfree(pShareBuf);
        }
    }
    else
    {
        /* NAND flash */
        int i;
        char states[] = {BOOT_SET_NEW_IMAGE, BOOT_SET_OLD_IMAGE,
            BOOT_SET_NEW_IMAGE_ONCE};
        char boot_state_name[] = "/data/" NAND_BOOT_STATE_FILE_NAME;

        /* The boot state is stored as the last character of a file name on
         * the data partition, /data/boot_state_X, where X is
         * BOOT_SET_NEW_IMAGE, BOOT_SET_OLD_IMAGE or BOOT_SET_NEW_IMAGE_ONCE.
         */
        for( i = 0; i < sizeof(states); i++ )
        {
            struct file *fp;
            boot_state_name[strlen(boot_state_name) - 1] = states[i];
            fp = filp_open(boot_state_name, O_RDONLY, 0);
            if (!IS_ERR(fp) )
            {
                filp_close(fp, NULL);

                ret = (int) states[i];
                break;
            }
        }

        if( ret == -1 && writeBootImageState( -1, BOOT_SET_NEW_IMAGE ) == 0 )
            ret = BOOT_SET_NEW_IMAGE;
    }

    return( ret );
}

/***********************************************************************
 * Function Name: dirent_rename
 * Description  : Renames file oldname to newname by parsing NAND flash
 *                blocks with JFFS2 file system entries.  When the JFFS2
 *                directory entry inode for oldname is found, it is modified
 *                for newname.  This differs from a file system rename
 *                operation that creates a new directory entry with the same
 *                inode value and greater version number.  The benefit of
 *                this method is that by having only one directory entry
 *                inode, the CFE ROM can stop at the first occurance which
 *                speeds up the boot by not having to search the entire file
 *                system partition.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
static int dirent_rename(char *oldname, char *newname)
{
    int ret = -1;
    struct mtd_info *mtd;
    unsigned char *buf;

    mtd = get_mtd_device_nm("bootfs_update");

    if( IS_ERR_OR_NULL(mtd) )
        mtd = get_mtd_device_nm("rootfs_update"); 

    buf = (mtd) ? retriedKmalloc(mtd->erasesize) : NULL;

    if( mtd && buf && strlen(oldname) == strlen(newname) )
    {
        int namelen = strlen(oldname);
        int blk, done, retlen;

        /* Read NAND flash blocks in the rootfs_update JFFS2 file system
         * partition to search for a JFFS2 directory entry for the oldname
         * file.
         */
        for(blk = 0, done = 0; blk < mtd->size && !done; blk += mtd->erasesize)
        {
            if(mtd_read(mtd, blk, mtd->erasesize, &retlen, buf) == 0)
            {
                struct jffs2_raw_dirent *pdir;
                unsigned char *p = buf;

                while( p < buf + mtd->erasesize )
                {
                    pdir = (struct jffs2_raw_dirent *) p;
                    if( je16_to_cpu(pdir->magic) == JFFS2_MAGIC_BITMASK )
                    {
                        if( je16_to_cpu(pdir->nodetype) ==
                                JFFS2_NODETYPE_DIRENT &&
                            !memcmp(oldname, pdir->name, namelen) &&
                            je32_to_cpu(pdir->ino) != 0 )
                        {
                            /* Found the oldname directory entry.  Change the
                             * name to newname.
                             */
                            memcpy(pdir->name, newname, namelen);
                            je32_to_cpu(pdir->name_crc) = crc32(0, pdir->name,
                                (unsigned int) namelen);

                            /* Write the modified block back to NAND flash. */
                            if(nandEraseBlk( mtd, blk ) == 0)
                            {
                                if( nandWriteBlk(mtd, blk, mtd->erasesize, buf, TRUE) == 0 )
                                {
                                    ret = 0;
                                }
                            }

                            done = 1;
                            break;
                        }

                        p += (je32_to_cpu(pdir->totlen) + 0x03) & ~0x03;
                    }
                    else
                        break;
                }
            }
        }
    }
    else
        printk("%s: Error renaming file\n", __FUNCTION__);

    if( mtd )
        put_mtd_device(mtd);

    if( buf )
        retriedKfree(buf);

    return( ret );
}

#ifdef SKY
int kerSysSetSequenceNumber(int bank, int seq) {
	int ret = -1;

    mutex_lock(&flashImageMutex);
    if(bootFromNand == 0) {
		PFILE_TAG pTag = NULL;
        char *pShareBuf = NULL;
        int blk;

		pTag = kerSysSetTagSequenceNumber(bank, seq);
        blk = *(int *) (pTag + 1);

        if (pTag && (pShareBuf = getSharedBlks(blk, 1)) != NULL) {
            memcpy(pShareBuf, pTag, sizeof(FILE_TAG));
            setSharedBlks(blk, 1, pShareBuf);
            retriedKfree(pShareBuf);
			ret = 0;
        }
		else {
			ret = -1;
		}		
	}
	else {
		ret = -1;
	}
    mutex_unlock(&flashImageMutex);
	return ret;
}
#endif


/***********************************************************************
 * Function Name: updateSequenceNumber
 * Description  : updates the sequence number on the specified partition
 *                to be the highest.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
static int updateSequenceNumber(int incSeqNumPart, int seqPart1, int seqPart2)
{
    int ret = -1;

    mutex_lock(&flashImageMutex);
    if(bootFromNand == 0)
    {
        /* NOR flash */
        char *pShareBuf = NULL;
        PFILE_TAG pTag;
        int blk;

        pTag = kerSysUpdateTagSequenceNumber(incSeqNumPart);
        blk = *(int *) (pTag + 1);

        if ((pShareBuf = getSharedBlks(blk, 1)) != NULL)
        {
            memcpy(pShareBuf, pTag, sizeof(FILE_TAG));
            setSharedBlks(blk, 1, pShareBuf);
            retriedKfree(pShareBuf);
        }
    }
    else
    {
        /* NAND flash */

        /* The sequence number on NAND flash is updated by renaming file
         * cferam.XXX where XXX is the sequence number. The rootfs partition
         * is usually read-only. Therefore, only the cferam.XXX file on the
         * rootfs_update partiton is modified. Increase or decrase the
         * sequence number on the rootfs_update partition so the desired
         * partition boots.
         */
        int seq = -1;
        unsigned long rootfs_ofs = 0xff;

        kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs);

        if(rootfs_ofs == inMemNvramData.ulNandPartOfsKb[NP_ROOTFS_1] )
        {
            /* Partition 1 is the booted partition. */
            if( incSeqNumPart == 2 )
            {
                /* Increase sequence number in rootfs_update partition. */
                if( seqPart1 >= seqPart2 )
                    seq = seqPart1 + 1;
            }
            else
            {
                /* Decrease sequence number in rootfs_update partition. */
                if( seqPart2 >= seqPart1 && seqPart1 != 0 )
                    seq = seqPart1 - 1;
            }
        }
        else
        {
            /* Partition 2 is the booted partition. */
            if( incSeqNumPart == 1 )
            {
                /* Increase sequence number in rootfs_update partition. */
                if( seqPart2 >= seqPart1 )
                    seq = seqPart2 + 1;
            }
            else
            {
                /* Decrease sequence number in rootfs_update partition. */
                if( seqPart1 >= seqPart2 && seqPart2 != 0 )
                    seq = seqPart2 - 1;
            }
        }

        if( seq != -1 )
        {
            struct mtd_info *mtd;
            char mtd_part[32];

            /* Find the sequence number of the non-booted partition. */
            mm_segment_t fs;

            fs = get_fs();
            set_fs(get_ds());

            mtd = get_mtd_device_nm("bootfs_update");
            if( !IS_ERR_OR_NULL(mtd) )
            {
                strcpy(mtd_part, "mtd:bootfs_update");
                put_mtd_device(mtd);
            }
            else
                strcpy(mtd_part, "mtd:rootfs_update");


            if( sys_mount(mtd_part, "/mnt", "jffs2", 0, NULL) == 0 )
            {
                char fname[] = NAND_CFE_RAM_NAME;
                char cferam_old[32], cferam_new[32], cferam_fmt[32]; 
                int i;

                fname[strlen(fname) - 3] = '\0'; /* remove last three chars */
                strcpy(cferam_fmt, "/mnt/");
                strcat(cferam_fmt, fname);
                strcat(cferam_fmt, "%3.3d");

                for( i = 0; i < 999; i++ )
                {
                    struct file *fp;
                    sprintf(cferam_old, cferam_fmt, i);
                    fp = filp_open(cferam_old, O_RDONLY, 0);
                    if (!IS_ERR(fp) )
                    {
                        filp_close(fp, NULL);

                        /* Change the boot sequence number in the rootfs_update
                         * partition by renaming file cferam.XXX where XXX is
                         * a sequence number.
                         */
                        sprintf(cferam_new, cferam_fmt, seq);
                        if( NAND_FULL_PARTITION_SEARCH )
                        {
                            sys_rename(cferam_old, cferam_new);
                            sys_umount("/mnt", 0);
                        }
                        else
                        {
                            sys_umount("/mnt", 0);
                            dirent_rename(cferam_old + strlen("/mnt/"),
                                cferam_new + strlen("/mnt/"));
                        }
                        break;
                    }
                }
            }
            set_fs(fs);
        }
    }
    mutex_unlock(&flashImageMutex);

    return( ret );
}

/***********************************************************************
 * Function Name: kerSysSetBootImageState
 * Description  : Persistently sets the state of an image update.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
#ifdef SKY
int kerSysSetBootImageState( int newState, int renumber )
#else
int kerSysSetBootImageState( int newState )
#endif
{
    int ret = -1;
    int incSeqNumPart = -1;
    int writeImageState = 0;
    int currState = readBootImageState();
    int seq1 = kerSysGetSequenceNumber(1);
    int seq2 = kerSysGetSequenceNumber(2);

    /* Update the image state persistently using "new image" and "old image"
     * states.  Convert "partition" states to "new image" state for
     * compatibility with the non-OMCI image update.
     */

    switch(newState)
    {
    case BOOT_SET_PART1_IMAGE:
        if( seq1 != -1 )
        {
            if( seq1 < seq2 )
                incSeqNumPart = 1;
            newState = BOOT_SET_NEW_IMAGE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_PART2_IMAGE:
        if( seq2 != -1 )
        {
            if( seq2 < seq1 )
                incSeqNumPart = 2;
            newState = BOOT_SET_NEW_IMAGE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_PART1_IMAGE_ONCE:
        if( seq1 != -1 )
        {
            if( seq1 < seq2 )
                incSeqNumPart = 1;
            newState = BOOT_SET_NEW_IMAGE_ONCE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_PART2_IMAGE_ONCE:
        if( seq2 != -1 )
        {
            if( seq2 < seq1 )
                incSeqNumPart = 2;
            newState = BOOT_SET_NEW_IMAGE_ONCE;
            writeImageState = 1;
        }
        break;

    case BOOT_SET_OLD_IMAGE:
    case BOOT_SET_NEW_IMAGE:
    case BOOT_SET_NEW_IMAGE_ONCE:
        /* The boot image state is stored as a word in flash memory where
         * the most significant three bytes are a "magic number" and the
         * least significant byte is the state constant.
         */
        if( currState == newState )
        {
            ret = 0;
        }
        else
        {
            writeImageState = 1;

            if(newState==BOOT_SET_NEW_IMAGE && currState==BOOT_SET_OLD_IMAGE)
            {
                /* The old (previous) image is being set as the new
                 * (current) image. Make sequence number of the old
                 * image the highest sequence number in order for it
                 * to become the new image.
                 */
                incSeqNumPart = 0;
            }
        }
        break;

    default:
        break;
    }

    if( writeImageState ) {
		ret = kerWriteBootImageState(currState, newState);
	}

    if( incSeqNumPart != -1 && renumber)
        updateSequenceNumber(incSeqNumPart, seq1, seq2);

    return( ret );
}


int kerWriteBootImageState(int currState, int newState) {
	int ret = 0;
	mutex_lock(&spMutex);
	ret = writeBootImageState(currState, newState);
	mutex_unlock(&spMutex);
	return ret;
}

/***********************************************************************
 * Function Name: kerSysGetBootImageState
 * Description  : Gets the state of an image update from flash.
 * Returns      : state constant or -1 for failure
 ***********************************************************************/
int kerSysGetBootImageState( void )
{
    int ret = readBootImageState();

    if( ret != -1 )
    {
        int seq1 = kerSysGetSequenceNumber(1);
        int seq2 = kerSysGetSequenceNumber(2);

        switch(ret)
        {
        case BOOT_SET_NEW_IMAGE:
            if( seq1 == -1 || seq1 < seq2 )
                ret = BOOT_SET_PART2_IMAGE;
            else
                ret = BOOT_SET_PART1_IMAGE;
            break;

        case BOOT_SET_NEW_IMAGE_ONCE:
            if( seq1 == -1 || seq1 < seq2 )
                ret = BOOT_SET_PART2_IMAGE_ONCE;
            else
                ret = BOOT_SET_PART1_IMAGE_ONCE;
            break;

        case BOOT_SET_OLD_IMAGE:
            if( seq1 == -1 || seq1 > seq2 )
                ret = BOOT_SET_PART2_IMAGE;
            else
                ret = BOOT_SET_PART1_IMAGE;
            break;

        default:
            ret = -1;
            break;
        }
    }

    return( ret );
}

#ifdef SKY
int kerSysBootImageStateGet( void )
{
    int sts = 0;

    mutex_lock(&spMutex);
	sts = readBootImageState();
    mutex_unlock(&spMutex);
    return sts;
}
#endif

/***********************************************************************
 * Function Name: kerSysSetOpticalPowerValues
 * Description  : Saves optical power values to flash that are obtained
 *                during the  manufacturing process. These values are
 *                stored in NVRAM_DATA which should not be erased.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysSetOpticalPowerValues(UINT16 rxReading, UINT16 rxOffset, 
    UINT16 txReading)
{
    NVRAM_DATA nd;

    kerSysNvRamGet((char *) &nd, sizeof(nd), 0);

    nd.opticRxPwrReading = rxReading;
    nd.opticRxPwrOffset  = rxOffset;
    nd.opticTxPwrReading = txReading;
    
    nd.ulCheckSum = 0;
    nd.ulCheckSum = crc32(CRC32_INIT_VALUE, &nd, sizeof(NVRAM_DATA));

    return(kerSysNvRamSet((char *) &nd, sizeof(nd), 0));
}

/***********************************************************************
 * Function Name: kerSysGetOpticalPowerValues
 * Description  : Retrieves optical power values from flash that were
 *                saved during the manufacturing process.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysGetOpticalPowerValues(UINT16 *prxReading, UINT16 *prxOffset, 
    UINT16 *ptxReading)
{
    NVRAM_DATA nd;

    kerSysNvRamGet((char *) &nd, sizeof(nd), 0);

    *prxReading = nd.opticRxPwrReading;
    *prxOffset  = nd.opticRxPwrOffset;
    *ptxReading = nd.opticTxPwrReading;

    return(0);
}


#if !defined(CONFIG_BRCM_IKOS)

int kerSysEraseFlash(unsigned long eraseaddr, unsigned long len)
{
    int blk;
    int bgnBlk = flash_get_blk(eraseaddr);
    int endBlk = flash_get_blk(eraseaddr + len);
    unsigned long bgnAddr = (unsigned long) flash_get_memptr(bgnBlk);
    unsigned long endAddr = (unsigned long) flash_get_memptr(endBlk);

#ifdef DEBUG_FLASH
    printk("kerSysEraseFlash blk[%d] eraseaddr[0x%08x] len[%lu]\n",
    bgnBlk, (int)eraseaddr, len);
#endif

    if ( bgnAddr != eraseaddr)
    {
       printk("ERROR: kerSysEraseFlash eraseaddr[0x%08x]"
              " != first block start[0x%08x]\n",
              (int)eraseaddr, (int)bgnAddr);
        return (len);
    }

    if ( (endAddr - bgnAddr) != len)
    {
        printk("ERROR: kerSysEraseFlash eraseaddr[0x%08x] + len[%lu]"
               " != last+1 block start[0x%08x]\n",
               (int)eraseaddr, len, (int) endAddr);
        return (len);
    }

    for (blk=bgnBlk; blk<endBlk; blk++)
        flash_sector_erase_int(blk);

    return 0;
}



unsigned long kerSysReadFromFlash( void *toaddr, unsigned long fromaddr,
    unsigned long len )
{
    int blk, offset, bytesRead;
    unsigned long blk_start;
    char * trailbyte = (char*) NULL;
    char val[2];

    blk = flash_get_blk((int)fromaddr); /* sector in which fromaddr falls */
    blk_start = (unsigned long)flash_get_memptr(blk); /* sector start address */
    offset = (int)(fromaddr - blk_start); /* offset into sector */

#ifdef DEBUG_FLASH
    printk("kerSysReadFromFlash blk[%d] fromaddr[0x%08x]\n",
           blk, (int)fromaddr);
#endif

    bytesRead = 0;

        /* cfiflash : hardcoded for bankwidths of 2 bytes. */
    if ( offset & 1 )   /* toaddr is not 2 byte aligned */
    {
        flash_read_buf(blk, offset-1, val, 2);
        *((char*)toaddr) = val[1];

        toaddr = (void*)((char*)toaddr+1);
        fromaddr += 1;
        len -= 1;
        bytesRead = 1;

        /* if len is 0 we could return here, avoid this if */

        /* recompute blk and offset, using new fromaddr */
        blk = flash_get_blk(fromaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(fromaddr - blk_start);
    }

        /* cfiflash : hardcoded for len of bankwidths multiples. */
    if ( len & 1 )
    {
        len -= 1;
        trailbyte = (char *)toaddr + len;
    }

        /* Both len and toaddr will be 2byte aligned */
    if ( len )
    {
       flash_read_buf(blk, offset, toaddr, len);
       bytesRead += len;
    }

        /* write trailing byte */
    if ( trailbyte != (char*) NULL )
    {
        fromaddr += len;
        blk = flash_get_blk(fromaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(fromaddr - blk_start);
        flash_read_buf(blk, offset, val, 2 );
        *trailbyte = val[0];
        bytesRead += 1;
    }

    return( bytesRead );
}

/*
 * Function: kerSysWriteToFlash
 *
 * Description:
 * This function assumes that the area of flash to be written was
 * previously erased. An explicit erase is therfore NOT needed 
 * prior to a write. This function ensures that the offset and len are
 * two byte multiple. [cfiflash hardcoded for bankwidth of 2 byte].
 *
 * Parameters:
 *      toaddr : destination flash memory address
 *      fromaddr: RAM memory address containing data to be written
 *      len : non zero bytes to be written
 * Return:
 *      FAILURE: number of bytes remaining to be written
 *      SUCCESS: 0 (all requested bytes were written)
 */
int kerSysWriteToFlash( unsigned long toaddr,
                        void * fromaddr, unsigned long len)
{
    int blk, offset, size, blk_size, bytesWritten;
    unsigned long blk_start;
    char * trailbyte = (char*) NULL;
    unsigned char val[2];

#ifdef DEBUG_FLASH
    printk("kerSysWriteToFlash flashAddr[0x%08x] fromaddr[0x%08x] len[%lu]\n",
    (int)toaddr, (int)fromaddr, len);
#endif

    blk = flash_get_blk(toaddr);    /* sector in which toaddr falls */
    blk_start = (unsigned long)flash_get_memptr(blk); /* sector start address */
    offset = (int)(toaddr - blk_start); /* offset into sector */

    /* cfiflash : hardcoded for bankwidths of 2 bytes. */
    if ( offset & 1 )   /* toaddr is not 2 byte aligned */
    {
        val[0] = 0xFF; // ignored
        val[1] = *((char *)fromaddr); /* write the first byte */
        bytesWritten = flash_write_buf(blk, offset-1, val, 2);
        if ( bytesWritten != 2 )
        {
#ifdef DEBUG_FLASH
           printk("ERROR kerSysWriteToFlash ... remaining<%lui>\n", len); 
#endif
           return len;
        }

        toaddr += 1;
        fromaddr = (void*)((char*)fromaddr+1);
        len -= 1;

    /* if len is 0 we could return bytesWritten, avoid this if */

    /* recompute blk and offset, using new toaddr */
        blk = flash_get_blk(toaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(toaddr - blk_start);
    }

    /* cfiflash : hardcoded for len of bankwidths multiples. */
    if ( len & 1 )
    {
    /* need to handle trailing byte seperately */
        len -= 1;
        trailbyte = (char *)fromaddr + len;
        toaddr += len;
    }

    /* Both len and toaddr will be 2byte aligned */
    while ( len > 0 )
    {
        blk_size = flash_get_sector_size(blk);
        if (FLASH_API_ERROR == blk_size) {
           return len;
        }
        size = blk_size - offset; /* space available in sector from offset */
        if ( size > len )
            size = len;

        bytesWritten = flash_write_buf(blk, offset, fromaddr, size); 
        if ( bytesWritten !=  size )
        {
#ifdef DEBUG_FLASH
           printk("ERROR kerSysWriteToFlash ... remaining<%lui>\n", 
               (len - bytesWritten + ((trailbyte == (char*)NULL)? 0 : 1)));
#endif
           return (len - bytesWritten + ((trailbyte == (char*)NULL)? 0 : 1));
        }

        fromaddr += size;
        len -= size;

        blk++;      /* Move to the next block */
        offset = 0; /* All further blocks will be written at offset 0 */
    }

    /* write trailing byte */
    if ( trailbyte != (char*) NULL )
    {
        blk = flash_get_blk(toaddr);
        blk_start = (unsigned long)flash_get_memptr(blk);
        offset = (int)(toaddr - blk_start);
        val[0] = *trailbyte; /* trailing byte */
        val[1] = 0xFF; // ignored
        bytesWritten = flash_write_buf(blk, offset, val, 2 );
        if ( bytesWritten != 2 )
        {
#ifdef DEBUG_FLASH
           printk("ERROR kerSysWriteToFlash ... remaining<%d>\n",1);
#endif
           return 1;
        }
    } 

    return len;
}
/*
 * Function: kerSysWriteToFlashREW
 * 
 * Description:
 * This function does not assume that the area of flash to be written was erased.
 * An explicit erase is therfore needed prior to a write.  
 * kerSysWriteToFlashREW uses a sector copy  algorithm. The first and last sectors
 * may need to be first read if they are not fully written. This is needed to
 * avoid the situation that there may be some valid data in the sector that does
 * not get overwritten, and would be erased.
 *
 * Due to run time costs for flash read, optimizations to read only that data
 * that will not be overwritten is introduced.
 *
 * Parameters:
 *  toaddr : destination flash memory address
 *  fromaddr: RAM memory address containing data to be written
 *  len : non zero bytes to be written
 * Return:
 *  FAILURE: number of bytes remaining to be written 
 *  SUCCESS: 0 (all requested bytes were written)
 *
 */
int kerSysWriteToFlashREW( unsigned long toaddr,
                        void * fromaddr, unsigned long len)
{
    int blk, offset, size, blk_size, bytesWritten;
    unsigned long sect_start;
    int mem_sz = 0;
    char * mem_p = (char*)NULL;

#ifdef DEBUG_FLASH
    printk("kerSysWriteToFlashREW flashAddr[0x%08x] fromaddr[0x%08x] len[%lu]\n",
    (int)toaddr, (int)fromaddr, len);
#endif

    blk = flash_get_blk( toaddr );
    sect_start = (unsigned long) flash_get_memptr(blk);
    offset = toaddr - sect_start;

    while ( len > 0 )
    {
        blk_size = flash_get_sector_size(blk);
        size = blk_size - offset; /* space available in sector from offset */

        /* bound size to remaining len in final block */
        if ( size > len )
            size = len;

        /* Entire blk written, no dirty data to read */
        if ( size == blk_size )
        {
            flash_sector_erase_int(blk);

            bytesWritten = flash_write_buf(blk, 0, fromaddr, blk_size);

            if ( bytesWritten != blk_size )
            {
                if ( mem_p != NULL )
                    retriedKfree(mem_p);
                return (len - bytesWritten);    /* FAILURE */
            }
        }
        else
        {
                /* Support for variable sized blocks, paranoia */
            if ( (mem_p != NULL) && (mem_sz < blk_size) )
            {
                retriedKfree(mem_p);    /* free previous temp buffer */
                mem_p = (char*)NULL;
            }

            if ( (mem_p == (char*)NULL)
              && ((mem_p = (char*)retriedKmalloc(blk_size)) == (char*)NULL) )
            {
                printk("\tERROR kerSysWriteToFlashREW fail to allocate memory\n");
                return len;
            }
            else
                mem_sz = blk_size;

            if ( offset ) /* First block */
            {
                if ( (offset + size) == blk_size)
                {
                   flash_read_buf(blk, 0, mem_p, offset);
                }
                else
                {  
                   /*
                    * Potential for future optimization:
                    * Should have read the begining and trailing portions
                    * of the block. If the len written is smaller than some
                    * break even point.
                    * For now read the entire block ... move on ...
                    */
                   flash_read_buf(blk, 0, mem_p, blk_size);
                }
            }
            else
            {
                /* Read the tail of the block which may contain dirty data*/
                flash_read_buf(blk, len, mem_p+len, blk_size-len );
            }

            flash_sector_erase_int(blk);

            memcpy(mem_p+offset, fromaddr, size); /* Rebuild block contents */

            bytesWritten = flash_write_buf(blk, 0, mem_p, blk_size);

            if ( bytesWritten != blk_size )
            {
                if ( mem_p != (char*)NULL )
                    retriedKfree(mem_p);
                return (len + (blk_size - size) - bytesWritten );
            }
        }

        /* take into consideration that size bytes were copied */
        fromaddr += size;
        toaddr += size;
        len -= size;

        blk++;          /* Move to the next block */
        offset = 0;     /* All further blocks will be written at offset 0 */

    }

    if ( mem_p != (char*)NULL )
        retriedKfree(mem_p);

    return ( len );
}

#endif

#ifdef SKY_IHR

/***********************************************************************
 * Function Name: kerSysGetFlashSectorSize
 * Description  : Wraps flash_get_sector_size()
 * Returns      : size of sector requested
 ***********************************************************************/
int kerSysGetFlashSectorSize( unsigned short sector )
{
    return flash_get_sector_size(sector);
}



static UINT32 KerSysgetCrc32(byte *pdata, UINT32 size, UINT32 crc)
{
    while (size-- > 0)
        crc = (crc >> 8) ^ Crc32_table[(crc ^ *pdata++) & 0xff];
	
    return crc;
}

void KerSysErasePSIAndSP(void)
{
	char *pBuf = NULL;
	kerSysScratchPadClearAll();
 	if ((pBuf = (char *) retriedKmalloc(fInfo.flash_persistent_length)) == NULL)
    {
   	   printk("failed to allocate memory with size: %d\n", fInfo.flash_persistent_length);
       return ;
   	}

    memset(pBuf, 0xff, fInfo.flash_persistent_length);

	kerSysPersistentSet(pBuf, fInfo.flash_persistent_length, 0);
    retriedKfree(pBuf);
}

void updateRestoreDefaultFlag(char  value)
{
    unsigned char *pShareBuf = NULL;
	if( (pShareBuf = getSharedBlks( fInfo.flash_sky_scratch_pad_start_blk,
				fInfo.flash_sky_scratch_pad_number_blk)) != NULL )
	{
		PSKY_IHR_SP_HEADER pHdr = (PSKY_IHR_SP_HEADER) (pShareBuf + fInfo.flash_sky_scratch_pad_blk_offset);
		pHdr->restoreDefault = value;
		setSharedBlks(fInfo.flash_sky_scratch_pad_start_blk, fInfo.flash_sky_scratch_pad_number_blk,  pShareBuf);
		retriedKfree(pShareBuf);

	}
	else
		printk(" Error Resetting Restore Default Flag \n");
}


/** get nvram data
 *
 * since it reads from in-memory copy of the nvram data, always successful.
 */
void kerSysCfeNvRamGet(char *string, int strLen, int offset)
{
    unsigned long flags;
    spin_lock_irqsave(&inMemCfeNvramData_spinlock, flags);
    memcpy(string, inMemCfeNvramData_buf + offset, strLen);
    spin_unlock_irqrestore(&inMemCfeNvramData_spinlock, flags);

    return;
}


int kerSysCfeNvRamSet(const char *string, int strLen, int offset)
{
    int sts = -1;  // initialize to failure
    char *pBuf = NULL;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);
    BCM_ASSERT_R(offset+strLen <= NVRAM_LENGTH, sts);
    if (bootFromNand == 0)
    {
        if ((pBuf = getSharedBlks(CFE_NVRAM_SECTOR, 1)) == NULL)
            return sts;

        // set string to the memory buffer
        memcpy((pBuf + CFE_NVRAM_DATA_OFFSET + offset), string, strLen);

        sts = setSharedBlks(CFE_NVRAM_SECTOR, 1, pBuf);
    
        retriedKfree(pBuf);
    }
    else
    {
        printk("kerSysNvRamSet: not supported when bootFromNand is 1\n");
    }

    return sts;
}



static int parsexdigit(char str)
{
    int digit;

    if ((str >= '0') && (str <= '9')) 
        digit = str - '0';
    else if ((str >= 'a') && (str <= 'f')) 
        digit = str - 'a' + 10;
    else if ((str >= 'A') && (str <= 'F')) 
        digit = str - 'A' + 10;
    else 
        return -1;

    return digit;
}

static int parsehwaddr(char *str,uint8_t *hwaddr)
{
    int digit1,digit2;
    int idx = 6;
    
    if (strlen(str) == (MAX_MAC_STR_LEN - 7)) {       // no ':' mac input format ie. 021800100801
        while (*str && (idx > 0)) {
	        digit1 = parsexdigit(*str);
	        if (digit1 < 0)
                return -1;
	        str++;
	        if (!*str)
                return -1;
	        digit2 = parsexdigit(*str);
	        if (digit2 < 0)
                return -1;
	        *hwaddr++ = (digit1 << 4) | digit2;
	        idx--;
            str++;
	    }
        return 0;
    }

    if (strlen(str) != MAX_MAC_STR_LEN-2)
        return -1;
    if (*(str+2) != ':' || *(str+5) != ':' || *(str+8) != ':' || *(str+11) != ':' || *(str+14) != ':')
        return -1;
    
    while (*str && (idx > 0)) {
	    digit1 = parsexdigit(*str);
	    if (digit1 < 0)
            return -1;
	    str++;
	    if (!*str)
            return -1;

	    if (*str == ':') {
	        digit2 = digit1;
	        digit1 = 0;
	    }
	    else {
	        digit2 = parsexdigit(*str);
	        if (digit2 < 0)
                return -1;
            str++;
	    }

	    *hwaddr++ = (digit1 << 4) | digit2;
	    idx--;

	    if (*str == ':') 
            str++;
	}
    return 0;
}
/* This function reads the contents from CFE part of NVRAM
	and copies the content to firmware part of NVRAM
*/
void KerSysCloneCfeNVRAMData(void)
{
	int scratchPadInitialised=0;
	UINT32 newcrc = CRC32_INIT_VALUE, savedCrc;
	unsigned char serialised_mac[NVRAM_MAC_ADDRESS_LEN];
//	int sect_size = 0;
//	char *pBuf = NULL;

//	printk("Entry: KerSysCloneCfeNVRAMData \n");
	inMemSkyScratchPad_buf = (unsigned char *) &inMemSkyScratchPad;
	memset(inMemSkyScratchPad_buf, UNINITIALIZED_FLASH_DATA_CHAR, sizeof(SKY_IHR_SP_HEADER));

	flash_read_buf (SKY_SP_SECTOR, SKY_SP_OFFSET,inMemSkyScratchPad_buf, sizeof (SKY_IHR_SP_HEADER)) ;
	/* Murthy: SKY_SP_SECTOR  will be initialised only if restoredefault is triggered at CFE
	otherwise it will not be initialised.
	Do not do anything if it is not initialised , if it is initialised then check if the restore default flag is set
	*/
	if(memcmp(&inMemSkyScratchPad.SPMagicNum, MAGIC_NUMBER, MAGIC_NUM_LEN) == 0)
		scratchPadInitialised =1;
	//else
		//printk("SKY Scratch pad is not initialized.\n");

		
	
#ifdef CONFIG_AUXFS_PERCENT
	  inMemNvramData.ucAuxFSPercent=CONFIG_AUXFS_PERCENT;  
#endif  
	savedCrc = inMemNvramData.ulCheckSum;
    inMemNvramData.ulCheckSum = 0;
    newcrc = KerSysgetCrc32(inMemNvramData_buf, sizeof(NVRAM_DATA), newcrc);
	inMemNvramData.ulCheckSum = savedCrc;

	
	/* Initialise Firmware NVRAM contents with CFE NVRAM contents in the following case
		1.  inMemNvramData.ulVersion == -1   -- This means the flash NVRAM is not initialised at all 
		2.  savedCrc != newcrc    -- Thsi is b'cas the NVRAM mapping has chagned or hange in memory mapping 
		3.  inMemSkyScratchPad.restoreDefault == 1  -- CFE has set restore default flag, this can happen is the used 
			has pressed restore default button or initiated software recovery by pressing WPS button
	
	*/

	inMemCfeNvramData_buf = (unsigned char *) &inMemCfeNvramData;
	memset(inMemCfeNvramData_buf, UNINITIALIZED_FLASH_DATA_CHAR, sizeof(CFE_NVRAM_DATA));
	
		// read CFE NVRAM  content to in memory
	flash_read_buf (CFE_NVRAM_SECTOR, CFE_NVRAM_DATA_OFFSET,inMemCfeNvramData_buf, sizeof (CFE_NVRAM_DATA));
	

			
	if((inMemNvramData.ulVersion == -1) || (savedCrc != newcrc) || ((scratchPadInitialised) && (inMemSkyScratchPad.restoreDefault == 1)))
	{
		if(inMemNvramData.ulVersion == -1)
			printk("Copying CFE NVRAM contents to Firmware NVRAM contents b'cas NVRAM is not initialised \n");
		else if (savedCrc != newcrc)
			printk("Copying CFE NVRAM contents to Firmware NVRAM contents b'cas NVRAM CRC mismatch is calculated\n");
		else if ((scratchPadInitialised) && (inMemSkyScratchPad.restoreDefault == 1))
			printk("Copying CFE NVRAM contents to Firmware NVRAM contents b'cas RestoreDefault Flag is set from CFE \n");

	/* Murthy ::   At the moment we do not have any NVRAM parameters which differs
	from CFE, hence just copy the CFE NVRAM contents directly to firmware NVRAM.

	In Future if we have to override some of the parameters, add the code below and 
	comment SKY_NOTREQD
	*/
	memcpy(&inMemNvramData,&inMemCfeNvramData,sizeof(CFE_NVRAM_DATA));
#ifdef SKY_NOTREQD	
		memcpy(inMemNvramData.szBootline, inMemCfeNvramData.szBootline, NVRAM_BOOTLINE_LEN);
		memcpy(inMemNvramData.szBoardId, inMemCfeNvramData.szBoardId, NVRAM_BOARD_ID_STRING_LEN);			
		inMemNvramData.ulMainTpNum = inMemCfeNvramData.ulMainTpNum;
		inMemNvramData.ulPsiSize = inMemCfeNvramData.ulPsiSize;
		inMemNvramData.ulNumMacAddrs = inMemCfeNvramData.ulNumMacAddrs;
		memcpy(inMemNvramData.ucaBaseMacAddr, inMemCfeNvramData.ucaBaseMacAddr, NVRAM_MAC_ADDRESS_LEN);
		inMemNvramData.backupPsi = inMemCfeNvramData.backupPsi;
		memcpy(inMemNvramData.gponSerialNumber, inMemCfeNvramData.gponSerialNumber, NVRAM_GPON_SERIAL_NUMBER_LEN);		
		memcpy(inMemNvramData.gponPassword, inMemCfeNvramData.gponPassword, NVRAM_GPON_PASSWORD_LEN);		
		memcpy(inMemNvramData.wpsDevicePin, inMemCfeNvramData.wpsDevicePin, NVRAM_WPS_DEVICE_PIN_LEN);		
		inMemNvramData.ulSyslogSize = inMemCfeNvramData.ulSyslogSize;
		memcpy(inMemNvramData.szVoiceBoardId, inMemCfeNvramData.szVoiceBoardId, NVRAM_BOARD_ID_STRING_LEN);		
		memcpy(inMemNvramData.afeId, inMemCfeNvramData.afeId, 2);
		inMemNvramData.ulVersion = NVRAM_VERSION_NUMBER;
		inMemNvramData.ulCheckSum = 0;
		inMemNvramData.ulCheckSum = KerSysgetCrc32(inMemNvramData_buf, sizeof(NVRAM_DATA), CRC32_INIT_VALUE);
#endif //SKY_NOTREQD		
		erasePSIandSP = 1;
		writeNvramToFlash = 1;
			

	}

	parsehwaddr(inMemSerialisationData.BaseMac, serialised_mac);
	if(memcmp(inMemNvramData.ucaBaseMacAddr, serialised_mac, NVRAM_MAC_ADDRESS_LEN) != 0)
	{
		printk("Copying MAC Address Form Serialization Data to Firmware NVRAM \n");
		/* MAC address is different so copy it
		This is a special case, when first time the router comes up, MAC address in Serialization data is not populated.
		later after the serialization data is populated, this MAC address needs to be copied to firmware MAC address
		*/
		memcpy(inMemNvramData.ucaBaseMacAddr, serialised_mac, NVRAM_MAC_ADDRESS_LEN);
		inMemNvramData.ulCheckSum = 0;
		inMemNvramData.ulCheckSum = KerSysgetCrc32(inMemNvramData_buf, sizeof(NVRAM_DATA), CRC32_INIT_VALUE);
		writeNvramToFlash = 1;
		
	}
    		
	/* NOTE:  This is very early part of initialization, we can not take spinlock
	to write into flash.  Delay the flash write till  function kerSysFlashInit().
	The contents of inMemNvramData are written back to flash in function
	kerSysFlashInit()
	*/

}

/** Write protect the sectors containing the serialisation data.
 * Must be called with flashImageMutex held
 *
 * @return 0 on success, -1 on failure.
 */
#ifdef ENABLE_SERIALISATION_WP
int kerSysSerialisationWP()
{
	int sts = -1;  // initialize to failure
	BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);
	if (bootFromNand == 0)
	{
         sts = flash_write_protect_serialisation_sectors();
	}
	else
	{
		 printk("kerSysSerialisationWP: not supported when bootFromNand is 1\n");
	}

	return sts;
}
#else
int kerSysSoftUnProt()
{
	int sts = -1;  // initialize to failure
	BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);
	if (bootFromNand == 0)
	{
         sts = flash_software_unprotect();
	}
	else
	{
		 printk("kerSysSoftWriteProt: not supported when bootFromNand is 1\n");
	}

	return sts;
}

int kerSysSoftWriteProt()
{
	int sts = -1;  // initialize to failure
	BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);
	if (bootFromNand == 0)
	{
         sts = flash_software_write_protect();
	}
	else
	{
		 printk("kerSysSoftWriteProt: not supported when bootFromNand is 1\n");
	}

	return sts;
}
#endif

#ifdef ENABLE_REPAIR_SW
/** set sector data
 * Must be called with flashImageMutex held
 *
 * @return 0 on success, -1 on failure.
 */

int kerSysSectorSet(const char *string, int strLen, int sectorNumber)
{
    int sts = -1;  // initialize to failure
    char *pBuf = NULL;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    if (bootFromNand == 0)
    {
        if ((pBuf = getSharedBlks(sectorNumber, 1)) == NULL)
            return sts;

        // set string to the memory buffer
        memcpy(pBuf, string, strLen);

        sts = setSharedBlks(sectorNumber, 1, pBuf);
    
        retriedKfree(pBuf);
    }
    else
    {
        printk("kerSysSectorSet: not supported when bootFromNand is 1\n");
    }

    return sts;
}
#endif

/** set Serialisation data
 * Must be called with flashImageMutex held
 *
 * @return 0 on success, -1 on failure.
 */
int kerSysSerialisationSet(const char *string, int strLen, int offset)
{
    int sts = -1;  // initialize to failure
    char *pBuf = NULL;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);
    BCM_ASSERT_R(offset+strLen <= sizeof(SERIALISATION_DATA), sts);

    if (bootFromNand == 0)
    {
        if ((pBuf = getSharedBlks(SERIALISATION_SECTOR, 1)) == NULL)
            return sts;

        // set string to the memory buffer
        memcpy((pBuf + SERIALISATION_DATA_OFFSET + offset), string, strLen);

        sts = setSharedBlks(SERIALISATION_SECTOR, 1, pBuf);
    
        retriedKfree(pBuf);

        if (0 == sts)
        {
            // write to flash was OK, now update in-memory copy
            updateInMemSerialisationData((unsigned char *) string, strLen, offset);
        }
    }
    else
    {
        printk("kerSysSerialisationSet: not supported when bootFromNand is 1\n");
    }

    return sts;
}


#ifdef SKY_FLASH_API
void kerSysFlashDump(unsigned int sector, int offset, int length)
{
	unsigned char *pBuf = NULL;
	unsigned int usedBlkSize;
	int i = 0;
	
	down(&semflash);
	
	usedBlkSize = flash_get_sector_size(sector);
	if ((pBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
        printk("failed to allocate memory with size: %d\n", usedBlkSize);
        up(&semflash);
        return;
    }
	
    flash_read_buf(sector, offset, pBuf, length);
	up(&semflash);
	printk("Flash dump:\n");
	for(i=0;i<length;i++)
		printk ("0x%x\t",pBuf[i]);
	printk("\n");	
	retriedKfree(pBuf);

    return;
}

void kerSysFlashWrite(unsigned int sector, int offset, int length)
{
	unsigned char *readBuf = NULL;
	unsigned int usedBlkSize;
	down(&semflash);
	
	usedBlkSize = flash_get_sector_size(sector);

	if ((readBuf = (char *) retriedKmalloc(usedBlkSize)) == NULL)
    {
        printk("failed to allocate memory with size: %d\n", usedBlkSize);
        up(&semflash);
        return;
    }
	
    flash_read_buf(sector, 0, readBuf, usedBlkSize);
	memset((readBuf + offset), 0xAA,  length);

	printk("Writing data 0xAA into the flash...\n");
	flash_sector_erase_int(sector);
	if (flash_write_buf(sector, 0, readBuf, usedBlkSize) != usedBlkSize)
    {
        printk("Error writing flash sector %d.", sector);
    }
	
	up(&semflash);
	retriedKfree(readBuf);
	return;
}
#endif // SKY_FLASH_API



/** get Serialisation data
 *
 * since it reads from in-memory copy of the Serialisation data, always successful.
 */
void kerSysSerialisationGet(char *string, int strLen, int offset)
{
    unsigned long flags;
    spin_lock_irqsave(&inMemSerialisationData_spinlock, flags);
    memcpy(string, inMemSerialisationData_buf + offset, strLen);
    spin_unlock_irqrestore(&inMemSerialisationData_spinlock, flags);

    return;
}

/** get Public Key
 *
 * since it reads from in-memory copy of the Public Key data, always successful.
 */
void kerSysPublicKeyGet(char *string, int strLen, int offset)
{
    memcpy(string, inMemPublicKey_buf + offset, strLen);

    return;
}


/** Update the in-Memory copy of the Serialisation with the given data.
 *
 * @data: pointer to new nvram data
 * @len: number of valid bytes in nvram data
 * @offset: offset of the given data in the nvram data
 */
void updateInMemSerialisationData(const unsigned char *data, int len, int offset)
{
    unsigned long flags;

    spin_lock_irqsave(&inMemSerialisationData_spinlock, flags);
    memcpy(inMemSerialisationData_buf + offset, data, len);
    spin_unlock_irqrestore(&inMemSerialisationData_spinlock, flags);
}
/* Method to erase AUXFS sectores */
int KerSysEraseAuxfs(void)
{
        char *pAuxfs = NULL;
        int ret = 0;
        FLASH_PARTITION_INFO fPartAuxFS;
        /*Get correct AUXFS blk, sector,length */
        kerSysFlashPartInfoGet(& fPartAuxFS );

    if ((pAuxfs = getSharedBlks(fPartAuxFS.start_blk,
        fPartAuxFS.number_blk)) == NULL)
        return -1;

        memset(pAuxfs, 0xff, fPartAuxFS.total_len);

    if (setSharedBlks(fPartAuxFS.start_blk,
        fPartAuxFS.number_blk, pAuxfs) != 0)
        ret = -1;

        retriedKfree(pAuxfs);

    return ret;
}

#endif

