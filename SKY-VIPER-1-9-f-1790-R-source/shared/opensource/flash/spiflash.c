/*
   <:copyright-BRCM:2012:DUAL/GPL:standard
   
      Copyright (c) 2012 Broadcom Corporation
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

/** Includes. **/
#ifdef _CFE_                                                
#include "lib_types.h"
#include "lib_printf.h"
#include "lib_string.h"
#include "bcm_map_part.h"  
#define printk  printf
#else       // linux
#include <linux/version.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#include <linux/semaphore.h>
#endif
#include <linux/hardirq.h>

#include <bcm_map_part.h> 
#endif

#include "bcmtypes.h"
#include "bcm_hwdefs.h"
#include "flash_api.h"
#include "bcmSpiRes.h"

#if defined(_BCM968500_) || defined(CONFIG_BCM968500)
#define CONFIG_SF_DEFAULT_SPEED 	50000000

void lilac_spi_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode);
#endif

/** Defines. **/
#define OSL_DELAY(X)                        \
    do { { int i; for( i = 0; i < (X) * 500; i++ ) ; } } while(0)

#define MAX_RETRY           3

#ifndef NULL
#define NULL 0
#endif

#if (INC_BTRM_BUILD==1)
/* reduce the memory usage for BTRM which only need to access up to 512KB */
#define MAXSECTORS          128     
#else
#define MAXSECTORS          8192    /* maximum number of sectors supported */
#endif

#define FLASH_PAGE_256      256
#define SECTOR_SIZE_4K      (4 * 1024)
#define SECTOR_SIZE_64K     (64 * 1024)

/* Standard Boolean declarations */
#define TRUE                1
#define FALSE               0

/* Command codes for the flash_command routine */
#define FLASH_WRST          0x01    /* write status register */
#define FLASH_PROG          0x02    /* program data into memory array */
#define FLASH_READ          0x03    /* read data from memory array */
#define FLASH_WRDI          0x04    /* reset write enable latch */
#define FLASH_RDSR          0x05    /* read status register */
#define FLASH_WREN          0x06    /* set write enable latch */
#define FLASH_READ_FAST     0x0B    /* read data from memory array */
#define FLASH_SERASE        0x20    /* erase one sector in memory array */
#ifdef SKY
#define FLASH_RCR           0x35    /*  Read config register in memory array */
#endif /* SKY */
#define FLASH_BERASE        0xD8    /* erase one block in memory array */
#define FLASH_RDID          0x9F    /* read manufacturer and product id */
#define FLASH_EN4B          0xB7    /* Enable 4 byte address mode */
#define FLASH_EXIT4B        0xE9    /* Exit 4 byte address mode */
#define FLASH_READ_DOR      0x3B    /* dual output read */
#define FLASH_READ_DIOR     0xBB    /* dual i/o read */

/* Spansion Extended Addressing opcodes for 4byte addresses */
#define FLASH_4PROG         0x12    /* program data into memory array */
#define FLASH_4READ         0x13    /* read data from memory array */
#define FLASH_4READ_FAST    0x0C    /* read data from memory array */
#define FLASH_4SERASE       0x21    /* erase one sector in memory array */
#define FLASH_4BERASE       0xDC    /* erase one block in memory array */
#define FLASH_4READ_DOR     0x3C    /* dual output read */
#define FLASH_4READ_DIOR    0xBC    /* dual i/o read */


/* Spansion specific commands */  
#define FLASH_RDCR          0x35    /* Read Configuration Register */


/* RDSR return status bit definition */
#define SR_WPEN             0x80
#define SR_BP3              0x20
#define SR_BP2              0x10
#define SR_BP1              0x08
#define SR_BP0              0x04
#define SR_WEN              0x02
#define SR_RDY              0x01

#ifdef SKY	
/* RCR return status bit definition */
#define CR_TBPROT           0x20
#define CR_NA               0x10//Don't use this.  Doesn't do anything
#define CR_BPNV             0x08
#define CR_TBPARAM          0x04
#define CR_QUAD             0x02
#define CR_FREEZE           0x01
#endif /* SKY */
/* Return codes from flash_status */
#define STATUS_READY        0       /* ready for action */
#define STATUS_BUSY         1       /* operation in progress */
#define STATUS_TIMEOUT      2       /* operation timed out */
#define STATUS_ERROR        3       /* unclassified but unhappy status */

/* Define different type of flash */
#define FLASH_UNDEFINED     0
#define FLASH_SPAN          2

/* SST's manufacturer ID */
#define SSTPART             0xBF
/* A list of SST device IDs */
#define ID_SST25VF016       0x41
#define ID_SST25VF032       0x4A
#define ID_SST25VF064       0x4B

/* SPANSION manufacturer IDs */
#define SPANPART            0x01
/* SPANSION device ID's */
#define ID_SPAN25FL016      0x14
#define ID_SPAN25FL032      0x15
#define ID_SPAN25FL064      0x16
#define ID_SPAN25FL164      0x17
#define ID_SPAN25FL128      0x18
#define ID_SPAN25FL256      0x19


/* EON manufacturer ID */
#define EONPART             0x1C
/* NUMONYX manufacturer ID */
#define NUMONYXPART         0x20
/* AMIC manufacturer ID */
#define AMICPART            0x37
/* Macronix manufacturer ID */
#define MACRONIXPART        0xC2
/* Winbond's manufacturer ID */
#define WBPART              0xEF

/* JEDEC device IDs */
#define ID_M25P16           0x15
#define ID_M25P32           0x16
#define ID_M25P64           0x17
#define ID_M25P128          0x18
#define ID_M25P256          0x19

/* flash memory type, the second byte returned from the READ ID instruction*/
#define FLASH_MEMTYPE_NULL  0x00
#define FLASH_MEMTYPE_20    0x20
#define FLASH_MEMTYPE_BA    0xBA

#define SPI_MAKE_ID(A,B)    \
    (((unsigned short) (A) << 8) | ((unsigned short) B & 0xff))

#define SPI_FLASH_DEVICES                                   \
    {{SPI_MAKE_ID(SSTPART, ID_SST25VF016), "SST25VF016"},   \
     {SPI_MAKE_ID(SSTPART, ID_SST25VF032), "SST25VF032"},   \
     {SPI_MAKE_ID(SSTPART, ID_SST25VF064), "SST25VF064"},   \
     {SPI_MAKE_ID(SPANPART, ID_SPAN25FL016), "S25FL016"},   \
     {SPI_MAKE_ID(SPANPART, ID_SPAN25FL032), "S25FL032"},   \
     {SPI_MAKE_ID(SPANPART, ID_SPAN25FL064), "S25FL064"},   \
     {SPI_MAKE_ID(SPANPART, ID_SPAN25FL164), "S25FL164"},   \
     {SPI_MAKE_ID(SPANPART, ID_SPAN25FL128), "S25FL128"},   \
     {SPI_MAKE_ID(SPANPART, ID_SPAN25FL256), "S25FL256"},   \
     {SPI_MAKE_ID(WBPART, ID_M25P16), "ID_W25X16"},         \
     {SPI_MAKE_ID(WBPART, ID_M25P32), "ID_W25X32"},         \
     {SPI_MAKE_ID(WBPART, ID_M25P64), "ID_W25X64"},         \
     {SPI_MAKE_ID(WBPART, ID_M25P128), "ID_W25X128"},       \
     {SPI_MAKE_ID(EONPART, ID_M25P16), "EN25P16"},          \
     {SPI_MAKE_ID(EONPART, ID_M25P32), "EN25P32"},          \
     {SPI_MAKE_ID(EONPART, ID_M25P64), "EN25P64"},          \
     {SPI_MAKE_ID(EONPART, ID_M25P128), "EN25P128"},        \
     {SPI_MAKE_ID(AMICPART, ID_M25P16), "A25L016"},         \
     {SPI_MAKE_ID(AMICPART, ID_M25P32), "A25L032"},         \
     {SPI_MAKE_ID(NUMONYXPART, ID_M25P16), "NMNXM25P16"},   \
     {SPI_MAKE_ID(NUMONYXPART, ID_M25P32), "NMNXM25P32"},   \
     {SPI_MAKE_ID(NUMONYXPART, ID_M25P64), "NMNXM25P64"},   \
     {SPI_MAKE_ID(NUMONYXPART, ID_M25P128), "NMNXM25P128"}, \
     {SPI_MAKE_ID(NUMONYXPART, ID_M25P256), "MCRNN25Q256"}, \
     {SPI_MAKE_ID(MACRONIXPART, ID_M25P16), "MX25L16"},     \
     {SPI_MAKE_ID(MACRONIXPART, ID_M25P32), "MX25L32"},     \
     {SPI_MAKE_ID(MACRONIXPART, ID_M25P64), "MX25L64"},     \
     {SPI_MAKE_ID(MACRONIXPART, ID_M25P128), "MX25L128"},   \
     {SPI_MAKE_ID(MACRONIXPART, ID_M25P256), "MX25L256"},   \
     {0,""}                                                 \
    }

/** Structs. **/
/* A structure for identifying a flash part.  There is one for each
 * of the flash part definitions.  We need to keep track of the
 * sector organization, the address register used, and the size
 * of the sectors.
 */
struct flashinfo {
    char *name;         /* "AT25F512", etc. */
    unsigned long addr; /* physical address, once translated */
    int nsect;          /* # of sectors */
    struct {
        long size;      /* # of bytes in this sector */
        long base;      /* offset from beginning of device */
    } sec[MAXSECTORS];  /* per-sector info */
};

#ifndef INC_BTRM_BOOT
#define INC_BTRM_BOOT 0
#endif

/** Prototypes. **/
static int my_spi_read( struct spi_transfer *xfer );
static int my_spi_write( unsigned char *msg_buf, int nbytes );

int spi_flash_init(flash_device_info_t **flash_info);
#ifdef SKY	
#ifdef ENABLE_SERIALISATION_WP
static int spi_flash_write_protect_serialisation_sectors();
#else
static int spi_flash_software_unprotect(void);
static int spi_flash_software_write_protect(void);
static int spi_flash_is_write_protected(void);
#endif
#endif /* SKY	*/
static int spi_flash_sector_erase_int(unsigned short sector);
static int spi_flash_reset(void);
#if (INC_BTRM_BOOT==0) && defined(_CFE_) && defined(CFG_RAMAPP)
static 
#endif
int spi_flash_read_buf(unsigned short sector, int offset, unsigned char *buffer, int nbytes);
static int spi_flash_ub(unsigned short sector);
static int spi_flash_write(unsigned short sector, int offset,
    unsigned char *buffer, int nbytes);
static int spi_flash_write_buf(unsigned short sector, int offset,
    unsigned char *buffer, int nbytes);
static int spi_flash_get_numsectors(void);
#if (INC_BTRM_BOOT==0) && defined(_CFE_) && defined(CFG_RAMAPP)
static 
#endif
int spi_flash_get_sector_size(unsigned short sector);
static unsigned char *spi_get_flash_memptr(unsigned short sector);
static unsigned char *spi_flash_get_memptr(unsigned short sector);
static int spi_flash_status(void);
static unsigned short spi_flash_get_device_id(unsigned int* memtype);
static int spi_flash_get_blk(int addr);
static int spi_flash_get_total_size(void);
static int spi_flash_en4b(void);
static int spi_flash_disable4b(void);
static void spi_flash_multibit_en(void);

/** Variables. **/
static flash_device_info_t flash_spi_dev =
    {
        0xffff,
        FLASH_IFC_SPI,
        "",
#ifdef SKY		
#ifdef ENABLE_SERIALISATION_WP
        spi_flash_write_protect_serialisation_sectors,
#else   
        spi_flash_software_unprotect,
        spi_flash_software_write_protect,
		spi_flash_is_write_protected,
#endif
#endif /* SKY */
        spi_flash_sector_erase_int,
        spi_flash_read_buf,
        spi_flash_write_buf,
        spi_flash_get_numsectors,
        spi_flash_get_sector_size,
        spi_flash_get_memptr,
        spi_flash_get_blk,
        spi_flash_get_total_size
    };

static struct flash_name_from_id fnfi[] = SPI_FLASH_DEVICES;

/* the controller will handle operati0ns that are greater than the FIFO size
   code that relies on READ_BUF_LEN_MAX, READ_BUF_LEN_MIN or spi_max_op_len
   could be changed */
#define READ_BUF_LEN_MAX   544    /* largest of the maximum transaction sizes for SPI */
#define READ_BUF_LEN_MIN   60     /* smallest of the maximum transaction sizes for SPI */
/* this is the slave ID of the SPI flash for use with the SPI controller */
#define SPI_FLASH_SLAVE_DEV_ID    0
/* clock defines for the flash */
#define SPI_FLASH_DEF_CLOCK       781000

/* default to smallest transaction size - updated later */
static unsigned int spi_max_op_len = READ_BUF_LEN_MIN; 
static int spi_read_cmd            = FLASH_READ_FAST;
static int spi_dualOut_read_cmd    = FLASH_READ_DOR;
static int spi_dualIO_read_cmd     = FLASH_READ_DIOR;
static int spi_write_cmd           = FLASH_PROG;
static int spi_erase_cmd           = FLASH_BERASE;
static int spi_serase_cmd          = FLASH_SERASE;
static int spi_dummy_bytes         = 1;
static int spi_multibit_en         = 0;
static int flash_page_size         = FLASH_PAGE_256;

/* default to legacy controller - updated later */
static int spi_flash_clock  = SPI_FLASH_DEF_CLOCK;
static int spi_flash_busnum = LEG_SPI_BUS_NUM;

#ifndef _CFE_
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
static DEFINE_SEMAPHORE(spi_flash_lock);
#else
static DECLARE_MUTEX(spi_flash_lock);
#endif
static bool bSpiFlashSlaveRes = FALSE;
#endif

static struct flashinfo meminfo; /* Flash information structure */
static int totalSize = 0;
static int addr32 = FALSE;
static int flashManufacturer = SPANPART;
static int flashMemtype = FLASH_MEMTYPE_NULL;

static int my_spi_read(struct spi_transfer *xfer)
{
    int status;

#ifndef _CFE_
    if ( FALSE == bSpiFlashSlaveRes )
#endif
    {
        status = BcmSpi_MultibitRead(xfer, spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID);
    }
#ifndef _CFE_
    else
    {
        status = BcmSpiSyncMultTrans(xfer, 1, spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID);
    }
#endif

    return status;
}

static int my_spi_write(unsigned char *msg_buf, int nbytes)
{
    int status; 

#ifndef _CFE_
    if ( FALSE == bSpiFlashSlaveRes )
#endif
    {
        status = BcmSpi_Write(msg_buf, nbytes, spi_flash_busnum, 
                              SPI_FLASH_SLAVE_DEV_ID, spi_flash_clock);
    }
#ifndef _CFE_
    else
    {
        status = BcmSpiSyncTrans(msg_buf, NULL, 0, nbytes, spi_flash_busnum,
                                 SPI_FLASH_SLAVE_DEV_ID);
    }
#endif
    return status;
}


/*********************************************************************/
/* Init_flash is used to build a sector table. This information is   */
/* translated from erase_block information to base:offset information*/
/* for each individual sector. This information is then stored       */
/* in the meminfo structure, and used throughout the driver to access*/
/* sector information.                                               */
/*                                                                   */
/* This is more efficient than deriving the sector base:offset       */
/* information every time the memory map switches (since on the      */
/* development platform can only map 64k at a time).  If the entire  */
/* flash memory array can be mapped in, then the addition static     */
/* allocation for the meminfo structure can be eliminated, but the   */
/* drivers will have to be re-written.                               */
/*                                                                   */
/* The meminfo struct occupies 44 bytes of heap space, depending     */
/* on the value of the define MAXSECTORS.  Adjust to suit            */
/* application                                                       */ 
/*********************************************************************/

int spi_flash_init(flash_device_info_t **flash_info)
{
    struct flash_name_from_id *fnfi_ptr;
    int i=0, count=0;
    int basecount=0L;
    unsigned short device_id;
    int sectorsize = 0;
    int numsector = 0;
#if !defined(_BCM968500_) && !defined(CONFIG_BCM968500)
    int spiCtrlState;
#endif

#if defined(_BCM96816_) || defined(CONFIG_BCM96816) || defined(_BCM96818_) || defined(CONFIG_BCM96818)
    uint32 miscStrapBus = MISC->miscStrapBus;

    if ( miscStrapBus & MISC_STRAP_BUS_LS_SPIM_ENABLED )
    {
        spi_flash_busnum = LEG_SPI_BUS_NUM;
        if ( miscStrapBus & MISC_STRAP_BUS_SPI_CLK_FAST )
        {
            spi_flash_clock = 20000000;
        }
        else
        {
            spi_flash_clock = 781000;
        }
    }
    else
    {
        spi_flash_busnum = HS_SPI_BUS_NUM;
        if ( miscStrapBus & MISC_STRAP_BUS_SPI_CLK_FAST )
        {
            spi_flash_clock = 20000000;
        }
        else
        {
            spi_flash_clock = 66666667;
        }
    }
#endif
#if defined(_BCM96328_) || defined(CONFIG_BCM96328)
    uint32 miscStrapBus = MISC->miscStrapBus;

    spi_flash_busnum = HS_SPI_BUS_NUM;
    if ( miscStrapBus & MISC_STRAP_BUS_HS_SPIM_FAST_B_MASK )
        spi_flash_clock = 33333334;
    else
        spi_flash_clock = 16666667;
#endif
#if defined(_BCM96362_) || defined(CONFIG_BCM96362) || defined(_BCM963268_) || defined(CONFIG_BCM963268) || defined(_BCM96828_) || defined(CONFIG_BCM96828)
    uint32 miscStrapBus = MISC->miscStrapBus;

    spi_flash_busnum = HS_SPI_BUS_NUM;
    if ( miscStrapBus & MISC_STRAP_BUS_HS_SPIM_CLK_SLOW_N_FAST )
        spi_flash_clock = 50000000;
    else
        spi_flash_clock = 20000000;
#endif
#if defined(_BCM96368_) || defined(CONFIG_BCM96368)
    uint32 miscStrapBus = GPIO->StrapBus;
 
    if ( miscStrapBus & MISC_STRAP_BUS_SPI_CLK_FAST )
       spi_flash_clock = 20000000;
    else
       spi_flash_clock = 781000;
#endif

#if defined(_BCM96318_) || defined(CONFIG_BCM96318)
       spi_flash_busnum = HS_SPI_BUS_NUM;
       spi_flash_clock = 62500000; 
       //spi_flash_clock = 12500000; Default clock
#endif

#if defined(_BCM96838_) || defined(CONFIG_BCM96838)
       spi_flash_busnum = HS_SPI_BUS_NUM;
       spi_flash_clock = 20000000; // reset value 
#endif

#if defined(_BCM963381_) || defined(CONFIG_BCM963381)
       spi_flash_busnum = HS_SPI_BUS_NUM;
       //FIXME choose the right clock
       spi_flash_clock = 20000000; // reset value 
#endif

#if defined(_BCM968500_) || defined(CONFIG_BCM968500)
    lilac_spi_setup_slave (0, 6, CONFIG_SF_DEFAULT_SPEED, SPI_MODE_3);
#else
    /* retrieve the maximum read/write transaction length from the SPI controller */
    spi_max_op_len = BcmSpi_GetMaxRWSize( spi_flash_busnum, 1 );

    /* set the controller state, spi_mode_0 */
    spiCtrlState = SPI_CONTROLLER_STATE_DEFAULT;
    if ( spi_flash_clock > SPI_CONTROLLER_MAX_SYNC_CLOCK )
       spiCtrlState |= SPI_CONTROLLER_STATE_ASYNC_CLOCK;
    BcmSpi_SetCtrlState(spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, SPI_MODE_DEFAULT, spiCtrlState);

    if (HS_SPI_BUS_NUM == spi_flash_busnum)
        flash_spi_dev.flash_type = FLASH_IFC_HS_SPI;
#endif

    *flash_info = &flash_spi_dev;
#if 0
    /* 
     * in case of flash corrupt, the following steps can erase the flash
     * 1. jumper USE_SPI_SLAVE to make SPI in slave mode
     * 2. start up JTAG debuger and remove the USE_SPI_SLAVE jumper 
     * 3. run the following code to erase the flash
     */
    flash_sector_erase_int(0);
    flash_sector_erase_int(1);
    printk("flash_init: erase all sectors\n");
    return FLASH_API_OK;
#endif
#ifdef SKY
#if 0 //TMS
{
//tms if you try to write something to write protected flash - this magic command will allow you to recover
//	  just after hang load a ram cfe with this code uncommented
    unsigned char buf[4];
    struct spi_transfer xfer;
    buf[0] = 0x30;
    memset(&xfer, 0, sizeof(struct spi_transfer));
    xfer.tx_buf      = buf;
    xfer.rx_buf      = buf;
    xfer.len         = 1;
    xfer.speed_hz    = spi_flash_clock;
    xfer.prepend_cnt = 1;
    my_spi_read(&xfer);
	
    memset(&xfer, 0, sizeof(struct spi_transfer));
    xfer.tx_buf      = buf;
    xfer.rx_buf      = buf;
    xfer.len         = 1;
    xfer.speed_hz    = spi_flash_clock;
    xfer.prepend_cnt = 1;
    buf[0] = FLASH_RDSR;
    my_spi_read(&xfer);

    printf("tms> after recovery  spi_flash_status 0x%x \n",buf[0]);    
}
#endif
#endif /* SKY */
    flash_spi_dev.flash_device_id = device_id = spi_flash_get_device_id((unsigned int*)&flashMemtype);
    flashManufacturer = device_id >> 8;

    switch( device_id >> 8 ) {
        case SSTPART:
            sectorsize = SECTOR_SIZE_4K;
            switch ((unsigned char)(device_id & 0x00ff)) {
                case ID_SST25VF016:
                    numsector = 512;
                    break;
                case ID_SST25VF032:
                    numsector = 1024;
                    break;
                case ID_SST25VF064:
                    numsector = 2048;
                    break;
            }
            break;

        case SPANPART:
            sectorsize = SECTOR_SIZE_64K;
            switch ((unsigned short)(device_id & 0x00ff)) {
                case ID_SPAN25FL016:
                    numsector = 32;
                    break;
                case ID_SPAN25FL032:
                    numsector = 64;
                    break;
                case ID_SPAN25FL064:
                case ID_SPAN25FL164:
                    numsector = 128;
                    break;
                case ID_SPAN25FL128:
                    numsector = 256;
                    break;
                case ID_SPAN25FL256:
                    numsector = 512; 
                    addr32 = TRUE;
                    break;
            }
            break;

        case EONPART:
            sectorsize = SECTOR_SIZE_64K;
            switch ((unsigned short)(device_id & 0x00ff)) {
                case ID_M25P16:
                    numsector = 32;
                    break;
                case ID_M25P32:
                    numsector = 64;
                    break;
                case ID_M25P64:
                    numsector = 128;
                    break;
                case ID_M25P128:
                    numsector = 256;
                    break;
            }
            break;

        case NUMONYXPART:
        case MACRONIXPART:
        case WBPART:
        case AMICPART:
            sectorsize = SECTOR_SIZE_4K;
            switch ((unsigned short)(device_id & 0x00ff)) {
                case ID_M25P16:
                    numsector = 512;
                    break;
                case ID_M25P32:
                    numsector = 1024;
                    break;
                case ID_M25P64:
                    numsector = 2048;
                    break;
                case ID_M25P128:
                    numsector = 4096;
                    break;
                case ID_M25P256:
                    addr32 = TRUE;
                    numsector = 8192;
#if 1
                    // WARNING_13588
                    // This will speed up large flash devices but 
                    // only enable this change if it will be in CFE and
                    // in EVERY version of Linux to be used.
                    // If this is not consistent between CFE and Linux,
                    // and an AUXFS is defined with a size that is not
                    // an exact multiple of 64KB, bad things will happen.
                    if( (((device_id >> 8) == NUMONYXPART) && flashMemtype == FLASH_MEMTYPE_BA) || 
                       ((device_id >> 8) == MACRONIXPART) )
		    {
		        /* use 64K sector size to speed up the large flash part image update*/ 
                        sectorsize = SECTOR_SIZE_64K;
                        numsector = 512;
		    }
#endif
                    break;
            }
            break;

        default:
#if (INC_BTRM_BUILD==1)
	    /* make best effort to allow btrm to read first 512KB for unknown dev */
	    sectorsize = SECTOR_SIZE_4K;
	    numsector = MAXSECTORS;
            break;
#else
            meminfo.addr = 0L;
            meminfo.nsect = 1;
            meminfo.sec[0].size = SECTOR_SIZE_4K;
            meminfo.sec[0].base = 0x00000;
            return FLASH_API_ERROR;
#endif
    }

    if( numsector >  MAXSECTORS )
        numsector = MAXSECTORS;

    meminfo.addr = 0L;
    meminfo.nsect = numsector;
    for (i = 0; i < numsector; i++) {
        meminfo.sec[i].size = sectorsize;
        meminfo.sec[i].base = basecount;
        basecount += meminfo.sec[i].size;
        count++;
    }
    totalSize = meminfo.sec[count-1].base + meminfo.sec[count-1].size;

    for( fnfi_ptr = fnfi; fnfi_ptr->fnfi_id != 0; fnfi_ptr++ ) {
        if( fnfi_ptr->fnfi_id == device_id ) {
            strcpy( flash_spi_dev.flash_device_name, fnfi_ptr->fnfi_name ); 
            break;
        }
    }

    /* check to see if multibit mode is supported */
    switch( device_id >> 8 ) {
        case SPANPART:
            if ( HS_SPI_BUS_NUM == spi_flash_busnum )
            {
               spi_flash_multibit_en();
            }
            break;

        default:
            break;
    }

#if !defined(_BCM968500_) && !defined(CONFIG_BCM968500)
    BcmSpi_SetFlashCtrl(spi_read_cmd, 1, spi_dummy_bytes, spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, spi_flash_clock, spi_multibit_en);
#endif

    return (FLASH_API_OK);
}

/*********************************************************************/
/* Flash_sector_erase_int() wait until the erase is completed before */
/* returning control to the calling function.  This can be used in   */
/* cases which require the program to hold until a sector is erased, */
/* without adding the wait check external to this function.          */
/*********************************************************************/

static int spi_flash_sector_erase_int(unsigned short sector)
{
    unsigned char buf[6];
    unsigned int cmd_length;
    unsigned int addr;

#ifndef _CFE_
    down(&spi_flash_lock);
#endif

    if ( addr32 )
         spi_flash_en4b();

    /* set device to write enabled */
    spi_flash_ub(sector);

    /* erase the sector  */
    addr = (unsigned int) spi_get_flash_memptr(sector);

    cmd_length = 0;
    if (meminfo.sec[sector].size == SECTOR_SIZE_4K)
        buf[cmd_length++] = spi_serase_cmd;
    else
        buf[cmd_length++] = spi_erase_cmd;

    if ( addr32 )
        buf[cmd_length++] = (unsigned char)((addr & 0xff000000) >> 24);

    buf[cmd_length++] = (unsigned char)((addr & 0x00ff0000) >> 16);
    buf[cmd_length++] = (unsigned char)((addr & 0x0000ff00) >> 8);
    buf[cmd_length++] = (unsigned char)(addr & 0x000000ff);

    /* check device is ready */
    if (my_spi_write(buf, cmd_length) == SPI_STATUS_OK) {
        while (spi_flash_status() != STATUS_READY);
    }

    spi_flash_reset();

    if ( addr32 )
    {
        spi_flash_disable4b();
    }

#ifndef _CFE_
    up(&spi_flash_lock);
#endif

    return(FLASH_API_OK);
}

/*********************************************************************/
/* spi_flash_en4b() will enable the flash part to start using 4 byte addressing mode. */
/*********************************************************************/

static int spi_flash_en4b(void)
{
    unsigned char buf[4];
    static int initialized = 0;

    if (flashManufacturer == SPANPART)
    {        
        int latencyCode = 0;
        struct spi_transfer xfer;

        if (!initialized)
        {
            initialized             = 1;
            spi_read_cmd            = FLASH_4READ_FAST;
            spi_dualOut_read_cmd    = FLASH_4READ_DOR;
            spi_dualIO_read_cmd     = FLASH_4READ_DIOR;
            spi_write_cmd           = FLASH_4PROG;
            spi_erase_cmd           = FLASH_4BERASE;
            spi_serase_cmd          = FLASH_4SERASE;
     
            memset(&xfer, 0, sizeof(struct spi_transfer));
            buf[0]           = FLASH_RDCR;
            xfer.tx_buf      = buf;
            xfer.rx_buf      = buf;
            xfer.len         = 1;
            xfer.speed_hz    = spi_flash_clock;
            xfer.prepend_cnt = 1;
            my_spi_read(&xfer);
            while (spi_flash_status() != STATUS_READY);
    
            latencyCode = (buf[0] & 0xC0) >> 6;
            if (latencyCode == 0x3)
            {
                spi_dummy_bytes = 0;  
            }
        }
    }
    else
    {
        if ( (flashManufacturer == NUMONYXPART) && (flashMemtype == FLASH_MEMTYPE_BA) && !initialized)
        {
            initialized             = 1;
            spi_read_cmd            = FLASH_4READ_FAST;
            spi_dualOut_read_cmd    = FLASH_4READ_DOR;
            spi_dualIO_read_cmd     = FLASH_4READ_DIOR;
            spi_write_cmd           = FLASH_4PROG;
            spi_erase_cmd           = FLASH_4BERASE;
            spi_serase_cmd          = FLASH_4SERASE;
        }
        /* set device to enter 4 byte addressing mode */
        buf[0] = FLASH_EN4B;
        my_spi_write(buf, 1);
        while (spi_flash_status() != STATUS_READY);
    }


    return(FLASH_API_OK);
}

/*********************************************************************/
/* spi_flash_disable4b() will disable the 4byte addressing mode on the flash part. */
/*********************************************************************/

static int spi_flash_disable4b(void)
{
    if (flashManufacturer != SPANPART)
    {       
        unsigned char buf[4];
        buf[0] = FLASH_EXIT4B; 
        my_spi_write(buf, 1);
        while (spi_flash_status() != STATUS_READY);
    }

    return(FLASH_API_OK);

}


/*********************************************************************/
/* flash_reset() will reset the flash device to reading array data.  */
/* It is good practice to call this function after autoselect        */
/* sequences had been performed.                                     */
/*********************************************************************/

static int spi_flash_reset(void)
{
    unsigned char buf[4];

    /* set device to write disabled */
    buf[0]        = FLASH_WRDI;
    my_spi_write(buf, 1);
    while (spi_flash_status() != STATUS_READY);

    return(FLASH_API_OK);
}

/*********************************************************************/
/* flash_read_buf() reads buffer of data from the specified          */
/* offset from the sector parameter.                                 */
/*********************************************************************/

#if (INC_BTRM_BOOT==0) && defined(_CFE_) && defined(CFG_RAMAPP)
static
#endif
int spi_flash_read_buf(unsigned short sector, int offset,
    unsigned char *buffer, int nbytes)
{
    unsigned char buf[READ_BUF_LEN_MAX];
    unsigned int cmd_length;
    unsigned int addr;
    int maxread;
    int size_read = 0;
    int multiOffset = 0;
    struct spi_transfer xfer;

    memset(&xfer, 0, sizeof(struct spi_transfer));
#ifndef _CFE_
    down(&spi_flash_lock);
#endif

    addr = (unsigned int) spi_get_flash_memptr(sector);
    addr += offset;
    
    if ( addr32 )
    {
        spi_flash_en4b();
    }

    while (nbytes > 0) {
        maxread = (nbytes < spi_max_op_len) ? nbytes : spi_max_op_len;

        cmd_length = 0;
        buf[cmd_length++] = spi_read_cmd;
        if ( addr32 )
            buf[cmd_length++] = (unsigned char)((addr & 0xff000000) >> 24);
        buf[cmd_length++] = (unsigned char)((addr & 0x00ff0000) >> 16);
        buf[cmd_length++] = (unsigned char)((addr & 0x0000ff00) >> 8);
        buf[cmd_length++] = (unsigned char)(addr & 0x000000ff);
        if (spi_dummy_bytes)
            buf[cmd_length++] = (unsigned char)0xff;

        if (spi_multibit_en)
        {
            if ( spi_read_cmd == spi_dualOut_read_cmd )
               multiOffset = cmd_length; /* only read data is multibit */
            else
               multiOffset = 1; /* addr and data is multibit */
        }

        xfer.tx_buf                 = buf;
        xfer.rx_buf                 = buffer;
        xfer.len                    = maxread;
        xfer.speed_hz               = spi_flash_clock;
        xfer.prepend_cnt            = cmd_length;
        xfer.multi_bit_en           = spi_multibit_en;
        xfer.multi_bit_start_offset = multiOffset;
        xfer.addr_len               = (addr32 ? 4 : 3);
        xfer.addr_offset            = 1;
        xfer.hdr_len                = cmd_length;
        xfer.unit_size              = 1;
        my_spi_read(&xfer);

        while (spi_flash_status() != STATUS_READY);

        buffer += maxread;
        nbytes -= maxread;
        addr += maxread;
        size_read += maxread;
    }

    if ( addr32 )
    {
        spi_flash_disable4b();
    }
    

#ifndef _CFE_
    up(&spi_flash_lock);
#endif

    return size_read;
}

/*********************************************************************/
/* flash_ub() places the flash into unlock bypass mode.  This        */
/* is REQUIRED to be called before any of the other unlock bypass    */
/* commands will become valid (most will be ignored without first    */
/* calling this function.                                            */
/*********************************************************************/

static int spi_flash_ub(unsigned short sector)
{
    unsigned char buf[4];
    struct spi_transfer xfer;
#ifndef SKY
//TMS this won't work if soft protection is enforced. Added additional unprotection funtion.
    do {
        buf[0]           = FLASH_RDSR;
        memset(&xfer, 0, sizeof(struct spi_transfer));
        xfer.tx_buf      = buf;
        xfer.rx_buf      = buf;
        xfer.len         = 1;
        xfer.speed_hz    = spi_flash_clock;
        xfer.prepend_cnt = 1;
        if (my_spi_read(&xfer) == SPI_STATUS_OK) {
            while (spi_flash_status() != STATUS_READY);
            if (buf[0] & (SR_BP3|SR_BP2|SR_BP1|SR_BP0)) {
                /* Sector is write protected. Unprotect it */
                buf[0] = FLASH_WREN;
                if (my_spi_write(buf, 1) == SPI_STATUS_OK) {
                    buf[0] = FLASH_WRST;
                    buf[1] = 0;
                    if (my_spi_write(buf, 2) == SPI_STATUS_OK)
                        while (spi_flash_status() != STATUS_READY);
                }
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    } while (1);
#endif /* SKY */
    /* set device to write enabled */
    buf[0] = FLASH_WREN;

    /* check device is ready */
    if (my_spi_write(buf, 1) == SPI_STATUS_OK) {
        while (spi_flash_status() != STATUS_READY);
        do {
            buf[0]           = FLASH_RDSR;
            memset(&xfer, 0, sizeof(struct spi_transfer));
            xfer.tx_buf      = buf;
            xfer.rx_buf      = buf;
            xfer.len         = 1;
            xfer.speed_hz    = spi_flash_clock;
            xfer.prepend_cnt = 1;
            if (my_spi_read(&xfer) == SPI_STATUS_OK) {
                while (spi_flash_status() != STATUS_READY);
                if (buf[0] & SR_WEN) {
                    break;
                }
            } 
            else {
                break;
            }
        } while (1);
    }

    return(FLASH_API_OK);
}

/*********************************************************************/
/* flash_write_buf() utilizes                                        */
/* the unlock bypass mode of the flash device.  This can remove      */
/* significant overhead from the bulk programming operation, and     */
/* when programming bulk data a sizeable performance increase can be */
/* observed.                                                         */
/*********************************************************************/

static int spi_flash_write(unsigned short sector, int offset,
    unsigned char *buffer, int nbytes)
{
    unsigned char buf[FLASH_PAGE_256 + 6];
    unsigned int cmd_length;
    unsigned int addr;
    int maxwrite;
    int pagelimit;
    int bytes_written = 0;

#ifndef _CFE_
    down(&spi_flash_lock);
#endif

    addr = (unsigned int) spi_get_flash_memptr(sector);
    addr += offset;

    if ( addr32 )
    {
        spi_flash_en4b();
    }    

    while (nbytes > 0) {
        spi_flash_ub(sector); /* enable write */

        cmd_length = 0;
        buf[cmd_length++] = spi_write_cmd;
        if ( addr32 )
            buf[cmd_length++] = (unsigned char)((addr & 0xff000000) >> 24);
        buf[cmd_length++] = (unsigned char)((addr & 0x00ff0000) >> 16);
        buf[cmd_length++] = (unsigned char)((addr & 0x0000ff00) >> 8);
        buf[cmd_length++] = (unsigned char)(addr & 0x000000ff);

        /* set length to the smaller of controller limit (spi_max_op_len) or nbytes
           spi_max_op_len considers both controllers */
        maxwrite = (nbytes < (spi_max_op_len - cmd_length)) ? nbytes : (spi_max_op_len - cmd_length);
        /* maxwrite is limit to page boundary */
        pagelimit = flash_page_size - (addr & (flash_page_size - 1));
        maxwrite = (maxwrite < pagelimit) ? maxwrite : pagelimit;

        memcpy(&buf[cmd_length], buffer, maxwrite);
        my_spi_write(buf, maxwrite + cmd_length);

        while (spi_flash_status() != STATUS_READY);

        buffer += maxwrite;
        nbytes -= maxwrite;
        addr += maxwrite;
        bytes_written += maxwrite;
    }

    spi_flash_reset();

    if ( addr32 )
    {
        spi_flash_disable4b();
    }

#ifndef _CFE_
    up(&spi_flash_lock);
#endif

    return( bytes_written );
}

/*********************************************************************/
/* flash_write_buf() utilizes                                        */
/* the unlock bypass mode of the flash device.  This can remove      */
/* significant overhead from the bulk programming operation, and     */
/* when programming bulk data a sizeable performance increase can be */
/* observed.                                                         */
/*********************************************************************/
static int spi_flash_write_buf(unsigned short sector, int offset,
    unsigned char *buffer, int nbytes)
{
    int ret;

    ret = spi_flash_write(sector, offset, buffer, nbytes);

#if (INC_BTRM_BOOT==0)
    if( ret == FLASH_API_ERROR )
        printk( "Flash write error. Verify failed\n" );
#endif

    return( ret );
}

/*********************************************************************/
/* Usefull funtion to return the number of sectors in the device.    */
/* Can be used for functions which need to loop among all the        */
/* sectors, or wish to know the number of the last sector.           */
/*********************************************************************/

static int spi_flash_get_numsectors(void)
{
    return meminfo.nsect;
}

/*********************************************************************/
/* flash_get_sector_size() is provided for cases in which the size   */
/* of a sector is required by a host application.  The sector size   */
/* (in bytes) is returned in the data location pointed to by the     */
/* 'size' parameter.                                                 */
/*********************************************************************/
#if (INC_BTRM_BOOT==0) && defined(_CFE_) && defined(CFG_RAMAPP)
static
#endif
int spi_flash_get_sector_size(unsigned short sector)
{
    return meminfo.sec[sector].size;
}

/*********************************************************************/
/* The purpose of get_flash_memptr() is to return a memory pointer   */
/* which points to the beginning of memory space allocated for the   */
/* flash.  All function pointers are then referenced from this       */
/* pointer.                                  */
/*                                                                   */
/* Different systems will implement this in different ways:          */
/* possibilities include:                                            */
/*  - A direct memory pointer                                        */
/*  - A pointer to a memory map                                      */
/*  - A pointer to a hardware port from which the linear             */
/*    address is translated                                          */
/*  - Output of an MMU function / service                            */
/*                                                                   */
/* Also note that this function expects the pointer to a specific    */
/* sector of the device.  This can be provided by dereferencing      */
/* the pointer from a translated offset of the sector from a         */
/* global base pointer (e.g. flashptr = base_pointer + sector_offset)*/
/*                                                                   */
/* Important: Many AMD flash devices need both bank and or sector    */
/* address bits to be correctly set (bank address bits are A18-A16,  */
/* and sector address bits are A18-A12, or A12-A15).  Flash parts    */
/* which do not need these bits will ignore them, so it is safe to   */
/* assume that every part will require these bits to be set.         */
/*********************************************************************/

static unsigned char *spi_get_flash_memptr(unsigned short sector)
{
    unsigned char *memptr = (unsigned char*)
        (meminfo.sec[sector].base);

    return (memptr);
}

static unsigned char *spi_flash_get_memptr(unsigned short sector)
{
    return( FLASH_BASE + spi_get_flash_memptr(sector) );
}

/*********************************************************************/
/* Flash_status return an appropriate status code                    */
/*********************************************************************/

static int spi_flash_status(void)
{
    unsigned char buf[4];
    int retry = 10;
    struct spi_transfer xfer;

    /* check device is ready */
    memset(&xfer, 0, sizeof(struct spi_transfer));
    do {
        buf[0]           = FLASH_RDSR;
        xfer.tx_buf      = buf;
        xfer.rx_buf      = buf;
        xfer.len         = 1;
        xfer.speed_hz    = spi_flash_clock;
        xfer.prepend_cnt = 1;
        if (my_spi_read(&xfer) == SPI_STATUS_OK) {
            if (!(buf[0] & SR_RDY)) {
                return STATUS_READY;
            }
        } else {
            return STATUS_ERROR;
        }
        OSL_DELAY(10);
    } while (retry--);

    return STATUS_TIMEOUT;
}

/*********************************************************************/
/* flash_get_device_id() return the device id of the component.      */
/*********************************************************************/

static unsigned short spi_flash_get_device_id(unsigned int* memtype)
{
    unsigned char buf[4];
    struct spi_transfer xfer;

    memset(&xfer, 0, sizeof(struct spi_transfer));
    buf[0]           = FLASH_RDID;
    xfer.tx_buf      = buf;
    xfer.rx_buf      = buf;
    xfer.len         = 3;
    xfer.speed_hz    = spi_flash_clock;
    xfer.prepend_cnt = 1;
    my_spi_read(&xfer);
    while (spi_flash_status() != STATUS_READY);
    *memtype = (unsigned int)(buf[1]);
    buf[1] = buf[2];

    /* return manufacturer code and device code */
    return( *(unsigned short *)&buf[0] );
}

/*********************************************************************/
/* The purpose of flash_get_blk() is to return a the block number */
/* for a given memory address.                                       */
/*********************************************************************/

static int spi_flash_get_blk(int addr)
{
    int blk_start, i;
    int last_blk = spi_flash_get_numsectors();
    int relative_addr = addr - (int) FLASH_BASE;

    for(blk_start=0, i=0; i < relative_addr && blk_start < last_blk; blk_start++)
        i += spi_flash_get_sector_size(blk_start);

    if( (unsigned int)i > (unsigned int)relative_addr ) {
        blk_start--;        // last blk, dec by 1
    } else {
        if( blk_start == last_blk )
        {
#if (INC_BTRM_BOOT==0)
            printk("Address is too big.\n");
#endif
            blk_start = -1;
        }
    }

    return( blk_start );
}

/************************************************************************/
/* The purpose of flash_get_total_size() is to return the total size of */
/* the flash                                                            */
/************************************************************************/
static int spi_flash_get_total_size(void)
{
    return totalSize;
}

static void spi_flash_multibit_en( void )
{
   unsigned char       buf[16];
   unsigned char       bufCmp[16];
   unsigned int        cmd_length;
   unsigned int        addr;
   struct spi_transfer xfer;
   int                 i;

   memset(&xfer, 0, sizeof(struct spi_transfer));

#ifndef _CFE_
   down(&spi_flash_lock);
#endif

   /* read 16 bytes of data form the first sector of flash
      this will be used to compare to the data read using the DOR
      and DIOR commands */
#ifndef SKY
   addr              = (unsigned int) spi_get_flash_memptr(NVRAM_SECTOR);
#else
   // Murthy This is to make sure we read sector 0 always
   addr              = (unsigned int) spi_get_flash_memptr(CFE_SECTOR);
#endif /* SKY */
   cmd_length        = 0;
   buf[cmd_length++] = spi_read_cmd;
   if ( addr32 )
   {
       spi_flash_en4b();
       buf[cmd_length++] = (unsigned char)((addr & 0xff000000) >> 24);
   }
   buf[cmd_length++] = (unsigned char)((addr & 0x00ff0000) >> 16);
   buf[cmd_length++] = (unsigned char)((addr & 0x0000ff00) >> 8);
   buf[cmd_length++] = (unsigned char)(addr & 0x000000ff);
   if ( spi_dummy_bytes ) 
      buf[cmd_length++] = (unsigned char)0xff;

   xfer.tx_buf      = buf;
   xfer.rx_buf      = buf;
   xfer.len         = 16;
   xfer.speed_hz    = spi_flash_clock;
   xfer.prepend_cnt = cmd_length;
   my_spi_read(&xfer);
   while (spi_flash_status() != STATUS_READY);

   /* if the data read above is all 1's then we cannot determine if multibit is
      supported by the flash so just return */
   for ( i = 0; i < 16; i++)
   {
      if ( buf[i] != 0xFF )
      {
         break;
      }
      if ( 15 == i )
      {
          if ( addr32 )
          {
              spi_flash_disable4b();
          }
      
#ifndef _CFE_

          up(&spi_flash_lock);
#endif      
         return;
      }
   }

   /* try the DIOR instruction
      if the data matches the previously read data then it is supported */
#ifndef SKY
   addr              = (unsigned int) spi_get_flash_memptr(NVRAM_SECTOR);
#else
   // Murthy This is to make sure we read sector 0 always
   addr              = (unsigned int) spi_get_flash_memptr(CFE_SECTOR);
#endif
   cmd_length        = 0;
   bufCmp[cmd_length++] = spi_dualIO_read_cmd;

   if ( addr32 )
      bufCmp[cmd_length++] = (unsigned char)((addr & 0xff000000) >> 24);
   
   bufCmp[cmd_length++] = (unsigned char)((addr & 0x00ff0000) >> 16);
   bufCmp[cmd_length++] = (unsigned char)((addr & 0x0000ff00) >> 8);
   bufCmp[cmd_length++] = (unsigned char)(addr & 0x000000ff);
   if (spi_dummy_bytes)
   {
       bufCmp[cmd_length++] = (unsigned char)0xff;
   }

   xfer.tx_buf                 = bufCmp;
   xfer.rx_buf                 = bufCmp;
   xfer.len                    = 16;
   xfer.speed_hz               = spi_flash_clock;
   xfer.prepend_cnt            = cmd_length;
   xfer.multi_bit_en           = 1;
   xfer.multi_bit_start_offset = 1;
   my_spi_read(&xfer);
   while (spi_flash_status() != STATUS_READY);

   if ( 0 == memcmp(buf, bufCmp, 16) )
   {
      /* DIOR command is supported */
      spi_read_cmd    = spi_dualIO_read_cmd;
      spi_multibit_en = 1;
      if ( addr32 )
      {
          spi_flash_disable4b();
      }
      
#ifndef _CFE_
      up(&spi_flash_lock);
#endif
      return;
   }

   /* try the DOR command
      if the data matches the previously read data then it is supported */
#ifndef SKY
   addr              = (unsigned int) spi_get_flash_memptr(NVRAM_SECTOR);
#else
   // Murthy This is to make sure we read sector 0 always
   addr              = (unsigned int) spi_get_flash_memptr(CFE_SECTOR);
#endif
   cmd_length        = 0;
   bufCmp[cmd_length++] = spi_dualOut_read_cmd;
   if ( addr32 )
      bufCmp[cmd_length++] = (unsigned char)((addr & 0xff000000) >> 24);
   bufCmp[cmd_length++] = (unsigned char)((addr & 0x00ff0000) >> 16);
   bufCmp[cmd_length++] = (unsigned char)((addr & 0x0000ff00) >> 8);
   bufCmp[cmd_length++] = (unsigned char)(addr & 0x000000ff);
   if (spi_dummy_bytes)
   {
       bufCmp[cmd_length++] = (unsigned char)0xff;
   }
   
   xfer.tx_buf                 = bufCmp;
   xfer.rx_buf                 = bufCmp;
   xfer.len                    = 16;
   xfer.speed_hz               = spi_flash_clock;
   xfer.prepend_cnt            = cmd_length;
   xfer.multi_bit_en           = 1;
   xfer.multi_bit_start_offset = cmd_length;
   my_spi_read(&xfer);
   while (spi_flash_status() != STATUS_READY);

   if ( 0 == memcmp(buf, bufCmp, 16) )
   {
      /* DOR command is supported */
      spi_read_cmd    = spi_dualOut_read_cmd;
      spi_multibit_en = 1;
      if ( addr32 )
      {
          spi_flash_disable4b();
      }
      
#ifndef _CFE_
      up(&spi_flash_lock);
#endif
      return;
   }
   
   if ( addr32 )
   {
       spi_flash_disable4b();
   }

#ifndef _CFE_
    up(&spi_flash_lock);
#endif

}

#ifndef _CFE_
static int __init BcmSpiflash_init(void)
{
    int flashType;
    int spiCtrlState;

    /* If serial flash is present then register the device. Otherwise do nothing */
    flashType  = flash_get_flash_type();
    if ((FLASH_IFC_SPI == flashType) || (FLASH_IFC_HS_SPI == flashType))
    {
        /* register the device */
        spiCtrlState = SPI_CONTROLLER_STATE_DEFAULT;
        if ( spi_flash_clock > SPI_CONTROLLER_MAX_SYNC_CLOCK )
           spiCtrlState |= SPI_CONTROLLER_STATE_ASYNC_CLOCK;
        
        BcmSpiReserveSlave2(spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, spi_flash_clock, 
                            SPI_MODE_DEFAULT, spiCtrlState);
        bSpiFlashSlaveRes = TRUE;
        spi_max_op_len    = BcmSpi_GetMaxRWSize( spi_flash_busnum, 1 );
    }

    return 0;
}
module_init(BcmSpiflash_init);

static void __exit BcmSpiflash_exit(void)
{
    bSpiFlashSlaveRes = FALSE;
}
module_exit(BcmSpiflash_exit);
#endif


#ifdef SKY
/*********************************************************************/
/* spi_flash_write_protect_serialisation_sectors()                   */
/* write protects the serialisation sectors                          */
/* sector                                                            */
/*********************************************************************/
#ifdef ENABLE_SERIALISATION_WP
static int spi_flash_write_protect_serialisation_sectors()
{
#if 0
    uint8 buf[4];
	int   stat = SPI_STATUS_OK;
	uint8 status_byte = 0;  //Byte for writing to RDSR
	uint8 config_byte = 0;  //Byte for writing to RCR
	bool written_locking_data = false;
    struct spi_transfer xfer;
    memset(&xfer, 0, sizeof(struct spi_transfer));
	
	do
	{
		buf[0] = FLASH_RDSR;//read status
		xfer.tx_buf      = buf;
        xfer.rx_buf      = buf;
        xfer.len         = 1;
        xfer.speed_hz    = spi_flash_clock;
        xfer.prepend_cnt = 1;
		if (my_spi_read(&xfer) == SPI_STATUS_OK)
		{
			while (spi_flash_status() != STATUS_READY)
				;

			if ((buf[0] & SR_SRWD) != SR_SRWD) //Check to see if the write protect bits are already hardware locked.
			{
				status_byte = buf[0];  //Set status byte to the current contents of the flash status register
				/* SRWD is low so we can edit the BP bits to write protect sectors */
				printk("Status register]:0x%x\n",buf[0]);
			    memset(&xfer, 0, sizeof(struct spi_transfer));
				buf[0] = FLASH_RCR;
				xfer.tx_buf      = buf;
		        xfer.rx_buf      = buf;
		        xfer.len         = 1;
		        xfer.speed_hz    = spi_flash_clock;
		        xfer.prepend_cnt = 1;
				if (my_spi_read(&xfer) == SPI_STATUS_OK) //Need to check the TBPROT setting to find out if we are using high address/low address
				{
					while (spi_flash_status() != STATUS_READY);

					printk("Config register: 0x%x\n",buf[0]);

					printk("Comparison value: 0x%x\n",(buf[0] & CR_TBPROT));
					if((buf[0] & CR_TBPROT) == CR_TBPROT)//If flash is set to use high address then we need to change it to low address as we
					{                      // are trying to protect the first two sectors

						printk("Setting up the config byte\n");
						config_byte = buf[0]; //Set config byte to the same as the current contents of the flash config register.
						buf[0] = FLASH_WREN;
						if(my_spi_write(buf, 1) == SPI_STATUS_OK) //Enable writes
						{
                            buf[0] = FLASH_WRST;
                            buf[1] = status_byte;
                            config_byte |= CR_TBPROT;
                            buf[2] = config_byte;

                            stat = my_spi_write(buf, 3);//Write new config byte to the flash device.  THIS IS PERMANANT AND CANNOT BE UNDONE
                            while (spi_flash_status() != STATUS_READY);

//                            //DHP TEMP CODE for testing //////////////
//                            buf[0] = FLASH_RCR;
//                            my_spi_read(buf,1,1);
//                            printk("config register after write: 0x%x\n",buf[0]);
//                            /////////////////////////////////////////
						}
					}

					if(stat == SPI_STATUS_OK)  //Config byte should be correctly configured at this point so now we do the RDSR
					{
						buf[0] = FLASH_WREN;
						if(my_spi_write(buf, 1) == SPI_STATUS_OK) //Enable writes
						{
							buf[0] = FLASH_WRST; //WRR command to write to status and config registers.
							//Need to reset the BP bits.
							status_byte &= ~SR_BP0;
							status_byte &= ~SR_BP1;
							status_byte &= ~SR_BP2;

							buf[1] = status_byte |= (SR_BP0 | SR_SRWD); //This should protect the first two sectors and enable HW protection mode.
							buf[2] = config_byte;

							///DHP --tmp code!!!!
//							written_locking_data = true;
							printk("Config byte: 0x%x\n",config_byte);
							printk("Status byte: 0x%x\n",status_byte);
//							break;
							/////////////////////
							if(my_spi_write(buf, 3)== SPI_STATUS_OK) //THIS IS PERMANANT AND CANNOT BE UNDONE
							{
								written_locking_data = true;
							}
							else
							{
								written_locking_data = false;
							}


							while (spi_flash_status() != STATUS_READY);

						}
					}//if(stat == SPI_STATUS_OK)
				}
			}//if (!(buf[0] & SR_SRWD)) //Check to see if the write protect bits are already hardware locked.
			else
			{
				if(written_locking_data)
				{
				    stat = 0;
				}
				else
				{
				    stat -1;
				}
				printk("Device is already hardware locked, cannot modify the SR_BP bits\n");
				break;
			}
		}//if (my_spi_read(buf, 1, 1) == SPI_STATUS_OK)
		else
		{
			stat = -1;
			break;
		}
	} while (1);
	return stat;
#else
    printk("tms> spi_flash_write_protect_serialisation_sectors() DISABLED !!!\n");    
#endif
}
#else
static int spi_flash_software_unprotect()
{
    //TMS code taken from spi_flash_ub() function.
    struct spi_transfer xfer;
    unsigned char buf[4];
#ifndef _CFE_
	down(&spi_flash_lock);	
#endif
    memset(&xfer, 0, sizeof(struct spi_transfer));
    do {
       buf[0] = FLASH_RDSR;
        xfer.tx_buf      = buf;
        xfer.rx_buf      = buf;
        xfer.len         = 1;
        xfer.speed_hz    = spi_flash_clock;
        xfer.prepend_cnt = 1;
       if (my_spi_read(&xfer) == SPI_STATUS_OK)
       {
           while (spi_flash_status() != STATUS_READY);

           if (buf[0] & (SR_BP2|SR_BP1|SR_BP0))
           {
               /* Sector is write protected. Unprotect it */
               buf[0] = FLASH_WREN;
               if (my_spi_write(buf, 1) == SPI_STATUS_OK)
               {
                   buf[0] = FLASH_WRST;
                   buf[1] = 0; //TODO clear only SR_BP0-2 bits
                   if (my_spi_write(buf, 2) == SPI_STATUS_OK)
                       while (spi_flash_status() != STATUS_READY);
               }
           }
           else
           {
               break;
           }
       }
       else
       {
           break;
       }
    } while (0); //TMS avoid looping forever if flash protection is used
#ifndef _CFE_
    up(&spi_flash_lock);
#endif

    //printk("tms> spi_flash_software_unprotect status 0x%x\n",buf[0]);
    return 0;
}


static int spi_flash_is_write_protected() {
    uint8 buf[4];
    struct spi_transfer xfer;
	int protected = 1;

    memset(&xfer, 0, sizeof(struct spi_transfer));

#ifndef _CFE_
    down(&spi_flash_lock);
#endif

	buf[0] = FLASH_RDSR;//read status
	xfer.tx_buf      = buf;
	xfer.rx_buf      = buf;
	xfer.len         = 1;
	xfer.speed_hz    = spi_flash_clock;
	xfer.prepend_cnt = 1;

	if (my_spi_read(&xfer) == SPI_STATUS_OK)
	{
		printk("Status register]:0x%x\n",buf[0]);
		protected = ( buf[0] & SR_BP0 ) ? 1 : 0;
		while (spi_flash_status() != STATUS_READY);                            
	}

#ifndef _CFE_
    up(&spi_flash_lock);
#endif

	return protected;
}


static int spi_flash_software_write_protect()
{
    uint8 buf[4];
	int   stat = SPI_STATUS_OK;
	uint8 status_byte = 0;  //Byte for writing to RDSR
	uint8 config_byte = 0;  //Byte for writing to RCR
    struct spi_transfer xfer;
    //printk("tms> spi_flash_software_write_protect()\n");
    memset(&xfer, 0, sizeof(struct spi_transfer));
#ifndef _CFE_
    down(&spi_flash_lock);
#endif
	{
		buf[0] = FLASH_RDSR;//read status
        xfer.tx_buf      = buf;
        xfer.rx_buf      = buf;
        xfer.len         = 1;
        xfer.speed_hz    = spi_flash_clock;
        xfer.prepend_cnt = 1;
		if (my_spi_read(&xfer) == SPI_STATUS_OK)
		{
			while (spi_flash_status() != STATUS_READY);

				status_byte = buf[0];  //Set status byte to the current contents of the flash status register
				//printk("Status register]:0x%x\n",buf[0]);
			    memset(&xfer, 0, sizeof(struct spi_transfer));
				buf[0] = FLASH_RCR;
				xfer.tx_buf      = buf;
		        xfer.rx_buf      = buf;
        		xfer.len         = 1;
		        xfer.speed_hz    = spi_flash_clock;
        		xfer.prepend_cnt = 1;
				if (my_spi_read(&xfer) == SPI_STATUS_OK) //Need to check the TBPROT setting to find out if we are using high address/low address
				{
					while (spi_flash_status() != STATUS_READY);
                    config_byte = buf[0]; //Set config byte to the same as the current contents of the flash config register.

					//printk("Config register: 0x%x\n",buf[0]);
                
					if(stat == SPI_STATUS_OK)  //Config byte should be correctly configured at this point so now we do the RDSR
					{
						buf[0] = FLASH_WREN;
						if(my_spi_write(buf, 1) == SPI_STATUS_OK) //Enable writes
						{
							buf[0] = FLASH_WRST; //WRR command to write to status and config registers.
							//Need to reset the BP bits.
							status_byte &= ~SR_BP0;
							status_byte &= ~SR_BP1;
							status_byte &= ~SR_BP2;

							buf[1] = status_byte | SR_BP0 ; // just BP bits | SR_SRWD will cause hw protection

//							buf[2] = config_byte | CR_TBPROT | CR_FREEZE ;
							buf[2] = config_byte | CR_TBPROT ;


							stat = my_spi_write(buf, 3);
							while (spi_flash_status() != STATUS_READY);
                            buf[0] = FLASH_RCR;
						    memset(&xfer, 0, sizeof(struct spi_transfer));
					        xfer.tx_buf      = buf;
					        xfer.rx_buf      = buf;
					        xfer.len         = 1;
					        xfer.speed_hz    = spi_flash_clock;
					        xfer.prepend_cnt = 1;
                            my_spi_read(&xfer);                            
                            while (spi_flash_status() != STATUS_READY);
                            //printk("config register after write: 0x%x\n",buf[0]);
						    memset(&xfer, 0, sizeof(struct spi_transfer));
			                buf[0] = FLASH_RDSR;                            
							xfer.tx_buf      = buf;
					        xfer.rx_buf      = buf;
			        		xfer.len         = 1;
					        xfer.speed_hz    = spi_flash_clock;
			        		xfer.prepend_cnt = 1;            
                            my_spi_read(&xfer);                            
                            while (spi_flash_status() != STATUS_READY);                            
                            //printk("status register after write: 0x%x\n",buf[0]);

						}
					}
				}
		}
		else
		{
			stat = -1;
		}
	};
#ifndef _CFE_
    up(&spi_flash_lock);
#endif

	return stat;


}
#endif
#endif /* SKY */