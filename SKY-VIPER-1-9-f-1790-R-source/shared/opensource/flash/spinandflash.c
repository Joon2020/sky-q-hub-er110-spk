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
#include "lib_types.h"
#include "lib_printf.h"
#include "lib_string.h"
#include "bcm_map_part.h"  

#include "bcmtypes.h"
#include "bcm_hwdefs.h"
#include "flash_api.h"
#include "bcmSpiRes.h"

#define MAX_RETRY           3

//#undef DEBUG_NAND
//#define DEBUG_NAND
#if defined(DEBUG_NAND) && defined(CFG_RAMAPP)
#define DBG_PRINTF printf
#else
#define DBG_PRINTF(...)
#endif


/* Command codes for the flash_command routine */
#define FLASH_PROG          0x02    /* program load data to cache */
#define FLASH_READ          0x03    /* read data from cache */
#define FLASH_WRDI          0x04    /* reset write enable latch */
#define FLASH_WREN          0x06    /* set write enable latch */
#define FLASH_READ_FAST     0x0B    /* read data from cache */
#define FLASH_GFEAT         0x0F    /* get feature option */
#define FLASH_PEXEC         0x10    /* program cache data to memory array*/
#define FLASH_PREAD         0x13    /* read from memory array to cache */
#define FLASH_SFEAT         0x1F    /* set feature option */
#define FLASH_PROG_RAN      0x84    /* program load data to cache at offset*/
#define FLASH_BERASE        0xD8    /* erase one block in memory array */
#define FLASH_RDID          0x9F    /* read manufacturer and product id */
#define FLASH_RESET         0xFF    /* reset flash */

#define FEATURE_PROT_ADDR   0xA0
#define FEATURE_FEAT_ADDR   0xB0
#define FEATURE_STAT_ADDR   0xC0

/* Feature protectin bit defintion */
#define PROT_BRWD           0x80
#define PROT_BP_MASK        0x38
#define PROT_BP_SHIFT       0x3
#define PROT_BP_ALL         0x7
#define PROT_BP_NONE        0x0
/* giga device only */
#define PROT_INV            0x04
#define PROT_CMP            0x02

/* Feature feature bit defintion */
#define FEAT_OPT_EN         0x40
#define FEAT_ECC_EN         0x10
/* giga device only */
#define FEAT_BBI            0x04
#define FEAT_QE             0x01

/* Feature status bit definition */
#define STAT_ECC_MASK         0x30
#define STAT_ECC_SHIFT        4
#define STAT_ECC_GOOD         0
#define STAT_ECC_CORR1        1     /* corrected, bit number 1 to 7*/
#define STAT_ECC_UNCORR       2     /* uncorrectable error*/
#define STAT_ECC_CORR2        3     /* reserved for micron, corrected bit 8 for gigadevice*/
#define STAT_PFAIL            0x8   /* program fail*/
#define STAT_EFAIL            0x4   /* erase fail*/
#define STAT_WEL              0x2   /* write enable latch*/
#define STAT_OIP              0x1   /* operatin in progress*/

/* Return codes from flash_status */
#define STATUS_READY        0       /* ready for action */
#define STATUS_BUSY         1       /* operation in progress */
#define STATUS_TIMEOUT      2       /* operation timed out */
#define STATUS_ERROR        3       /* unclassified but unhappy status */

/* Micron manufacturer ID */
#define MICRONPART          0x2C
#define ID_MT29F1G01        0x12
#define ID_MT29F2G01        0x22
#define ID_MT29F4G01        0x32
 
/* Giga Device manufacturer ID */
#define GIGADEVPART         0xC8
#define ID_GD5F1GQ4         0xF1


#define SPI_MAKE_ID(A,B)    \
    (((unsigned short) (A) << 8) | ((unsigned short) B & 0xff))

#define SPI_MANU_ID(devId)    \
    ((unsigned char)((devId>>8)&0xff))

#define SPI_NAND_DEVICES                                              \
    {{SPI_MAKE_ID(GIGADEVPART, ID_GD5F1GQ4), "GigaDevice GD5F1GQ4"},   \
     {SPI_MAKE_ID(MICRONPART, ID_MT29F1G01), "Micron MT29F1G01"},      \
     {SPI_MAKE_ID(MICRONPART, ID_MT29F2G01), "Micron MT29F2G01"},      \
     {SPI_MAKE_ID(MICRONPART, ID_MT29F4G01), "Micron MT29F4G01"},      \
     {0,""}                                                            \
    }

#define SPI_NAND_MANUFACTURERS           \
  {{MICRONPART, "Micron"},               \
   {GIGADEVPART, "GigaDevice"},          \
   {0,""}                                \
  }

typedef struct CfeSpiNandChip
{
    char *chip_name;
    unsigned long chip_device_id;
    unsigned long chip_total_size;
    unsigned long chip_num_blocks;
    unsigned long chip_block_size;
    unsigned long chip_page_size;
    unsigned long chip_spare_size;
    unsigned long chip_spare_step_size;
    unsigned char *chip_spare_mask;
    unsigned long chip_bi_index_1;
    unsigned long chip_bi_index_2;
    unsigned short chip_block_shift;
    unsigned short chip_page_shift;
    unsigned short chip_num_plane;
} CFE_SPI_NAND_CHIP, *PCFE_SPI_NAND_CHIP;


CFE_SPI_NAND_CHIP g_spinand_chip;

/** Prototypes. **/
#if defined(CFG_RAMAPP)
int spi_nand_init(flash_device_info_t **flash_info);
static int spi_nand_write_buf(unsigned short blk, int offset, unsigned char *buffer, int len);
static int spi_nand_write_enable(void);
static void spi_nand_set_feat(unsigned char feat_addr, unsigned char feat_val);
static int spi_nand_is_blk_bad(PCFE_SPI_NAND_CHIP pchip, unsigned short blk);
static int spi_nand_write_enable(void);
static int spi_nand_write_disable(void);
static void spi_nand_row_addr(PCFE_SPI_NAND_CHIP pchip, unsigned int page_addr, unsigned char* buf);
static void spi_nand_col_addr(PCFE_SPI_NAND_CHIP pchip, unsigned int page_addr, unsigned int page_offset, unsigned char* buf);
static int spi_nand_sector_erase_int(unsigned short blk);
static unsigned short spi_nand_get_device_id(void);
static int spi_nand_get_total_size(void);
static int spi_nand_get_blk(int addr);
static int spi_nand_wel(void);
static void spi_nand_copy_from_spare(unsigned char *buffer, int numbytes);
static unsigned char *spi_nand_get_memptr(unsigned short blk);
#else
int spi_nand_init(void);
#endif

static int spi_read( struct spi_transfer *xfer );
static int spi_write( unsigned char *msg_buf, int nbytes );
static int spi_nand_read_cfg(PCFE_SPI_NAND_CHIP pchip);

static int spi_nand_status(void);
static int spi_nand_reset(void);
static int spi_nand_ready(void);
static int spi_nand_ecc(void);
static int spi_nand_read_page(PCFE_SPI_NAND_CHIP pchip, unsigned long start_addr, unsigned char *buffer, int len);
int spi_nand_read_buf(unsigned short blk, int offset, unsigned char *buffer, int len);

static int spi_nand_is_blk_cleanmarker(PCFE_SPI_NAND_CHIP pchip, unsigned long start_addr, int write_if_not);
static int spi_nand_get_feat(unsigned char feat_addr);

int spi_nand_get_sector_size(unsigned short blk);
int spi_nand_get_numsectors(void);

extern void cfe_usleep(int usec);

#if defined(CFG_RAMAPP)
/** Variables. **/
static flash_device_info_t flash_spi_nand_dev =
    {
        0xffff,
        FLASH_IFC_SPINAND,
        "",
        spi_nand_sector_erase_int,
        spi_nand_read_buf,
        spi_nand_write_buf,
        spi_nand_get_numsectors,
        spi_nand_get_sector_size,
        spi_nand_get_memptr,
        spi_nand_get_blk,
        spi_nand_get_total_size
    };

static struct flash_name_from_id fnfi[] = SPI_NAND_DEVICES;
#endif

/* the controller will handle operati0ns that are greater than the FIFO size
   code that relies on READ_BUF_LEN_MAX, READ_BUF_LEN_MIN or spi_max_op_len
   could be changed */
#define READ_BUF_LEN_MAX   544    /* largest of the maximum transaction sizes for SPI */
#define READ_BUF_LEN_MIN   60     /* smallest of the maximum transaction sizes for SPI */
/* this is the slave ID of the SPI flash for use with the SPI controller */
#define SPI_FLASH_SLAVE_DEV_ID    0
/* clock defines for the flash */
#define SPI_FLASH_DEF_CLOCK       781000
#define SPARE_MAX_SIZE          (27 * 16)
#define CTRLR_CACHE_SIZE        512
#define ECC_MASK_BIT(ECCMSK, OFS)   (ECCMSK[OFS / 8] & (1 << (OFS % 8)))

/* default to smallest transaction size - updated later */
static unsigned int spi_max_op_len = READ_BUF_LEN_MIN; 
static int spi_dummy_bytes         = 0;

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

static int spi_read(struct spi_transfer *xfer)
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

static int spi_write(unsigned char *msg_buf, int nbytes)
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

static int spi_nand_read_cfg(PCFE_SPI_NAND_CHIP pchip)
{
    int ret = FLASH_API_OK;

    /* currently all the support chip has same page size, block size and spare settin, 
       over write it in the switch case if new chip has different size */
    pchip->chip_page_size = 2048;
    pchip->chip_page_shift = 11;
    pchip->chip_block_size = 128*1024;   //64 page per block 
    pchip->chip_block_shift = 17;
    pchip->chip_spare_size = 64;
    pchip->chip_spare_step_size = 16;
   
    switch(pchip->chip_device_id)
    {
    case SPI_MAKE_ID(GIGADEVPART, ID_GD5F1GQ4):
    case SPI_MAKE_ID(MICRONPART, ID_MT29F1G01):
        pchip->chip_num_blocks = 1024;       //1024 blocks total
        pchip->chip_total_size = 128*1024*1024; 
        pchip->chip_num_plane = 1;
        break;

    case SPI_MAKE_ID(MICRONPART, ID_MT29F2G01):
        pchip->chip_num_blocks = 2048;       //2048 blocks total
        pchip->chip_total_size = 256*1024*1024; 
        pchip->chip_num_plane = 2;
        break;

    case SPI_MAKE_ID(MICRONPART, ID_MT29F4G01):
        pchip->chip_num_blocks = 4096;       //2048 blocks total
        pchip->chip_total_size = 512*1024*1024; 
        pchip->chip_num_plane = 2;
        break;

    default:
        printf("unsupported spi nand device id 0x%x, use default cfg setting\n", pchip->chip_device_id);
        pchip->chip_num_blocks = 1024;       //1024 blocks total
        pchip->chip_total_size = 128*1024*1024; 
        pchip->chip_num_plane = 1;
        break;
    }

    /* setup ecc layout info here */
    return ret;
}
#if defined(CFG_RAMAPP)
int spi_nand_init(flash_device_info_t **flash_info)
{
    struct flash_name_from_id *fnfi_ptr;
#else
int spi_nand_init(void)
{
#endif
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;  
    int spiCtrlState;
    int ret = FLASH_API_OK;
    /* micron MT29F1G01 only support up to 50MHz, update to 50Mhz if it is more than that */
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
            spi_flash_clock = 50000000;
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
       spi_flash_clock = 50000000; 
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

    /* retrieve the maximum read/write transaction length from the SPI controller */
    spi_max_op_len = BcmSpi_GetMaxRWSize( spi_flash_busnum, 1 );

    /* set the controller state, spi_mode_0 */
    spiCtrlState = SPI_CONTROLLER_STATE_DEFAULT;
    if ( spi_flash_clock > SPI_CONTROLLER_MAX_SYNC_CLOCK )
       spiCtrlState |= SPI_CONTROLLER_STATE_ASYNC_CLOCK;
    BcmSpi_SetCtrlState(spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, SPI_MODE_DEFAULT, spiCtrlState);

    BcmSpi_SetFlashCtrl(0x3, 1, spi_dummy_bytes, spi_flash_busnum, SPI_FLASH_SLAVE_DEV_ID, spi_flash_clock, 0);

    spi_nand_reset();
#if defined(CFG_RAMAPP)
    pchip->chip_device_id = flash_spi_nand_dev.flash_device_id = spi_nand_get_device_id();

    for( fnfi_ptr = fnfi; fnfi_ptr->fnfi_id != 0; fnfi_ptr++ )
    {
        if( fnfi_ptr->fnfi_id == flash_spi_nand_dev.flash_device_id )
        {
            strcpy(flash_spi_nand_dev.flash_device_name, fnfi_ptr->fnfi_name);
            break;
        }
    }

    *flash_info = &flash_spi_nand_dev;
#endif
    if( (ret = spi_nand_read_cfg(pchip)) !=  FLASH_API_OK )
        return ret;
#if 0
    spi_nand_init_cleanmarker(pchip);
    /* If the first block's spare area is not a JFFS2 cleanmarker,
     * initialize all block's spare area to a cleanmarker.
     */
    if( !spi_nand_is_blk_cleanmarker(pchip, 0, 0) )
        ret = spi_nand_initialize_spare_area(pchip, 0);
#endif
        
    return ret;
}

/*********************************************************************/
/*  row address is 24 bit length. so buf must be at least 3 bytes. */
/*  For gigadevcie GD5F1GQ4 part(2K page size, 64 page per block and 1024 blocks) */
/*  Row Address. RA<5:0> selects a page inside a block, and RA<15:6> selects a block and */
/*  first byte is dummy byte */
/*********************************************************************/
static void spi_nand_row_addr(PCFE_SPI_NAND_CHIP pchip, unsigned int page_addr, unsigned char* buf)
{
    buf[0] = (unsigned char)(page_addr>>(pchip->chip_page_shift+16)); //dummy byte 
    buf[1] = (unsigned char)(page_addr>>(pchip->chip_page_shift+8));
    buf[2] = (unsigned char)(page_addr>>(pchip->chip_page_shift));
     
    return;
}

/*********************************************************************/
/*  column address select the offset within the page. For gigadevcie GD5F1GQ4 part(2K page size and 2112 with spare) */
/*  is 12 bit length. so buf must be at least 2 bytes. The 12 bit address is capable of address from 0 to 4095 bytes */
/*  however only byte 0 to 2111 are valid. */
/*********************************************************************/

static void spi_nand_col_addr(PCFE_SPI_NAND_CHIP pchip,  unsigned int page_addr, unsigned int page_offset, unsigned char* buf)
{

    page_offset = page_offset&((1<<(pchip->chip_page_shift+1))-1);  /* page size + spare area size */
    /* the upper 4 bits of buf[0] is either wrap bits for gigadevice or dummy bit[3:1] + plane select bit[0] for micron
     */
    if( SPI_MANU_ID(pchip->chip_device_id) == MICRONPART )
    {
        /* setup plane bit if more than one plane. otherwise that bit is always 0 */
        if( pchip->chip_num_plane > 1 )
            buf[0] = (unsigned char)(((page_offset>>8)&0xf)|((page_addr>>pchip->chip_block_shift)&0x1)<<4); //plane bit is the first bit of the block number RowAddr[6]
        else
           buf[0] = (unsigned char)((page_offset>>8)&0xf);
    }
    else
    {
        /* use default wrap option 0, wrap length 2112 */
        buf[0] = (unsigned char)((page_offset>>8)&0xf);
    }
    buf[1] = (unsigned char)(page_offset);
     
    return;
}

/*********************************************************************/
/* some devices such as Micron MT29F1G01 require explicit reset before */
/* access to the device.                                             */
/*********************************************************************/

static int spi_nand_reset(void)
{
    unsigned char buf[4];
#if defined(CONFIG_BRCM_IKOS)
    unsigned int i;
    for( i = 0; i < 250; i++);
#else
    cfe_usleep(300);
#endif

    buf[0]        = FLASH_RESET;
    spi_write(buf, 1);

#if defined(CONFIG_BRCM_IKOS)
    for( i = 0; i < 3000; i++);
#else
    /* device is availabe after 5ms */ 
    cfe_usleep(5000);
#endif

    return(FLASH_API_OK);
}

/***************************************************************************
 * Function Name: spi_nand_read_page
 * Description  : Reads up to a NAND block of pages into the specified buffer.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int spi_nand_read_page(PCFE_SPI_NAND_CHIP pchip, unsigned long start_addr,
    unsigned char *buffer, int len)
{
    unsigned char buf[READ_BUF_LEN_MAX];
    int ret = FLASH_API_OK;
    int maxread, nbytes;
    struct spi_transfer xfer;


    if( len <= pchip->chip_block_size )
    {
        unsigned int page_addr = start_addr & ~(pchip->chip_page_size - 1);
        unsigned int offset = start_addr - page_addr;
        unsigned int size = pchip->chip_page_size - offset;
    
        if(size > len)
            size = len;

        do
	{
            DBG_PRINTF("spi_nand_read_page - page addr 0x%x, offset 0x%x, size 0x%x\n", page_addr, offset, size);

            /* The PAGE READ (13h) command transfers the data from the NAND Flash array to the 
             * cache register.  The PAGE READ command requires a 24-bit address consisting of 
             * 8 dummy bits followed by a 16-bit block/page address.                        
             */
            buf[0] = FLASH_PREAD;
            spi_nand_row_addr(pchip, page_addr, buf+1);
            DBG_PRINTF("spi_nand_read_page - spi cmd 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0],buf[1],buf[2], buf[3]);
            spi_write(buf, 4);
   
            /* GET FEATURES (0Fh)  command to read the status */
            while(!spi_nand_ready());

            /* check ecc from status bits */
            if((ret = spi_nand_ecc())==FLASH_API_ERROR)
            {
                printf("Uncorrectable ECC error page address 0x%x\n", page_addr);
                return ret;
	    }

            nbytes = size;
            while (nbytes > 0) {
               /* Random data read (0Bh or 03h) command to read the page data from the cache  
                  The RANDOM DATA READ command requires 4 dummy bits, followed by a 12-bit column 
                  address for the starting byte address and a dummy byte for waiting data. 
                  This is only for 2K page size, the format will change for other page size. 
	       */  
               maxread = (nbytes < spi_max_op_len) ? nbytes : spi_max_op_len;

               buf[0] = FLASH_READ;
               spi_nand_col_addr(pchip, page_addr, offset, buf+1);
               buf[3] = 0; //dummy byte
            
               DBG_PRINTF("spi_nand_read_page - spi cmd 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0],buf[1],buf[2],buf[3]);
               DBG_PRINTF("spi_nand_read_page - spi read len 0x%x, offset 0x%x, remaining 0x%x\n", maxread, offset, nbytes-maxread); 

               memset(&xfer, 0, sizeof(struct spi_transfer));
               xfer.tx_buf                 = buf;
               xfer.rx_buf                 = buffer;
               xfer.len                    = maxread;
               xfer.speed_hz               = spi_flash_clock;
               xfer.prepend_cnt            = 4;
               xfer.addr_len               = 3;
               xfer.addr_offset            = 1;
               xfer.hdr_len                = 4;
               xfer.unit_size              = 1;
               spi_read(&xfer);
               while (!spi_nand_ready());
            
               buffer += maxread;
               nbytes -= maxread;
               offset += maxread;
            }
            
            /* update len and page addr for the next page read */
            len -= size;
            page_addr += pchip->chip_page_size;
            if(len > pchip->chip_page_size)
                size = pchip->chip_page_size;
            else
                size = len;
            offset = 0;

	}while(len);

        /* Verify that the spare area contains a JFFS2 cleanmarker. */
        if( !spi_nand_is_blk_cleanmarker(pchip,
            start_addr & ~(pchip->chip_page_size - 1), 0) )
        {
            ret = FLASH_API_ERROR;
            DBG_PRINTF("spi_nand_read_page: cleanmarker not found at 0x%8.8lx\n",
                page_addr);
        }
    }
    else
        ret = FLASH_API_ERROR;

    return ret;
}

/*********************************************************************/
/* flash_read_buf() reads buffer of data from the specified          */
/* offset from the sector parameter.                                 */
/*********************************************************************/
int spi_nand_read_buf(unsigned short blk, int offset, unsigned char *buffer, int len)
{

    int ret = len;
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned int start_addr;
    unsigned int blk_addr;
    unsigned int blk_offset;
    unsigned int size;
    unsigned int numBlocksInChip = pchip->chip_num_blocks;    

    DBG_PRINTF(">> nand_flash_read_buf - 1 blk=0x%8.8lx, offset=%d, len=%lu\n",
        blk, offset, len);

    start_addr = (blk * pchip->chip_block_size) + offset;
    blk_addr = start_addr & ~(pchip->chip_block_size - 1);
    blk_offset = start_addr - blk_addr;
    size = pchip->chip_block_size - blk_offset;

    if(size > len)
        size = len;

    do
    {
    
        if (blk >= numBlocksInChip)
        {
            printf("Attempt to read block number(%d) beyond the nand max blk(%d) \n", blk, numBlocksInChip-1);
            return FLASH_API_ERROR;
        }
        
#if defined(CFG_RAMAPP)        
        if (spi_nand_is_blk_bad(pchip, blk))
        {
            printf("nand_flash_read_buf(): Attempt to read bad nand block %d\n", blk);
            ret = FLASH_API_ERROR;
            break;
        }
#endif        
         
        if(spi_nand_read_page(pchip,start_addr,buffer,size) != FLASH_API_OK)
        {
            ret = FLASH_API_ERROR;
            break;
        }

        len -= size;
        if( len )
        {
            blk++;

            DBG_PRINTF(">> nand_flash_read_buf - 2 blk=0x%8.8lx, len=%lu\n",
                blk, len);

            start_addr = blk * pchip->chip_block_size;
            buffer += size;
            if(len > pchip->chip_block_size)
                size = pchip->chip_block_size;
            else
                size = len;
        }
    } while(len);

    DBG_PRINTF(">> nand_flash_read_buf - ret=%d\n", ret);

    return( ret ) ;
}

/*********************************************************************/
/* Flash_status return the feature status byte                       */
/*********************************************************************/
static int spi_nand_status(void)
{
    return spi_nand_get_feat(FEATURE_STAT_ADDR);
}

/* check device ready bit */
static int spi_nand_ready(void)
{
  return (spi_nand_status()&STAT_OIP) ? 0 : 1;
}

static int spi_nand_is_blk_cleanmarker(PCFE_SPI_NAND_CHIP pchip,
    unsigned long start_addr, int write_if_not)
{
  /* Always return TRUE for now */
    return(1);
}

/*********************************************************************/
/*  spi_nand_get_feat return the feature byte at feat_addr            */
/*********************************************************************/
static int spi_nand_get_feat(unsigned char feat_addr)
{
    unsigned char buf[4];
    struct spi_transfer xfer;

    /* check device is ready */
    memset(&xfer, 0, sizeof(struct spi_transfer));
    buf[0]           = FLASH_GFEAT;
    buf[1]           = feat_addr;
    xfer.tx_buf      = buf;
    xfer.rx_buf      = buf;
    xfer.len         = 1;
    xfer.speed_hz    = spi_flash_clock;
    xfer.prepend_cnt = 2;
    spi_read(&xfer);
   
    DBG_PRINTF("spi_nand_get_feat at 0x%x 0x%x\n", feat_addr, buf[0]);

    return buf[0];
}

static int spi_nand_ecc(void)
{
    int status, ret;

    status = spi_nand_status();
    status = (status&STAT_ECC_MASK) >> STAT_ECC_SHIFT;
    if( status  == STAT_ECC_GOOD )
        ret = FLASH_API_OK;
    else if( status == STAT_ECC_UNCORR )
    {
        printf("uncorretable ECC error detected!\n");
        ret = FLASH_API_ERROR;
    }
    else
    {
        printf("corretable ECC error detected and fixed\n");
        ret = FLASH_API_OK;
    }

    return ret;
}

/*********************************************************************/
/* Useful function to return the number of blks in the device.       */
/* Can be used for functions which need to loop among all the        */
/* blks, or wish to know the number of the last blk.                 */
/*********************************************************************/
int spi_nand_get_numsectors(void)
{
    return g_spinand_chip.chip_num_blocks;  
}

/*********************************************************************/
/* flash_get_sector_size() is provided for cases in which the size   */
/* of a sector is required by a host application.  The sector size   */
/* (in bytes) is returned in the data location pointed to by the     */
/* 'size' parameter.                                                 */
/*********************************************************************/

int spi_nand_get_sector_size(unsigned short blk)
{
    return g_spinand_chip.chip_block_size;
}

/*********************************************************************/
/* Flash_sector_erase_int() wait until the erase is completed before */
/* returning control to the calling function.  This can be used in   */
/* cases which require the program to hold until a sector is erased, */
/* without adding the wait check external to this function.          */
/*********************************************************************/
#if defined(CFG_RAMAPP)        
static int spi_nand_sector_erase_int(unsigned short blk)
{
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned char buf[6];
    unsigned int page_addr;
    int ret = FLASH_API_OK, status;

    if (blk >= pchip->chip_num_blocks)
    {
        printf("Attempt to erase block number(%d) beyond the nand max blk(%d) \n", blk, pchip->chip_num_blocks-1);
        return FLASH_API_ERROR;
    }

    page_addr = (blk * pchip->chip_block_size);

    /* set device to write enabled */
    spi_nand_write_enable();
    buf[0] = FLASH_BERASE;
    spi_nand_row_addr(pchip, page_addr, buf+1);
    spi_write(buf, 4);
    while(!spi_nand_ready());     
   
    status = spi_nand_status();
    if( status&STAT_EFAIL )
    {
        printf("erase block 0x%x failed, sts 0x%x\n", blk, status);
        ret = FLASH_API_ERROR;
    }

    spi_nand_write_disable();

    return ret;
}

/**********************************************************************/
/* flash_write_enable() must be called before any change to the       */
/* device such as write, erase. It also unlock the blocks if they are */
/* previouly locked.                                                  */
/**********************************************************************/
static int spi_nand_write_enable(void)
{
    unsigned char buf[4], prot;

    /* make sure it is not locked first */
    prot = spi_nand_get_feat(FEATURE_PROT_ADDR);
    if( prot != 0 )
    {
        prot = 0;
        spi_nand_set_feat(FEATURE_PROT_ADDR, prot);
    }

    /* send write enable cmd and check feature status WEL latch bit */
    buf[0]           = FLASH_WREN;
    spi_write(buf, 1);
    while(!spi_nand_ready());
    while(!spi_nand_wel()); 

    return(FLASH_API_OK);
}

static int spi_nand_write_disable(void)
{
    unsigned char buf[4];

    buf[0]           = FLASH_WRDI;
    spi_write(buf, 1);
    while(!spi_nand_ready());
    while(spi_nand_wel()); 

    return(FLASH_API_OK);
}

/***************************************************************************
 * Function Name: spi_nand_write_page
 * Description  : Writes up to a NAND block of pages from the specified buffer.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
static int spi_nand_write_page(PCFE_SPI_NAND_CHIP pchip, unsigned long start_addr,
    unsigned char *buffer, int len)
{
    unsigned char buf[512];  /* HS_SPI_BUFFER_LEN SPI controller fifo size is current at 512 bytes*/
    unsigned int nbytes;
    int maxwrite, status;
    int ret = FLASH_API_OK;

    if( len <= pchip->chip_block_size )
    {
        unsigned int page_addr = start_addr & ~(pchip->chip_page_size - 1);
        unsigned int page_offset = start_addr - page_addr;
        unsigned int size = pchip->chip_page_size - page_offset;

        /* Verify that the spare area contains a JFFS2 cleanmarker. */
        if( spi_nand_is_blk_cleanmarker(pchip, page_addr, 1) )
        {
            if(size > len)
                size = len;
  
            do
            {
                DBG_PRINTF("spi_nand_write_page - page addr 0x%x, offset 0x%x, size 0x%x, len 0x%x\n", page_addr, page_offset, size, len);

#if 0           /* do we need this for SPI NAND???? */
                /* Don't write page if data is all 0xFFs.  This allows the
                 * Linux NAND flash driver to successfully write the page.
                */
                UINT32 *pdw = (UINT32 *) (buffer + index);
                ret = FLASH_API_OK_BLANK;
                for( i = 0; i < pchip->chip_page_size / sizeof(long); i++ )
                {
                    if( *pdw++ != 0xffffffff )
                    {
                        ret = FLASH_API_OK;
                        break;
                    }
                }
           
                if( ret == FLASH_API_OK )
#endif
                {
                    /* Send Write enable Command (0x06) */
                    spi_nand_write_enable();
                    
                    nbytes = size;
                    while (nbytes > 0) {
                        /* Send Program Load Random Data Command (0x84) to load data to cache register. 
                         * PROGRAM LOAD consists of an 8-bit Op code, followed by 4 bit dummy  and a 
                         * 12-bit column address, then the data bytes to be programmed. */
             	        buf[0] = FLASH_PROG_RAN;
                        spi_nand_col_addr(pchip, page_addr, page_offset, buf+1);

                        /* set length to the smaller of controller limit (spi_max_op_len, assume it is 510 now) or nbytes */
     	                maxwrite = (nbytes < (spi_max_op_len - 3)) ? nbytes : (spi_max_op_len - 3);
                        memcpy(&buf[3], buffer, maxwrite);
                        DBG_PRINTF("spi_nand_write_page - spi cmd 0x%x, 0x%x, 0x%x\n", buf[0],buf[1],buf[2]);
                        DBG_PRINTF("spi_nand_write_page - spi write len 0x%x, offset 0x%x, remaining 0x%x\n", maxwrite, page_offset, nbytes-maxwrite); 

                        spi_write(buf, maxwrite + 3);
                        
                        buffer += maxwrite;
                        nbytes -= maxwrite;
                        page_offset += maxwrite;
		    }

                    /* Send Program Execute command (0x10) to write cache data to memory array 
                     * Send address (24bit): 8 bit dummy + 16 bit address (page/Block)
                     */
                    buf[0] = FLASH_PEXEC;
                    spi_nand_row_addr(pchip, page_addr, buf+1);
                    DBG_PRINTF("spi_nand_write_page - spi cmd 0x%x, 0x%x, 0x%x, 0x%x\n", buf[0],buf[1],buf[2], buf[3]);
                    spi_write(buf, 4);
                    while(!spi_nand_ready());                    
                    
                    status = spi_nand_status();
                    if( status&STAT_PFAIL )
                    {
                        printf("Page program failed at 0x%x page address, sts 0x%x\n", page_addr, status);
                        ret = FLASH_API_ERROR;
                    }
	        }


                if(ret == FLASH_API_ERROR)
                    break;

                if(ret == FLASH_API_OK_BLANK)
                    ret = FLASH_API_OK;

                page_offset = 0;
                page_addr += pchip->chip_page_size;
                len -= size;
                if(len > pchip->chip_page_size)
                    size = pchip->chip_page_size;
                else
                    size = len;
            }while(len);

            spi_nand_write_disable();
        }
        else    
            DBG_PRINTF("spi_nand_write_page: cleanmarker not found at 0x%8.8lx\n",
                page_addr);
    }

    return ret;
}

/***************************************************************************
 * Function Name: nand_flash_write_buf
 * Description  : Writes to flash memory. Erase block must called first. 
 * Returns      : number of bytes written or FLASH_API_ERROR
 ***************************************************************************/
static int spi_nand_write_buf(unsigned short blk, int offset,
    unsigned char *buffer, int len)
{
    int ret = len;
    PCFE_SPI_NAND_CHIP pchip = &g_spinand_chip;
    unsigned int start_addr;
    unsigned int blk_addr;
    unsigned int blk_offset;
    unsigned int size;
    unsigned int numBlocksInChip = pchip->chip_num_blocks;

    DBG_PRINTF(">> nand_flash_write_buf - 1 blk=0x%8.8lx, offset=%d, len=%d\n",
       blk, offset, len);

    start_addr = (blk * pchip->chip_block_size) + offset;
    blk_addr = start_addr & ~(pchip->chip_block_size - 1);
    blk_offset = start_addr - blk_addr;
    size = pchip->chip_block_size - blk_offset;

    if(size > len)
        size = len;

    do
    {
        if (blk >= numBlocksInChip)
        {
            printf("Attempt to write block number(%d) beyond the nand max blk(%d) \n", blk, numBlocksInChip-1);
            return ret-len;
        }

        if (spi_nand_is_blk_bad(pchip, blk))
        {
            printf("nand_flash_write_buf(): Attempt to write bad nand block %d\n", blk);
            ret = ret - len;
            break;
        }

        if(spi_nand_write_page(pchip,start_addr,buffer,size) != FLASH_API_OK)
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

                DBG_PRINTF(">> nand_flash_write_buf- 2 blk=0x%8.8lx, len=%d\n",
                    blk, len);

                offset = 0;
                start_addr = blk * pchip->chip_block_size;
                buffer += size;
                if(len > pchip->chip_block_size)
                    size = pchip->chip_block_size;
                else
                    size = len;
            }
        }
    } while(len);

    DBG_PRINTF(">> nand_flash_write_buf - ret=%d\n", ret);

    return( ret ) ;
}

/*********************************************************************/
/*  spi_nand_set_feat set the feature byte at feat_addr              */
/*********************************************************************/
static void spi_nand_set_feat(unsigned char feat_addr, unsigned char feat_val)
{
    unsigned char buf[3];

    /* check device is ready */
    buf[0]           = FLASH_SFEAT;
    buf[1]           = feat_addr;
    buf[2]           = feat_val;
    spi_write(buf, 3);
   
    while(!spi_nand_ready());

    return;
}

/* check device write enable latch bit */
static int spi_nand_wel(void)
{
  return (spi_nand_status()&STAT_WEL) ? 1 : 0;
}

/*********************************************************************/
/* flash_get_device_id() return the device id of the component.      */
/*********************************************************************/

static unsigned short spi_nand_get_device_id(void)
{
    unsigned char buf[4];
    struct spi_transfer xfer;

    memset(&xfer, 0, sizeof(struct spi_transfer));
    buf[0]           = FLASH_RDID;
    buf[1]           = 0;
    xfer.tx_buf      = buf;
    xfer.rx_buf      = buf;
    xfer.len         = 2;
    xfer.speed_hz    = spi_flash_clock;
    xfer.prepend_cnt = 2;
    spi_read(&xfer);
    while(!spi_nand_ready());
    
    DBG_PRINTF("spi_nand_get_device_id 0x%x 0x%x\n", buf[0], buf[1]);
    /* return manufacturer code buf[0] in msb and device code buf[1] in lsb*/
#if defined(__MIPSEL) || defined(__ARMEL__)
     buf[2] = buf[1];
     buf[1] = buf[0];
     buf[0] = buf[2];
#endif

    /* return manufacturer code and device code */
    return( *(unsigned short *)&buf[0] );
}

static int spi_nand_is_blk_bad(PCFE_SPI_NAND_CHIP pchip, unsigned short blk)
{
    return 0;
}

/***************************************************************************
 * Function Name: spi_nand_get_memptr
 * Description  : Returns the base MIPS memory address for the specfied flash
 *                sector.
 * Returns      : Base MIPS memory address for the specfied flash sector.
 ***************************************************************************/

static unsigned char *spi_nand_get_memptr(unsigned short blk)
{
    return((unsigned char *) (FLASH_BASE + (blk * g_spinand_chip.chip_block_size)));
}

/*********************************************************************
 * The purpose of flash_get_blk() is to return the block number
 * for a given memory address.
 *********************************************************************/

static int spi_nand_get_blk(int addr)
{
   return((int) ((unsigned long) addr - FLASH_BASE) >> g_spinand_chip.chip_block_shift);
}

/************************************************************************/
/* The purpose of flash_get_total_size() is to return the total size of */
/* the flash                                                            */
/************************************************************************/
static int spi_nand_get_total_size(void)
{
   return(g_spinand_chip.chip_total_size);
}

/***************************************************************************
 * Function Name: spi_nand_copy_from_spare
 * Description  : Copies data from the chip NAND spare registers to a local
 *                memory buffer.
 * Returns      : None.
 ***************************************************************************/
static inline void spi_nand_copy_from_spare(unsigned char *buffer,
    int numbytes)
{
    volatile unsigned long *spare_area = (volatile unsigned long *) &NAND->NandSpareAreaReadOfs0;
    unsigned char *pbuff = buffer;
    unsigned long i, j, k;

    for(i=0; i < 4; ++i)
    {
         j = spare_area[i];
         *pbuff++ = (unsigned char) ((j >> 24) & 0xff);
         *pbuff++ = (unsigned char) ((j >> 16) & 0xff);
         *pbuff++ = (unsigned char) ((j >>  8) & 0xff);
         *pbuff++ = (unsigned char) ((j >>  0) & 0xff);
    }

    /* Spare bytes greater than 16 are for the ECC. */
    if( NAND->NandRevision > 0x00000202 && numbytes > 16 )
    {
        spare_area = (unsigned long *) &NAND->NandSpareAreaReadOfs10;
        for(i=0, k=16; i < 4 && k < numbytes; ++i)
        {
            j = spare_area[i];
            if( k < numbytes )
                pbuff[k++] = (unsigned char) ((j >> 24) & 0xff);
            if( k < numbytes )
                pbuff[k++] = (unsigned char) ((j >> 16) & 0xff);
            if( k < numbytes )
                pbuff[k++] = (unsigned char) ((j >>  8) & 0xff);
            if( k < numbytes )
                pbuff[k++] = (unsigned char) ((j >>  0) & 0xff);
        }
    }
} /* spi_nand_copy_from_spare */
#else
/***************************************************************************
 * Function Name: rom_spi_nand_init
 * Description  : Initialize flash part just enough to read blocks.
 * Returns      : FLASH_API_OK or FLASH_API_ERROR
 ***************************************************************************/
void rom_spi_nand_init(void);
void rom_spi_nand_init(void)
{
    spi_nand_init();
}
#endif
