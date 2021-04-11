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

/***********************************************************************/
/*                                                                     */
/*   MODULE:  bcm_hwdefs.h                                             */
/*   PURPOSE: Define all base device addresses and HW specific macros. */
/*                                                                     */
/***********************************************************************/
#ifndef _BCM_HWDEFS_H
#define _BCM_HWDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INC_BTRM_BOOT
#define INC_BTRM_BOOT         0
#endif

#define	DYING_GASP_API

/*****************************************************************************/
/*                    Physical Memory Map                                    */
/*****************************************************************************/

#define PHYS_DRAM_BASE          0x00000000      /* Dynamic RAM Base */
#if defined(CONFIG_BRCM_IKOS)
#define PHYS_FLASH_BASE         0x18000000      /* Flash Memory     */
#else
#define PHYS_FLASH_BASE         0x1FC00000      /* Flash Memory     */
#endif

/*****************************************************************************/
/* Note that the addresses above are physical addresses and that programs    */
/* have to use converted addresses defined below:                            */
/*****************************************************************************/
#define DRAM_BASE           (0x80000000 | PHYS_DRAM_BASE)   /* cached DRAM */
#define DRAM_BASE_NOCACHE   (0xA0000000 | PHYS_DRAM_BASE)   /* uncached DRAM */

/* Binary images are always built for a standard MIPS boot address */
#define IMAGE_BASE          (0xA0000000 | PHYS_FLASH_BASE)

/* Some chips don't support alternative boot vector */
#if defined(CONFIG_BRCM_IKOS)
#if defined(_BCM963381_) || defined(CONFIG_BCM963381)
#if (INC_BTRM_BOOT==1)
#define FLASH_BASE          0xB8100000
#else
#define FLASH_BASE          0xB8000000
#endif
#else
#define FLASH_BASE          (0xA0000000 | PHYS_FLASH_BASE)  /* uncached Flash  */
#endif
#define BOOT_OFFSET         0
#else //CONFIG_BRCM_IKOS
#if defined(_BCM96838_) || defined(CONFIG_BCM96838)
#define FLASH_BASE	    0xBFC00000
#elif defined(_BCM96328_) || defined(CONFIG_BCM96328) || defined(_BCM96362_) || defined(CONFIG_BCM96362) || defined(_BCM963268_) || defined(CONFIG_BCM963268) || defined(_BCM96828_) || defined(CONFIG_BCM96828) || defined(_BCM96318_) || defined(CONFIG_BCM96318) 
#define FLASH_BASE          0xB8000000
#elif defined(_BCM963381_) || defined(CONFIG_BCM963381)
#if (INC_BTRM_BOOT==1)
#define FLASH_BASE          0xB8100000
#else
#define FLASH_BASE          0xB8000000
#endif
#elif defined(_BCM968500_) || defined(CONFIG_BCM968500)
#define FLASH_BASE          0xBE000000
#else
#define FLASH_BASE          (0xA0000000 | (MPI->cs[0].base & 0xFFFFFF00))
#endif
#define BOOT_OFFSET         (FLASH_BASE - IMAGE_BASE)
#endif

/*****************************************************************************/
/*  Select the PLL value to get the desired CPU clock frequency.             */
/*****************************************************************************/
#define FPERIPH            50000000

/*****************************************************************************/
/* Board memory type offset                                                  */
/*****************************************************************************/
#define ONEK                            1024

#if ((INC_BTRM_BOOT==1) && defined(_BCM963268_)) || defined(_BCM968500_)
#define FLASH_LENGTH_BOOT_ROM           (64*2*ONEK)
#else
#define FLASH_LENGTH_BOOT_ROM           (64*ONEK)
#endif

#ifndef SKY
/*****************************************************************************/
/*       NVRAM Offset and definition                                         */
/*****************************************************************************/

#define NVRAM_SECTOR                    0
#define NVRAM_DATA_OFFSET               0x0580
#else // SKY

#define CFE_SECTOR 	0
#define CFE_NVRAM_SECTOR                	1


#if (CONFIG_FLASH_CHIP_SIZE==8)
	#define SKY_NVRAM_SECTOR                    127
	#define SKY_PSI_SECTOR 						126
	#define CFE_NVRAM_DATA_OFFSET               0xfc00

#elif (CONFIG_FLASH_CHIP_SIZE==16)
	#define SKY_NVRAM_SECTOR                    255
	#define SKY_PSI_SECTOR 						254
	#define CFE_NVRAM_DATA_OFFSET               0xfc00

#elif (CONFIG_FLASH_CHIP_SIZE==32)
	#define SKY_NVRAM_SECTOR                    511
	#define SKY_PSI_SECTOR 						510
	#define CFE_NVRAM_DATA_OFFSET               0xe000
#else
#error "Flash chip size not supported. If building CFE add 'FLASH_SIZE=X' to command line where X is flash size in MBytes."
#endif //CONFIG_FLASH_CHIP_SIZE


#define SKY_SP_SECTOR					SKY_NVRAM_SECTOR
#define SKY_IHR_SP_SECTOR				SKY_SP_SECTOR
#define SKY_SP_OFFSET				    0xff00  // 256 bytes scratch pad
#define SKY_IHR_SP_OFFSET				SKY_SP_OFFSET

#ifdef CONFIG_SKY_REBOOT_DIAGNOSTICS
#define SKY_DIAGNOSTICS_SECTOR			SKY_SP_SECTOR
#define SKY_DIAGNOSTICS_OFFSET			(SKY_SP_OFFSET - sizeof(SKY_DIAGNOSTICS_DATA))
#endif

/*****************************************************************************/
/* Public key offset		                                                 */
/*****************************************************************************/
#define PUBLIC_KEY_SECTOR				0
#define PUBLIC_KEY_DATA_OFFSET			0x580
#define PUBLIC_KEY_LENGTH				0x8a

#ifdef SKY_IHR_CFE
#define NVRAM_SECTOR                    CFE_NVRAM_SECTOR
#define NVRAM_DATA_OFFSET               CFE_NVRAM_DATA_OFFSET
#else // SKY_IHR_CFE */

#define NVRAM_SECTOR                    SKY_NVRAM_SECTOR

#if (CONFIG_FLASH_CHIP_SIZE==32)
#define NVRAM_DATA_OFFSET               0x0
#else
#define NVRAM_DATA_OFFSET               0x2f00
#endif

#endif // SKY_IHR_CFE */

#ifdef SKY_EXTENDER
#define DEFAULT_BOARD_ID	"BSKYB_VIPER_EXT"
#elif defined (CONFIG_SKY_ETHAN) && !defined(SKY_EXTENDER)
#define DEFAULT_BOARD_ID	"BSKYB_VIPER"
#else //not Extender and not Ethan
#ifdef CONFIG_SKY_VDSL
#define DEFAULT_BOARD_ID	"BSKYB_63168"
#else /* SKY_VDSL */
#define DEFAULT_BOARD_ID	"Meerkat"
#endif /* SKY_VDSL */
#endif //SKY_EXTENDER

#define SERIALISATION_SECTOR            1
//#define SERIALISATION_LENGTH            (64*1024)  // 1 64k sector
#define SERIALISATION_DATA_OFFSET        0x0000    // Base of first sector

/* This is the factory serialisation structure */
#define DEFAULT_SER_FIELD_LEN 64

#endif /* SKY */
#define NVRAM_DATA_ID                   0x0f1e2d3c  // This is only for backwards compatability

#if (INC_BTRM_BOOT==1) && defined(_BCM963268_)
#define NVRAM_LENGTH                    (3*ONEK) // 3k nvram
#define NVRAM_VERSION_NUMBER    	7
#else
#define NVRAM_LENGTH                    ONEK     // 1k nvram
#define NVRAM_VERSION_NUMBER    	6
#endif

#define NVRAM_FULL_LEN_VERSION_NUMBER   5 /* version in which the checksum was
                                             placed at the end of the NVRAM
                                             structure */

#define NVRAM_BOOTLINE_LEN              256
#define NVRAM_BOARD_ID_STRING_LEN       16
#define NVRAM_MAC_ADDRESS_LEN           6

#define NVRAM_GPON_SERIAL_NUMBER_LEN    13
#define NVRAM_GPON_PASSWORD_LEN         11


#define NVRAM_WLAN_PARAMS_LEN      256
#define NVRAM_WPS_DEVICE_PIN_LEN   8

#if (INC_BTRM_BOOT==1) && defined(_BCM963268_)

#define NVRAM_BOOTLDR_SIG_LEN           256
#define NVRAM_BOOTLDR_SIG_OFFSET        1024

#define NVRAM_CV_KEY_LEN                514
#define NVRAM_MFG_CV_KEY_OFFSET         1280
#define NVRAM_OP_CV_KEY_OFFSET          1794

#define NVRAM_ENC_KEY_LEN               32

#define NVRAM_BOOTLDR_ENC_KEY_OFFSET    2308
#define NVRAM_IMAGE_ENC_KEY_OFFSET      2340

#define NVRAM_ENC_IV_LEN                32

#define NVRAM_BOOTLDR_ENC_IV_OFFSET     2372
#define NVRAM_IMAGE_ENC_IV_OFFSET       2404

#endif

#define THREAD_NUM_ADDRESS_OFFSET       (NVRAM_DATA_OFFSET + 4 + NVRAM_BOOTLINE_LEN + NVRAM_BOARD_ID_STRING_LEN)
#define THREAD_NUM_ADDRESS              (0x80000000 + THREAD_NUM_ADDRESS_OFFSET)

#ifdef SKY
#ifdef SKY_EXTENDER
#define DEFAULT_BOOTLINE    "e=192.168.2.1:ffffff00 h=192.168.2.100 g= r=f f=vmlinux i=bcm963xx_fs_kernel d=1 p=0 c= a= "
#define DEFAULT_BOARD_IP    "192.168.2.1"
#else
#define DEFAULT_BOOTLINE    "e=192.168.0.1:ffffff00 h=192.168.0.100 g= r=f f=vmlinux i=bcm963xx_fs_kernel d=1 p=0 c= a= "
#define DEFAULT_BOARD_IP    "192.168.0.1"
#endif // SKY_EXTENDER
#else //SKY
#define DEFAULT_BOOTLINE    "e=192.168.1.1:ffffff00 h=192.168.1.100 g= r=f f=vmlinux i=bcm963xx_fs_kernel d=1 p=0 c= a= "
#define DEFAULT_BOARD_IP    "192.168.1.1"
#endif //SKY

#define DEFAULT_MAC_NUM     10
#define DEFAULT_BOARD_MAC   "00:10:18:00:00:00"
#define DEFAULT_TP_NUM      0

#ifdef SKY
#define DEFAULT_PSI_SIZE    40 
#else // SKY
#define DEFAULT_PSI_SIZE    24
#endif //SKY

#define DEFAULT_GPON_SN     "BRCM12345678"
#define DEFAULT_GPON_PW     "          "
#define DEFAULT_LOG_SIZE    0
#define DEFAULT_FLASHBLK_SIZE  64
#define MAX_FLASHBLK_SIZE      128

#ifdef SKY
#define DEFAULT_AUXFS_PERCENT 10
#define MAX_AUXFS_PERCENT   20
#else //SKY
#define DEFAULT_AUXFS_PERCENT 0
#define MAX_AUXFS_PERCENT   80
#endif //SKY

#define DEFAUT_BACKUP_PSI  0

#define DEFAULT_WPS_DEVICE_PIN     "12345670"
#define DEFAULT_VOICE_BOARD_ID     "NONE"

#define NVRAM_MAC_COUNT_MAX         32
#define NVRAM_MAX_PSI_SIZE          64
#define NVRAM_MAX_SYSLOG_SIZE       256
#define NVRAM_OPTICAL_PARAMS_SIZE   64

#define NP_BOOT             0
#define NP_ROOTFS_1         1
#define NP_ROOTFS_2         2
#define NP_DATA             3
#define NP_BBT              4
#define NP_TOTAL            5

#define NAND_DATA_SIZE_KB       4096
#define NAND_BBT_THRESHOLD_KB   (512 * 1024)
#define NAND_BBT_SMALL_SIZE_KB  1024
#define NAND_BBT_BIG_SIZE_KB    4096

#define NAND_CFE_RAM_NAME           "cferam.000"
#define NAND_RFS_OFS_NAME           "NAND_RFS_OFS"
#define NAND_COMMAND_NAME           "NANDCMD"
#define NAND_BOOT_STATE_FILE_NAME   "boot_state_x"
#define NAND_SEQ_MAGIC              0x53510000

#define NAND_FULL_PARTITION_SEARCH  0

#if (NAND_FULL_PARTITION_SEARCH == 1)
#define MAX_NOT_JFFS2_VALUE         0 /* infinite */
#else
#define MAX_NOT_JFFS2_VALUE         10
#endif

#define MAGIC_NUM_LEN       8
#define MAGIC_NUMBER        "gOGoBrCm"
#define TOKEN_NAME_LEN      16
#define SP_VERSION          1
#define CFE_NVRAM_DATA2_LEN 20

#ifndef _LANGUAGE_ASSEMBLY

#ifdef SKY
/*  Apparenently CFE_NVRAM_DATA structure is a clone of NVRAM_DATA,
	but NVRAM_DATA structure can change in the future, but CFE_NVRAM_DATA
	will remain as it is


	The structure is kept as it is to make the clone easier.
	 
	During Broadcom integration, 
	It is necessary to keep this CFE_NVRAM_DATA and sky_allocs_rdp structure unchanged
	if you are  not planning to upgrade the router only with new Firmware, 
	Otherwise align  this structure same as  NVRAM_DATA  and sky_allocs_rdp strcture  respectively
	*/
	
	struct sky_allocs_rdp {
    unsigned char tmsize;
    unsigned char mcsize;
    unsigned char reserved[2];
};

typedef struct
{
    unsigned int ulVersion;
    char szBootline[NVRAM_BOOTLINE_LEN];
    char szBoardId[NVRAM_BOARD_ID_STRING_LEN];
    unsigned int ulMainTpNum;
    unsigned int ulPsiSize;
    unsigned int ulNumMacAddrs;
    unsigned char ucaBaseMacAddr[NVRAM_MAC_ADDRESS_LEN];
    char pad;
    char backupPsi;  /**< if 0x01, allocate space for a backup PSI */
    unsigned int ulCheckSumV4;
    char gponSerialNumber[NVRAM_GPON_SERIAL_NUMBER_LEN];
    char gponPassword[NVRAM_GPON_PASSWORD_LEN];
    char wpsDevicePin[NVRAM_WPS_DEVICE_PIN_LEN];
    char wlanParams[NVRAM_WLAN_PARAMS_LEN];
    unsigned int ulSyslogSize; /**< number of KB to allocate for persistent syslog */
    unsigned int ulNandPartOfsKb[NP_TOTAL];
    unsigned int ulNandPartSizeKb[NP_TOTAL];
    char szVoiceBoardId[NVRAM_BOARD_ID_STRING_LEN];
    unsigned int afeId[2];
    unsigned short opticRxPwrReading;   // optical initial rx power reading
    unsigned short opticRxPwrOffset;    // optical rx power offset
    unsigned short opticTxPwrReading;   // optical initial tx power reading
    unsigned char ucUnused2[58];
    unsigned char ucFlashBlkSize;
    unsigned char ucAuxFSPercent;
    unsigned int ulBoardStuffOption;   // board options. bit0-3 is for DECT    
    union {
        unsigned int reserved;
        struct sky_allocs_rdp alloc_rdp;
    } allocs;
    /* Add any new non-secure related elements here */
    char chUnused[286]; /* Adjust chUnused such that everything above + chUnused[] + ulCheckSum = 1k */

#if (INC_BTRM_BOOT==1) && defined(_BCM963268_)
    unsigned int ulCheckSum_1k;
    char signature[NVRAM_BOOTLDR_SIG_LEN];
    char mfgCvKey[NVRAM_CV_KEY_LEN];
    char opCvKey[NVRAM_CV_KEY_LEN];
    char bek[NVRAM_ENC_KEY_LEN];
    char iek[NVRAM_ENC_KEY_LEN];
    char biv[NVRAM_ENC_IV_LEN];
    char iiv[NVRAM_ENC_IV_LEN];
    /* Add any secure boot related elements here */
    char chSecUnused[632];
#endif

    unsigned int ulCheckSum;
} CFE_NVRAM_DATA, *PCFE_NVRAM_DATA;

typedef struct
{
   char BuildString[DEFAULT_SER_FIELD_LEN];
   char BaseMac[DEFAULT_SER_FIELD_LEN];
   char ADSLUser[DEFAULT_SER_FIELD_LEN];
   char ADSLPass[DEFAULT_SER_FIELD_LEN];
   char factorySSID[DEFAULT_SER_FIELD_LEN];
   char factoryPublicKey[DEFAULT_SER_FIELD_LEN];
   char wifiPassword[DEFAULT_SER_FIELD_LEN];
   char factorySerialNumber[DEFAULT_SER_FIELD_LEN];
   char factoryHardwareVersion[DEFAULT_SER_FIELD_LEN];
   char factoryModel[DEFAULT_SER_FIELD_LEN];
   char factoryPermit[DEFAULT_SER_FIELD_LEN];   
   char WPSPin[DEFAULT_SER_FIELD_LEN];
   unsigned long ulCheckSum;
} SERIALISATION_DATA, *PSERIALISATION_DATA;

typedef struct _SKY_SP
{
    char SPMagicNum[8];             // 8 bytes of magic number
    unsigned int  restoreDefault;             // restoredefault flag used between CFE and firmware
    char NvramData2[CFE_NVRAM_DATA2_LEN];       // not related to scratch pad
} SKY_IHR_SP_HEADER, *PSKY_IHR_SP_HEADER;


#ifdef CONFIG_SKY_REBOOT_DIAGNOSTICS
typedef struct _SKY_DIAGNOSTICS_DATA {
	char dying_gasp[16];
	int kernel_panic;
	int kernel_die;
} SKY_DIAGNOSTICS_DATA;
#endif

#endif	//SKY

#ifdef SKY_IHR_CFE
#define NVRAM_DATA CFE_NVRAM_DATA 
#define PNVRAM_DATA PCFE_NVRAM_DATA
#else // SKY_IHR_CFE
struct allocs_rdp {
    unsigned char tmsize;
    unsigned char mcsize;
    unsigned char reserved[2];
};

typedef struct
{
    unsigned int ulVersion;
    char szBootline[NVRAM_BOOTLINE_LEN];
    char szBoardId[NVRAM_BOARD_ID_STRING_LEN];
    unsigned int ulMainTpNum;
    unsigned int ulPsiSize;
    unsigned int ulNumMacAddrs;
    unsigned char ucaBaseMacAddr[NVRAM_MAC_ADDRESS_LEN];
    char pad;
    char backupPsi;  /**< if 0x01, allocate space for a backup PSI */
    unsigned int ulCheckSumV4;
    char gponSerialNumber[NVRAM_GPON_SERIAL_NUMBER_LEN];
    char gponPassword[NVRAM_GPON_PASSWORD_LEN];
    char wpsDevicePin[NVRAM_WPS_DEVICE_PIN_LEN];
    char wlanParams[NVRAM_WLAN_PARAMS_LEN];
    unsigned int ulSyslogSize; /**< number of KB to allocate for persistent syslog */
    unsigned int ulNandPartOfsKb[NP_TOTAL];
    unsigned int ulNandPartSizeKb[NP_TOTAL];
    char szVoiceBoardId[NVRAM_BOARD_ID_STRING_LEN];
    unsigned int afeId[2];
    unsigned short opticRxPwrReading;   // optical initial rx power reading
    unsigned short opticRxPwrOffset;    // optical rx power offset
    unsigned short opticTxPwrReading;   // optical initial tx power reading
    unsigned char ucUnused2[58];
    unsigned char ucFlashBlkSize;
    unsigned char ucAuxFSPercent;
    unsigned int ulBoardStuffOption;   // board options. bit0-3 is for DECT    
    union {
        unsigned int reserved;
        struct allocs_rdp alloc_rdp;
    } allocs;
    /* Add any new non-secure related elements here */
    char chUnused[286]; /* Adjust chUnused such that everything above + chUnused[] + ulCheckSum = 1k */

#if (INC_BTRM_BOOT==1) && defined(_BCM963268_)
    unsigned int ulCheckSum_1k;
    char signature[NVRAM_BOOTLDR_SIG_LEN];
    char mfgCvKey[NVRAM_CV_KEY_LEN];
    char opCvKey[NVRAM_CV_KEY_LEN];
    char bek[NVRAM_ENC_KEY_LEN];
    char iek[NVRAM_ENC_KEY_LEN];
    char biv[NVRAM_ENC_IV_LEN];
    char iiv[NVRAM_ENC_IV_LEN];
    /* Add any secure boot related elements here */
    char chSecUnused[632];
#endif

    unsigned int ulCheckSum;
} NVRAM_DATA, *PNVRAM_DATA;
#endif //SKY_IHR_CFE
#endif

/*****************************************************************************/
/*       Misc Offsets                                                        */
/*****************************************************************************/
#define CFE_VERSION_OFFSET           0x0570
#define CFE_VERSION_MARK_SIZE        5
#define CFE_VERSION_SIZE             5

/*****************************************************************************/
/*       Scratch Pad Defines                                                 */
/*****************************************************************************/
/* SP - Persistent Scratch Pad format:
       sp header        : 32 bytes
       tokenId-1        : 8 bytes
       tokenId-1 len    : 4 bytes
       tokenId-1 data    
       ....
       tokenId-n        : 8 bytes
       tokenId-n len    : 4 bytes
       tokenId-n data    
*/

#ifndef _LANGUAGE_ASSEMBLY

#ifdef SKY_IHR_CFE
#define SP_HEADER SKY_IHR_SP_HEADER
#define PSP_HEADER PSKY_IHR_SP_HEADER
#else /* SKY_IHR_CFE */
typedef struct _SP_HEADER
{
    char SPMagicNum[MAGIC_NUM_LEN];             // 8 bytes of magic number
    int SPVersion;                              // version number
    char NvramData2[CFE_NVRAM_DATA2_LEN];       // not related to scratch pad
                                                // additional NVRAM_DATA
} SP_HEADER, *PSP_HEADER;                       // total 32 bytes

typedef struct _TOKEN_DEF
{
    char tokenName[TOKEN_NAME_LEN];
    int tokenLen;
} SP_TOKEN, *PSP_TOKEN;
#endif

#endif

/*****************************************************************************/
/*       Boot Loader Parameters                                              */
/*****************************************************************************/

#define BLPARMS_MAGIC               0x424c504d

#define BOOTED_IMAGE_ID_NAME        "boot_image"

#define BOOTED_NEW_IMAGE            1
#define BOOTED_OLD_IMAGE            2
#define BOOTED_ONLY_IMAGE           3
#define BOOTED_PART1_IMAGE          4
#define BOOTED_PART2_IMAGE          5

#define BOOT_SET_NEW_IMAGE          '0'
#define BOOT_SET_OLD_IMAGE          '1'
#define BOOT_SET_NEW_IMAGE_ONCE     '2'
#define BOOT_GET_IMAGE_VERSION      '3'
#define BOOT_GET_BOOTED_IMAGE_ID    '4'
#define BOOT_SET_PART1_IMAGE        '5'
#define BOOT_SET_PART2_IMAGE        '6'
#define BOOT_SET_PART1_IMAGE_ONCE   '7'
#define BOOT_SET_PART2_IMAGE_ONCE   '8'
#define BOOT_GET_BOOT_IMAGE_STATE   '9'

#define FLASH_PARTDEFAULT_REBOOT    0x00000000
#define FLASH_PARTDEFAULT_NO_REBOOT 0x00000001
#define FLASH_PART1_REBOOT          0x00010000
#define FLASH_PART1_NO_REBOOT       0x00010001
#define FLASH_PART2_REBOOT          0x00020000
#define FLASH_PART2_NO_REBOOT       0x00020001

#define FLASH_IS_NO_REBOOT(X)       ((X) & 0x0000ffff)
#define FLASH_GET_PARTITION(X)      ((unsigned long) (X) >> 16)

/*****************************************************************************/
/*       Split Partition Parameters                                          */
/*****************************************************************************/
#define BCM_BCMFS_TAG		"BcmFs-"
#define BCM_BCMFS_TYPE_UBIFS	"ubifs"
#define BCM_BCMFS_TYPE_JFFS2	"jffs2"

/*****************************************************************************/
/*       Global Shared Parameters                                            */
/*****************************************************************************/

#define BRCM_MAX_CHIP_NAME_LEN	16


#ifdef __cplusplus
}
#endif

#endif /* _BCM_HWDEFS_H */
