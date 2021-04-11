/*
<:copyright-BRCM:2002:GPL/GPL:standard

   Copyright (c) 2002 Broadcom Corporation
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
/***************************************************************************
* File Name  : board.c
*
* Description: This file contains Linux character device driver entry
*              for the board related ioctl calls: flash, get free kernel
*              page and dump kernel memory, etc.
*
*
***************************************************************************/

/* Includes. */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/if.h>
#include <linux/pci.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/smp.h>
#include <linux/version.h>
#include <linux/reboot.h>
#include <linux/bcm_assert_locks.h>
#include <asm/delay.h>
#include <linux/fcntl.h>
#include <linux/syscalls.h>
#include <linux/fs.h>
#include <linux/mtd/mtd.h>

#include <bcmnetlink.h>
#include <net/sock.h>
#include <bcmtypes.h>
#include <bcm_map_part.h>
#include <board.h>
#if defined(CONFIG_BCM_6802_MoCA)
#include "./bbsi/bbsi.h"
#else
#include <spidevices.h>
#endif

#define  BCMTAG_EXE_USE
#include <bcmTag.h>
#include <boardparms.h>
#include <boardparms_voice.h>
#include <flash_api.h>
#include <bcm_intr.h>
#include <flash_common.h>
#include <shared_utils.h>
#include <bcmpci.h>
#include <linux/bcm_log.h>
#include <bcmSpiRes.h>
//extern unsigned int flash_get_reserved_bytes_at_end(const FLASH_ADDR_INFO *fInfo);

#if defined(CONFIG_BCM968500)
#include "bl_lilac_brd.h"
#include "bl_lilac_ic.h"
#include "bl_lilac_ddr.h"
#endif

#if defined(CONFIG_BCM96838)
#include "pmc_drv.h"
#endif

#if defined(CONFIG_BCM_EXT_TIMER)
#include "bcm_ext_timer.h"
#endif

/* Typedefs. */

#define SES_EVENT_BTN_PRESSED      0x00000001
#define SES_EVENTS                 SES_EVENT_BTN_PRESSED /*OR all values if any*/
#if defined (WIRELESS)
#define SES_LED_OFF                0
#define SES_LED_ON                 1
#define SES_LED_BLINK              2
#ifdef SKY
#define LED_ERROR  3


extern int KerSysEraseAuxfs(void);
#endif /* SKY */

#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM96828)
#define WLAN_ONBOARD_SLOT	WLAN_ONCHIP_DEV_SLOT
#else
#define WLAN_ONBOARD_SLOT       1 /* Corresponds to IDSEL -- EBI_A11/PCI_AD12 */
#endif

#define BRCM_VENDOR_ID       0x14e4
#define BRCM_WLAN_DEVICE_IDS 0x4300
#define BRCM_WLAN_DEVICE_IDS_DEC 43

#define WLAN_ON   1
#define WLAN_OFF  0
#endif


typedef struct
{
    unsigned long ulId;
    char chInUse;
    char chReserved[3];
} MAC_ADDR_INFO, *PMAC_ADDR_INFO;

typedef struct
{
    unsigned long ulNumMacAddrs;
    unsigned char ucaBaseMacAddr[NVRAM_MAC_ADDRESS_LEN];
    MAC_ADDR_INFO MacAddrs[1];
} MAC_INFO, *PMAC_INFO;

typedef struct
{
    unsigned char gponSerialNumber[NVRAM_GPON_SERIAL_NUMBER_LEN];
    unsigned char gponPassword[NVRAM_GPON_PASSWORD_LEN];
} GPON_INFO, *PGPON_INFO;

typedef struct
{
    unsigned long eventmask;
} BOARD_IOC, *PBOARD_IOC;


/*Dyinggasp callback*/
typedef void (*cb_dgasp_t)(void *arg);
typedef struct _CB_DGASP__LIST
{
    struct list_head list;
    char name[IFNAMSIZ];
    cb_dgasp_t cb_dgasp_fn;
    void *context;
}CB_DGASP_LIST , *PCB_DGASP_LIST;

#ifdef SKY
static DEFINE_SPINLOCK(lockled);
#endif /* SKY */
/* Externs. */
extern struct file *fget_light(unsigned int fd, int *fput_needed);
extern unsigned long getMemorySize(void);
extern void __init boardLedInit(void);
extern void boardLedCtrl(BOARD_LED_NAME, BOARD_LED_STATE);

/* Prototypes. */
static void set_mac_info( void );
#if !defined(CONFIG_BCM968500) || defined(CONFIG_BCM_GPON_STACK) || defined(CONFIG_BCM_GPON_STACK_MODULE)
static void set_gpon_info( void );
#endif
static int board_open( struct inode *inode, struct file *filp );
static ssize_t board_read(struct file *filp,  char __user *buffer, size_t count, loff_t *ppos);
static unsigned int board_poll(struct file *filp, struct poll_table_struct *wait);
static int board_release(struct inode *inode, struct file *filp);
static int board_ioctl( struct inode *inode, struct file *flip, unsigned int command, unsigned long arg );
#if defined(HAVE_UNLOCKED_IOCTL)
static long board_unlocked_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);
#endif

static BOARD_IOC* borad_ioc_alloc(void);
static void borad_ioc_free(BOARD_IOC* board_ioc);

/*
 * flashImageMutex must be acquired for all write operations to
 * nvram, CFE, or fs+kernel image.  (cfe and nvram may share a sector).
 */
DEFINE_MUTEX(flashImageMutex);
static void writeNvramDataCrcLocked(PNVRAM_DATA pNvramData);
static PNVRAM_DATA readNvramData(void);

#if defined(HAVE_UNLOCKED_IOCTL)
static DEFINE_MUTEX(ioctlMutex);
#endif

/* DyingGasp function prototype */
static irqreturn_t kerSysDyingGaspIsr(int irq, void * dev_id);
static void __init kerSysInitDyingGaspHandler( void );
static void __exit kerSysDeinitDyingGaspHandler( void );
/* -DyingGasp function prototype - */
/* dgaspMutex protects list add and delete, but is ignored during isr. */
static DEFINE_MUTEX(dgaspMutex);
static int ConfigCs(BOARD_IOCTL_PARMS *parms);

#if defined(CONFIG_BCM96318)
static void __init kerSysInit6318Reset( void );
#endif

#if !defined(CONFIG_BCM968500)
static irqreturn_t mocaHost_isr(int irq, void *dev_id);

static irqreturn_t  sesBtn_isr(int irq, void *dev_id);
static Bool         sesBtn_pressed(void);
static void __init  sesBtn_mapIntr(int context);
#endif
static void __init  ses_board_init(void);
static void __exit  ses_board_deinit(void);

#if defined (WIRELESS)
#if !defined(CONFIG_BCM968500)
static unsigned int sesBtn_poll(struct file *file, struct poll_table_struct *wait);
static ssize_t sesBtn_read(struct file *file,  char __user *buffer, size_t count, loff_t *ppos);
#endif
static void __init sesLed_mapGpio(void);
static void sesLed_ctrl(int action);

static void __init kerSysScreenPciDevices(void);
#if !defined(CONFIG_BCM968500)
static void kerSetWirelessPD(int state);
#endif
#endif

static void str_to_num(char* in, char *out, int len);
static int add_proc_files(void);
static int del_proc_files(void);
static int proc_get_param(char *page, char **start, off_t off, int cnt, int *eof, void *data);
static int proc_set_param(struct file *f, const char *buf, unsigned long cnt, void *data);
static int proc_set_led(struct file *f, const char *buf, unsigned long cnt, void *data);

#if !defined(CONFIG_BCM968500)
static irqreturn_t reset_isr(int irq, void *dev_id);
#endif
#ifdef SKY
static void pwrLed_ctrl(int action);
static int proc_get_led_mem(char *page, char **start, off_t off, int cnt, int *eof, void *data);
static int proc_write_control_led(struct file *f, const char *buf, unsigned long cnt, void *data);
void reg_dumpaddr( unsigned char *pAddr, int nLen );
static int gpio_dump(char *str);
static int led_dump(char *str);

#ifdef SKY_FLASH_API
static void sky_flash_dump_write(char *str, int dump);
#endif // SKY_FLASH_API

#define MYLED ((volatile unsigned long* const) LED_BASE)
#define MYGPIO ((volatile unsigned long * const) GPIO_BASE)
static unsigned long io_start_addr = 0x00000000;
static unsigned long io_end_addr = 0x00000000;
static int led_io_read(char *str);
static int gpio_io_read(char *str);
static int gpio_io_write(char *str);
static int led_io_write(char *str);
static Bool reset_line_held(void);
static int wait_reset_release( void *data );
static PSERIALISATION_DATA readSerialisationData(void);
static PCFE_NVRAM_DATA readCfeNvramData(void);
static void pwrLed_ctrl(int action);
static int proc_write_control_led(struct file *f, const char *buf, unsigned long cnt, void *data);
static int gpio_io_read(char *str);
static int led_dump(char *str);
static int gpio_dump(char *str);
void reg_dumpaddr( unsigned char *pAddr, int nLen );
static int gpio_io_write(char *str);
static int led_io_read(char *str);
static int led_io_write(char *str);
static int proc_get_led_mem(char *page, char **start, off_t off, int cnt, int *eof, void *data);
#ifdef CONFIG_SKY_FEATURE_WATCHDOG
static void initHWWatchDog(void);
void restartHWWatchDog(void);
static void stopHWWatchDog(void);
#endif /*  CONFIG_SKY_FEATURE_WATCHDOG */

#ifdef CONFIG_SKY_REBOOT_DIAGNOSTICS
static int proc_get_dg_mem(char *page, char **star, off_t off, int cnt, int *eof, void *data);
static int proc_set_dg_mem(struct file *f, const char *buf, unsigned long cnt, void *data);
static int proc_get_kern_panic(char *page, char **star, off_t off, int cnt, int *eof, void *data);
static int proc_set_kern_panic(struct file *f, const char *buf, unsigned long cnt, void *data);
static int proc_get_kern_die(char *page, char **star, off_t off, int cnt, int *eof, void *data);
static int proc_set_kern_die(struct file *f, const char *buf, unsigned long cnt, void *data);
static int proc_get_rst_ctl(char *page, char **star, off_t off, int cnt, int *eof, void *data);
#endif

#endif /* SKY */
// macAddrMutex is used by kerSysGetMacAddress and kerSysReleaseMacAddress
// to protect access to g_pMacInfo
static DEFINE_MUTEX(macAddrMutex);
static PMAC_INFO g_pMacInfo = NULL;
static PGPON_INFO g_pGponInfo = NULL;
static unsigned long g_ulSdramSize;
#if defined(CONFIG_BCM96368)
static unsigned long g_ulSdramWidth;
#endif
static int g_ledInitialized = 0;
static wait_queue_head_t g_board_wait_queue;
static CB_DGASP_LIST *g_cb_dgasp_list_head = NULL;

#define MAX_PAYLOAD_LEN 64
static struct sock *g_monitor_nl_sk;
static int g_monitor_nl_pid = 0 ;
static void kerSysInitMonitorSocket( void );
static void kerSysCleanupMonitorSocket( void );

#if defined(CONFIG_BCM96368)
static void ChipSoftReset(void);
static void ResetPiRegisters( void );
static void PI_upper_set( volatile uint32 *PI_reg, int newPhaseInt );
static void PI_lower_set( volatile uint32 *PI_reg, int newPhaseInt );
static void TurnOffSyncMode( void );
#endif

#if defined(CONFIG_BCM96816)
void board_Init6829( void );
#endif

#if defined(CONFIG_BCM_6802_MoCA)
void board_mocaInit(int mocaChipNum);
#endif

static kerSysMacAddressNotifyHook_t kerSysMacAddressNotifyHook = NULL;
static kerSysPushButtonNotifyHook_t kerSysPushButtonNotifyHook = NULL;

#if !defined(CONFIG_BCM968500)
/* restore default work structure */
static struct work_struct restoreDefaultWork;
#endif

static struct file_operations board_fops =
{
    open:       board_open,
#if defined(HAVE_UNLOCKED_IOCTL)
    unlocked_ioctl: board_unlocked_ioctl,
#else
    ioctl:      board_ioctl,
#endif
    poll:       board_poll,
    read:       board_read,
    release:    board_release,
};

uint32 board_major = 0;
#if !defined(CONFIG_BCM968500)
static unsigned short sesBtn_irq = BP_NOT_DEFINED;
static unsigned short sesBtn_gpio = BP_NOT_DEFINED;
static unsigned short sesBtn_polling = 0;
static struct timer_list sesBtn_timer;
static atomic_t sesBtn_active;
static unsigned short resetBtn_gpio = BP_NOT_DEFINED;

#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818) || defined(CONFIG_BCM96838)
#define NUM_EXT_INT    (INTERRUPT_ID_EXTERNAL_5-INTERRUPT_ID_EXTERNAL_0+1)
#else
#define NUM_EXT_INT    (INTERRUPT_ID_EXTERNAL_3-INTERRUPT_ID_EXTERNAL_0+1)
#endif
static unsigned int extIntrInfo[NUM_EXT_INT];
static MOCA_INTR_ARG mocaIntrArg[BP_MOCA_MAX_NUM];
static BP_MOCA_INFO mocaInfo[BP_MOCA_MAX_NUM];
static int mocaChipNum = BP_MOCA_MAX_NUM;
static int restore_in_progress = 0;
#endif //#if !defined(CONFIG_BCM968500)

#if defined (WIRELESS)
static unsigned short sesLed_gpio = BP_NOT_DEFINED;
#endif

#if defined(CONFIG_SKY_MESH_SUPPORT)
static atomic_t sesBtn_Notified;
#endif

#if defined(MODULE)
int init_module(void)
{
    return( brcm_board_init() );
}

void cleanup_module(void)
{
    if (MOD_IN_USE)
        printk("brcm flash: cleanup_module failed because module is in use\n");
    else
        brcm_board_cleanup();
}
#endif //MODULE

#if defined(CONFIG_BCM968500)
unsigned int BcmHalMapInterrupt(FN_HANDLER pfunc, unsigned int param, unsigned int irq)
{
        /* 'devname' must be static - the buffer scope should exist after
	 * exiting from this function. */
	static char devname[16];
	unsigned long lock;

	sprintf(devname, "brcm_%d", irq);

	local_irq_save(lock);
	request_irq(irq, (void *)pfunc, IRQF_SHARED, devname, (void *)param);
	bl_lilac_ic_mask_irq(irq - INT_NUM_IRQ0);
	bl_lilac_ic_isr_ack(irq - INT_NUM_IRQ0);
	local_irq_restore(lock);

	return 0;
}

void enable_brcm_irq_irq(unsigned int irq)
{
	unsigned long lock;

	local_irq_save(lock);
	bl_lilac_ic_unmask_irq(irq - INT_NUM_IRQ0);
	local_irq_restore(lock);
}

void disable_brcm_irq_irq(unsigned int irq)
{
	unsigned long lock;

	local_irq_save(lock);
	bl_lilac_ic_mask_irq(irq - INT_NUM_IRQ0);
	local_irq_restore(lock);
}
#endif

#ifndef CONFIG_BCM968500
static int map_external_irq (int irq)
{
    int map_irq;
    irq &= ~BP_EXT_INTR_FLAGS_MASK;

    switch (irq) {
    case BP_EXT_INTR_0   :
        map_irq = INTERRUPT_ID_EXTERNAL_0;
        break ;
    case BP_EXT_INTR_1   :
        map_irq = INTERRUPT_ID_EXTERNAL_1;
        break ;
    case BP_EXT_INTR_2   :
        map_irq = INTERRUPT_ID_EXTERNAL_2;
        break ;
    case BP_EXT_INTR_3   :
        map_irq = INTERRUPT_ID_EXTERNAL_3;
        break ;
#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818) || defined(CONFIG_BCM96838)
    case BP_EXT_INTR_4   :
        map_irq = INTERRUPT_ID_EXTERNAL_4;
        break ;
    case BP_EXT_INTR_5   :
        map_irq = INTERRUPT_ID_EXTERNAL_5;
        break ;
#endif
    default           :
        printk ("Invalid External Interrupt definition \n") ;
        map_irq = 0 ;
        break ;
    }

    return (map_irq) ;
}

static int set_ext_irq_info(unsigned short ext_irq)
{
	int irq_idx, rc = 0;

	irq_idx = (ext_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;

	if( extIntrInfo[irq_idx] == (unsigned int)(-1) )
		extIntrInfo[irq_idx] = ext_irq;
	else
	{
		/* make sure all the interrupt sharing this irq number has the trigger type and shared */
		if( ext_irq != (unsigned int)extIntrInfo[irq_idx] )
		{
			printk("Invalid ext intr type for BP_EXT_INTR_%d: 0x%x vs 0x%x\r\n", irq_idx, ext_irq, extIntrInfo[irq_idx]);
			extIntrInfo[irq_idx] |= BP_EXT_INTR_CONFLICT_MASK;
			rc = -1;
		}
	}

	return rc;
}

static void init_ext_irq_info(void)
{
	int i, j;
	unsigned short intr;

	/* mark each entry invalid */
	for(i=0; i<NUM_EXT_INT; i++)
		extIntrInfo[i] = (unsigned int)(-1);

	/* collect all the external interrupt info from bp */
	if( BpGetResetToDefaultExtIntr(&intr) == BP_SUCCESS )
		set_ext_irq_info(intr);

	if( BpGetWirelessSesExtIntr(&intr) == BP_SUCCESS )
		set_ext_irq_info(intr);

	for( i = 0; i < mocaChipNum; i++ )
	{
		for( j = 0; j < BP_MOCA_MAX_INTR_NUM; j++ )
		{
			if( mocaInfo[i].intr[j] != BP_NOT_DEFINED )
				set_ext_irq_info(mocaInfo[i].intr[j]);
		}
	}
	return;
}

PBP_MOCA_INFO boardGetMocaInfo(int dev)
{
	if( dev >= mocaChipNum)
		return NULL;
	else
		return &mocaInfo[dev];
}
#endif //#ifndef CONFIG_BCM968500

/* A global variable used by Power Management and other features to determine if Voice is idle or not */
volatile int isVoiceIdle = 1;
EXPORT_SYMBOL(isVoiceIdle);

#if defined(CONFIG_BCM96368)
static unsigned long getMemoryWidth(void)
{
    unsigned long memCfg;

    memCfg = MEMC->Config;
    memCfg &= MEMC_WIDTH_MASK;
    memCfg >>= MEMC_WIDTH_SHFT;

    return memCfg;
}
#endif

static int __init brcm_board_init( void )
{
#ifdef CONFIG_BCM968500
	extern unsigned long bl_lilac_get_phys_mem_size(void);

    int ret;

    ret = register_chrdev(BOARD_DRV_MAJOR, "brcmboard", &board_fops );
    if (ret < 0)
        printk( "brcm_board_init(major %d): fail to register device.\n",BOARD_DRV_MAJOR);
    else
    {
        printk("brcmboard: brcm_board_init entry\n");
        board_major = BOARD_DRV_MAJOR;

		g_ulSdramSize = bl_lilac_get_phys_mem_size();

		set_mac_info();
#if defined(CONFIG_BCM_GPON_STACK) || defined(CONFIG_BCM_GPON_STACK_MODULE)
		set_gpon_info();
#endif

		init_waitqueue_head(&g_board_wait_queue);
#if defined (WIRELESS)
		kerSysScreenPciDevices();
#endif
#if defined(PCIE_BASE)
//		kerSysCheckPowerDownPcie();
#endif

		ses_board_init();

		kerSysInitMonitorSocket();
		kerSysInitDyingGaspHandler();
		boardLedInit();
		g_ledInitialized = 1;
    }

    add_proc_files();

#else
    unsigned short rstToDflt_irq;
    int ret, ext_irq_idx;
    bcmLogSpiCallbacks_t loggingCallbacks;
#if defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
	initHWWatchDog();
#endif /* defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG) */

    ret = register_chrdev(BOARD_DRV_MAJOR, "brcmboard", &board_fops );
    if (ret < 0)
        printk( "brcm_board_init(major %d): fail to register device.\n",BOARD_DRV_MAJOR);
    else
    {
        printk("brcmboard: brcm_board_init entry\n");
        board_major = BOARD_DRV_MAJOR;

        g_ulSdramSize = getMemorySize();
#if defined(CONFIG_BCM96368)
        g_ulSdramWidth = getMemoryWidth();
#endif
        set_mac_info();
        set_gpon_info();

        BpGetMocaInfo(mocaInfo, &mocaChipNum);

        init_ext_irq_info();

        init_waitqueue_head(&g_board_wait_queue);
#if defined (WIRELESS)
        kerSysScreenPciDevices();
        kerSetWirelessPD(WLAN_ON);
#endif
        ses_board_init();

        kerSysInitMonitorSocket();
        kerSysInitDyingGaspHandler();
#if defined(CONFIG_BCM96318)
        kerSysInit6318Reset();
#endif

        boardLedInit();
        g_ledInitialized = 1;
#ifdef SKY
		if (reset_line_held())
        {/* In this case wait for it to release */
            printk("Reset line is held down, ignoring reset until cleared\n");
            kernel_thread(wait_reset_release, NULL, CLONE_KERNEL);
        }
		else
#endif /* SKY */


        if( BpGetResetToDefaultExtIntr(&rstToDflt_irq) == BP_SUCCESS )
        {
        	ext_irq_idx = (rstToDflt_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;
        	if (!IsExtIntrConflict(extIntrInfo[ext_irq_idx]))
        	{
        		static int dev = -1;
        		int hookisr = 1;

                if (IsExtIntrShared(rstToDflt_irq))
                {
                    /* get the gpio and make it input dir */
                    if( BpGetResetToDefaultExtIntrGpio(&resetBtn_gpio) == BP_SUCCESS )
                    {
                    	resetBtn_gpio &= BP_GPIO_NUM_MASK;
                        printk("brcm_board_init: Reset config Interrupt gpio is %d\n", resetBtn_gpio);
                        kerSysSetGpioDirInput(resetBtn_gpio); // Set gpio for input.
                        dev = resetBtn_gpio;
                    }
                    else
                    {
                        printk("brcm_board_init: Reset config gpio definition not found \n");
                        hookisr= 0;
                    }
                }

                if(hookisr)
                {
                    rstToDflt_irq = map_external_irq (rstToDflt_irq);
        		    BcmHalMapInterrupt((FN_HANDLER)reset_isr, (unsigned int)&dev, rstToDflt_irq);
                    BcmHalInterruptEnable(rstToDflt_irq);
                }
        	}
        }

#if defined(CONFIG_BCM_EXT_TIMER)
        init_hw_timers();
#endif

#if defined(CONFIG_BCM_CPLD1)
        // Reserve SPI bus to control external CPLD for Standby Timer
        BcmCpld1Initialize();
#endif
    }

    add_proc_files();

#if defined(CONFIG_BCM96816)
    board_Init6829();
    loggingCallbacks.kerSysSlaveRead   = kerSysBcmSpiSlaveRead;
    loggingCallbacks.kerSysSlaveWrite  = kerSysBcmSpiSlaveWrite;
    loggingCallbacks.bpGet6829PortInfo = BpGet6829PortInfo;
#elif defined(CONFIG_BCM_6802_MoCA)
    board_mocaInit(mocaChipNum);
    loggingCallbacks.kerSysSlaveRead   = kerSysBcmSpiSlaveRead;
    loggingCallbacks.kerSysSlaveWrite  = kerSysBcmSpiSlaveWrite;
    loggingCallbacks.bpGet6829PortInfo = NULL;
#endif
    loggingCallbacks.reserveSlave      = BcmSpiReserveSlave;
    loggingCallbacks.syncTrans         = BcmSpiSyncTrans;
    bcmLog_registerSpiCallbacks(loggingCallbacks);

#endif //#ifdef CONFIG_BCM968500

 return ret;
}

static void __init set_mac_info( void )
{
    NVRAM_DATA *pNvramData;
    unsigned long ulNumMacAddrs;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("set_mac_info: could not read nvram data\n");
        return;
    }

    ulNumMacAddrs = pNvramData->ulNumMacAddrs;

    if( ulNumMacAddrs > 0 && ulNumMacAddrs <= NVRAM_MAC_COUNT_MAX )
    {
        unsigned long ulMacInfoSize =
            sizeof(MAC_INFO) + ((sizeof(MAC_ADDR_INFO)) * (ulNumMacAddrs-1));

        g_pMacInfo = (PMAC_INFO) kmalloc( ulMacInfoSize, GFP_KERNEL );

        if( g_pMacInfo )
        {
            memset( g_pMacInfo, 0x00, ulMacInfoSize );
            g_pMacInfo->ulNumMacAddrs = pNvramData->ulNumMacAddrs;
            memcpy( g_pMacInfo->ucaBaseMacAddr, pNvramData->ucaBaseMacAddr,
                NVRAM_MAC_ADDRESS_LEN );
        }
        else
            printk("ERROR - Could not allocate memory for MAC data\n");
    }
    else
        printk("ERROR - Invalid number of MAC addresses (%ld) is configured.\n",
        ulNumMacAddrs);
    kfree(pNvramData);
}

#if !defined(CONFIG_BCM968500) || defined(CONFIG_BCM_GPON_STACK) || defined(CONFIG_BCM_GPON_STACK_MODULE)
static int gponParamsAreErased(NVRAM_DATA *pNvramData)
{
    int i;
    int erased = 1;

    for(i=0; i<NVRAM_GPON_SERIAL_NUMBER_LEN-1; ++i) {
        if((pNvramData->gponSerialNumber[i] != (char) 0xFF) &&
            (pNvramData->gponSerialNumber[i] != (char) 0x00)) {
                erased = 0;
                break;
        }
    }

    if(!erased) {
        for(i=0; i<NVRAM_GPON_PASSWORD_LEN-1; ++i) {
            if((pNvramData->gponPassword[i] != (char) 0xFF) &&
                (pNvramData->gponPassword[i] != (char) 0x00)) {
                    erased = 0;
                    break;
            }
        }
    }

    return erased;
}

static void __init set_gpon_info( void )
{
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("set_gpon_info: could not read nvram data\n");
        return;
    }

    g_pGponInfo = (PGPON_INFO) kmalloc( sizeof(GPON_INFO), GFP_KERNEL );

    if( g_pGponInfo )
    {
        if ((pNvramData->ulVersion < NVRAM_FULL_LEN_VERSION_NUMBER) ||
            gponParamsAreErased(pNvramData))
        {
            strcpy( g_pGponInfo->gponSerialNumber, DEFAULT_GPON_SN );
            strcpy( g_pGponInfo->gponPassword, DEFAULT_GPON_PW );
        }
        else
        {
            strncpy( g_pGponInfo->gponSerialNumber, pNvramData->gponSerialNumber,
                NVRAM_GPON_SERIAL_NUMBER_LEN );
            g_pGponInfo->gponSerialNumber[NVRAM_GPON_SERIAL_NUMBER_LEN-1]='\0';
            strncpy( g_pGponInfo->gponPassword, pNvramData->gponPassword,
                NVRAM_GPON_PASSWORD_LEN );
            g_pGponInfo->gponPassword[NVRAM_GPON_PASSWORD_LEN-1]='\0';
        }
    }
    else
    {
        printk("ERROR - Could not allocate memory for GPON data\n");
    }
    kfree(pNvramData);
}
#endif

void __exit brcm_board_cleanup( void )
{
    printk("brcm_board_cleanup()\n");
    del_proc_files();

    if (board_major != -1)
    {
#if !defined(CONFIG_BCM968500)
        ses_board_deinit();
#endif
        kerSysDeinitDyingGaspHandler();
        kerSysCleanupMonitorSocket();
        unregister_chrdev(board_major, "board_ioctl");

    }
}

static BOARD_IOC* borad_ioc_alloc(void)
{
    BOARD_IOC *board_ioc =NULL;
    board_ioc = (BOARD_IOC*) kmalloc( sizeof(BOARD_IOC) , GFP_KERNEL );
    if(board_ioc)
    {
        memset(board_ioc, 0, sizeof(BOARD_IOC));
    }
    return board_ioc;
}

static void borad_ioc_free(BOARD_IOC* board_ioc)
{
    if(board_ioc)
    {
        kfree(board_ioc);
    }
}


static int board_open( struct inode *inode, struct file *filp )
{
    filp->private_data = borad_ioc_alloc();

    if (filp->private_data == NULL)
        return -ENOMEM;

    return( 0 );
}

static int board_release(struct inode *inode, struct file *filp)
{
    BOARD_IOC *board_ioc = filp->private_data;

    wait_event_interruptible(g_board_wait_queue, 1);
    borad_ioc_free(board_ioc);

    return( 0 );
}


static unsigned int board_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
#if defined (WIRELESS)
#if !defined(CONFIG_BCM968500)
    BOARD_IOC *board_ioc = filp->private_data;
#endif
#endif

    poll_wait(filp, &g_board_wait_queue, wait);
#if defined (WIRELESS)
#if !defined(CONFIG_BCM968500)
    if(board_ioc->eventmask & SES_EVENTS){
        mask |= sesBtn_poll(filp, wait);
    }
#endif
#endif

    return mask;
}

static ssize_t board_read(struct file *filp,  char __user *buffer, size_t count, loff_t *ppos)
{
#if defined (WIRELESS)
#if !defined(CONFIG_BCM968500)
    BOARD_IOC *board_ioc = filp->private_data;
    if(board_ioc->eventmask & SES_EVENTS){
        return sesBtn_read(filp, buffer, count, ppos);
    }
#endif
#endif
    return 0;
}

/***************************************************************************
// Function Name: getCrc32
// Description  : caculate the CRC 32 of the given data.
// Parameters   : pdata - array of data.
//                size - number of input data bytes.
//                crc - either CRC32_INIT_VALUE or previous return value.
// Returns      : crc.
****************************************************************************/
static UINT32 getCrc32(byte *pdata, UINT32 size, UINT32 crc)
{
    while (size-- > 0)
        crc = (crc >> 8) ^ Crc32_table[(crc ^ *pdata++) & 0xff];

    return crc;
}

/** calculate the CRC for the nvram data block and write it to flash.
 * Must be called with flashImageMutex held.
 */
static void writeNvramDataCrcLocked(PNVRAM_DATA pNvramData)
{
    UINT32 crc = CRC32_INIT_VALUE;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    pNvramData->ulCheckSum = 0;
    crc = getCrc32((char *)pNvramData, sizeof(NVRAM_DATA), crc);
    pNvramData->ulCheckSum = crc;
    kerSysNvRamSet((char *)pNvramData, sizeof(NVRAM_DATA), 0);
}


/** read the nvramData struct from the in-memory copy of nvram.
 * The caller is not required to have flashImageMutex when calling this
 * function.  However, if the caller is doing a read-modify-write of
 * the nvram data, then the caller must hold flashImageMutex.  This function
 * does not know what the caller is going to do with this data, so it
 * cannot assert flashImageMutex held or not when this function is called.
 *
 * @return pointer to NVRAM_DATA buffer which the caller must free
 *         or NULL if there was an error
 */
static PNVRAM_DATA readNvramData(void)
{
    UINT32 crc = CRC32_INIT_VALUE, savedCrc;
    NVRAM_DATA *pNvramData;

    // use GFP_ATOMIC here because caller might have flashImageMutex held
    if (NULL == (pNvramData = kmalloc(sizeof(NVRAM_DATA), GFP_ATOMIC)))
    {
        printk("readNvramData: could not allocate memory\n");
        return NULL;
    }

    kerSysNvRamGet((char *)pNvramData, sizeof(NVRAM_DATA), 0);
    savedCrc = pNvramData->ulCheckSum;
    pNvramData->ulCheckSum = 0;
    crc = getCrc32((char *)pNvramData, sizeof(NVRAM_DATA), crc);
    if (savedCrc != crc)
    {
        // this can happen if we write a new cfe image into flash.
        // The new image will have an invalid nvram section which will
        // get updated to the inMemNvramData.  We detect it here and
        // commonImageWrite will restore previous copy of nvram data.
        kfree(pNvramData);
        pNvramData = NULL;
    }

    return pNvramData;
}



//**************************************************************************************
// Utitlities for dump memory, free kernel pages, mips soft reset, etc.
//**************************************************************************************

/***********************************************************************
* Function Name: dumpaddr
* Description  : Display a hex dump of the specified address.
***********************************************************************/
void dumpaddr( unsigned char *pAddr, int nLen )
{
    static char szHexChars[] = "0123456789abcdef";
    char szLine[80];
    char *p = szLine;
    unsigned char ch, *q;
    int i, j;
    unsigned long ul;

    while( nLen > 0 )
    {
        sprintf( szLine, "%8.8lx: ", (unsigned long) pAddr );
        p = szLine + strlen(szLine);

        for(i = 0; i < 16 && nLen > 0; i += sizeof(long), nLen -= sizeof(long))
        {
            ul = *(unsigned long *) &pAddr[i];
            q = (unsigned char *) &ul;
            for( j = 0; j < sizeof(long); j++ )
            {
                *p++ = szHexChars[q[j] >> 4];
                *p++ = szHexChars[q[j] & 0x0f];
                *p++ = ' ';
            }
        }

        for( j = 0; j < 16 - i; j++ )
            *p++ = ' ', *p++ = ' ', *p++ = ' ';

        *p++ = ' ', *p++ = ' ', *p++ = ' ';

        for( j = 0; j < i; j++ )
        {
            ch = pAddr[j];
            *p++ = (ch > ' ' && ch < '~') ? ch : '.';
        }

        *p++ = '\0';
        printk( "%s\r\n", szLine );

        pAddr += i;
    }
    printk( "\r\n" );
} /* dumpaddr */


/* On 6318, the Sleep mode is used to force a reset on PVT Monitor and ASB blocks */
#if defined(CONFIG_BCM96318)
static void __init kerSysInit6318Reset( void )
{
    /* Re-initialize the sleep registers because they are not cleared on reset */
    RTC->RtcSleepRtcEnable &= ~RTC_SLEEP_RTC_ENABLE;
    RTC->RtcSleepRtcEvent &= ~RTC_SLEEP_RTC_EVENT;

    /* A magic word is saved in scratch register to identify unintentional resets */
    /* (Scratch register is not cleared on reset) */
    if (RTC->RtcSleepCpuScratchpad == 0x600DBEEF) {
        /* When Magic word is seen during reboot, there was an unintentional reset */
        printk("Previous reset was unintentional, performing full reset sequence\n");
        kerSysMipsSoftReset();
    }
    RTC->RtcSleepCpuScratchpad = 0x600DBEEF;
}

static void kerSys6318Reset( void )
{
    unsigned short plcGpio;

    /* Use GPIO to control the PLC and wifi chip reset on 6319 PLC board */
    if( BpGetPLCPwrEnGpio(&plcGpio) == BP_SUCCESS )
    {
        kerSysSetGpioState(plcGpio, kGpioInactive);
        /* Delay to ensure WiFi and PLC are powered down */
        udelay(5000);
    }

    /* On 6318, the Sleep mode is used to force a reset on PVT Monitor and ASB blocks */
    /* Configure the sleep mode and duration */
    /* then wait for system to come out of reset when the timer expires */
    PLL_PWR->PllPwrControlIddqCtrl &= ~IDDQ_SLEEP;
    RTC->RtcSleepWakeupMask = RTC_SLEEP_RTC_IRQ;
    RTC->RtcSleepCpuScratchpad = 0x01010101; // We are intentionally starting the reset sequence
    RTC->RtcSleepRtcCountL = 0x00020000; // Approx 5 ms
    RTC->RtcSleepRtcCountH = 0x0;
    RTC->RtcSleepRtcEnable = RTC_SLEEP_RTC_ENABLE;
    RTC->RtcSleepModeEnable = RTC_SLEEP_MODE_ENABLE;
    // while(1); // Spin occurs in calling function.  Do not spin here
}
#endif


/** this function actually does two things, stop other cpu and reset mips.
 * Kept the current name for compatibility reasons.  Image upgrade code
 * needs to call the two steps separately.
 */
void kerSysMipsSoftReset(void)
{
	unsigned long cpu;
	cpu = smp_processor_id();
	printk(KERN_INFO "kerSysMipsSoftReset: called on cpu %lu\n", cpu);

	stopOtherCpu();
	local_irq_disable();  // ignore interrupts, just execute reset code now
	resetPwrmgmtDdrMips();
}

extern void stop_other_cpu(void);  // in arch/mips/kernel/smp.c

void stopOtherCpu(void)
{
#if defined(CONFIG_SMP)
    stop_other_cpu();
#elif defined(CONFIG_BCM_ENDPOINT_MODULE) && defined(CONFIG_BCM_BCMDSP_MODULE)
    unsigned long cpu = (read_c0_diag3() >> 31) ? 0 : 1;

	// Disable interrupts on the other core and allow it to complete processing
	// and execute the "wait" instruction
    printk(KERN_INFO "stopOtherCpu: stopping cpu %lu\n", cpu);
    PERF->IrqControl[cpu].IrqMask = 0;
    mdelay(5);
#endif
}

void resetPwrmgmtDdrMips(void)
{
#if defined(CONFIG_SKY_ETHAN)
    unsigned short plcGpio;
#endif
#ifdef SKY
    printk(KERN_INFO "resetPwrmgmtDdrMips: Reset called\n");
#endif //SKY
#if defined(CONFIG_BCM968500)
	kernel_restart(NULL);
#elif defined(CONFIG_BCM96838)
    // FIXMET - mark not to clean half way WD interrupt
    PERF->TimerControl |= SOFT_RESET_0;
    WATCHDOG->WD0DefCount = 1000000 * (FPERIPH/1000000);
    WATCHDOG->WD0Ctl = 0xFF00;
    WATCHDOG->WD0Ctl = 0x00FF;
#else
#if defined (CONFIG_BCM963268)
    MISC->miscVdslControl &= ~(MISC_VDSL_CONTROL_VDSL_MIPS_RESET | MISC_VDSL_CONTROL_VDSL_MIPS_POR_RESET );
#endif
#if !defined (CONFIG_BCM96816) && !defined (CONFIG_BCM96818) && !defined (CONFIG_BCM96838)
    // Power Management on Ethernet Ports may have brought down EPHY PLL
    // and soft reset below will lock-up 6362 if the PLL is not up
    // therefore bring it up here to give it time to stabilize
    GPIO->RoboswEphyCtrl &= ~EPHY_PWR_DOWN_DLL;
#endif

    // let UART finish printing
    udelay(100);


#if defined(CONFIG_BCM_CPLD1)
    // Determine if this was a request to enter Standby mode
    // If yes, this call won't return and a hard reset will occur later
    BcmCpld1CheckShutdownMode();
#endif

#if defined (CONFIG_BCM96368)
    {
        volatile int delay;
        volatile int i;
        local_irq_disable();
        // after we reset DRAM controller we can't access DRAM, so
        // the first iteration put things in i-cache and the scond interation do the actual reset
        for (i=0; i<2; i++) {
            DDR->DDR1_2PhaseCntl0 &= i - 1;
            DDR->DDR3_4PhaseCntl0 &= i - 1;

            if( i == 1 )
                ChipSoftReset();

            delay = 1000;
            while (delay--);
            PERF->pll_control |= SOFT_RESET*i;
            for(;i;) {} // spin mips and wait soft reset to take effect
        }
    }
#endif

#if defined(CONFIG_SKY_ETHAN)
    /* Use GPIO to reset  PLC Chip*/
    if( BpGetPLCPwrEnGpio(&plcGpio) == BP_SUCCESS )
    {
    	kerSysSetGpioState(plcGpio, kGpioInactive);
		/* 500 Micro seconds delay */
		udelay(5000);
    }
#endif //CONFIG_SKY_ETHAN
#if defined(CONFIG_BCM96328)
    TIMER->SoftRst = 1;
#elif defined(CONFIG_BCM96318)
    kerSys6318Reset();
#else
#if defined (CONFIG_BCM96818)
    /* if the software boot strap overide bit has been turned on to configure
     * roboswitch port 2 and 3 in RGMII mode in the pin compatible 6818 device,
     * we have to clear this bit to workaround soft reset hang problem in 6818
     */
    if(UtilGetChipIsPinCompatible())
        MISC->miscStrapOverride &= ~MISC_STRAP_OVERRIDE_STRAP_OVERRIDE;
#endif
#if defined (CONFIG_BCM96816) || defined(CONFIG_BCM96818)
    /* Work around reset issues */
    HVG_MISC_REG_CHANNEL_A->mask |= HVG_SOFT_INIT_0;
    HVG_MISC_REG_CHANNEL_B->mask |= HVG_SOFT_INIT_0;

    {
        unsigned char portInfo6829;
        /* for BHRGR board we need to toggle GPIO30 to
           reset - on early BHR baords this is the GPHY2
           link100 so setting it does not matter */
        if ( (BP_SUCCESS == BpGet6829PortInfo(&portInfo6829)) &&
             (0 != portInfo6829))
        {
            GPIO->GPIODir |= 1<<30;
            GPIO->GPIOio  &= ~(1<<30);
        }
    }
#endif
    PERF->pll_control |= SOFT_RESET;    // soft reset mips
#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818)
    PERF->pll_control = 0;
#endif
#endif
#endif //#ifdef CONFIG_BCM968500
    for(;;) {} // spin mips and wait soft reset to take effect
}

unsigned long kerSysGetMacAddressType( unsigned char *ifName )
{
    unsigned long macAddressType = MAC_ADDRESS_ANY;

    if(strstr(ifName, IF_NAME_ETH))
    {
        macAddressType = MAC_ADDRESS_ETH;
    }
    else if(strstr(ifName, IF_NAME_USB))
    {
        macAddressType = MAC_ADDRESS_USB;
    }
    else if(strstr(ifName, IF_NAME_WLAN))
    {
        macAddressType = MAC_ADDRESS_WLAN;
    }
    else if(strstr(ifName, IF_NAME_MOCA))
    {
        macAddressType = MAC_ADDRESS_MOCA;
    }
    else if(strstr(ifName, IF_NAME_ATM))
    {
        macAddressType = MAC_ADDRESS_ATM;
    }
    else if(strstr(ifName, IF_NAME_PTM))
    {
        macAddressType = MAC_ADDRESS_PTM;
    }
    else if(strstr(ifName, IF_NAME_GPON) || strstr(ifName, IF_NAME_VEIP))
    {
        macAddressType = MAC_ADDRESS_GPON;
    }
    else if(strstr(ifName, IF_NAME_EPON))
    {
        macAddressType = MAC_ADDRESS_EPON;
    }

    return macAddressType;
}

static inline void kerSysMacAddressNotify(unsigned char *pucaMacAddr, MAC_ADDRESS_OPERATION op)
{
    if(kerSysMacAddressNotifyHook)
    {
        kerSysMacAddressNotifyHook(pucaMacAddr, op);
    }
}

int kerSysMacAddressNotifyBind(kerSysMacAddressNotifyHook_t hook)
{
    int nRet = 0;

    if(hook && kerSysMacAddressNotifyHook)
    {
        printk("ERROR: kerSysMacAddressNotifyHook already registered! <0x%08lX>\n",
               (unsigned long)kerSysMacAddressNotifyHook);
        nRet = -EINVAL;
    }
    else
    {
        kerSysMacAddressNotifyHook = hook;
    }

    return nRet;
}

int kerSysPushButtonNotifyBind(kerSysPushButtonNotifyHook_t hook)
{
    if(hook && kerSysPushButtonNotifyHook)
    {
        if (kerSysPushButtonNotifyHook)
        {
            printk("ERROR: kerSysPushButtonNotifyHook already registered! <%p>\n", kerSysPushButtonNotifyHook);
        }
        return -EINVAL;
    }
    else
    {
        kerSysPushButtonNotifyHook = hook;
    }

    return 0;
}

#if defined (WIRELESS)
void kerSysSesEventTrigger( void )
{
   wake_up_interruptible(&g_board_wait_queue);
}
#endif

int kerSysGetMacAddress( unsigned char *pucaMacAddr, unsigned long ulId )
{
    const unsigned long constMacAddrIncIndex = 3;
    int nRet = 0;
    PMAC_ADDR_INFO pMai = NULL;
    PMAC_ADDR_INFO pMaiFreeNoId = NULL;
    PMAC_ADDR_INFO pMaiFreeId = NULL;
    unsigned long i = 0, ulIdxNoId = 0, ulIdxId = 0, baseMacAddr = 0;

    mutex_lock(&macAddrMutex);

    /* baseMacAddr = last 3 bytes of the base MAC address treated as a 24 bit integer */
    memcpy((unsigned char *) &baseMacAddr,
        &g_pMacInfo->ucaBaseMacAddr[constMacAddrIncIndex],
        NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex);
    baseMacAddr >>= 8;

    for( i = 0, pMai = g_pMacInfo->MacAddrs; i < g_pMacInfo->ulNumMacAddrs;
        i++, pMai++ )
    {
        if( ulId == pMai->ulId || ulId == MAC_ADDRESS_ANY )
        {
            /* This MAC address has been used by the caller in the past. */
            baseMacAddr = (baseMacAddr + i) << 8;
            memcpy( pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,
                constMacAddrIncIndex);
            memcpy( pucaMacAddr + constMacAddrIncIndex, (unsigned char *)
                &baseMacAddr, NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex );
            pMai->chInUse = 1;
            pMaiFreeNoId = pMaiFreeId = NULL;
            break;
        }
        else
            if( pMai->chInUse == 0 )
            {
                if( pMai->ulId == 0 && pMaiFreeNoId == NULL )
                {
                    /* This is an available MAC address that has never been
                    * used.
                    */
                    pMaiFreeNoId = pMai;
                    ulIdxNoId = i;
                }
                else
                    if( pMai->ulId != 0 && pMaiFreeId == NULL )
                    {
#ifndef SKY
                        /* This is an available MAC address that has been used
                        * before.  Use addresses that have never been used
                        * first, before using this one.
                        */
                        pMaiFreeId = pMai;
                        ulIdxId = i;
#else /* SKY */
						/* if this MAC address is to be used for an ATM interface then it doesn't matter */
						/* if it has previously been used, we want ATM interfaces to use the Base MAC */
						if( (MAC_ADDRESS_ATM & ulId) || (MAC_ADDRESS_PTM & ulId) )
						{
							//pMaiFreeNoId = pMai;
							//ulIdxNoId = i;
                            /*ATM or PTM interface mac address is to be BaseMac+2*/
                            pMaiFreeNoId = g_pMacInfo->MacAddrs+2;
                            ulIdxNoId=2;
							break;
						}
						else
						{
							/* This is an available MAC address that has been used
							* before.  Use addresses that have never been used
							* first, before using this one.
							*/
							pMaiFreeId = pMai;
							ulIdxId = i;
						}
#endif /* SKY */
                    }
            }
    }

    if( pMaiFreeNoId || pMaiFreeId )
    {
        /* An available MAC address was found. */
        memcpy(pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,NVRAM_MAC_ADDRESS_LEN);
        if( pMaiFreeNoId )
        {
            baseMacAddr = (baseMacAddr + ulIdxNoId) << 8;
            memcpy( pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,
                constMacAddrIncIndex);
            memcpy( pucaMacAddr + constMacAddrIncIndex, (unsigned char *)
                &baseMacAddr, NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex );
            pMaiFreeNoId->ulId = ulId;
            pMaiFreeNoId->chInUse = 1;
        }
        else
        {
            baseMacAddr = (baseMacAddr + ulIdxId) << 8;
            memcpy( pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,
                constMacAddrIncIndex);
            memcpy( pucaMacAddr + constMacAddrIncIndex, (unsigned char *)
                &baseMacAddr, NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex );
            pMaiFreeId->ulId = ulId;
            pMaiFreeId->chInUse = 1;
        }

        kerSysMacAddressNotify(pucaMacAddr, MAC_ADDRESS_OP_GET);
    }
    else
        if( i == g_pMacInfo->ulNumMacAddrs )
            nRet = -EADDRNOTAVAIL;

    mutex_unlock(&macAddrMutex);

    return( nRet );
} /* kerSysGetMacAddr */

/* Allocates requested number of consecutive MAC addresses */
int kerSysGetMacAddresses( unsigned char *pucaMacAddr, unsigned int num_addresses, unsigned long ulId )
{
    const unsigned long constMacAddrIncIndex = 3;
    int nRet = 0;
    PMAC_ADDR_INFO pMai = NULL;
    PMAC_ADDR_INFO pMaiFreeId = NULL, pMaiFreeIdTemp;
    unsigned long i = 0, j = 0, ulIdxId = 0, baseMacAddr = 0;

    mutex_lock(&macAddrMutex);

    /* baseMacAddr = last 3 bytes of the base MAC address treated as a 24 bit integer */
    memcpy((unsigned char *) &baseMacAddr,
        &g_pMacInfo->ucaBaseMacAddr[constMacAddrIncIndex],
        NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex);
    baseMacAddr >>= 8;

#if defined(CONFIG_BCM96828)||defined(CONFIG_BCM96838)
    /*As epon mac should not be dynamicly changed, always use last 1(SLLID) or 8(MLLID) mac address(es)*/
    if (ulId == MAC_ADDRESS_EPONONU)
    {
        i = g_pMacInfo->ulNumMacAddrs - num_addresses;

        for (j = 0, pMai = &g_pMacInfo->MacAddrs[i]; j < num_addresses; j++) {
            pMaiFreeIdTemp = pMai + j;
            if (pMaiFreeIdTemp->chInUse != 0 && pMaiFreeIdTemp->ulId != MAC_ADDRESS_EPONONU) {
                printk("kerSysGetMacAddresses: epon mac address allocate failed, g_pMacInfo[%ld] reserved by 0x%lx\n", i+j, pMaiFreeIdTemp->ulId);
                break;
            }
        }

        if (j >= num_addresses) {
            pMaiFreeId = pMai;
            ulIdxId = i;
        }
    }
    else
#endif
    {
        for( i = 0, pMai = g_pMacInfo->MacAddrs; i < g_pMacInfo->ulNumMacAddrs;
            i++, pMai++ )
        {
            if( ulId == pMai->ulId || ulId == MAC_ADDRESS_ANY )
            {
                /* This MAC address has been used by the caller in the past. */
                baseMacAddr = (baseMacAddr + i) << 8;
                memcpy( pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,
                    constMacAddrIncIndex);
                memcpy( pucaMacAddr + constMacAddrIncIndex, (unsigned char *)
                    &baseMacAddr, NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex );
                pMai->chInUse = 1;
                pMaiFreeId = NULL;
                break;
            } else if( pMai->chInUse == 0 ) {
                /* check if it there are num_addresses to be checked starting from found MAC address */
                if ((i + num_addresses) >= g_pMacInfo->ulNumMacAddrs) {
                    nRet = -EADDRNOTAVAIL;
                    break;
                }

                for (j = 0; j < num_addresses; j++) {
                    pMaiFreeIdTemp = pMai + j;
                    if (pMaiFreeIdTemp->chInUse != 0) {
                        break;
                    }
                }
                if (j >= num_addresses) {
                    pMaiFreeId = pMai;
                    ulIdxId = i;
                    break;
                }
            }
        }
    }

    if(pMaiFreeId )
    {
        /* An available MAC address was found. */
        memcpy(pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,NVRAM_MAC_ADDRESS_LEN);
        baseMacAddr = (baseMacAddr + ulIdxId) << 8;
        memcpy( pucaMacAddr, g_pMacInfo->ucaBaseMacAddr,
                constMacAddrIncIndex);
        memcpy( pucaMacAddr + constMacAddrIncIndex, (unsigned char *)
                &baseMacAddr, NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex );
        pMaiFreeIdTemp = pMai;
        for (j = 0; j < num_addresses; j++) {
            pMaiFreeIdTemp->ulId = ulId;
            pMaiFreeIdTemp->chInUse = 1;
            pMaiFreeIdTemp++;
        }
    }
    else {
        nRet = -EADDRNOTAVAIL;
    }

    mutex_unlock(&macAddrMutex);

    return( nRet );
} /* kerSysGetMacAddr */

int kerSysReleaseMacAddresses( unsigned char *pucaMacAddr, unsigned int num_addresses )
{
    const unsigned long constMacAddrIncIndex = 3;
    int i, nRet = -EINVAL;
    unsigned long ulIdx = 0;
    unsigned long baseMacAddr = 0;
    unsigned long relMacAddr = 0;

    mutex_lock(&macAddrMutex);

    /* baseMacAddr = last 3 bytes of the base MAC address treated as a 24 bit integer */
    memcpy((unsigned char *) &baseMacAddr,
        &g_pMacInfo->ucaBaseMacAddr[constMacAddrIncIndex],
        NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex);
    baseMacAddr >>= 8;

    /* Get last 3 bytes of MAC address to release. */
    memcpy((unsigned char *) &relMacAddr, &pucaMacAddr[constMacAddrIncIndex],
        NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex);
    relMacAddr >>= 8;

    ulIdx = relMacAddr - baseMacAddr;

    if( ulIdx < g_pMacInfo->ulNumMacAddrs )
    {
        for(i=0; i<num_addresses; i++) {
            if ((ulIdx + i) < g_pMacInfo->ulNumMacAddrs) {
                PMAC_ADDR_INFO pMai = &g_pMacInfo->MacAddrs[ulIdx + i];
                if( pMai->chInUse == 1 )
                {
                    pMai->chInUse = 0;
                    nRet = 0;
                }
            } else {
                printk("Request to release %d addresses failed as "
                    " the one or more of the addresses, starting from"
                    " %dth address from given address, requested for release"
                    " is not in the list of available MAC addresses \n", num_addresses, i);
                break;
            }
        }
    }

    mutex_unlock(&macAddrMutex);

    return( nRet );
} /* kerSysReleaseMacAddr */


int kerSysReleaseMacAddress( unsigned char *pucaMacAddr )
{
    const unsigned long constMacAddrIncIndex = 3;
    int nRet = -EINVAL;
    unsigned long ulIdx = 0;
    unsigned long baseMacAddr = 0;
    unsigned long relMacAddr = 0;

    mutex_lock(&macAddrMutex);

    /* baseMacAddr = last 3 bytes of the base MAC address treated as a 24 bit integer */
    memcpy((unsigned char *) &baseMacAddr,
        &g_pMacInfo->ucaBaseMacAddr[constMacAddrIncIndex],
        NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex);
    baseMacAddr >>= 8;

    /* Get last 3 bytes of MAC address to release. */
    memcpy((unsigned char *) &relMacAddr, &pucaMacAddr[constMacAddrIncIndex],
        NVRAM_MAC_ADDRESS_LEN - constMacAddrIncIndex);
    relMacAddr >>= 8;

    ulIdx = relMacAddr - baseMacAddr;

    if( ulIdx < g_pMacInfo->ulNumMacAddrs )
    {
        PMAC_ADDR_INFO pMai = &g_pMacInfo->MacAddrs[ulIdx];
        if( pMai->chInUse == 1 )
        {
            kerSysMacAddressNotify(pucaMacAddr, MAC_ADDRESS_OP_RELEASE);

            pMai->chInUse = 0;
            nRet = 0;
        }
    }

    mutex_unlock(&macAddrMutex);

    return( nRet );
} /* kerSysReleaseMacAddr */


void kerSysGetGponSerialNumber( unsigned char *pGponSerialNumber )
{
    strcpy( pGponSerialNumber, g_pGponInfo->gponSerialNumber );
}


void kerSysGetGponPassword( unsigned char *pGponPassword )
{
    strcpy( pGponPassword, g_pGponInfo->gponPassword );
}

int kerSysGetSdramSize( void )
{
    return( (int) g_ulSdramSize );
} /* kerSysGetSdramSize */


#if defined(CONFIG_BCM96368)
/*
 * This function returns:
 * MEMC_32BIT_BUS for 32-bit SDRAM
 * MEMC_16BIT_BUS for 16-bit SDRAM
 */
unsigned int kerSysGetSdramWidth( void )
{
    return (unsigned int)(g_ulSdramWidth);
} /* kerSysGetSdramWidth */
#endif


/*Read Wlan Params data from CFE */
int kerSysGetWlanSromParams( unsigned char *wlanParams, unsigned short len)
{
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("kerSysGetWlanSromParams: could not read nvram data\n");
        return -1;
    }

    memcpy( wlanParams,
           (char *)pNvramData + ((size_t) &((NVRAM_DATA *)0)->wlanParams),
            len );
    kfree(pNvramData);

    return 0;
}

/*Read Wlan Params data from CFE */
int kerSysGetAfeId( unsigned long *afeId )
{
    NVRAM_DATA *pNvramData;

    if (NULL == (pNvramData = readNvramData()))
    {
        printk("kerSysGetAfeId: could not read nvram data\n");
        return -1;
    }

    afeId [0] = pNvramData->afeId[0];
    afeId [1] = pNvramData->afeId[1];
    kfree(pNvramData);

    return 0;
}

void kerSysLedCtrl(BOARD_LED_NAME ledName, BOARD_LED_STATE ledState)
{
    if (g_ledInitialized)
       boardLedCtrl(ledName, ledState);
}

/*functionto receive message from usersapce
 * Currently we dont expect any messages fromm userspace
 */
void kerSysRecvFrmMonitorTask(struct sk_buff *skb)
{

   /*process the message here*/
   printk(KERN_WARNING "unexpected skb received at %s \n",__FUNCTION__);
   kfree_skb(skb);
   return;
}

void kerSysInitMonitorSocket( void )
{
   g_monitor_nl_sk = netlink_kernel_create(&init_net, NETLINK_BRCM_MONITOR, 0, kerSysRecvFrmMonitorTask, NULL, THIS_MODULE);

   if(!g_monitor_nl_sk)
   {
      printk(KERN_ERR "Failed to create a netlink socket for monitor\n");
      return;
   }

}


void kerSysSendtoMonitorTask(int msgType, char *msgData, int msgDataLen)
{

   struct sk_buff *skb =  NULL;
   struct nlmsghdr *nl_msgHdr = NULL;
   unsigned int payloadLen =sizeof(struct nlmsghdr);

   if(!g_monitor_nl_pid)
   {
      printk(KERN_INFO "message received before monitor task is initialized %s \n",__FUNCTION__);
      return;
   }

   if(msgData && (msgDataLen > MAX_PAYLOAD_LEN))
   {
      printk(KERN_ERR "invalid message len in %s",__FUNCTION__);
      return;
   }

   payloadLen += msgDataLen;
   payloadLen = NLMSG_SPACE(payloadLen);

   /*Alloc skb ,this check helps to call the fucntion from interrupt context */

   if(in_atomic())
   {
      skb = alloc_skb(payloadLen, GFP_ATOMIC);
   }
   else
   {
      skb = alloc_skb(payloadLen, GFP_KERNEL);
   }

   if(!skb)
   {
      printk(KERN_ERR "failed to alloc skb in %s",__FUNCTION__);
      return;
   }

   nl_msgHdr = (struct nlmsghdr *)skb->data;
   nl_msgHdr->nlmsg_type = msgType;
   nl_msgHdr->nlmsg_pid=0;/*from kernel */
   nl_msgHdr->nlmsg_len = payloadLen;
   nl_msgHdr->nlmsg_flags =0;

   if(msgData)
   {
      memcpy(NLMSG_DATA(nl_msgHdr),msgData,msgDataLen);
   }

   NETLINK_CB(skb).pid = 0; /*from kernel */

   skb->len = payloadLen;

   netlink_unicast(g_monitor_nl_sk, skb, g_monitor_nl_pid, MSG_DONTWAIT);
   return;
}

void kerSysCleanupMonitorSocket(void)
{
   g_monitor_nl_pid = 0 ;
   sock_release(g_monitor_nl_sk->sk_socket);
}

// Must be called with flashImageMutex held
static PFILE_TAG getTagFromPartition(int imageNumber)
{
    // Define space for file tag structures for two partitions.  Make them static
    // so caller does not have to worry about allocation/deallocation.
    // Make sure they're big enough for the file tags plus an block number
    // (an integer) appended.
    static unsigned char sectAddr1[sizeof(FILE_TAG) + sizeof(int)];
    static unsigned char sectAddr2[sizeof(FILE_TAG) + sizeof(int)];

    int blk = 0;
    UINT32 crc;
    PFILE_TAG pTag = NULL;
    unsigned char *pBase = flash_get_memptr(0);
    unsigned char *pSectAddr = NULL;

    unsigned  int reserverdBytersAuxfs = flash_get_reserved_bytes_auxfs();

    /* The image tag for the first image is always after the boot loader.
     * The image tag for the second image, if it exists, is at one half
     * of the flash size.
     */
    if( imageNumber == 1 )
    {
        // Get the flash info and block number for parition 1 at the base of the flash
        FLASH_ADDR_INFO flash_info;

        kerSysFlashAddrInfoGet(&flash_info);
        blk = flash_get_blk((int)(pBase+flash_info.flash_rootfs_start_offset));
        pSectAddr = sectAddr1;
    }
    else if( imageNumber == 2 )
    {
        // Get block number for partition2 at middle of the device (not counting space for aux
        // file system).
        blk = flash_get_blk((int) (pBase + ((flash_get_total_size()-reserverdBytersAuxfs) / 2)));
        pSectAddr = sectAddr2;
    }

    // Now that you have a block number, use it to read in the file tag
    if( blk )
    {
        int *pn;    // Use to append block number at back of file tag

        // Clear out space for file tag structures
        memset(pSectAddr, 0x00, sizeof(FILE_TAG));

        // Get file tag
        flash_read_buf((unsigned short) blk, 0, pSectAddr, sizeof(FILE_TAG));

        // Figure out CRC of file tag so we can check it below
        crc = CRC32_INIT_VALUE;
        crc = getCrc32(pSectAddr, (UINT32)TAG_LEN-TOKEN_LEN, crc);

        // Get ready to return file tag pointer
        pTag = (PFILE_TAG) pSectAddr;

        // Append block number after file tag
        pn = (int *) (pTag + 1);
        *pn = blk;

        // One final check - if the file tag CRC is not OK, return NULL instead
        if (crc != (UINT32)(*(UINT32*)(pTag->tagValidationToken)))
            pTag = NULL;
    }

    // All done - return file tag pointer
    return( pTag );
}

// must be called with flashImageMutex held
static int getPartitionFromTag( PFILE_TAG pTag )
{
    int ret = 0;

    if( pTag )
    {
        PFILE_TAG pTag1 = getTagFromPartition(1);
        PFILE_TAG pTag2 = getTagFromPartition(2);
        int sequence = simple_strtoul(pTag->imageSequence,  NULL, 10);
        int sequence1 = (pTag1) ? simple_strtoul(pTag1->imageSequence, NULL, 10)
            : -1;
        int sequence2 = (pTag2) ? simple_strtoul(pTag2->imageSequence, NULL, 10)
            : -1;

        if( pTag1 && sequence == sequence1 )
            ret = 1;
        else
            if( pTag2 && sequence == sequence2 )
                ret = 2;
    }

    return( ret );
}

// must be called with flashImageMutex held
static PFILE_TAG getBootImageTag(void)
{
    static int displayFsAddr = 1;
    PFILE_TAG pTag = NULL;
    PFILE_TAG pTag1 = getTagFromPartition(1);
    PFILE_TAG pTag2 = getTagFromPartition(2);

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    if( pTag1 && pTag2 )
    {
        /* Two images are flashed. */
        int sequence1 = simple_strtoul(pTag1->imageSequence, NULL, 10);
        int sequence2 = simple_strtoul(pTag2->imageSequence, NULL, 10);
        int imgid = 0;

        kerSysBlParmsGetInt(BOOTED_IMAGE_ID_NAME, &imgid);
        if( imgid == BOOTED_OLD_IMAGE )
            pTag = (sequence2 < sequence1) ? pTag2 : pTag1;
        else
            pTag = (sequence2 > sequence1) ? pTag2 : pTag1;
    }
    else
        /* One image is flashed. */
        pTag = (pTag2) ? pTag2 : pTag1;

    if( pTag && displayFsAddr )
    {
        displayFsAddr = 0;
        printk("File system address: 0x%8.8lx\n",
            simple_strtoul(pTag->rootfsAddress, NULL, 10) + BOOT_OFFSET);
    }

    return( pTag );
}

// Must be called with flashImageMutex held
static void UpdateImageSequenceNumber( unsigned char *imageSequence )
{
    int newImageSequence = 0;
    PFILE_TAG pTag = getTagFromPartition(1);

    if( pTag )
        newImageSequence = simple_strtoul(pTag->imageSequence, NULL, 10);

    pTag = getTagFromPartition(2);
    if(pTag && simple_strtoul(pTag->imageSequence, NULL, 10) > newImageSequence)
        newImageSequence = simple_strtoul(pTag->imageSequence, NULL, 10);

    newImageSequence++;
    sprintf(imageSequence, "%d", newImageSequence);
}


/* Must be called with flashImageMutex held */
static int flashFsKernelImage( unsigned char *imagePtr, int imageLen,
    int flashPartition, int *numPartitions )
{
    int status = 0;
    PFILE_TAG pTag = (PFILE_TAG) imagePtr;
    int rootfsAddr = simple_strtoul(pTag->rootfsAddress, NULL, 10);
    int kernelAddr = simple_strtoul(pTag->kernelAddress, NULL, 10);
    char *tagFs = imagePtr;
    unsigned int baseAddr = (unsigned int) flash_get_memptr(0);
    unsigned int totalSize = (unsigned int) flash_get_total_size();
    unsigned int reservedBytesAtEnd;
    unsigned int reserverdBytersAuxfs;
    unsigned int availableSizeOneImg;
    unsigned int reserveForTwoImages;
    unsigned int availableSizeTwoImgs;

#ifndef CONFIG_SKY_ETHAN
    unsigned int newImgSize = simple_strtoul(pTag->rootfsLen, NULL, 10) +
        simple_strtoul(pTag->kernelLen, NULL, 10);
#else
    unsigned int newImgSize = simple_strtoul(pTag->rootfsLen, NULL, 10) +
        simple_strtoul(pTag->kernelLen, NULL, 10) + TAG_LEN;
#endif

    PFILE_TAG pCurTag = getBootImageTag();
    int nCurPartition = getPartitionFromTag( pCurTag );
    int should_yield =
        (flashPartition == 0 || flashPartition == nCurPartition) ? 0 : 1;
    UINT32 crc;
    unsigned int curImgSize = 0;
    unsigned int rootfsOffset = (unsigned int) rootfsAddr - IMAGE_BASE - TAG_LEN;
    FLASH_ADDR_INFO flash_info;

    NVRAM_DATA *pNvramData;
    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    if (NULL == (pNvramData = readNvramData()))
    {
        return -ENOMEM;
    }

    kerSysFlashAddrInfoGet(&flash_info);
    if( rootfsOffset < flash_info.flash_rootfs_start_offset )
    {
        // Increase rootfs and kernel addresses by the difference between
        // rootfs offset and what it needs to be.
        rootfsAddr += flash_info.flash_rootfs_start_offset - rootfsOffset;
        kernelAddr += flash_info.flash_rootfs_start_offset - rootfsOffset;
        sprintf(pTag->rootfsAddress,"%lu", (unsigned long) rootfsAddr);
        sprintf(pTag->kernelAddress,"%lu", (unsigned long) kernelAddr);
        crc = CRC32_INIT_VALUE;
        crc = getCrc32((unsigned char *)pTag, (UINT32)TAG_LEN-TOKEN_LEN, crc);
        *(unsigned long *) &pTag->tagValidationToken[0] = crc;
    }

#ifdef CONFIG_SKY_ETHAN
	// this is a dirty workaround to force writing of the signature
	// The signature is at the end (its around 140 bytes), followed by SKY_HEADER
	if ((newImgSize + SKY_SIGNATURE_SIZE) > imageLen) {
		newImgSize = imageLen;
	}
	else {
		newImgSize += SKY_SIGNATURE_SIZE;
	}
#endif

    rootfsAddr += BOOT_OFFSET;
    kernelAddr += BOOT_OFFSET;

    reservedBytesAtEnd = flash_get_reserved_bytes_at_end(&flash_info);
    reserverdBytersAuxfs = flash_get_reserved_bytes_auxfs();
    availableSizeOneImg = totalSize - ((unsigned int) rootfsAddr - baseAddr) -
        reservedBytesAtEnd- reserverdBytersAuxfs;

    reserveForTwoImages =
        (flash_info.flash_rootfs_start_offset > reservedBytesAtEnd)
        ? flash_info.flash_rootfs_start_offset : reservedBytesAtEnd;
    availableSizeTwoImgs = ((totalSize-reserverdBytersAuxfs)/ 2) - reserveForTwoImages;

    printk("availableSizeOneImage=%dKB availableSizeTwoImgs=%dKB reserverdBytersAuxfs=%dKB reserve=%dKB\n",
        availableSizeOneImg/1024, availableSizeTwoImgs/1024, reserverdBytersAuxfs/1024, reserveForTwoImages/1024);

    if( pCurTag )
    {
        curImgSize = simple_strtoul(pCurTag->rootfsLen, NULL, 10) +
            simple_strtoul(pCurTag->kernelLen, NULL, 10);
    }

    if( newImgSize > availableSizeOneImg)
    {
        printk("Illegal image size %d.  Image size must not be greater "
            "than %d.\n", newImgSize, availableSizeOneImg);
        kfree(pNvramData);
        return -1;
    }

    *numPartitions = (curImgSize <= availableSizeTwoImgs &&
         newImgSize <= availableSizeTwoImgs &&
         flashPartition != nCurPartition) ? 2 : 1;

    // If the current image fits in half the flash space and the new
    // image to flash also fits in half the flash space, then flash it
    // in the partition that is not currently being used to boot from.
    if( curImgSize <= availableSizeTwoImgs &&
        newImgSize <= availableSizeTwoImgs &&
        ((nCurPartition == 1 && flashPartition != 1) || flashPartition == 2) )
    {
        // Update rootfsAddr to point to the second boot partition.
		int offset = ((totalSize-reserverdBytersAuxfs) / 2) + TAG_LEN;
        sprintf(((PFILE_TAG) tagFs)->kernelAddress, "%lu",
            (unsigned long) IMAGE_BASE + offset + (kernelAddr - rootfsAddr));
        kernelAddr = baseAddr + offset + (kernelAddr - rootfsAddr);

        sprintf(((PFILE_TAG) tagFs)->rootfsAddress, "%lu",
            (unsigned long) IMAGE_BASE + offset);
        rootfsAddr = baseAddr + offset;
    }

    UpdateImageSequenceNumber( ((PFILE_TAG) tagFs)->imageSequence );
    crc = CRC32_INIT_VALUE;
    crc = getCrc32((unsigned char *)tagFs, (UINT32)TAG_LEN-TOKEN_LEN, crc);
    *(unsigned long *) &((PFILE_TAG) tagFs)->tagValidationToken[0] = crc;

    // Now, perform the actual flash write
#ifndef CONFIG_SKY_ETHAN
    if( (status = kerSysBcmImageSet((rootfsAddr-TAG_LEN), tagFs, TAG_LEN + newImgSize, should_yield)) != 0 )
#else
    if( (status = kerSysBcmImageSet((rootfsAddr-TAG_LEN), tagFs, newImgSize, should_yield)) != 0 )
#endif
    {
        printk("Failed to flash root file system. Error: %d\n", status);
        kfree(pNvramData);
        return status;
    }

    // Free buffers
    kfree(pNvramData);
    return(status);
}

#define IMAGE_VERSION_FILE_NAME "/etc/image_version"
#define IMAGE_VERSION_MAX_SIZE  64
static int getImageVersion( int imageNumber, char *verStr, int verStrSize)
{
    static char imageVersions[2][IMAGE_VERSION_MAX_SIZE] = {{'\0'}, {'\0'}};
    int ret = 0; /* zero bytes copied to verStr so far */

    if( (imageNumber == 1 && imageVersions[0][0] != '\0') ||
        (imageNumber == 2 && imageVersions[1][0] != '\0') )
    {
        /* The image version was previously retrieved and saved.  Copy it to
         * the caller's buffer.
         */
        if( verStrSize > IMAGE_VERSION_MAX_SIZE )
            ret = IMAGE_VERSION_MAX_SIZE;
        else
            ret = verStrSize;
        memcpy(verStr, imageVersions[imageNumber - 1], ret);
    }
    else
    {
        unsigned long rootfs_ofs;
        if( kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs) == -1 )
        {
            /* NOR Flash */
            PFILE_TAG pTag = NULL;

            if( imageNumber == 1 )
                pTag = getTagFromPartition(1);
            else
                if( imageNumber == 2 )
                    pTag = getTagFromPartition(2);

            if( pTag )
            {
                if( verStrSize > sizeof(pTag->imageVersion) )
                    ret = sizeof(pTag->imageVersion);
                else
                    ret = verStrSize;

                memcpy(verStr, pTag->imageVersion, ret);

                /* Save version string for subsequent calls to this function. */
                memcpy(imageVersions[imageNumber - 1], verStr, ret);
            }
        }
        else
        {
            /* NAND Flash */
            NVRAM_DATA *pNvramData;

            if( (imageNumber == 1 || imageNumber == 2) &&
                (pNvramData = readNvramData()) != NULL )
            {
                char *pImgVerFileName = NULL;

                mm_segment_t fs;
                struct file *fp;
                int updatePart, getFromCurPart;

                // updatePart is the partition number that is not booted
                // getFromCurPart is 1 to retrieve info from the booted partition
                updatePart =
                    (rootfs_ofs==pNvramData->ulNandPartOfsKb[NP_ROOTFS_1])
                    ? 2 : 1;
                getFromCurPart = (updatePart == imageNumber) ? 0 : 1;

                fs = get_fs();
                set_fs(get_ds());
                if( getFromCurPart == 0 )
                {
                    struct mtd_info *mtd;
                    pImgVerFileName = "/mnt/" IMAGE_VERSION_FILE_NAME;

                    mtd = get_mtd_device_nm("bootfs_update");
                    if( !IS_ERR_OR_NULL(mtd) )
                    {
                        sys_mount("mtd:bootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
                        put_mtd_device(mtd);
                    }
                    else
                        sys_mount("mtd:rootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
                }
                else
                    pImgVerFileName = IMAGE_VERSION_FILE_NAME;

                fp = filp_open(pImgVerFileName, O_RDONLY, 0);
                if( !IS_ERR(fp) )
                {
                    /* File open successful, read version string from file. */
                    if(fp->f_op && fp->f_op->read)
                    {
                        fp->f_pos = 0;
                        ret = fp->f_op->read(fp, (void *) verStr, verStrSize,
                            &fp->f_pos);
                        verStr[ret] = '\0';

                        /* Save version string for subsequent calls to this
                         * function.
                         */
                        memcpy(imageVersions[imageNumber - 1], verStr, ret);
                    }
                    filp_close(fp, NULL);
                }

                if( getFromCurPart == 0 )
                    sys_umount("/mnt", 0);

                set_fs(fs);
                kfree(pNvramData);
            }
        }
    }

    return( ret );
}

#ifdef SKY
PFILE_TAG kerSysSetTagSequenceNumber(int imageNumber, int seq) {
    PFILE_TAG pTag = NULL;
    UINT32 crc;

    switch( imageNumber ) {
    case 0:
        pTag = getBootImageTag();
        break;

    case 1:
        pTag = getTagFromPartition(1);
        break;

    case 2:
        pTag = getTagFromPartition(2);
        break;

    default:
        break;
    }

    if( pTag ) {
        sprintf(pTag->imageSequence, "%d", seq);
        crc = CRC32_INIT_VALUE;
        crc = getCrc32((unsigned char *)pTag, (UINT32)TAG_LEN-TOKEN_LEN, crc);
        *(unsigned long *) &pTag->tagValidationToken[0] = crc;
    }

    return(pTag);
}


#ifdef SKY_FLASH_API
static void sky_flash_dump_write(char *str, int dump)
{
	int param[3];
	int i =0;
	char **bp = &str;
	char *pch;
	char *endptr=NULL;
	int sector = 0, length = 0, offset = 0;

#ifndef CONFIG_FLASH_CHIP_SIZE
#error CONFIG_FLASH_CHIP_SIZE is not defined
#endif

	unsigned int last = ((CONFIG_FLASH_CHIP_SIZE*1024*1024)/flash_get_sector_size(0));


	pch = strsep(bp,":");
	param[0]=(int)(simple_strtoul(pch,&endptr,0));
	while (i <3)
	{
		param[i++] = simple_strtoul(pch,&endptr,0);
		pch = strsep(bp,":");
	}
	printk("sector =%d, offset =%d, len =%d\n", param[0], param[1], param[2]);
	sector = param[0];
	offset = param[1];
	length = param[2];
	if(sector < 0 || sector >= last)
	{
		printk("invalid Sector number - %d/%d\n",sector,last);
		return;
	}
	if(offset < 0 || offset > 65534)
	{
		printk("invalid Offset, offset should be between 0 - 65534 - %d\n",offset);
		return;
	}

	if(length < 1 || length > 65535)
	{
		printk("invalid length, length should be between 1 - 65535 - %d\n",offset);
		return;
	}
	if(dump)
	{
		kerSysFlashDump((unsigned int)sector, offset, length);
	}
	else
	{
		kerSysFlashWrite((unsigned int)sector, offset, length);
	}

	return;
}
#endif // SKY_FLASH_API


#endif


PFILE_TAG kerSysUpdateTagSequenceNumber(int imageNumber)
{
    PFILE_TAG pTag = NULL;
    UINT32 crc;

    switch( imageNumber )
    {
    case 0:
        pTag = getBootImageTag();
        break;

    case 1:
        pTag = getTagFromPartition(1);
        break;

    case 2:
        pTag = getTagFromPartition(2);
        break;

    default:
        break;
    }

    if( pTag )
    {
        UpdateImageSequenceNumber( pTag->imageSequence );
        crc = CRC32_INIT_VALUE;
        crc = getCrc32((unsigned char *)pTag, (UINT32)TAG_LEN-TOKEN_LEN, crc);
        *(unsigned long *) &pTag->tagValidationToken[0] = crc;
    }

    return(pTag);
}

int kerSysGetSequenceNumber(int imageNumber)
{
    int seqNumber = -1;
    unsigned long rootfs_ofs;
    if( kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs) == -1 )
    {
        /* NOR Flash */
        PFILE_TAG pTag = NULL;

        switch( imageNumber )
        {
        case 0:
            pTag = getBootImageTag();
            break;

        case 1:
            pTag = getTagFromPartition(1);
            break;

        case 2:
            pTag = getTagFromPartition(2);
            break;

        default:
            break;
        }

        if( pTag )
            seqNumber= simple_strtoul(pTag->imageSequence, NULL, 10);
    }
    else
    {
        /* NAND Flash */
        NVRAM_DATA *pNvramData;

        if( (pNvramData = readNvramData()) != NULL )
        {
            char fname[] = NAND_CFE_RAM_NAME;
            char cferam_buf[32], cferam_fmt[32];
            int i;

            mm_segment_t fs;
            struct file *fp;
            int updatePart, getFromCurPart;

            // updatePart is the partition number that is not booted
            // getFromCurPart is 1 to retrieive info from the booted partition
            updatePart = (rootfs_ofs==pNvramData->ulNandPartOfsKb[NP_ROOTFS_1])
                ? 2 : 1;
            getFromCurPart = (updatePart == imageNumber) ? 0 : 1;

            fs = get_fs();
            set_fs(get_ds());
            if( getFromCurPart == 0 )
            {
                struct mtd_info *mtd;
                strcpy(cferam_fmt, "/mnt/");
                mtd = get_mtd_device_nm("bootfs_update");
                if( !IS_ERR_OR_NULL(mtd) )
                {
                    sys_mount("mtd:bootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
                    put_mtd_device(mtd);
                }
                else
                    sys_mount("mtd:rootfs_update", "/mnt","jffs2",MS_RDONLY,NULL);
            }
            else
            {
                struct mtd_info *mtd;
                mtd = get_mtd_device_nm("bootfs");
                if( !IS_ERR_OR_NULL(mtd) )
                {
                    strcpy(cferam_fmt, "/bootfs/");
                    put_mtd_device(mtd);
                }
                else
                    cferam_fmt[0] = '\0';
            }

            /* Find the sequence number of the specified partition. */
            fname[strlen(fname) - 3] = '\0'; /* remove last three chars */
            strcat(cferam_fmt, fname);
            strcat(cferam_fmt, "%3.3d");

            for( i = 0; i < 999; i++ )
            {
                sprintf(cferam_buf, cferam_fmt, i);
                fp = filp_open(cferam_buf, O_RDONLY, 0);
                if (!IS_ERR(fp) )
                {
                    filp_close(fp, NULL);

                    /* Seqence number found. */
                    seqNumber = i;
                    break;
                }
            }

            if( getFromCurPart == 0 )
                sys_umount("/mnt", 0);

            set_fs(fs);
            kfree(pNvramData);
        }
    }

    return(seqNumber);
}

static int getBootedValue(int getBootedPartition)
{
    static int s_bootedPartition = -1;
    int ret = -1;
    int imgId = -1;

    kerSysBlParmsGetInt(BOOTED_IMAGE_ID_NAME, &imgId);

    /* The boot loader parameter will only be "new image", "old image" or "only
     * image" in order to be compatible with non-OMCI image update. If the
     * booted partition is requested, convert this boot type to partition type.
     */
    if( imgId != -1 )
    {
        if( getBootedPartition )
        {
            if( s_bootedPartition != -1 )
                ret = s_bootedPartition;
            else
            {
                /* Get booted partition. */
                int seq1 = kerSysGetSequenceNumber(1);
                int seq2 = kerSysGetSequenceNumber(2);

                switch( imgId )
                {
                case BOOTED_NEW_IMAGE:
                    if( seq1 == -1 || seq2 > seq1 )
                        ret = BOOTED_PART2_IMAGE;
                    else
                        if( seq2 == -1 || seq1 >= seq2 )
                            ret = BOOTED_PART1_IMAGE;
                    break;

                case BOOTED_OLD_IMAGE:
                    if( seq1 == -1 || seq2 < seq1 )
                        ret = BOOTED_PART2_IMAGE;
                    else
                        if( seq2 == -1 || seq1 <= seq2 )
                            ret = BOOTED_PART1_IMAGE;
                    break;

                case BOOTED_ONLY_IMAGE:
                    ret = (seq1 == -1) ? BOOTED_PART2_IMAGE : BOOTED_PART1_IMAGE;
                    break;

                default:
                    break;
                }

                s_bootedPartition = ret;
            }
        }
        else
            ret = imgId;
    }

    return( ret );
}


#if !defined(CONFIG_BRCM_IKOS)
#ifdef SKY
PFILE_TAG kerSysImageTagGet(unsigned int bank)
#else
PFILE_TAG kerSysImageTagGet(void)
#endif
{
    PFILE_TAG tag;

    mutex_lock(&flashImageMutex);
#ifdef SKY
	switch( bank )
	{
		case 1:
		case 2:
			tag = getTagFromPartition(bank);
			break;

		case 0:
		default:
#endif
			tag = getBootImageTag();
#ifdef SKY
			break;
	}
#endif

    mutex_unlock(&flashImageMutex);

    return tag;
}
#else
PFILE_TAG kerSysImageTagGet(void)
{
    return( (PFILE_TAG) (FLASH_BASE + FLASH_LENGTH_BOOT_ROM));
}
#endif

/*
 * Common function used by BCM_IMAGE_CFE and BCM_IMAGE_WHOLE ioctls.
 * This function will acquire the flashImageMutex
 *
 * @return 0 on success, -1 on failure.
 */
#ifndef SKY
static int commonImageWrite(int flash_start_addr, char *string, int size,
    int *pnoReboot, int partition)
#else /* SKY */
static int commonImageWrite(int flash_start_addr, char *string, int size,
    int *pnoReboot, int partition,char *buf)
#endif /* SKY */
{
    NVRAM_DATA * pNvramDataOrig;
    NVRAM_DATA * pNvramDataNew=NULL;
    int ret;

#if defined (CONFIG_BCM968500)
    /* check if CFE ROM in image is compiled for right DDR type */
    char *ddr_type = (char *)string + 0x980;
    DDR_BRIDGE_GENERAL_CONFIG_DDR_PARAMS_DTE ddr_params;

    BL_DDR_BRIDGE_GENERAL_CONFIG_DDR_PARAMS_READ(ddr_params);
    if((!memcmp(ddr_type, "DDR2", 4) && ddr_params.type) || (!memcmp(ddr_type, "DDR3", 4) && !ddr_params.type))
    {
        printk("\nImage contains CFE ROM compiled for wrong DDR type\n");
        return -1;
    }
#endif

    mutex_lock(&flashImageMutex);

    // Get a copy of the nvram before we do the image write operation
    if (NULL != (pNvramDataOrig = readNvramData()))
    {
        unsigned long rootfs_ofs;

        if( kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *) &rootfs_ofs) == -1 )
        {
            /* NOR flash */
#ifndef SKY
            ret = kerSysBcmImageSet(flash_start_addr, string, size, 0);
#else /* SKY */
      ret = kerSysBcmImageSet(flash_start_addr, string, size, 0); // Sajjan made it to 0, no need of any other process to get hold of cpu
       //ret = kerSysBcmImageSet(flash_start_addr, string, size, buf==0?0:1); // Nk made it 1
#endif /* SKY */
        }
        else
        {
            /* NAND flash */
            char *rootfs_part = "image_update";

            if( partition && rootfs_ofs == pNvramDataOrig->ulNandPartOfsKb[
                NP_ROOTFS_1 + partition - 1] )
            {
                /* The image to be flashed is the booted image. Force board
                 * reboot.
                 */
                rootfs_part = "image";
                if( pnoReboot )
                    *pnoReboot = 0;
            }

            ret = kerSysBcmNandImageSet(rootfs_part, string, size,
                (pnoReboot) ? *pnoReboot : 0);
        }

        /*
         * After the image is written, check the nvram.
         * If nvram is bad, write back the original nvram.
         */
        pNvramDataNew = readNvramData();
        if ((0 != ret) ||
            (NULL == pNvramDataNew) ||
            (BpSetBoardId(pNvramDataNew->szBoardId) != BP_SUCCESS)
#if defined (CONFIG_BCM_ENDPOINT_MODULE)
            || (BpSetVoiceBoardId(pNvramDataNew->szVoiceBoardId) != BP_SUCCESS)
#endif
            )
        {
            // we expect this path to be taken.  When a CFE or whole image
            // is written, it typically does not have a valid nvram block
            // in the image.  We detect that condition here and restore
            // the previous nvram settings.  Don't print out warning here.
            writeNvramDataCrcLocked(pNvramDataOrig);

            // don't modify ret, it is return value from kerSysBcmImageSet
        }
    }
    else
    {
        ret = -1;
    }

    mutex_unlock(&flashImageMutex);

    if (pNvramDataOrig)
        kfree(pNvramDataOrig);
    if (pNvramDataNew)
        kfree(pNvramDataNew);

    return ret;
}

struct file_operations monitor_fops;

#if defined(HAVE_UNLOCKED_IOCTL)
static long board_unlocked_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    struct inode *inode;
    long rt;

    inode = filep->f_dentry->d_inode;

    mutex_lock(&ioctlMutex);
    rt = board_ioctl( inode, filep, cmd, arg );
    mutex_unlock(&ioctlMutex);
    return rt;

}
#endif


//********************************************************************************************
// misc. ioctl calls come to here. (flash, led, reset, kernel memory access, etc.)
//********************************************************************************************
static int board_ioctl( struct inode *inode, struct file *flip,
                       unsigned int command, unsigned long arg )
{
    int ret = 0;
    BOARD_IOCTL_PARMS ctrlParms;
    unsigned char ucaMacAddr[NVRAM_MAC_ADDRESS_LEN];

    switch (command) {
    case BOARD_IOCTL_FLASH_WRITE:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {

            switch (ctrlParms.action) {
            case SCRATCH_PAD:
                if (ctrlParms.offset == -1)
                    ret =  kerSysScratchPadClearAll();
                else
                    ret = kerSysScratchPadSet(ctrlParms.string, ctrlParms.buf, ctrlParms.offset);
                break;

            case PERSISTENT:
                ret = kerSysPersistentSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case BACKUP_PSI:
                ret = kerSysBackupPsiSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case SYSLOG:
                ret = kerSysSyslogSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;
            case SKY_AUXFS_SECTOR:
                ret = KerSysEraseAuxfs();
                break;
            case NVRAM:
            {
                NVRAM_DATA * pNvramData;

                /*
                 * Note: even though NVRAM access is protected by
                 * flashImageMutex at the kernel level, this protection will
                 * not work if two userspaces processes use ioctls to get
                 * NVRAM data, modify it, and then use this ioctl to write
                 * NVRAM data.  This seems like an unlikely scenario.
                 */
                mutex_lock(&flashImageMutex);
                if (NULL == (pNvramData = readNvramData()))
                {
                    mutex_unlock(&flashImageMutex);
                    return -ENOMEM;
                }
                if ( !strncmp(ctrlParms.string, "WLANDATA", 8 ) ) { //Wlan Data data
                    memset((char *)pNvramData + ((size_t) &((NVRAM_DATA *)0)->wlanParams),
                        0, sizeof(pNvramData->wlanParams) );
                    memcpy( (char *)pNvramData + ((size_t) &((NVRAM_DATA *)0)->wlanParams),
                        ctrlParms.string+8,
                        ctrlParms.strLen-8);
                    writeNvramDataCrcLocked(pNvramData);
                }
                else {
                    // assumes the user has calculated the crc in the nvram struct
                    ret = kerSysNvRamSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                }
                mutex_unlock(&flashImageMutex);
                kfree(pNvramData);
                break;
            }
#ifdef SKY
					case SERIALISATION:
						{
							SERIALISATION_DATA * pSerialisationData;

							/* A copy of the NVRAM workflow above */
							mutex_lock(&flashImageMutex);
							if (NULL == (pSerialisationData = readSerialisationData()))
							{
								mutex_unlock(&flashImageMutex);
								return -ENOMEM;
							}
							// assumes the user has calculated the crc in the nvram struct
							ret = kerSysSerialisationSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
							mutex_unlock(&flashImageMutex);
							kfree(pSerialisationData);
							break;
						}

					case IMAGE_TAG:
						{
							unsigned char bank;
							unsigned int seq;

							bank = (unsigned char)ctrlParms.string[0];
							seq = (unsigned int)(*((unsigned int *)&ctrlParms.string[1]));
							ret = kerSysSetSequenceNumber(bank, seq);
						}
						// modification of the sequence numbers
						/* kerSysSetSequenceNumber */
						break;

#ifdef ENABLE_SERIALISATION_WP
					case SERIALISATION_WP:
						{
							/* Write protect the serialisation sectors */
							mutex_lock(&flashImageMutex);
							ret = kerSysSerialisationWP();
							mutex_unlock(&flashImageMutex);
							break;
						}
#else
					case SOFT_UNPROT:
						{
							/* Write protect the serialisation sectors */
							mutex_lock(&flashImageMutex);
							ret = kerSysSoftUnProt();
							mutex_unlock(&flashImageMutex);
							break;
						}
					case SOFT_WRITE_PROT:
						{
							/* Write protect the serialisation sectors */
							mutex_lock(&flashImageMutex);
							ret = kerSysSoftWriteProt();
							mutex_unlock(&flashImageMutex);
							break;
						}
#endif
#endif /* SKY */

            case BCM_IMAGE_CFE:
                {
                unsigned long not_used;

                if(kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *)&not_used)==0)
                {
                    printk("\nERROR: Image does not support a NAND flash device.\n\n");
                    ret = -1;
                    break;
                }
                if( ctrlParms.strLen <= 0 || ctrlParms.strLen > FLASH_LENGTH_BOOT_ROM )
                {
                    printk("Illegal CFE size [%d]. Size allowed: [%d]\n",
                        ctrlParms.strLen, FLASH_LENGTH_BOOT_ROM);
                    ret = -1;
                    break;
                }
#ifndef SKY

                ret = commonImageWrite(ctrlParms.offset + BOOT_OFFSET,
                    ctrlParms.string, ctrlParms.strLen, NULL, 0);

#else /* SKY */
				ret = commonImageWrite(ctrlParms.offset + BOOT_OFFSET, ctrlParms.string, ctrlParms.strLen,NULL,0,ctrlParms.buf);
#endif /* SKY */
                }
                break;

            case BCM_IMAGE_FS:
                {
                int numPartitions = 1;
                int noReboot = FLASH_IS_NO_REBOOT(ctrlParms.offset);
                int partition = FLASH_GET_PARTITION(ctrlParms.offset);
                unsigned long not_used;

                if(kerSysBlParmsGetInt(NAND_RFS_OFS_NAME, (int *)&not_used)==0)
                {
                    printk("\nERROR: Image does not support a NAND flash device.\n\n");
                    ret = -1;
                    break;
                }

                mutex_lock(&flashImageMutex);
                ret = flashFsKernelImage(ctrlParms.string, ctrlParms.strLen,
                    partition, &numPartitions);
                mutex_unlock(&flashImageMutex);

                if(ret == 0 && (numPartitions == 1 || noReboot == 0))
                    resetPwrmgmtDdrMips();
                }
                break;

            case BCM_IMAGE_KERNEL:  // not used for now.
                break;
#ifdef SKY
#ifdef ENABLE_REPAIR_SW
							case SKY_SECTOR_SET:
							/* A copy of the NVRAM workflow above */
							mutex_lock(&flashImageMutex);
							ret = kerSysSectorSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
							mutex_unlock(&flashImageMutex);
							break;
#endif
#endif /* SKY */
            case BCM_IMAGE_WHOLE:
                {
                int noReboot = FLASH_IS_NO_REBOOT(ctrlParms.offset);
                int partition = FLASH_GET_PARTITION(ctrlParms.offset);

                if(ctrlParms.strLen <= 0)
                {
                    printk("Illegal flash image size [%d].\n", ctrlParms.strLen);
                    ret = -1;
                    break;
                }
#ifndef SKY

                ret = commonImageWrite(FLASH_BASE, ctrlParms.string,
                    ctrlParms.strLen, &noReboot, partition );

#else /* SKY */
								ctrlParms.offset += FLASH_BASE;

								if(ctrlParms.offset == FLASH_BASE)
								{
									SERIALISATION_DATA * pSerialisationData;
									CFE_NVRAM_DATA *pCfeNvramData;

									printk("Writing Whole FLASH image,  size [%d].\n", ctrlParms.strLen);
									/* A copy of the NVRAM workflow above */
									mutex_lock(&flashImageMutex);
									if (NULL == (pSerialisationData = readSerialisationData()))
									{
										printk("Error Reading Ser Data\n");
										mutex_unlock(&flashImageMutex);
										return -ENOMEM;
									}

									if (NULL == (pCfeNvramData = readCfeNvramData()))
									{
										printk("Error Reading readCfeNvramData Data\n");
										mutex_unlock(&flashImageMutex);
										return -ENOMEM;
									}



									// Unportect CFE and Serialization sector
									if(kerSysSoftUnProt() == -1)
									{
										printk("Error Unprotecting CFE and SER Data sector, Cant write the Whole Image.\n");
										return -ENOMEM;
									}


									mutex_unlock(&flashImageMutex);
									ret = commonImageWrite(ctrlParms.offset, ctrlParms.string, ctrlParms.strLen,&noReboot, partition,ctrlParms.buf);
									// assumes the user has calculated the crc in the nvram struct
									if(ret == 0)
									{
										mutex_lock(&flashImageMutex);
										kerSysSerialisationSet((char *)pSerialisationData, sizeof(SERIALISATION_DATA), 0);
										kerSysCfeNvRamSet((char *)pCfeNvramData, sizeof(CFE_NVRAM_DATA), 0);

										mutex_unlock(&flashImageMutex);
										kfree(pSerialisationData);
										kfree(pCfeNvramData);
									}
									else
									{

										// If flashing the image faile, write protect CFE+SERDATA sectors
										mutex_lock(&flashImageMutex);
										if(kerSysSoftWriteProt() == -1)
											printk("Error Write Protecting CFE and SER Data sector \n");

										mutex_unlock(&flashImageMutex);

									}
								}
								else

								{
									printk("Writing partial FLASH image,  size [%d].\n", ctrlParms.strLen);
									ret = commonImageWrite(ctrlParms.offset, ctrlParms.string, ctrlParms.strLen,&noReboot, partition,ctrlParms.buf);
								}

								if (ret == 0 && !strcasecmp(ctrlParms.buf,"SKY"))
								{
									printk("Rebooting after flash writing\n");
									resetPwrmgmtDdrMips(); /* SAJJAN Changed it avoid flash corruption */
								}
								else
#endif /* SKY */
                if(ret == 0 && noReboot == 0)
                {
                    resetPwrmgmtDdrMips();
                }
                else
                {
                    if (ret != 0)
                        printk("flash of whole image failed, ret=%d\n", ret);
                }
                }
                break;

            default:
                ret = -EINVAL;
                printk("flash_ioctl_command: invalid command %d\n", ctrlParms.action);
                break;
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_FLASH_READ:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            switch (ctrlParms.action) {
            case SCRATCH_PAD:
                ret = kerSysScratchPadGet(ctrlParms.string, ctrlParms.buf, ctrlParms.offset);
                break;

            case PERSISTENT:
                ret = kerSysPersistentGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case BACKUP_PSI:
                ret = kerSysBackupPsiGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case SYSLOG:
                ret = kerSysSyslogGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                break;

            case NVRAM:
                kerSysNvRamGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
                ret = 0;
                break;

            case FLASH_SIZE:
                ret = kerSysFlashSizeGet();
                break;
#ifdef SKY
						case SERIALISATION:
							kerSysSerialisationGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
							ret = 0;
							break;

						case SECTOR_SIZE:
							ret = kerSysGetFlashSectorSize((unsigned short) ctrlParms.offset);
							break;

						case PUBLIC_KEY:
							kerSysPublicKeyGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
							ret = 0;
							break;

						// used to fetch the information about the image's FILE_TAG & Sequence number
						case IMAGE_TAG:
							{
								PFILE_TAG pTag = kerSysImageTagGet(*ctrlParms.buf);
								if (!pTag) {
									ret = -EFAULT;
								}
								else {
									memcpy(ctrlParms.string,
											((char *)pTag + ctrlParms.offset),
											ctrlParms.strLen);
									ret = 0;
								}
							}
							break;
#endif /* SKY */

            default:
                ret = -EINVAL;
                printk("Not supported.  invalid command %d\n", ctrlParms.action);
                break;
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_FLASH_LIST:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            switch (ctrlParms.action) {
            case SCRATCH_PAD:
                ret = kerSysScratchPadList(ctrlParms.buf, ctrlParms.offset);
                break;

            default:
                ret = -EINVAL;
                printk("Not supported.  invalid command %d\n", ctrlParms.action);
                break;
            }
            ctrlParms.result = ret;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_DUMP_ADDR:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            dumpaddr( (unsigned char *) ctrlParms.string, ctrlParms.strLen );
            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_SET_MEMORY:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            unsigned long  *pul = (unsigned long *)  ctrlParms.string;
            unsigned short *pus = (unsigned short *) ctrlParms.string;
            unsigned char  *puc = (unsigned char *)  ctrlParms.string;
            switch( ctrlParms.strLen ) {
            case 4:
                *pul = (unsigned long) ctrlParms.offset;
                break;
            case 2:
                *pus = (unsigned short) ctrlParms.offset;
                break;
            case 1:
                *puc = (unsigned char) ctrlParms.offset;
                break;
            }
#if !defined(CONFIG_BCM96816) && !defined(CONFIG_BCM96818)
            /* This is placed as MoCA blocks are 32-bit only
            * accessible and following call makes access in terms
            * of bytes. Probably MoCA address range can be checked
            * here.
            */
            dumpaddr( (unsigned char *) ctrlParms.string, sizeof(long) );
#endif
            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_MIPS_SOFT_RESET:
        kerSysMipsSoftReset();
        break;

    case BOARD_IOCTL_LED_CTRL:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            kerSysLedCtrl((BOARD_LED_NAME)ctrlParms.strLen, (BOARD_LED_STATE)ctrlParms.offset);
            ret = 0;
        }
        break;

    case BOARD_IOCTL_GET_ID:
        if (copy_from_user((void*)&ctrlParms, (void*)arg,
            sizeof(ctrlParms)) == 0)
        {
            if( ctrlParms.string )
            {
                char p[NVRAM_BOARD_ID_STRING_LEN];
                kerSysNvRamGetBoardId(p);
                if( strlen(p) + 1 < ctrlParms.strLen )
                    ctrlParms.strLen = strlen(p) + 1;
                __copy_to_user(ctrlParms.string, p, ctrlParms.strLen);
            }

            ctrlParms.result = 0;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
        }
        break;

    case BOARD_IOCTL_GET_MAC_ADDRESS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            ctrlParms.result = kerSysGetMacAddress( ucaMacAddr,
                ctrlParms.offset );

            if( ctrlParms.result == 0 )
            {
                __copy_to_user(ctrlParms.string, ucaMacAddr,
                    sizeof(ucaMacAddr));
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_ALLOC_MAC_ADDRESSES:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            ctrlParms.result = kerSysGetMacAddresses( ucaMacAddr,
                *((UINT32 *)ctrlParms.buf), ctrlParms.offset );

            if( ctrlParms.result == 0 )
            {
                __copy_to_user(ctrlParms.string, ucaMacAddr,
                    sizeof(ucaMacAddr));
                ret = 0;
            } else {
                ret = -EFAULT;
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_RELEASE_MAC_ADDRESSES:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            if (copy_from_user((void*)ucaMacAddr, (void*)ctrlParms.string, \
                NVRAM_MAC_ADDRESS_LEN) == 0)
            {
                ctrlParms.result = kerSysReleaseMacAddresses( ucaMacAddr, *((UINT32 *)ctrlParms.buf) );
            }
            else
            {
                ctrlParms.result = -EACCES;
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_RELEASE_MAC_ADDRESS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            if (copy_from_user((void*)ucaMacAddr, (void*)ctrlParms.string, \
                NVRAM_MAC_ADDRESS_LEN) == 0)
            {
                ctrlParms.result = kerSysReleaseMacAddress( ucaMacAddr );
            }
            else
            {
                ctrlParms.result = -EACCES;
            }

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_GET_PSI_SIZE:
        {
            FLASH_ADDR_INFO fInfo;
            kerSysFlashAddrInfoGet(&fInfo);
            ctrlParms.result = fInfo.flash_persistent_length;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        break;

    case BOARD_IOCTL_GET_BACKUP_PSI_SIZE:
        {
            FLASH_ADDR_INFO fInfo;
            kerSysFlashAddrInfoGet(&fInfo);
            // if number_blks > 0, that means there is a backup psi, but length is the same
            // as the primary psi (persistent).

            ctrlParms.result = (fInfo.flash_backup_psi_number_blk > 0) ?
                fInfo.flash_persistent_length : 0;
            printk("backup_psi_number_blk=%d result=%d\n", fInfo.flash_backup_psi_number_blk, fInfo.flash_persistent_length);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        break;

    case BOARD_IOCTL_GET_SYSLOG_SIZE:
        {
            FLASH_ADDR_INFO fInfo;
            kerSysFlashAddrInfoGet(&fInfo);
            ctrlParms.result = fInfo.flash_syslog_length;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        break;

    case BOARD_IOCTL_GET_SDRAM_SIZE:
        ctrlParms.result = (int) g_ulSdramSize;
        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        ret = 0;
        break;

    case BOARD_IOCTL_GET_BASE_MAC_ADDRESS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            __copy_to_user(ctrlParms.string, g_pMacInfo->ucaBaseMacAddr, NVRAM_MAC_ADDRESS_LEN);
            ctrlParms.result = 0;

            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else
            ret = -EFAULT;
        break;

    case BOARD_IOCTL_GET_CHIP_ID:
        ctrlParms.result = kerSysGetChipId();

        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        ret = 0;
        break;

    case BOARD_IOCTL_GET_CHIP_REV:
#if defined(CONFIG_BCM968500)
	ret = -1;
#else
        ctrlParms.result = (int) (PERF->RevID & REV_ID_MASK);
        __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        ret = 0;
#endif
        break;

    case BOARD_IOCTL_GET_NUM_ENET_MACS:
    case BOARD_IOCTL_GET_NUM_ENET_PORTS:
        {
            ETHERNET_MAC_INFO EnetInfos[BP_MAX_ENET_MACS];
            int i, cnt, numEthPorts = 0;
            if (BpGetEthernetMacInfo(EnetInfos, BP_MAX_ENET_MACS) == BP_SUCCESS) {
                for( i = 0; i < BP_MAX_ENET_MACS; i++) {
                    if (EnetInfos[i].ucPhyType != BP_ENET_NO_PHY) {
                        bitcount(cnt, EnetInfos[i].sw.port_map);
                        numEthPorts += cnt;
                    }
                }
                ctrlParms.result = numEthPorts;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_CFE_VER:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            unsigned char vertag[CFE_VERSION_MARK_SIZE+CFE_VERSION_SIZE];
            kerSysCfeVersionGet(vertag, sizeof(vertag));
            if (ctrlParms.strLen < CFE_VERSION_SIZE) {
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = -EFAULT;
            }
            else if (strncmp(vertag, "cfe-v", 5)) { // no tag info in flash
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ctrlParms.result = 1;
                __copy_to_user(ctrlParms.string, vertag+CFE_VERSION_MARK_SIZE, CFE_VERSION_SIZE);
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
        }
        else {
            ret = -EFAULT;
        }
        break;

#if defined (WIRELESS)
    case BOARD_IOCTL_GET_WLAN_ANT_INUSE:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            unsigned short antInUse = 0;
            if (BpGetWirelessAntInUse(&antInUse) == BP_SUCCESS) {
                if (ctrlParms.strLen == sizeof(antInUse)) {
                    __copy_to_user(ctrlParms.string, &antInUse, sizeof(antInUse));
                    ctrlParms.result = 0;
                    __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                    ret = 0;
                } else
                    ret = -EFAULT;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif
    case BOARD_IOCTL_SET_TRIGGER_EVENT:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            BOARD_IOC *board_ioc = (BOARD_IOC *)flip->private_data;
            ctrlParms.result = -EFAULT;
            ret = -EFAULT;
            if (ctrlParms.strLen == sizeof(unsigned long)) {
                board_ioc->eventmask |= *((int*)ctrlParms.string);
#if defined (WIRELESS)
#if !defined(CONFIG_BCM968500)
                if((board_ioc->eventmask & SES_EVENTS)) {
                    ctrlParms.result = 0;
                    ret = 0;
                }
#endif
#endif
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            }
            break;
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_GET_TRIGGER_EVENT:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            BOARD_IOC *board_ioc = (BOARD_IOC *)flip->private_data;
            if (ctrlParms.strLen == sizeof(unsigned long)) {
                __copy_to_user(ctrlParms.string, &board_ioc->eventmask, sizeof(unsigned long));
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            } else
                ret = -EFAULT;

            break;
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_UNSET_TRIGGER_EVENT:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            if (ctrlParms.strLen == sizeof(unsigned long)) {
                BOARD_IOC *board_ioc = (BOARD_IOC *)flip->private_data;
                board_ioc->eventmask &= (~(*((int*)ctrlParms.string)));
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            } else
                ret = -EFAULT;

            break;
        }
        else {
            ret = -EFAULT;
        }
        break;
#if defined (WIRELESS)
    case BOARD_IOCTL_SET_SES_LED:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            if (ctrlParms.strLen == sizeof(int)) {
                sesLed_ctrl(*(int*)ctrlParms.string);
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            } else
                ret = -EFAULT;

            break;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif

    case BOARD_IOCTL_GET_GPIOVERLAYS:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
	    unsigned long GPIOOverlays = 0;
	    ret = 0;
            if (BP_SUCCESS == (ctrlParms.result = BpGetGPIOverlays(&GPIOOverlays) )) {
	      __copy_to_user(ctrlParms.string, &GPIOOverlays, sizeof(unsigned long));

	        if(__copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS))!=0)
		    ret = -EFAULT;
	    }
	}else
            ret = -EFAULT;

        break;

    case BOARD_IOCTL_SET_MONITOR_FD:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {

           g_monitor_nl_pid =  ctrlParms.offset;
           printk(KERN_INFO "monitor task is initialized pid= %d \n",g_monitor_nl_pid);
        }
        break;

    case BOARD_IOCTL_WAKEUP_MONITOR_TASK:
        kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_WAKEUP_MONITOR_TASK, NULL, 0);
        break;

    case BOARD_IOCTL_SET_CS_PAR:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            ret = ConfigCs(&ctrlParms);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_SET_GPIO:

        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
    		kerSysSetGpioState(ctrlParms.strLen, ctrlParms.offset);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;

#if defined(CONFIG_BCM_CPLD1)
    case BOARD_IOCTL_SET_SHUTDOWN_MODE:
        BcmCpld1SetShutdownMode();
        ret = 0;
        break;

    case BOARD_IOCTL_SET_STANDBY_TIMER:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            BcmCpld1SetStandbyTimer(ctrlParms.offset);
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;
#endif

    case BOARD_IOCTL_BOOT_IMAGE_OPERATION:
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
            switch(ctrlParms.offset)
            {
            case BOOT_SET_NEW_IMAGE:
				// don't renumber banks if request came from image tool
                ctrlParms.result = kerSysSetBootImageState(ctrlParms.offset,
						strcmp(ctrlParms.string, "dont_renumber"));
				break;

            case BOOT_SET_PART1_IMAGE:
            case BOOT_SET_PART2_IMAGE:
            case BOOT_SET_PART1_IMAGE_ONCE:
            case BOOT_SET_PART2_IMAGE_ONCE:
            case BOOT_SET_OLD_IMAGE:
            case BOOT_SET_NEW_IMAGE_ONCE:
                ctrlParms.result = kerSysSetBootImageState(ctrlParms.offset, 1);
                break;

            case BOOT_GET_BOOT_IMAGE_STATE:
#ifdef SKY
				ctrlParms.result = kerSysBootImageStateGet();
#else
                ctrlParms.result = kerSysGetBootImageState();
#endif
                break;

            case BOOT_GET_IMAGE_VERSION:
                /* ctrlParms.action is parition number */
                ctrlParms.result = getImageVersion((int) ctrlParms.action,
                    ctrlParms.string, ctrlParms.strLen);
                break;

            case BOOT_GET_BOOTED_IMAGE_ID:
                /* ctrlParm.strLen == 1: partition or == 0: id (new or old) */
                ctrlParms.result = getBootedValue(ctrlParms.strLen);
                break;

            default:
                ctrlParms.result = -EFAULT;
                break;
            }
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
        }
        else {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_GET_TIMEMS:
        ret = jiffies_to_msecs(jiffies - INITIAL_JIFFIES);
        break;

    case BOARD_IOCTL_GET_DEFAULT_OPTICAL_PARAMS:
    {
        unsigned char ucDefaultOpticalParams[BP_OPTICAL_PARAMS_LEN];

        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            ret = 0;
            if (BP_SUCCESS == (ctrlParms.result = BpGetDefaultOpticalParams(ucDefaultOpticalParams)))
            {
                __copy_to_user(ctrlParms.string, ucDefaultOpticalParams, BP_OPTICAL_PARAMS_LEN);

                if (__copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS)) != 0)
                {
                    ret = -EFAULT;
                }
            }
        }
        else
        {
            ret = -EFAULT;
        }

        break;
    }

    break;
    case BOARD_IOCTL_GET_GPON_OPTICS_TYPE:

        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
            unsigned short Temp=0;
            BpGetGponOpticsType(&Temp);
            *((UINT32*)ctrlParms.buf) = Temp;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
        }
        ret = 0;

        break;

#if defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM_6802_MoCA)
    case BOARD_IOCTL_SPI_SLAVE_INIT:
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'result' field to specify the SPI device */
             if (kerSysBcmSpiSlaveInit(ctrlParms.result) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }
        }
        else
        {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_SPI_SLAVE_READ:
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'result' field to specify the SPI device for reads */
             if (kerSysBcmSpiSlaveRead(ctrlParms.result, ctrlParms.offset, (unsigned long *)ctrlParms.buf, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }
             else
             {
                   __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
             }
        }
        else
        {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_SPI_SLAVE_WRITE_BUF:
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'result' field to specify the SPI device for write buf */
             if (kerSysBcmSpiSlaveWriteBuf(ctrlParms.result, ctrlParms.offset, (unsigned long *)ctrlParms.buf, ctrlParms.strLen, 4) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }
             else
             {
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
             }
        }
        else
        {
            ret = -EFAULT;
        }
        break;

    case BOARD_IOCTL_SPI_SLAVE_WRITE:
        ret = 0;
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'buf' field to specify the SPI device for writes */
             if (kerSysBcmSpiSlaveWrite((uint32_t)ctrlParms.buf, ctrlParms.offset, ctrlParms.result, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }
             else
             {
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
             }
        }
        else
        {
            ret = -EFAULT;
        }
        break;
    case BOARD_IOCTL_SPI_SLAVE_SET_BITS:
        ret = 0;
#if defined(CONFIG_BCM_6802_MoCA)
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'buf' field to specify the SPI device for set bits */
             if (kerSysBcmSpiSlaveModify((uint32_t)ctrlParms.buf, ctrlParms.offset, ctrlParms.result, ctrlParms.result, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }
             else
             {
                 __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
             }
        }
        else
        {
            ret = -EFAULT;
        }
#endif
        break;
    case BOARD_IOCTL_SPI_SLAVE_CLEAR_BITS:
        ret = 0;
#if defined(CONFIG_BCM_6802_MoCA)
        if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
        {
             /* Using the 'buf' field to specify the SPI device for clear bits */
             if (kerSysBcmSpiSlaveModify((uint32_t)ctrlParms.buf, ctrlParms.offset, 0, ctrlParms.result, ctrlParms.strLen) != SPI_STATUS_OK)
             {
                 ret = -EFAULT;
             }
             else
             {
                   __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
             }
        }
        else
        {
            ret = -EFAULT;
        }
#endif
        break;
#endif

#if defined(CONFIG_EPON_SDK)
     case BOARD_IOCTL_GET_PORT_MAC_TYPE:
        {
            unsigned short port;
            unsigned long mac_type;

            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
            {
                port = ctrlParms.offset;
                if (BpGetPortMacType(port, &mac_type) == BP_SUCCESS) {
                    ctrlParms.result = (unsigned int)mac_type;
                    __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                    ret = 0;
                }
                else {
                    ret = -EFAULT;
                }
                break;
            }
        }

    case BOARD_IOCTL_GET_NUM_FE_PORTS:
        {
            unsigned long fe_ports;
            if (BpGetNumFePorts(&fe_ports) == BP_SUCCESS) {
                ctrlParms.result = fe_ports;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_NUM_GE_PORTS:
        {
            unsigned long ge_ports;
            if (BpGetNumGePorts(&ge_ports) == BP_SUCCESS) {
                ctrlParms.result = ge_ports;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_NUM_VOIP_PORTS:
        {
            unsigned long voip_ports;
            if (BpGetNumVoipPorts(&voip_ports) == BP_SUCCESS) {
                ctrlParms.result = voip_ports;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }

    case BOARD_IOCTL_GET_SWITCH_PORT_MAP:
        {
            unsigned long port_map;
            if (BpGetSwitchPortMap(&port_map) == BP_SUCCESS) {
                ctrlParms.result = port_map;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,  sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else {
                ret = -EFAULT;
            }
            break;
        }


    case BOARD_IOCTL_GET_EPON_GPIOS:
        {
        	int i, rc = 0, gpionum;
        	unsigned short *pusGpio, gpio;
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
            {
                if( ctrlParms.string )
                {
                    /* walk through the epon gpio list */
                	i = 0;
                	pusGpio = (unsigned short *)ctrlParms.string;
                	gpionum =  ctrlParms.strLen/sizeof(unsigned short);
                    for(;;)
                    {
                     	rc = BpGetEponGpio(i, &gpio);
                       	if( rc == BP_MAX_ITEM_EXCEEDED || i >= gpionum )
                       		break;
                       	else
                       	{
                       		if( rc == BP_SUCCESS )
                       			*pusGpio = gpio;
                       		else
                       			*pusGpio = BP_GPIO_NONE;
                       		pusGpio++;
                       	}
                       	i++;
                     }
                     ctrlParms.result = 0;
                     __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                }
                else
                {
                    ret = -EFAULT;
                }
            }
            break;
        }
#endif
#ifdef SKY
				case BOARD_IOCTL_SET_PWR_LED:
				if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {
					if (ctrlParms.strLen == sizeof(int)) {
						pwrLed_ctrl(*(int*)ctrlParms.string);
						ctrlParms.result = 0;
						__copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
						ret = 0;
					} else
						ret = -EFAULT;

					break;
				}
				else {
					ret = -EFAULT;
				}


				break;
				case BOARD_IOCTL_SET_PLC_GPIO:

					if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) {

						if(ctrlParms.strLen==16)
						{
							unsigned short gpio;
							if( BpGetPLCPwrEnGpio(&gpio) == BP_SUCCESS )
							{
								//printk("\nERROR:BOARD_IOCTL_SET_PLC_GPIO GPIO PIN %d\n",gpio);
								kerSysSetGpioState(gpio, ctrlParms.offset);
							}
							else
							{
								printk("\nERROR:BOARD_IOCTL_SET_PLC_GPIO GPIO PIN not found");
							}
						}
						//kerSysSetGpioState(ctrlParms.strLen, ctrlParms.offset);
						__copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
						ret = 0;
					}
					else {
						ret = -EFAULT;
					}
					break;

#ifdef CONFIG_SKY_FEATURE_WATCHDOG
				case BOARD_IOCTL_RESET_WATCHDOG:
				restartHWWatchDog();
				break;

				case BOARD_IOCTL_STOP_WATCHDOG:
				stopHWWatchDog();
				break;
#endif /* CONFIG_SKY_FEATURE_WATCHDOG */
#endif /* SKY */
    default:
        ret = -EINVAL;
        ctrlParms.result = 0;
        printk("board_ioctl: invalid command %x, cmd %d .\n",command,_IOC_NR(command));
        break;

    } /* switch */

    return (ret);

} /* board_ioctl */

#ifndef CONFIG_BCM968500
/***************************************************************************
* SES Button ISR/GPIO/LED functions.
***************************************************************************/
static Bool sesBtn_pressed(void)
{
	unsigned int intSts = 0, extIntr, value = 0;
	int actHigh = 0;
	Bool pressed = 1;

	if( sesBtn_polling == 0 )
	{
#if defined(CONFIG_BCM96838)
		if ((sesBtn_irq >= INTERRUPT_ID_EXTERNAL_0) && (sesBtn_irq <= INTERRUPT_ID_EXTERNAL_5)) {
#else
		if ((sesBtn_irq >= INTERRUPT_ID_EXTERNAL_0) && (sesBtn_irq <= INTERRUPT_ID_EXTERNAL_3)) {
#endif
			intSts = PERF->ExtIrqCfg & (1 << (sesBtn_irq - INTERRUPT_ID_EXTERNAL_0 + EI_STATUS_SHFT));

		}
#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818)
		else if ((sesBtn_irq >= INTERRUPT_ID_EXTERNAL_4) || (sesBtn_irq <= INTERRUPT_ID_EXTERNAL_5)) {
			intSts = PERF->ExtIrqCfg1 & (1 << (sesBtn_irq - INTERRUPT_ID_EXTERNAL_4 + EI_STATUS_SHFT));
		}
#endif
		else
			return 0;

		extIntr = extIntrInfo[sesBtn_irq-INTERRUPT_ID_EXTERNAL_0];
		actHigh = IsExtIntrTypeActHigh(extIntr);

		if( ( actHigh && intSts ) || (!actHigh && !intSts ) )
		{
			//check the gpio status here too if shared.
			if( IsExtIntrShared(extIntr) )
			{
				 value = kerSysGetGpioValue(sesBtn_gpio);
				 if( (value&&!actHigh) || (!value&&actHigh) )
					 pressed = 0;
			}
		}
		else
			pressed = 0;
	}
	else
	{
		pressed = 0;
		if( sesBtn_gpio != BP_NOT_DEFINED )
		{
			actHigh = sesBtn_gpio&BP_ACTIVE_LOW ? 0 : 1;
			value = kerSysGetGpioValue(sesBtn_gpio);
		    if( (value&&actHigh) || (!value&&!actHigh) )
			    pressed = 1;
		}
	}

    return pressed;
}

static void sesBtn_timer_handler(unsigned long arg)
{
    if ( sesBtn_pressed() ) {

#if defined(CONFIG_SKY_MESH_SUPPORT)
	//Sky: push button should be pressed for min  of 1 sec
        mod_timer(&sesBtn_timer, (jiffies + ((HZ/1000) * 1000)));
		if((kerSysPushButtonNotifyHook) && (!(atomic_read(&sesBtn_Notified))))
        {
			atomic_set(&sesBtn_Notified, 1);
            kerSysPushButtonNotifyHook();
        }
#else
		mod_timer(&sesBtn_timer, (jiffies + ((HZ/1000) * 100)));
#endif
    }
    else {
        atomic_set(&sesBtn_active, 0);
        BcmHalInterruptEnable(sesBtn_irq);
    }
}

static irqreturn_t sesBtn_isr(int irq, void *dev_id)
{
    int ext_irq_idx = 0, value=0;
    irqreturn_t ret = IRQ_NONE;

    ext_irq_idx = irq - INTERRUPT_ID_EXTERNAL_0;
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]))
    {
        value = kerSysGetGpioValue(*(int *)dev_id);
        if( (IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) && value) || (IsExtIntrTypeActLow(extIntrInfo[ext_irq_idx]) && !value) )
        {
            ret = IRQ_HANDLED;
        }
    }
    else
    {
        ret = IRQ_HANDLED;
    }

    if (IRQ_HANDLED == ret) {
#if defined(CONFIG_SKY_MESH_SUPPORT)
	int timerSet = mod_timer(&sesBtn_timer, (jiffies + ((HZ/1000) * 1000)));

	 if ( 0 == timerSet ) {
		 atomic_set(&sesBtn_active, 1);
		 //SKy: notify push button	in sesBtn_timer_handler. after 1 sec
		 atomic_set(&sesBtn_Notified, 0);
		 if(!kerSysPushButtonNotifyHook)
			   wake_up_interruptible(&g_board_wait_queue);
	 	}
#else
        int timerSet = mod_timer(&sesBtn_timer, (jiffies + ((HZ/1000) * 100)));
        if ( 0 == timerSet ) {
            atomic_set(&sesBtn_active, 1);
            if(kerSysPushButtonNotifyHook)
            {
               kerSysPushButtonNotifyHook();
            }
            else
            {
               /* notifier not installed - handle event normally */
               wake_up_interruptible(&g_board_wait_queue);
            }
        }
#endif
    }

#ifndef CONFIG_BCM_6802_MoCA
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx])) {
        BcmHalInterruptEnable(sesBtn_irq);
    }
#endif

    return ret;
}

static void __init sesBtn_mapIntr(int context)
{
    int ext_irq_idx;

    if( BpGetWirelessSesExtIntr(&sesBtn_irq) == BP_SUCCESS )
    {
        BpGetWirelessSesExtIntrGpio(&sesBtn_gpio);
        if( sesBtn_irq != BP_EXT_INTR_NONE )
        {
            printk("SES: Button Interrupt 0x%x is enabled\n", sesBtn_irq);
        }
        else
        {
            if( sesBtn_gpio != BP_NOT_DEFINED )
            {
                printk("SES: Button Polling is enabled on gpio %x\n", sesBtn_gpio);
                kerSysSetGpioDirInput(sesBtn_gpio);
                sesBtn_polling = 1;
            }
        }
    }
    else
        return;

    if( sesBtn_irq != BP_EXT_INTR_NONE )
    {
        ext_irq_idx = (sesBtn_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;
        if (!IsExtIntrConflict(extIntrInfo[ext_irq_idx]))
        {
            static int dev = -1;
            int hookisr = 1;

            if (IsExtIntrShared(sesBtn_irq))
            {
                /* get the gpio and make it input dir */
                if( sesBtn_gpio != BP_NOT_DEFINED )
                {
                    sesBtn_gpio &= BP_GPIO_NUM_MASK;;
                    printk("SES: Button Interrupt gpio is %d\n", sesBtn_gpio);
                    kerSysSetGpioDirInput(sesBtn_gpio);
                    dev = sesBtn_gpio;
                }
                else
                {
                    printk("SES: Button Interrupt gpio definition not found \n");
                    hookisr = 0;
                }
            }

            if(hookisr)
            {
                init_timer(&sesBtn_timer);
                sesBtn_timer.function = sesBtn_timer_handler;
#if defined(CONFIG_SKY_MESH_SUPPORT)
				// SKY: For WPS event,	button should be pressed for a minimun of 1 sec
                atomic_set(&sesBtn_Notified, 0);
				sesBtn_timer.expires  = jiffies + ((HZ/1000) * 1000);

#else
                sesBtn_timer.expires  = jiffies + 100;
#endif
                sesBtn_timer.data     = 0;
                atomic_set(&sesBtn_active, 0);
                sesBtn_irq = map_external_irq (sesBtn_irq);
                BcmHalMapInterrupt((FN_HANDLER)sesBtn_isr, (unsigned int)&dev, sesBtn_irq);
                BcmHalInterruptEnable(sesBtn_irq);
            }
        }
    }

    return;
}

#if defined(WIRELESS)
static unsigned int sesBtn_poll(struct file *file, struct poll_table_struct *wait)
{
    if ( sesBtn_polling ) {
        if ( sesBtn_pressed() ) {
            return POLLIN;
        }
    }
    else if (atomic_read(&sesBtn_active)) {
        return POLLIN;
    }
    return 0;
}

static ssize_t sesBtn_read(struct file *file,  char __user *buffer, size_t count, loff_t *ppos)
{
    volatile unsigned int event=0;
    ssize_t ret=0;

    if ( sesBtn_polling ) {
        if ( 0 == sesBtn_pressed() ) {
            return ret;
        }
    }
    else if ( 0 == atomic_read(&sesBtn_active) ) {
        return ret;
    }
    event = SES_EVENTS;
    __copy_to_user((char*)buffer, (char*)&event, sizeof(event));
    count -= sizeof(event);
    buffer += sizeof(event);
    ret += sizeof(event);
    return ret;
}
#endif /* WIRELESS */


void kerSysRegisterMocaHostIntrCallback(MocaHostIntrCallback callback, void * userArg, int dev)
{
	int ext_irq_idx;
	unsigned short  mocaHost_irq;
	PBP_MOCA_INFO  pMocaInfo;
	PMOCA_INTR_ARG pMocaInt;

    if( dev >=  mocaChipNum )
    {
    	printk("kerSysRegisterMocaHostIntrCallback: Error, invalid dev %d\n", dev);
    	return;
    }

    pMocaInfo = &mocaInfo[dev];
    if( (mocaHost_irq = pMocaInfo->intr[BP_MOCA_HOST_INTR_IDX]) == BP_NOT_DEFINED )
    {
        printk("kerSysRegisterMocaHostIntrCallback: Error, no mocaHost_irq defined in boardparms\n");
        return;
    }

    printk("kerSysRegisterMocaHostIntrCallback: mocaHost_irq = 0x%x, is_mocaHostIntr_shared=%d\n", mocaHost_irq, IsExtIntrShared(mocaHost_irq));

    ext_irq_idx = (mocaHost_irq&~BP_EXT_INTR_FLAGS_MASK)-BP_EXT_INTR_0;
	if (!IsExtIntrConflict(extIntrInfo[ext_irq_idx]))
	{
		int hookisr = 1;

		pMocaInt = &mocaIntrArg[dev];
		pMocaInt->dev = dev;
		pMocaInt->intrGpio = -1;
		pMocaInt->userArg = userArg;
		pMocaInt->mocaCallback = callback;
		if (IsExtIntrShared(mocaHost_irq))
		{
			/* get the gpio and make it input dir */
			unsigned short gpio;
			if( (gpio = pMocaInfo->intrGpio[BP_MOCA_HOST_INTR_IDX]) != BP_NOT_DEFINED )
			{
				gpio &= BP_GPIO_NUM_MASK;
				printk("MoCA Interrupt gpio is %d\n", gpio);
				kerSysSetGpioDirInput(gpio);
				pMocaInt->intrGpio = gpio;
			}
			else
			{
				  printk("MoCA interrupt gpio definition not found \n");
				  /* still need to hook it because the early bhr board does not have
				   * gpio pin for the shared LAN moca interrupt
				   */
				  hookisr = 1;
			}
		}

		if(hookisr)
		{
		    mocaHost_irq = map_external_irq (mocaHost_irq);
		    pMocaInt->irq = mocaHost_irq;
		    BcmHalMapInterrupt((FN_HANDLER)mocaHost_isr, (unsigned int)pMocaInt, mocaHost_irq);
		    BcmHalInterruptEnable(mocaHost_irq);
		}
	}
}

void kerSysMocaHostIntrEnable(int dev)
{
	PMOCA_INTR_ARG  pMocaInt;

    if( dev <  mocaChipNum )
    {
    	pMocaInt = &mocaIntrArg[dev];
        BcmHalInterruptEnable(pMocaInt->irq);
    }
}

void kerSysMocaHostIntrDisable(int dev)
{
	PMOCA_INTR_ARG  pMocaInt;

	if( dev <  mocaChipNum )
	{
		pMocaInt = &mocaIntrArg[dev];
	    BcmHalInterruptDisable(pMocaInt->irq);
	}
}

static irqreturn_t mocaHost_isr(int irq, void *dev_id)
{
    PMOCA_INTR_ARG pMocaIntrArg = (PMOCA_INTR_ARG)dev_id;
    int isOurs = 1;
    PBP_MOCA_INFO pMocaInfo;
    int ext_irq_idx = 0, value=0, valueReset = 0, valueMocaW = 0;
    unsigned short gpio;

    //printk("mocaHost_isr called for chip %d, irq %d, gpio %d\n", pMocaIntrArg->dev, irq, pMocaIntrArg->intrGpio);
    ext_irq_idx = irq - INTERRUPT_ID_EXTERNAL_0;

    /* When MoCA and SES button share the interrupt, the MoCA handler must be called
       so that the interrupt is re-enabled */
#if defined (WIRELESS)
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]) && (irq != sesBtn_irq))
#else
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]))
#endif
    {
        if( pMocaIntrArg->intrGpio != -1 )
        {
            value = kerSysGetGpioValue(pMocaIntrArg->intrGpio);
            if( (IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) && value) || (IsExtIntrTypeActLow(extIntrInfo[ext_irq_idx]) && !value) )
                isOurs = 1;
            else
                isOurs = 0;
        }
        else
        {
            /* for BHR board, the L_HOST_INTR does not have gpio pin. this really sucks! have to check all other interrupt sharing gpio status,
             * if they are not triggering, then it is L_HOST_INTR.  next rev of the board will add gpio for L_HOST_INTR. in the future, all the
             * shared interrupt will have a dedicated gpio pin.
             */
            if( resetBtn_gpio != BP_NOT_DEFINED )
                valueReset = kerSysGetGpioValue(resetBtn_gpio);

               pMocaInfo = &mocaInfo[BP_MOCA_TYPE_WAN];
            if( (gpio = pMocaInfo->intrGpio[BP_MOCA_HOST_INTR_IDX]) != BP_NOT_DEFINED )
                valueMocaW = kerSysGetGpioValue(gpio);

            if( IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) )
            {
                if( (value = (valueReset|valueMocaW)) )
                    isOurs = 0;
            }
            else
            {
                if( (value = (valueReset&valueMocaW)) == 0 )
                    isOurs = 0;
            }

            //printk("BHR board moca_l interrupt: reset %d:%d, ses %d:%d, moca_w %d:%d, isours %d\n", resetBtn_gpio, valueReset,
            //    sesBtn_gpio, valueSes, gpio&BP_GPIO_NUM_MASK, valueMocaW, isOurs);
        }
    }

    if (isOurs)
    {
        pMocaIntrArg->mocaCallback(irq, pMocaIntrArg->userArg);
        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}
#endif //#ifndef CONFIG_BCM968500

#if defined(WIRELESS)
static void __init sesLed_mapGpio()
{
    if( BpGetWirelessSesLedGpio(&sesLed_gpio) == BP_SUCCESS )
    {
        printk("SES: LED GPIO 0x%x is enabled\n", sesLed_gpio);
    }
}

static void sesLed_ctrl(int action)
{
    char blinktype = ((action >> 24) & 0xff); /* extract blink type for SES_LED_BLINK  */

    BOARD_LED_STATE led;

    if(sesLed_gpio == BP_NOT_DEFINED)
        return;

    action &= 0xff; /* extract led */

    switch (action) {
    case SES_LED_ON:
        led = kLedStateOn;
        break;
    case SES_LED_BLINK:
        if(blinktype)
            led = blinktype;
        else
            led = kLedStateSlowBlinkContinues;
        break;
    case SES_LED_OFF:
    default:
        led = kLedStateOff;
    }

    kerSysLedCtrl(kLedSes, led);
}
#endif

static void __init ses_board_init()
{
#ifndef CONFIG_BCM968500
    sesBtn_mapIntr(0);
#endif
#if defined(WIRELESS)
    sesLed_mapGpio();
#endif
}

static void __exit ses_board_deinit()
{
#ifndef CONFIG_BCM968500
    if( sesBtn_polling == 0 && sesBtn_irq != BP_NOT_DEFINED )
    {
        int ext_irq_idx = sesBtn_irq - INTERRUPT_ID_EXTERNAL_0;
        if(sesBtn_irq) {
            del_timer(&sesBtn_timer);
            atomic_set(&sesBtn_active, 0);
            if (!IsExtIntrShared(extIntrInfo[ext_irq_idx])) {
                BcmHalInterruptDisable(sesBtn_irq);
            }
        }
    }
#endif
}


/***************************************************************************
* Dying gasp ISR and functions.
***************************************************************************/

#if defined(CONFIG_BCM968500)
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_ic.h"
#include "bl_os_wraper.h"

/* interrupt 0-9 => intruupts number 48-57
   interrupt 0-2 => gpio 0-2
   interrupt 3-4 => gpio 36-37
   interrupt 5-7 => gpio 40-42
   interrupt 8-9 => gpio 47-48
*/

static int convert_pin_to_intr(unsigned long pin)
{
	if( (pin>=0) && (pin<=2) )
		return pin+48;
	else if( (pin==36) || (pin==37) )
		return pin+15;
	else if( (pin>=40) && (pin<=42) )
		return pin+13;
	else if( (pin==47) || (pin==48) )
		return pin+9;

	return -1;
}
#endif

static irqreturn_t kerSysDyingGaspIsr(int irq, void * dev_id)
{
#if defined(CONFIG_BCM96318)
    unsigned short plcGpio;
#endif
    struct list_head *pos;
    CB_DGASP_LIST *tmp = NULL, *dslOrGpon = NULL;
#if !defined(CONFIG_BCM968500)
	unsigned short usPassDyingGaspGpio;		// The GPIO pin to propogate a dying gasp signal

#ifndef CONFIG_SKY_REBOOT_DIAGNOSTICS
    UART->Data = 'D';
    UART->Data = '%';
    UART->Data = 'G';
#else
#define SKY_DG_IDSTR "dg_1"
	kerSysSkyDiagnosticsDyingGaspWrite(SKY_DG_IDSTR, strlen(SKY_DG_IDSTR));
	printk (KERN_EMERG "DYING GASP EVENT LOGGED IN FLASH\n");
#endif

#if defined (WIRELESS)
    kerSetWirelessPD(WLAN_OFF);
#endif
#endif
    /* first to turn off everything other than dsl or gpon */
    list_for_each(pos, &g_cb_dgasp_list_head->list) {
        tmp = list_entry(pos, CB_DGASP_LIST, list);
        if(strncmp(tmp->name, "dsl", 3) && strncmp(tmp->name, "gpon", 4)) {
            (tmp->cb_dgasp_fn)(tmp->context);
        }else {
            dslOrGpon = tmp;
        }
    }

    // Invoke dying gasp handlers
    if(dslOrGpon)
        (dslOrGpon->cb_dgasp_fn)(dslOrGpon->context);

    /* reset and shutdown system */

#if defined (CONFIG_BCM96818)
    /* if the software boot strap overide bit has been turned on to configure
     * roboswitch port 2 and 3 in RGMII mode in the pin compatible 6818 device,
     * we have to clear this bit to workaround soft reset hang problem in 6818
     */
    if(UtilGetChipIsPinCompatible())
        MISC->miscStrapOverride &= ~MISC_STRAP_OVERRIDE_STRAP_OVERRIDE;
#endif

#if defined (CONFIG_BCM96318)
    /* Use GPIO to control the PLC and wifi chip reset on 6319 PLC board*/
    if( BpGetPLCPwrEnGpio(&plcGpio) == BP_SUCCESS )
    {
    	kerSysSetGpioState(plcGpio, kGpioInactive);
    }
#endif

#if !defined(CONFIG_BCM968500)
    /* Set WD to fire in 1 sec in case power is restored before reset occurs */
#if defined (CONFIG_BCM96838)
    WATCHDOG->WD0DefCount = 1000000 * (FPERIPH/1000000);
    WATCHDOG->WD0Ctl = 0xFF00;
    WATCHDOG->WD0Ctl = 0x00FF;
#else
    TIMER->WatchDogDefCount = 1000000 * (FPERIPH/1000000);
    TIMER->WatchDogCtl = 0xFF00;
    TIMER->WatchDogCtl = 0x00FF;
#endif

	// If configured, propogate dying gasp to other processors on the board
	if(BpGetPassDyingGaspGpio(&usPassDyingGaspGpio) == BP_SUCCESS)
	    {
	    // Dying gasp configured - set GPIO
	    kerSysSetGpioState(usPassDyingGaspGpio, kGpioInactive);
	    }
#endif

    // If power is going down, nothing should continue!
    while (1);
	return( IRQ_HANDLED );

}

static void __init kerSysInitDyingGaspHandler( void )
{
    CB_DGASP_LIST *new_node;

#if defined(CONFIG_BCM968500)
	int status, intr_num;
	unsigned long pin_num;
	BL_LILAC_IC_INTERRUPT_PROPERTIES dg_int =
	{
		.priority = 2,
		.configuration = BL_LILAC_IC_INTERRUPT_EDGE_FALLING,
		.reentrant = 0,
		.fast_interrupt = 0
	};

	if(BpGetDyingGaspIntrPin(&pin_num) != BP_SUCCESS)
	{
		printk("Dying gasp error - unable to read interrupt pin\n");
		return;
	}
	intr_num = convert_pin_to_intr(pin_num);
	if(intr_num==-1)
	{
		printk("Dying gasp error - illegal pin number %d\n", (int)pin_num);
		return;
	}

#endif

    if( g_cb_dgasp_list_head != NULL) {
        printk("Error: kerSysInitDyingGaspHandler: list head is not null\n");
        return;
    }
    new_node= (CB_DGASP_LIST *)kmalloc(sizeof(CB_DGASP_LIST), GFP_KERNEL);
    memset(new_node, 0x00, sizeof(CB_DGASP_LIST));
    INIT_LIST_HEAD(&new_node->list);
    g_cb_dgasp_list_head = new_node;

#if defined(CONFIG_BCM968500)
	/* set pin muxing */
	status = fi_bl_lilac_mux_connect_pin(pin_num,  INTERRUPT, intr_num-48, MIPSC);
	if (status != BL_LILAC_SOC_OK)
	{
		printk("Dying gasp - Error pin muxing [error %d]\n", status);
		return;
	}

	/* set interrupt (falling edge, handler etc') */
	status = bl_lilac_ic_init_int(intr_num, &dg_int);
	if (status != BL_LILAC_SOC_OK)
	{
		printk("Dying gasp - Error init interrupt [error %d]\n", status);
		return;
	}

	status = bl_lilac_ic_isr_ack(intr_num);
	if (status != BL_LILAC_SOC_OK)
	{
		printk("Dying gasp - Error ack interrupt (in initialization) [error %d]\n", status);
		return;
	}

	BcmHalMapInterrupt((FN_HANDLER)kerSysDyingGaspIsr, intr_num, INT_NUM_IRQ0 + intr_num);
	BcmHalInterruptEnable(INT_NUM_IRQ0 + intr_num);

#elif defined(CONFIG_BCM96838)
	GPIO->dg_control |= (1 << DG_EN_SHIFT);
    BcmHalMapInterrupt((FN_HANDLER)kerSysDyingGaspIsr, 0, INTERRUPT_ID_DG);
    BcmHalInterruptEnable(INTERRUPT_ID_DG);

#else
    BcmHalMapInterrupt((FN_HANDLER)kerSysDyingGaspIsr, 0, INTERRUPT_ID_DG);
    BcmHalInterruptEnable( INTERRUPT_ID_DG );
#endif
} /* kerSysInitDyingGaspHandler */

static void __exit kerSysDeinitDyingGaspHandler( void )
{
    struct list_head *pos;
    CB_DGASP_LIST *tmp;

    if(g_cb_dgasp_list_head == NULL)
        return;

    list_for_each(pos, &g_cb_dgasp_list_head->list) {
        tmp = list_entry(pos, CB_DGASP_LIST, list);
        list_del(pos);
        kfree(tmp);
    }

    kfree(g_cb_dgasp_list_head);
    g_cb_dgasp_list_head = NULL;

} /* kerSysDeinitDyingGaspHandler */

void kerSysRegisterDyingGaspHandler(char *devname, void *cbfn, void *context)
{
    CB_DGASP_LIST *new_node;

    // do all the stuff that can be done without the lock first
    if( devname == NULL || cbfn == NULL ) {
        printk("Error: kerSysRegisterDyingGaspHandler: register info not enough (%s,%x,%x)\n", devname, (unsigned int)cbfn, (unsigned int)context);
        return;
    }

    if (strlen(devname) > (IFNAMSIZ - 1)) {
        printk("Warning: kerSysRegisterDyingGaspHandler: devname too long, will be truncated\n");
    }

    new_node= (CB_DGASP_LIST *)kmalloc(sizeof(CB_DGASP_LIST), GFP_KERNEL);
    memset(new_node, 0x00, sizeof(CB_DGASP_LIST));
    INIT_LIST_HEAD(&new_node->list);
    strncpy(new_node->name, devname, IFNAMSIZ-1);
    new_node->cb_dgasp_fn = (cb_dgasp_t)cbfn;
    new_node->context = context;

    // OK, now acquire the lock and insert into list
    mutex_lock(&dgaspMutex);
    if( g_cb_dgasp_list_head == NULL) {
        printk("Error: kerSysRegisterDyingGaspHandler: list head is null\n");
        kfree(new_node);
    } else {
        list_add(&new_node->list, &g_cb_dgasp_list_head->list);
        printk("dgasp: kerSysRegisterDyingGaspHandler: %s registered \n", devname);
    }
    mutex_unlock(&dgaspMutex);

    return;
} /* kerSysRegisterDyingGaspHandler */

void kerSysDeregisterDyingGaspHandler(char *devname)
{
    struct list_head *pos;
    CB_DGASP_LIST *tmp;
    int found=0;

    if(devname == NULL) {
        printk("Error: kerSysDeregisterDyingGaspHandler: devname is null\n");
        return;
    }

    printk("kerSysDeregisterDyingGaspHandler: %s is deregistering\n", devname);

    mutex_lock(&dgaspMutex);
    if(g_cb_dgasp_list_head == NULL) {
        printk("Error: kerSysDeregisterDyingGaspHandler: list head is null\n");
    } else {
        list_for_each(pos, &g_cb_dgasp_list_head->list) {
            tmp = list_entry(pos, CB_DGASP_LIST, list);
            if(!strcmp(tmp->name, devname)) {
                list_del(pos);
                kfree(tmp);
                found = 1;
                printk("kerSysDeregisterDyingGaspHandler: %s is deregistered\n", devname);
                break;
            }
        }
        if (!found)
            printk("kerSysDeregisterDyingGaspHandler: %s not (de)registered\n", devname);
    }
    mutex_unlock(&dgaspMutex);

    return;
} /* kerSysDeregisterDyingGaspHandler */


/***************************************************************************
 *
 *
 ***************************************************************************/
static int ConfigCs (BOARD_IOCTL_PARMS *parms)
{
    int                     retv = 0;
#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818)
    int                     cs, flags;
    cs_config_pars_t        info;

    if (copy_from_user(&info, (void*)parms->buf, sizeof(cs_config_pars_t)) == 0)
    {
        cs = parms->offset;

        MPI->cs[cs].base = ((info.base & 0x1FFFE000) | (info.size >> 13));

        if ( info.mode == EBI_TS_TA_MODE )     // syncronious mode
            flags = (EBI_TS_TA_MODE | EBI_ENABLE);
        else
        {
            flags = ( EBI_ENABLE | \
                (EBI_WAIT_STATES  & (info.wait_state << EBI_WTST_SHIFT )) | \
                (EBI_SETUP_STATES & (info.setup_time << EBI_SETUP_SHIFT)) | \
                (EBI_HOLD_STATES  & (info.hold_time  << EBI_HOLD_SHIFT )) );
        }
        MPI->cs[cs].config = flags;
        parms->result = BP_SUCCESS;
        retv = 0;
    }
    else
    {
        retv -= EFAULT;
        parms->result = BP_NOT_DEFINED;
    }
#endif
    return( retv );
}


#if !defined(CONFIG_BCM968500)
/***************************************************************************
* Handle push of restore to default button
***************************************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
static void restore_to_default_thread(struct work_struct *work)
#else
static void restore_to_default_thread(void *arg)
#endif
{
    char buf[256];

    memset(buf, 0, sizeof(buf));
    printk("Restore to Factory Default Setting ***\n\n");
#ifndef SKY /* Don't do this for meerkat */
    // Do this in a kernel thread so we don't have any restriction
    kerSysPersistentSet( buf, sizeof(buf), 0 );
#endif /* SKY */
#if defined(CONFIG_BCM_PLC_BOOT)
    kerSysFsFileSet("/data/plc/plc_pconfig_state", buf, 1);
#endif

    // kernel_restart is a high level, generic linux way of rebooting.
    // It calls a notifier list and lets sub-systems know that system is
    // rebooting, and then calls machine_restart, which eventually
    // calls kerSysMipsSoftReset.
    kernel_restart(NULL);

    return;
}

static irqreturn_t reset_isr(int irq, void *dev_id)
{
    int isOurs = 1, ext_irq_idx = 0, value=0;

    //printk("reset_isr called irq %d, gpio %d 0x%lx\n", irq, *(int *)dev_id, PERF->IrqControl32[0].IrqMaskHi);

    ext_irq_idx = irq - INTERRUPT_ID_EXTERNAL_0;
    if (IsExtIntrShared(extIntrInfo[ext_irq_idx]))
    {
       value = kerSysGetGpioValue(*(int *)dev_id);
       if( (IsExtIntrTypeActHigh(extIntrInfo[ext_irq_idx]) && value) || (IsExtIntrTypeActLow(extIntrInfo[ext_irq_idx]) && !value) )
    	   isOurs = 1;
       else
    	   isOurs = 0;
    }

    if (isOurs)
    {
    	if( !restore_in_progress )
    	{
            printk("\n***reset button press detected***\n\n");

            INIT_WORK(&restoreDefaultWork, restore_to_default_thread);
            schedule_work(&restoreDefaultWork);
            restore_in_progress  = 1;
     	}
        return IRQ_HANDLED;
     }

     return IRQ_NONE;
}
#endif

#if defined(WIRELESS)
/***********************************************************************
* Function Name: kerSysScreenPciDevices
* Description  : Screen Pci Devices before loading modules
***********************************************************************/
static void __init kerSysScreenPciDevices(void)
{
    unsigned short wlFlag;

    if((BpGetWirelessFlags(&wlFlag) == BP_SUCCESS) && (wlFlag & BP_WLAN_EXCLUDE_ONBOARD)) {
        /*
        * scan all available pci devices and delete on board BRCM wireless device
        * if external slot presents a BRCM wireless device
        */
        int foundPciAddOn = 0;
        struct pci_dev *pdevToExclude = NULL;
        struct pci_dev *dev = NULL;

        while((dev=pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev))!=NULL) {
            printk("kerSysScreenPciDevices: 0x%x:0x%x:(slot %d) detected\n", dev->vendor, dev->device, PCI_SLOT(dev->devfn));
            if((dev->vendor == BRCM_VENDOR_ID) &&
                (((dev->device & 0xff00) == BRCM_WLAN_DEVICE_IDS)||
                ((dev->device/1000) == BRCM_WLAN_DEVICE_IDS_DEC))) {
                    if(PCI_SLOT(dev->devfn) != WLAN_ONBOARD_SLOT) {
                        foundPciAddOn++;
                    } else {
                        pdevToExclude = dev;
                    }
            }
        }

#ifdef CONFIG_PCI
        if(((wlFlag & BP_WLAN_EXCLUDE_ONBOARD_FORCE) || foundPciAddOn) && pdevToExclude) {
            printk("kerSysScreenPciDevices: 0x%x:0x%x:(onboard) deleted\n", pdevToExclude->vendor, pdevToExclude->device);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
            pci_remove_bus_device(pdevToExclude);
#else
            __pci_remove_bus_device(pdevToExclude);
#endif
        }
#else
#error ATTEMPT TO COMPILE WIRELESS WITHOUT PCI
#endif
    }
}

#if !defined(CONFIG_BCM968500)
/***********************************************************************
* Function Name: kerSetWirelessPD
* Description  : Control Power Down by Hardware if the board supports
***********************************************************************/
static void kerSetWirelessPD(int state)
{
    unsigned short wlanPDGpio;
    if((BpGetWirelessPowerDownGpio(&wlanPDGpio)) == BP_SUCCESS) {
        if (wlanPDGpio != BP_NOT_DEFINED) {
            if(state == WLAN_OFF)
                kerSysSetGpioState(wlanPDGpio, kGpioActive);
            else
                kerSysSetGpioState(wlanPDGpio, kGpioInactive);
        }
    }
}
#endif
#endif

extern unsigned char g_blparms_buf[];

/***********************************************************************
 * Function Name: kerSysBlParmsGetInt
 * Description  : Returns the integer value for the requested name from
 *                the boot loader parameter buffer.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysBlParmsGetInt( char *name, int *pvalue )
{
    char *p2, *p1 = g_blparms_buf;
    int ret = -1;

    *pvalue = -1;

    /* The g_blparms_buf buffer contains one or more contiguous NULL termianted
     * strings that ends with an empty string.
     */
    while( *p1 )
    {
        p2 = p1;

        while( *p2 != '=' && *p2 != '\0' )
            p2++;

        if( *p2 == '=' )
        {
            *p2 = '\0';

            if( !strcmp(p1, name) )
            {
                *p2++ = '=';
                *pvalue = simple_strtol(p2, &p1, 0);
                if( *p1 == '\0' )
                    ret = 0;
                break;
            }

            *p2 = '=';
        }

        p1 += strlen(p1) + 1;
    }

    return( ret );
}

/***********************************************************************
 * Function Name: kerSysBlParmsGetStr
 * Description  : Returns the string value for the requested name from
 *                the boot loader parameter buffer.
 * Returns      : 0 - success, -1 - failure
 ***********************************************************************/
int kerSysBlParmsGetStr( char *name, char *pvalue, int size )
{
    char *p2, *p1 = g_blparms_buf;
    int ret = -1;

    /* The g_blparms_buf buffer contains one or more contiguous NULL termianted
     * strings that ends with an empty string.
     */
    while( *p1 )
    {
        p2 = p1;

        while( *p2 != '=' && *p2 != '\0' )
            p2++;

        if( *p2 == '=' )
        {
            *p2 = '\0';

            if( !strcmp(p1, name) )
            {
                *p2++ = '=';
                strncpy(pvalue, p2, size);
                ret = 0;
                break;
            }

            *p2 = '=';
        }

        p1 += strlen(p1) + 1;
    }

    return( ret );
}

static int proc_get_wan_type(char *buf, char **start, off_t off, int cnt, int *eof, void *data)
{
    int n = 0;

#if defined(CONFIG_BCM96818) || defined(CONFIG_BCM96816)
    sprintf(buf, "gpon%n", &n);
    return n;
#else   /* defined(CONFIG_BCM96818) || defined(CONFIG_BCM96816) */
#if defined(CONFIG_BCM96828)
    sprintf(buf, "epon%n", &n);
    return n;
#else   /* defined(CONFIG_BCM96828) */
    unsigned long wan_type = 0, t;
    int i, j, len = 0;

    BpGetOpticalWan(&wan_type);
    if (wan_type == BP_NOT_DEFINED)
    {
        sprintf(buf, "none%n", &n);
        return n;
    }

    for (i = 0, j = 0; wan_type; i++)
    {
        t = wan_type & (1 << i);
        if (!t)
            continue;

        wan_type &= ~(1 << i);
        if (j++)
        {
            sprintf(buf + len, "\n");
            len++;
        }

        switch (t)
        {
        case BP_OPTICAL_WAN_GPON:
            sprintf(buf + len, "gpon%n", &n);
            break;
        case BP_OPTICAL_WAN_EPON:
            sprintf(buf + len, "epon%n", &n);
            break;
        case BP_OPTICAL_WAN_AE:
            sprintf(buf + len, "ae%n", &n);
            break;
        default:
            sprintf(buf + len, "unknown%n", &n);
            break;
        }
        len += n;
    }

    return len;
#endif   /* defined(CONFIG_BCM96828) */
#endif   /* defined(CONFIG_BCM96818) || defined(CONFIG_BCM96816) */
}

static int add_proc_files(void)
{
#define offset(type, elem) ((int)&((type *)0)->elem)

    static int BaseMacAddr[2] = {offset(NVRAM_DATA, ucaBaseMacAddr), NVRAM_MAC_ADDRESS_LEN};

    struct proc_dir_entry *p0;
    struct proc_dir_entry *p1;
    struct proc_dir_entry *p2;

#ifdef SKY
/* NK added for locking the I/O mapped memory */
    spin_lock_init(&lockled);
#endif /* SKY */
    p0 = proc_mkdir("nvram", NULL);

    if (p0 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1 = create_proc_entry("BaseMacAddr", 0644, p0);

    if (p1 == NULL)
    {
        printk("add_proc_files: failed to create proc files!\n");
        return -1;
    }

    p1->data        = BaseMacAddr;
    p1->read_proc   = proc_get_param;
    p1->write_proc  = proc_set_param;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

    p1 = create_proc_entry("led", 0644, NULL);
    if (p1 == NULL)
        return -1;

    p1->write_proc  = proc_set_led;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner       = THIS_MODULE;
#endif

    p2 = create_proc_entry("supported_optical_wan_types", 0444, p0);
    if (p2 == NULL)
        return -1;
    p2->read_proc = proc_get_wan_type;
#ifdef SKY
    p1 = create_proc_entry("led_control", 0644, NULL);
    if (p1 == NULL)
        return -1;
   p1->read_proc = proc_get_led_mem;
   p1->write_proc  = proc_write_control_led;
#endif /* SKY */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	//New linux no longer requires proc_dir_entry->owner field.
#else
    p2->owner       = THIS_MODULE;
#endif

#ifdef CONFIG_SKY_REBOOT_DIAGNOSTICS
	p1 = create_proc_entry("dying_gasp", 0666, p0);
    if (p1 == NULL)
        return -1;
    p1->read_proc   = proc_get_dg_mem;
    p1->write_proc  = proc_set_dg_mem;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner = THIS_MODULE;
#endif

	p1 = create_proc_entry("kernel_panic", 0666, p0);
    if (p1 == NULL)
        return -1;
    p1->read_proc   = proc_get_kern_panic;
    p1->write_proc  = proc_set_kern_panic;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner = THIS_MODULE;
#endif

	p1 = create_proc_entry("kernel_die", 0666, p0);
    if (p1 == NULL)
        return -1;
    p1->read_proc   = proc_get_kern_die;
    p1->write_proc  = proc_set_kern_die;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner = THIS_MODULE;
#endif

	p1 = create_proc_entry("clk_rst_ctl", 0666, p0);
    if (p1 == NULL)
        return -1;
    p1->read_proc   = proc_get_rst_ctl;
    p1->write_proc  = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
        //New linux no longer requires proc_dir_entry->owner field.
#else
    p1->owner = THIS_MODULE;
#endif


#endif /* CONFIG_SKY_REBOOT_DIAGNOSTICS */


    return 0;
}

static int del_proc_files(void)
{
    remove_proc_entry("nvram", NULL);
    remove_proc_entry("led", NULL);
    return 0;
}

static void str_to_num(char* in, char* out, int len)
{
    int i;
    memset(out, 0, len);

    for (i = 0; i < len * 2; i ++)
    {
        if ((*in >= '0') && (*in <= '9'))
            *out += (*in - '0');
        else if ((*in >= 'a') && (*in <= 'f'))
            *out += (*in - 'a') + 10;
        else if ((*in >= 'A') && (*in <= 'F'))
            *out += (*in - 'A') + 10;
        else
            *out += 0;

        if ((i % 2) == 0)
            *out *= 16;
        else
            out ++;

        in ++;
    }
    return;
}

static int proc_get_param(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int i = 0;
    int r = 0;
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];
    NVRAM_DATA *pNvramData;

    *eof = 1;

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;

    if (NULL != (pNvramData = readNvramData()))
    {
        for (i = 0; i < length; i ++)
            r += sprintf(page + r, "%02x ", ((unsigned char *)pNvramData)[offset + i]);
    }

    r += sprintf(page + r, "\n");
    if (pNvramData)
        kfree(pNvramData);
    return (r < cnt)? r: 0;
}

static int proc_set_param(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    NVRAM_DATA *pNvramData;
    char input[32];

    int i = 0;
    int r = cnt;
    int offset  = ((int *)data)[0];
    int length  = ((int *)data)[1];

    if ((offset < 0) || (offset + length > sizeof(NVRAM_DATA)))
        return 0;

    if ((cnt > 32) || (copy_from_user(input, buf, cnt) != 0))
        return -EFAULT;

    for (i = 0; i < r; i ++)
    {
        if (!isxdigit(input[i]))
        {
            memmove(&input[i], &input[i + 1], r - i - 1);
            r --;
            i --;
        }
    }

    mutex_lock(&flashImageMutex);

    if (NULL != (pNvramData = readNvramData()))
    {
        str_to_num(input, ((char *)pNvramData) + offset, length);
        writeNvramDataCrcLocked(pNvramData);
    }
    else
    {
        cnt = 0;
    }

    mutex_unlock(&flashImageMutex);

    if (pNvramData)
        kfree(pNvramData);

    return cnt;
}

/*
 * This function expect input in the form of:
 * echo "xxyy" > /proc/led
 * where xx is hex for the led number
 * and   yy is hex for the led state.
 * For example,
 *     echo "0301" > led
 * will turn on led 3
 */
static int proc_set_led(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    char leddata[16];
    char input[32];
    int i;
    int r;
    int num_of_octets;

    if (cnt > 32)
        cnt = 32;

    if (copy_from_user(input, buf, cnt) != 0)
        return -EFAULT;

    r = cnt;

    for (i = 0; i < r; i ++)
    {
        if (!isxdigit(input[i]))
        {
            memmove(&input[i], &input[i + 1], r - i - 1);
            r --;
            i --;
        }
    }

    num_of_octets = r / 2;

    if (num_of_octets != 2)
        return -EFAULT;

    str_to_num(input, leddata, num_of_octets);
    kerSysLedCtrl (leddata[0], leddata[1]);
    return cnt;
}


#if defined(CONFIG_BCM96368)

#define DSL_PHY_PHASE_CNTL      ((volatile uint32* const) 0xb00012a8)
#define DSL_CPU_PHASE_CNTL      ((volatile uint32* const) 0xb00012ac)
#define MIPS_PHASE_CNTL         ((volatile uint32* const) 0xb00012b0)
#define DDR1_2_PHASE_CNTL       ((volatile uint32* const) 0xb00012b4)
#define DDR3_4_PHASE_CNTL       ((volatile uint32* const) 0xb00012b8)

// The direction bit tells the automatic counters to count up or down to the
// desired value.
#define PI_VALUE_WIDTH 14
#define PI_COUNT_UP    ( 1 << PI_VALUE_WIDTH )
#define PI_MASK        ( PI_COUNT_UP - 1 )

// Turn off sync mode.  Set bit 28 of CP0 reg 22 sel 5.
static void TurnOffSyncMode( void )
{
    uint32 value;

    value = __read_32bit_c0_register( $22, 5 ) | (1<<28);
    __write_32bit_c0_register( $22, 5, value );
    //    Print( "Sync mode %x\n", value );
    value = DDR->MIPSPhaseCntl;

    // Reset the PH_CNTR_CYCLES to 7.
    // Set the phase counter cycles (bits 16-19) back to 7.
    value &= ~(0xf<<16);
    value |= (7<<16);

    // Set the LLMB counter cycles back to 7.
    value &= ~(0xf<<24);
    value |= (7<<24);
    // Set the UBUS counter cycles back to 7.
    value &= ~(0xf<<28);
    value |= (7<<28);

    // Turn off the LLMB counter, which is what maintains sync mode.
    // Clear bit 21, which is LLMB_CNTR_EN.
    value &= ~(1 << 21);
    // Turn off UBUS LLMB CNTR EN
    value &= ~(1 << 23);

    DDR->MIPSPhaseCntl = value;

    // Reset the MIPS phase to 0.
    PI_lower_set( MIPS_PHASE_CNTL, 0 );

    //Clear Count Bit
    value &= ~(1 << 14);
    DDR->MIPSPhaseCntl = value;

}

// Write the specified value in the lower half of a PI control register.  Each
// 32-bit register holds two values, but they can't be addressed separately.
static void
PI_lower_set( volatile uint32  *PI_reg,
             int               newPhaseInt )
{
    uint32  oldRegValue;
    uint32  saveVal;
    int32   oldPhaseInt;
    int32   newPhase;
    uint32  newVal;
    int     equalCount      = 0;

    oldRegValue = *PI_reg;
    // Save upper 16 bits, which is the other PI value.
    saveVal     = oldRegValue & 0xffff0000;

    // Sign extend the lower PI value, and shift it down into the lower 16 bits.
    // This gives us a 32-bit signed value which we can compare to the newPhaseInt
    // value passed in.

    // Shift the sign bit to bit 31
    oldPhaseInt = oldRegValue << ( 32 - PI_VALUE_WIDTH );
    // Sign extend and shift the lower value into the lower 16 bits.
    oldPhaseInt = oldPhaseInt >> ( 32 - PI_VALUE_WIDTH );

    // Take the low 10 bits as the new phase value.
    newPhase = newPhaseInt & PI_MASK;

    // If our new value is larger than the old value, tell the automatic counter
    // to count up.
    if ( newPhaseInt > oldPhaseInt )
    {
        newPhase = newPhase | PI_COUNT_UP;
    }

    // Or in the value originally in the upper 16 bits.
    newVal  = newPhase | saveVal;
    *PI_reg = newVal;

    // Wait until we match several times in a row.  Only the low 4 bits change
    // while the counter is working, so we can get a false "done" indication
    // when we read back our desired value.
    do
    {
        if ( *PI_reg == newVal )
        {
            equalCount++;
        }
        else
        {
            equalCount = 0;
        }

    } while ( equalCount < 3 );

}

// Write the specified value in the upper half of a PI control register.  Each
// 32-bit register holds two values, but they can't be addressed separately.
static void
PI_upper_set( volatile uint32  *PI_reg,
             int               newPhaseInt )
{
    uint32  oldRegValue;
    uint32  saveVal;
    int32   oldPhaseInt;
    int32   newPhase;
    uint32  newVal;
    int     equalCount      = 0;

    oldRegValue = *PI_reg;
    // Save lower 16 bits, which is the other PI value.
    saveVal     = oldRegValue & 0xffff;

    // Sign extend the upper PI value, and shift it down into the lower 16 bits.
    // This gives us a 32-bit signed value which we can compare to the newPhaseInt
    // value passed in.

    // Shift the sign bit to bit 31
    oldPhaseInt = oldRegValue << ( 16 - PI_VALUE_WIDTH );
    // Sign extend and shift the upper value into the lower 16 bits.
    oldPhaseInt = oldPhaseInt >> ( 32 - PI_VALUE_WIDTH );

    // Take the low 10 bits as the new phase value.
    newPhase = newPhaseInt & PI_MASK;

    // If our new value is larger than the old value, tell the automatic counter
    // to count up.
    if ( newPhaseInt > oldPhaseInt )
    {
        newPhase = newPhase | PI_COUNT_UP;
    }

    // Shift the new phase value into the upper 16 bits, and restore the value
    // originally in the lower 16 bits.
    newVal = (newPhase << 16) | saveVal;
    *PI_reg = newVal;

    // Wait until we match several times in a row.  Only the low 4 bits change
    // while the counter is working, so we can get a false "done" indication
    // when we read back our desired value.
    do
    {
        if ( *PI_reg == newVal )
        {
            equalCount++;
        }
        else
        {
            equalCount = 0;
        }

    } while ( equalCount < 3 );

}

// Reset the DDR PI registers to the default value of 0.
static void ResetPiRegisters( void )
{
    volatile int delay;
    uint32 value;

    //Skip This step for now load_ph should be set to 0 for this anyways.
    //Print( "Resetting DDR phases to 0\n" );
    //PI_lower_set( DDR1_2_PHASE_CNTL, 0 ); // DDR1 - Should be a NOP.
    //PI_upper_set( DDR1_2_PHASE_CNTL, 0 ); // DDR2
    //PI_lower_set( DDR3_4_PHASE_CNTL, 0 ); // DDR3 - Must remain at 90 degrees for normal operation.
    //PI_upper_set( DDR3_4_PHASE_CNTL, 0 ); // DDR4

    // Need to have VDSL back in reset before this is done.
    // Disable VDSL Mip's
    GPIO->VDSLControl = GPIO->VDSLControl & ~VDSL_MIPS_RESET;
    // Disable VDSL Core
    GPIO->VDSLControl = GPIO->VDSLControl & ~(VDSL_CORE_RESET | 0x8);


    value = DDR->DSLCpuPhaseCntr;

    // Reset the PH_CNTR_CYCLES to 7.
    // Set the VDSL Mip's phase counter cycles (bits 16-19) back to 7.
    value &= ~(0xf<<16);
    value |= (7<<16);

    // Set the VDSL PHY counter cycles back to 7.
    value &= ~(0xf<<24);
    value |= (7<<24);
    // Set the VDSL AFE counter cycles back to 7.
    value &= ~(0xf<<28);
    value |= (7<<28);

    // Turn off the VDSL MIP's PHY auto counter
    value &= ~(1 << 20);
    // Clear bit 21, which is VDSL PHY CNTR_EN.
    value &= ~(1 << 21);
    // Turn off the VDSL AFE auto counter
    value &= ~(1 << 22);

    DDR->DSLCpuPhaseCntr = value;

    // Reset the VDSL MIPS phase to 0.
    PI_lower_set( DSL_PHY_PHASE_CNTL, 0 ); // VDSL PHY - should be NOP
    PI_upper_set( DSL_PHY_PHASE_CNTL, 0 ); // VDSL AFE - should be NOP
    PI_lower_set( DSL_CPU_PHASE_CNTL, 0 ); // VDSL MIP's

    //Clear Count Bits for DSL CPU
    value &= ~(1 << 14);
    DDR->DSLCpuPhaseCntr = value;
    //Clear Count Bits for DSL Core
    DDR->DSLCorePhaseCntl &= ~(1<<30);
    DDR->DSLCorePhaseCntl &= ~(1<<14);
    // Allow some settle time.
    delay = 100;
    while (delay--);

    printk("\n****** DDR->DSLCorePhaseCntl=%lu ******\n\n", (unsigned long)
        DDR->DSLCorePhaseCntl);

    // Turn off the automatic counters.
    // Clear bit 20, which is PH_CNTR_EN.
    DDR->MIPSPhaseCntl &= ~(1<<20);
    // Turn Back UBUS Signals to reset state
    DDR->UBUSPhaseCntl = 0x00000000;
    DDR->UBUSPIDeskewLLMB0 = 0x00000000;
    DDR->UBUSPIDeskewLLMB1 = 0x00000000;

}

static void ChipSoftReset(void)
{
    TurnOffSyncMode();
    ResetPiRegisters();
}
#endif


/***************************************************************************
 * Function Name: kerSysGetUbusFreq
 * Description  : Chip specific computation.
 * Returns      : the UBUS frequency value in MHz.
 ***************************************************************************/
unsigned long kerSysGetUbusFreq(unsigned long miscStrapBus)
{
   unsigned long ubus = UBUS_BASE_FREQUENCY_IN_MHZ;

#if defined(CONFIG_BCM96362)
   /* Ref RDB - 6362 */
   switch (miscStrapBus) {

      case 0x4 :
      case 0xc :
      case 0x14:
      case 0x1c:
      case 0x15:
      case 0x1d:
         ubus = 100;
         break;
      case 0x2 :
      case 0xa :
      case 0x12:
      case 0x1a:
         ubus = 96;
         break;
      case 0x1 :
      case 0x9 :
      case 0x11:
      case 0xe :
      case 0x16:
      case 0x1e:
         ubus = 200;
         break;
      case 0x6:
         ubus = 183;
         break;
      case 0x1f:
         ubus = 167;
         break;
      default:
         ubus = 160;
         break;
   }
#endif

   return (ubus);

}  /* kerSysGetUbusFreq */


/***************************************************************************
 * Function Name: kerSysGetChipId
 * Description  : Map id read from device hardware to id of chip family
 *                consistent with  BRCM_CHIP
 * Returns      : chip id of chip family
 ***************************************************************************/
int kerSysGetChipId() {
        int r;
#if defined(CONFIG_BCM968500)
		r = 0x68500;
#elif defined(CONFIG_BCM96838)
        r = 0x6838;
#else
        int t;
        r = (int) ((PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT);
        /* Force BCM681x variants to be BCM6816 or BCM6818 correctly) */
        if( (r & 0xfff0) == 0x6810 )
        {
            if ((r == 0x6811) || (r == 0x6815) || (r == 0x6817)) {
                r = 0x6818;
            } else {
                r = 0x6816;
                t = (int) (PERF->RevID & REV_ID_MASK);
                if ((t & 0xf0) == 0xA0)
                {
                    r = 0x6818;
                }
            }
        }

        /* Force BCM6369 to be BCM6368) */
        if( (r & 0xfffe) == 0x6368 )
            r = 0x6368;

        /* Force 6821 and 6822 to be BCM6828 */
        if ((r == 0x6821) || (r == 0x6822))
            r = 0x6828;

        /* Force BCM63168, BCM63169, and BCM63269 to be BCM63268) */
        if( ( (r & 0xffffe) == 0x63168 )
          || ( (r & 0xffffe) == 0x63268 ))
            r = 0x63268;

        /* Force 6319 to be BCM6318 */
        if (r == 0x6319)
            r = 0x6318;

#endif

        return(r);
}

#if !defined(CONFIG_BCM968500)
/***************************************************************************
 * Function Name: kerSysGetDslPhyEnable
 * Description  : returns true if device should permit Phy to load
 * Returns      : true/false
 ***************************************************************************/
int kerSysGetDslPhyEnable() {
        int id;
        int r = 1;
        id = (int) ((PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT);
        if ((id == 0x63169) || (id == 0x63269)) {
	    r = 0;
        }
        return(r);
}
#endif

/***************************************************************************
 * Function Name: kerSysGetChipName
 * Description  : fills buf with the human-readable name of the device
 * Returns      : pointer to buf
 ***************************************************************************/
char *kerSysGetChipName(char *buf, int n) {
	return(UtilGetChipName(buf, n));
}

#if !defined(CONFIG_BCM968500)
/***************************************************************************
 * Function Name: kerSysGetExtIntInfo
 * Description  : return the external interrupt information which includes the
 *                trigger type, sharing enable.
 * Returns      : pointer to buf
 ***************************************************************************/
unsigned int kerSysGetExtIntInfo(unsigned int irq)
{
	return extIntrInfo[irq-INTERRUPT_ID_EXTERNAL_0];
}
#endif


int kerSysGetPciePortEnable(int port)
{
	int ret = 1;
#if defined (CONFIG_BCM96838)
	unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;

    switch (chipId)
	{
		case 1:		// 68380
			ret = 1;
			break;

		case 3:		// 68380F
			if(port == 0)
				ret = 1;
			else
				ret = 0;
			break;

		default:
			ret = 0;
			break;
    }
#endif
	return ret;
}
EXPORT_SYMBOL(kerSysGetPciePortEnable);

int kerSysGetUsbHostPortEnable(int port)
{
	int ret = 1;
#if defined (CONFIG_BCM96838)
	unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;

    switch (chipId)
	{
		case 1:		// 68380
			ret = 1;
			break;

		case 3:		// 68380F
			if(port == 0)
				ret = 1;
			else
				ret = 0;
			break;

		default:
			ret = 0;
			break;
    }
#endif
	return ret;
}
EXPORT_SYMBOL(kerSysGetUsbHostPortEnable);

int kerSysGetUsbDeviceEnable(void)
{
	int ret = 1;

#if defined (CONFIG_BCM96838)
	ret = 0;
#endif

	return ret;
}
EXPORT_SYMBOL(kerSysGetUsbDeviceEnable);

int kerSysSetUsbPower(int on, USB_FUNCTION func)
{
	int status = 0;
#if !defined(CONFIG_BRCM_IKOS)
#if defined (CONFIG_BCM96838)
	static int usbHostPwr = 1;
	static int usbDevPwr = 1;

	if(on)
	{
		if(!usbHostPwr && !usbDevPwr)
			status = PowerOnZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_Common);

		if(((func == USB_HOST_FUNC) || (func == USB_ALL_FUNC)) && !usbHostPwr && !status)
		{
			status = PowerOnZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Host);
			if(status)
				usbHostPwr = 1;
		}

		if(((func == USB_DEVICE_FUNC) || (func == USB_ALL_FUNC)) && !usbDevPwr && !status)
		{
			status = PowerOnZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Device);
			if(status)
				usbDevPwr = 1;
		}
	}
	else
	{
		if(((func == USB_HOST_FUNC) || (func == USB_ALL_FUNC)) && usbHostPwr)
		{
			status = PowerOffZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Host);
			if(status)
				usbHostPwr = 0;
		}

		if(((func == USB_DEVICE_FUNC) || (func == USB_ALL_FUNC)) && usbDevPwr)
		{
			status = PowerOffZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_USB_Device);
			if(status)
					usbDevPwr = 0;
		}

		if(!usbHostPwr && !usbDevPwr)
			status = PowerOffZone(PMB_ADDR_USB30_2X, USB30_2X_Zone_Common);
	}
#endif
#endif

	return status;
}
EXPORT_SYMBOL(kerSysSetUsbPower);

int kerSysSetPciePortPower(int on, int port)
{
	int status = 0;
#if !defined(CONFIG_BRCM_IKOS)
#if defined (CONFIG_BCM96838)
	int dev;

	if(port == 0)
		dev = PMB_ADDR_PCIE_UBUS_0;
	else
		dev = PMB_ADDR_PCIE_UBUS_1;

	if(on)
		status = PowerOnDevice(dev);
	else
		status = PowerOffDevice(dev, 0);
#endif
#endif

	return status;
}
EXPORT_SYMBOL(kerSysSetPciePortPower);

extern const struct obs_kernel_param __setup_start[], __setup_end[];
extern const struct kernel_param __start___param[], __stop___param[];

void kerSysSetBootParm(char *name, char *value)
{
    const struct obs_kernel_param *okp = __setup_start;
    const struct kernel_param *kp = __start___param;

    do {
        if (!strcmp(name, okp->str)) {
            if (okp->setup_func) {
                okp->setup_func(value);
                return;
            }
        }
        okp++;
    } while (okp < __setup_end);

    do {
        if (!strcmp(name, kp->name)) {
            if (kp->ops->set) {
                kp->ops->set(value, kp);
                return;
            }
        }
        kp++;
    } while (kp < __stop___param);
}

#ifdef SKY

static Bool reset_line_held(void)
{
unsigned short rstToDflt_irq;

   BpGetResetToDefaultExtIntr(&rstToDflt_irq);

   if (rstToDflt_irq != BP_EXT_INTR_0)
   {
      printk("Hmmm. That's odd - Reset line is %d and not %d \n",rstToDflt_irq , BP_EXT_INTR_0);
   }
   rstToDflt_irq = map_external_irq (rstToDflt_irq) ;

    if ((rstToDflt_irq >= INTERRUPT_ID_EXTERNAL_0) && (rstToDflt_irq <= INTERRUPT_ID_EXTERNAL_3)) {
        if (!(PERF->ExtIrqCfg & (1 << (rstToDflt_irq - INTERRUPT_ID_EXTERNAL_0 + EI_STATUS_SHFT)))) {
        return 1;
        }
        else
        {
        return 0;
        }
    }
#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96816)
    else if ((rstToDflt_irq >= INTERRUPT_ID_EXTERNAL_4) || (rstToDflt_irq <= INTERRUPT_ID_EXTERNAL_5)) {
        if (!(PERF->ExtIrqCfg1 & (1 << (rstToDflt_irq - INTERRUPT_ID_EXTERNAL_4 + EI_STATUS_SHFT)))) {
        return 1;
        }
        else
        {
        return 0;
        }
    }
#endif
    return 0;
}

/**
 * This is for cases where we boot with the reset held, in which case
 * we need to wait for a reset release before enabling the reset
 * interrupt.
 * We make a short lived thread to monitor the reset input and perform
 * the re-enable. This is almost certainly overkill for our case and
 * we should replace this with a tasklet or similar at some point.
 */
static int wait_reset_release( void *data )
{
unsigned short rstToDflt_irq;

   daemonize("reset-monitor");

   while (reset_line_held())
   {/* Sleep for 100msecond (HZ/10 jiffies) */
      set_current_state(TASK_INTERRUPTIBLE);
      schedule_timeout(HZ/10);
   }

   /* Now just setup the handler to catch the ISR */
   if( BpGetResetToDefaultExtIntr(&rstToDflt_irq) == BP_SUCCESS )
   {
      rstToDflt_irq = map_external_irq (rstToDflt_irq) ;
      BcmHalMapInterrupt((FN_HANDLER)reset_isr, 0, rstToDflt_irq);
      BcmHalInterruptEnable(rstToDflt_irq);
   }

    printk("reset-monitor: Reset IRQ Enabled\n");
    do_exit(0); /* Nobody else cares about us, we can just leave */
return 0;
}

/* The serialisation manipulation - these are mirrors of the NVRAM data
 * structures.
 * The serialisation use case is actually simpler - only one application
 * is allowed to write this section, and even then only in the factory
 */
static PSERIALISATION_DATA readSerialisationData(void)
{
    SERIALISATION_DATA *pSerialisationData;

    // use GFP_ATOMIC here because caller might have flashImageMutex held
    if (NULL == (pSerialisationData = kmalloc(sizeof(SERIALISATION_DATA), GFP_ATOMIC)))
    {
        printk("readSerialisationData: could not allocate memory\n");
        return NULL;
    }

    kerSysSerialisationGet((char *)pSerialisationData, sizeof(SERIALISATION_DATA), 0);
   /* We have a simpler use case, so leave the CRC check up to the caller */
return pSerialisationData;
}

static PCFE_NVRAM_DATA readCfeNvramData(void)
{
    CFE_NVRAM_DATA *pCfeNvramData;

    // use GFP_ATOMIC here because caller might have flashImageMutex held
    if (NULL == (pCfeNvramData = kmalloc(sizeof(CFE_NVRAM_DATA), GFP_ATOMIC)))
    {
        printk("readCfeNvramData: could not allocate memory\n");
        return NULL;
    }

    kerSysCfeNvRamGet((char *)pCfeNvramData, sizeof(CFE_NVRAM_DATA), 0);
   /* We have a simpler use case, so leave the CRC check up to the caller */
	return pCfeNvramData;
}

#ifdef CONFIG_SKY_FEATURE_WATCHDOG

static void initHWWatchDog()
{
    TIMER->WatchDogDefCount = (unsigned int)((unsigned int)(WATCHDOG_DEFCOUNT)*(unsigned int)(FPERIPH));  // 40 sec
	printk("Initalisg watchdog: WatchDogDefCount=%ld \n",TIMER->WatchDogDefCount);
//    TIMER->WDResetCount = 50000;
    TIMER->WatchDogCtl = 0xFF00;
    TIMER->WatchDogCtl = 0x00FF;
}

void restartHWWatchDog()
{
//	printk("Restarting watchdog \n");
    TIMER->WatchDogCtl = 0xFF00;
    TIMER->WatchDogCtl = 0x00FF;
}

static void stopHWWatchDog()
{
//	printk("Stopping watchdog \n");
    TIMER->WatchDogCtl = 0xEE00;
    TIMER->WatchDogCtl = 0x00EE;
}

#endif /* CONFIG_SKY_FEATURE_WATCHDOG */


#if 0
static void writeSerialisationDataCrcLocked(PSERIALISATION_DATA pSerialisationData)
{
    UINT32 crc = CRC32_INIT_VALUE;

    BCM_ASSERT_HAS_MUTEX_C(&flashImageMutex);

    pSerialisationData->ulCheckSum = 0;
    crc = getCrc32((char *)pSerialisationData, sizeof(SERIALISATION_DATA), crc);
    pSerialisationData->ulCheckSum = crc;
    kerSysSerialisationSet((char *)pSerialisationData, sizeof(SERIALISATION_DATA), 0);
}
#endif

/* This is newly added function for power LED control-NK */
static void pwrLed_ctrl(int action)
{
    BOARD_LED_STATE led;
    int state=action & 0xff;
    int  which_led=(action >> 8) & 0xff;
    int blinktype=	(action >> 24) & 0xff;
    //printk("pwrLed_ctrl: state= 0x%x which led=0x%x btype=%d \n", state,which_led,blinktype);

    switch (state) {
    case SES_LED_ON:
        led = kLedStateOn;
        break;
    case SES_LED_BLINK:
	 if(!blinktype)
        	led = kLedStateFastBlinkContinues;
	 else
	 	led=blinktype;
       break;
    case LED_ERROR:
	led = kLedStateFail;
	break;
    case SES_LED_OFF:
    default:
        led = kLedStateOff;
    }
      kerSysLedCtrl(which_led, led);

}

/* This function expect input in the form of:
 * "led_rd start:end" > /proc/led_control
 * where start is hex for the I/o mapped address
 * and   end is hex for I/O mapped end address
 * For example,
 *  "led_rd 0xb0000080:0xb0000084">/proc/led_control
 * will turn on led 3
 */
static int proc_write_control_led(struct file *f, const char *buf, unsigned long cnt, void *data)
{
   int ret=0;
   char str[32]={'\0'};

  if(cnt>(sizeof(str)-1))
       cnt=sizeof(str)-1;
 if (copy_from_user(str, buf, cnt) != 0)
        return -EFAULT;
  str[cnt]='\0';
   if (!strncmp(str,"led_rd", 6))
    {
        printk("led_rd -> ");
	ret=led_io_read((str+7));
    }
    else if (!strncmp(str,"led_wr", 6))
    {
        printk("led_wr -> ");
	ret=led_io_write(str+7);
    }
    else if (!strncmp(str,"led_dmp", 7))
    {
        printk("Led Dump -> ");
	ret=led_dump(str+8);
    }
#ifdef SKY_FLASH_API
	else if (!strncmp(str,"flash_write", 11))
    {
        printk("flash_write -> ");
		sky_flash_dump_write(str+12, 0);
    }
	else if (!strncmp(str,"flash_dump", 10))
    {
        printk("flash_read -> ");
		sky_flash_dump_write(str+11, 1);
    }
#endif // SKY_FLASH_API
    else if (!strncmp(str,"gpio_rd", 6))
    {
        printk("gpio_rd -> ");
	ret=gpio_io_read((str+7));
    }
    else if (!strncmp(str,"gpio_wr", 6))
    {
        printk("gpio_wr -> ");
	ret=gpio_io_write(str+7);
    }
    else if (!strncmp(str,"gpio_dmp", 8))
    {
        printk("gpio_dump -> ");
	ret=gpio_dump(str+9);
    }
    else
    {
	return -EINVAL;
     }

   if (ret)
   {
      return -EINVAL;
   }

   return cnt;
}
//#define MYLED ((volatile LedControl * const) LED_BASE)
static int gpio_io_read(char *str)
{
   unsigned int start=0,end=0;
   char *endptr=NULL;
   unsigned long flags; // to stores the current irq status flag-NK
   start=simple_strtoul(str,&endptr,0);
    end=start+0x4; // This is to read 4 bytes
  if((endptr!=NULL) && (*endptr!='\0'))
  {
     while((*endptr!='\0') && !isdigit(*endptr))
       endptr++;
      if(*endptr!='\0')
      end=simple_strtoul(endptr,NULL,0);
  }
  printk("\n gpio Rd-Start 0x%08x and end 0x%08x\n",start,end);
   spin_lock_irqsave(&lockled, flags);
   io_start_addr=(unsigned long)start;
   io_end_addr=(unsigned long)end;
  spin_unlock_irqrestore(&lockled, flags);
  return 0;
}

static int led_dump(char *str)
{
   unsigned int start=0,end=0;
   char *endptr=NULL;
   start=simple_strtoul(str,&endptr,0);
    end=start+0x4; // This is to read 4 bytes
  if((endptr!=NULL) && (*endptr!='\0'))
  {
     while((*endptr!='\0') && !isdigit(*endptr))
       endptr++;
      if(*endptr!='\0')
      end=simple_strtoul(endptr,NULL,0);
  }
  printk("\n led dump-Start 0x%08x and end 0x%08x\n",start,end);
  reg_dumpaddr((unsigned char *) start, (int)end);

 return 0;
}
static int gpio_dump(char *str)
{
   unsigned int start=0,end=0;
   char *endptr=NULL;
   start=simple_strtoul(str,&endptr,0);
    end=start+0x4; // This is to read 4 bytes
  if((endptr!=NULL) && (*endptr!='\0'))
  {
     while((*endptr!='\0') && !isdigit(*endptr))
       endptr++;
      if(*endptr!='\0')
      end=simple_strtoul(endptr,NULL,0);
  }
  printk("\n gpio dump-Start 0x%08x and end 0x%08x\n",start,end);
  reg_dumpaddr((unsigned char *) start, (int )end);

 return 0;
}

/* how to dump
 echo "led_dmp 0xb0001900:len">/proc/led_control  */
void reg_dumpaddr( unsigned char *pAddr, int nLen )
{
    static char szHexChars[] = "0123456789abcdef";
    char szLine[80];
    char *p = szLine;
    unsigned char ch, *q;
    int i = 0, j, size = 0;
    unsigned long ul;
    unsigned short us;

    if( ((unsigned long) pAddr & 0xff000000) == 0xff000000 ||
        ((unsigned long) pAddr & 0xff000000) == 0xb0000000 )
    {
        if (nLen == 2) {
            pAddr = (unsigned char *) ((unsigned long) pAddr & ~0x01);
        } else if (nLen != 1) {
            /* keeping the old logic as is. */
            if( ((unsigned long) pAddr & 0x03) != 0 )
                nLen += 4;
            pAddr = (unsigned char *) ((unsigned long) pAddr & ~0x03);
        }
    }
    while( nLen > 0 )
    {
        sprintf( szLine, "%8.8lx: ", (unsigned long) pAddr );
        p = szLine + strlen(szLine);

        if( ((unsigned long) pAddr & 0xff000000) == 0xff000000 ||
            ((unsigned long) pAddr & 0xff000000) == 0xb0000000 )
        {
            for(i = 0; i < 6 && nLen > 0; i += sizeof(long), nLen -= sizeof(long))
            {
                if (nLen == 1) {
                    q = pAddr;
                    size = 1;
                } else if (nLen == 2) {
                    us = *(unsigned short *)pAddr;
                    q = (unsigned char *) &us;
                    size = 2;
                } else {
                    ul = *(unsigned long *) &pAddr[i];
                    q = (unsigned char *) &ul;
                    size = sizeof(long);
                }
                for( j = 0; j < size; j++ )
                {
                    *p++ = szHexChars[q[j] >> 4];
                    *p++ = szHexChars[q[j] & 0x0f];
                }
                *p++ = ' ';
            }
        }
        else
        {
            for(i = 0; i < 16 && nLen > 0; i++, nLen-- )
            {
                ch =  pAddr[i];

                *p++ = szHexChars[ch >> 4];
                *p++ = szHexChars[ch & 0x0f];
                *p++ = ' ';
            }
        }

        for( j = 0; j < 16 - i; j++ )
            *p++ = ' ', *p++ = ' ', *p++ = ' ';

        *p++ = ' ', *p++ = ' ', *p++ = ' ';

        for( j = 0; j < i; j++ )
        {
            ch = pAddr[j];
            *p++ = (ch >= ' ' && ch <= '~') ? ch : '.';
        }

        *p++ = '\0';
        printk( "%s\r\n", szLine );

        pAddr += i;
    }
    printk( "\r\n" );
} /* ui_dumpaddr */
/* echo "gpio_wr 0xb0000080:0x00002400">/proc/led_control
  */
static int gpio_io_write(char *str)
{
 //  volatile char *ptr=((volatile char * const) GPIO_BASE);
   unsigned long start=0,end=0;
   char *endptr=NULL;
   volatile unsigned long *pul;

   unsigned long flags; // to stores the current irq status flag-NK
    start=simple_strtoul(str,&endptr,0);
    end=start+0x4; // This is to read 4 bytes
  if((endptr!=NULL) && (*endptr!='\0'))
  {
     while((*endptr!='\0') && !isdigit(*endptr))
       endptr++;
      if(*endptr!='\0')
      end=simple_strtoul(endptr,NULL,0);
  }
  printk("\n wr- gpio Start 0x%08lx and end 0x%08lx\n",start,end);
    spin_lock_irqsave(&lockled, flags);
    pul  = (volatile unsigned long *)  start;
    *pul = (unsigned long) end;
    spin_unlock_irqrestore(&lockled, flags);
  return 0;
}

static int led_io_read(char *str)
{
   unsigned int start=0,end=0;
   char *endptr=NULL;
   unsigned long flags; // to stores the current irq status flag-NK
   start=simple_strtoul(str,&endptr,0);
    end=start+0x4; // This is to read 4 bytes
  if((endptr!=NULL) && (*endptr!='\0'))
  {
     while((*endptr!='\0') && !isdigit(*endptr))
       endptr++;
      if(*endptr!='\0')
      end=simple_strtoul(endptr,NULL,0);
  }
  printk("\n led Rd-Start 0x%08x and end 0x%08x\n",start,end);
   spin_lock_irqsave(&lockled, flags);
   io_start_addr=(unsigned long)start;
   io_end_addr=(unsigned long) end;
  spin_unlock_irqrestore(&lockled, flags);
  return 0;
}
/* echo "led_wr 0xb0001900:0x00002400">/proc/led_control
  */
static int led_io_write(char *str)
{
  // volatile char *ptr=((volatile char * const) LED_BASE);
   unsigned long start=0,end=0;
   char *endptr=NULL;
   volatile unsigned long *pul;
   unsigned long flags; // to stores the current irq status flag-NK
    start=simple_strtoul(str,&endptr,0);
    end=start+0x4; // This is to read 4 bytes
  if((endptr!=NULL) && (*endptr!='\0'))
  {
     while((*endptr!='\0') && !isdigit(*endptr))
       endptr++;
      if(*endptr!='\0')
      end=simple_strtoul(endptr,NULL,0);
  }
  printk("\n led wr 0x%08lx and end 0x%08lx\n",start,end);
   spin_lock_irqsave(&lockled, flags);
     pul  = (volatile unsigned long *)  start;
    *pul = (unsigned long) end;
   spin_unlock_irqrestore(&lockled, flags);
  return 0;
}


#ifdef CONFIG_SKY_REBOOT_DIAGNOSTICS

/*
 *     |               |
 *     |               |
 * 510-+===============+ 0x0000 ---------------+--------+
 *     |               |                       ^        |
 *     |     META      | syslog, nvram backup  | 16k    |
 *     |               |                       v        |
 *     +---------------+ 0x4000 ---------------+--------+
 *     |               |                       ^        |
 *     |      SP       |                       | 8k     |
 *     |               |                       v        |
 *     +---------------+ 0x6000 ---------------+--------+
 *     |               |                       ^        |
 *     |      PSI      |                       | 40k    |
 *     |               |                       v        |
 * 511-+===============+ 0x0000 (0x2f00 SR101 & SR102) -+
 *     |               |                       ^        |
 *     |     NVRAM     |                       | 1k     |
 *     |               | 0x2f00 (0x32ff)       v        |
 *     +---------------+  ---------------------+--------+
 *     |               |                                |
 *     |               |                                |
 *     +---------------+ (SKY_SP_OFFSET - sizeof SKY_D) |
 *     |  SKY_DIAGNO   |                                |
 *     +---------------+ 0xff00 ---------------+--------+
 *     |               |                       ^        |
 *     |     SKY_SP    |                       | 256b   |
 *     |               |                       v        |
 *     +===============+-----------------------+--------+
 */

/* helper functions */
/* ========================================================================== */

static int proc_get_dg_mem(char *page, char **start, off_t off, int cnt, int *eof, void *data) {
	SKY_DIAGNOSTICS_DATA sd;
	int i = 0;

	kerSysSkyDiagnosticsGet(&sd);
	for (; i < sizeof(sd.dying_gasp); i++) {
		if ((unsigned char)sd.dying_gasp[i] == 0xff) {
			sd.dying_gasp[i] = '\0';
			break;
		}
	}
	return snprintf(page, PAGE_SIZE, "%s\n", sd.dying_gasp);
}


static int proc_get_kern_panic(char *page, char **star, off_t off, int cnt, int *eof, void *data) {
	SKY_DIAGNOSTICS_DATA sd;
	kerSysSkyDiagnosticsGet(&sd);
	return snprintf(page, PAGE_SIZE, "%x\n", sd.kernel_panic);
}


static int proc_get_kern_die(char *page, char **star, off_t off, int cnt, int *eof, void *data) {
	SKY_DIAGNOSTICS_DATA sd;
	kerSysSkyDiagnosticsGet(&sd);
	return snprintf(page, PAGE_SIZE, "%x\n", sd.kernel_die);
}


static void _sky_diagnostics_clear(size_t s, off_t o, const char* msg) {
	kerSysSkyDiagnosticsFieldClear(o, s);
	printk(msg);
}


static int proc_set_dg_mem(struct file *f, const char *buf, unsigned long cnt, void *data) {
	_sky_diagnostics_clear(sizeof(((SKY_DIAGNOSTICS_DATA *)0x00)->dying_gasp), offsetof(SKY_DIAGNOSTICS_DATA, dying_gasp), "dying_gasp_cleared\n");
	return cnt;
}


static int proc_set_kern_panic(struct file *f, const char *buf, unsigned long cnt, void *data) {
	_sky_diagnostics_clear(sizeof(((SKY_DIAGNOSTICS_DATA *)0x00)->kernel_panic), offsetof(SKY_DIAGNOSTICS_DATA, kernel_panic), "kernel_panic_cleared\n");
	return cnt;
}


static int proc_set_kern_die(struct file *f, const char *buf, unsigned long cnt, void *data) {
	_sky_diagnostics_clear(sizeof(((SKY_DIAGNOSTICS_DATA *)0x00)->kernel_die), offsetof(SKY_DIAGNOSTICS_DATA, kernel_die), "kernel_die_cleared\n");
	return cnt;
}


static int proc_get_rst_ctl(char *page, char **star, off_t off, int cnt, int *eof, void *data) {
	return snprintf(page, PAGE_SIZE, "%08x\n", (unsigned int)TIMER->ClkRstCtl);
}

#endif





/* Read the IO mapped address for GPIO and LED -NK */
static int proc_get_led_mem(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
//   volatile unsigned long *ptr=NULL;
   unsigned long flags;
   int len=0;
 //  int max_lines=0,i=0;
//   int words;



 //  max_lines = (PAGE_SIZE / 60);
   spin_lock_irqsave(&lockled, flags);
 #if 0
    /* get the start address as a pointer */
   ptr = (volatile unsigned long *) io_start_addr;
   words=(io_end_addr-io_start_addr)/sizeof(unsigned long);
   if ((words / 4) > max_lines)
   { words = 4 * max_lines; }



   /* dump all the memory */
   for (i = 0; i < words; i++)
   {
      if ((i % 4) == 0)
      {
   	 len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n0x%08lx :  ", (unsigned long)ptr);
      }

      len += snprintf((page + len), (PAGE_SIZE - len), "0x%08lx ", *ptr++);

   }

   len += snprintf((page + len), (PAGE_SIZE - len), "\n");
#endif
    printk("\n ####DUMPING ALL THE REGISTER OF LED CONTROLLER#### ## \n");
       len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n  LEDInit(0x%08lx): 0x%08lx  \n",(unsigned long) &LED->ledInit,(unsigned long)LED->ledInit);
      len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n LEDMode(0x%08lx): 0x%08lx 0x%08lx  \n", (unsigned long) &LED->ledMode,(unsigned long)LED->ledMode,(unsigned long)(LED+8));
      len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n ledHWDis(0x%08lx): 0x%08lx   \n",(unsigned long)&LED->ledHWDis, (unsigned long)LED->ledHWDis);
      len += snprintf((page + len), (PAGE_SIZE - len),
		     "\nledStrobe(0x%08lx): 0x%08lx   \n",(unsigned long)&LED->ledStrobe, (unsigned long)LED->ledStrobe);
      len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n ledLinkActSelHigh(0x%08lx): 0x%08lx   \n",(unsigned long)&LED->ledLinkActSelHigh, (unsigned long)LED->ledLinkActSelHigh);
     len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n ledLinkActSelLow(0x%08lx): 0x%08lx   \n", (unsigned long)&LED->ledLinkActSelLow,(unsigned long)LED->ledLinkActSelLow);
     len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n ledReadback(0x%08lx): 0x%08lx   \n",(unsigned long)&LED->ledReadback, (unsigned long)LED->ledReadback);
    len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n ledSerialMuxSelect(0x%08lx): 0x%08lx   \n",(unsigned long)&LED->ledSerialMuxSelect, (unsigned long)LED->ledSerialMuxSelect);

    printk("\n###### DUMPING ALL THE REGISTER OF GPIO PAD REGISTERS###### \n");
     len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n GPIO->GPIODir(0x%08lx): 0x%08lx  \n", (unsigned long)&GPIO->GPIODir,(unsigned long)GPIO->GPIODir);

 /*   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n GPIODir: 0x%08lx  \n", (unsigned long)GPIO->GPIODir); */

    len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n GPIO-LEDCtrl(0x%08lx): 0x%08lx  \n",(unsigned long)&GPIO->LEDCtrl,(unsigned long)GPIO->LEDCtrl);

    len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n SpiSlaveCfg(0x%08lx): 0x%08lx  \n", (unsigned long)&GPIO->SpiSlaveCfg,(unsigned long)GPIO->SpiSlaveCfg);

   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n GPIOMode(0x%08lx): 0x%08lx  \n", (unsigned long)&GPIO->GPIOMode,(unsigned long)GPIO->GPIOMode);


   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n GPIOCtrl(0x%08lx): 0x%08lx  \n", (unsigned long)&GPIO->GPIOCtrl,(unsigned long)GPIO->GPIOCtrl);

   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n TestControl: 0x%08lx  \n", (unsigned long)GPIO->TestControl);
   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n RoboSWLEDControl: 0x%08lx  \n", (unsigned long)GPIO->RoboSWLEDControl);
   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n RoboSWLEDLSR: 0x%08lx  \n", (unsigned long)GPIO->RoboSWLEDLSR);
   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n GPIOBaseMode(0x%08lx): 0x%08lx  \n",(unsigned long)&GPIO->GPIOBaseMode,(unsigned long)GPIO->GPIOBaseMode);
   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n RoboswEphyCtrl: 0x%08lx  \n", (unsigned long)GPIO->RoboswEphyCtrl);
   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n RoboswSwitchCtrl: 0x%08lx  \n", (unsigned long)GPIO->RoboswSwitchCtrl);
   len += snprintf((page + len), (PAGE_SIZE - len),
		     "\n RegFileTmCtl: 0x%08lx  ##\n", (unsigned long)GPIO->RegFileTmCtl);
   spin_unlock_irqrestore(&lockled, flags);
   return len;
 #if 0
  r += sprintf(page, "%s @0x%x-%d:\n\n ", "GPIO REGISTER DUMP",(unsigned int)GPIO,length);
   for (i = 0; i < length; i ++)
  {
      r += sprintf(page, "%02x ", ((unsigned char *)GPIO)[offset + i]);
   }
   r += sprintf(page, "%s @0x%x-%d:\n\n", "LED REGISTER DUMP ",(unsigned int)LED,length);
   for (i = 0; i < length; i ++)
  {
      r += sprintf(page, "%02x ", ((unsigned char *)LED)[offset + i]);
   }

   return (r?r:0);
#endif

}

#endif /* SKY */
EXPORT_SYMBOL(kerSysSetBootParm);


/***************************************************************************
* MACRO to call driver initialization and cleanup functions.
***************************************************************************/
module_init( brcm_board_init );
module_exit( brcm_board_cleanup );

EXPORT_SYMBOL(dumpaddr);
EXPORT_SYMBOL(kerSysGetChipId);
EXPORT_SYMBOL(kerSysGetChipName);
EXPORT_SYMBOL(kerSysMacAddressNotifyBind);
EXPORT_SYMBOL(kerSysGetMacAddressType);
EXPORT_SYMBOL(kerSysGetMacAddress);
EXPORT_SYMBOL(kerSysReleaseMacAddress);
EXPORT_SYMBOL(kerSysGetGponSerialNumber);
EXPORT_SYMBOL(kerSysGetGponPassword);
EXPORT_SYMBOL(kerSysGetSdramSize);
#if !defined(CONFIG_BCM968500)
EXPORT_SYMBOL(kerSysGetDslPhyEnable);
EXPORT_SYMBOL(kerSysGetExtIntInfo);
#else
EXPORT_SYMBOL(BcmHalMapInterrupt);
EXPORT_SYMBOL(enable_brcm_irq_irq);
EXPORT_SYMBOL(disable_brcm_irq_irq);
#endif
EXPORT_SYMBOL(kerSysSetOpticalPowerValues);
EXPORT_SYMBOL(kerSysGetOpticalPowerValues);
#if defined(CONFIG_BCM96368)
EXPORT_SYMBOL(kerSysGetSdramWidth);
#endif
EXPORT_SYMBOL(kerSysLedCtrl);
EXPORT_SYMBOL(kerSysRegisterDyingGaspHandler);
EXPORT_SYMBOL(kerSysDeregisterDyingGaspHandler);
EXPORT_SYMBOL(kerSysSendtoMonitorTask);
EXPORT_SYMBOL(kerSysGetWlanSromParams);
EXPORT_SYMBOL(kerSysGetAfeId);
EXPORT_SYMBOL(kerSysGetUbusFreq);
#if defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818) || defined(CONFIG_BCM963268)
EXPORT_SYMBOL(kerSysBcmSpiSlaveRead);
EXPORT_SYMBOL(kerSysBcmSpiSlaveReadReg32);
EXPORT_SYMBOL(kerSysBcmSpiSlaveWrite);
EXPORT_SYMBOL(kerSysBcmSpiSlaveWriteReg32);
EXPORT_SYMBOL(kerSysBcmSpiSlaveWriteBuf);
#if defined(CONFIG_BCM_6802_MoCA)
EXPORT_SYMBOL(kerSysBcmSpiSlaveReadBuf);
EXPORT_SYMBOL(kerSysBcmSpiSlaveModify);
#endif
EXPORT_SYMBOL(kerSysRegisterMocaHostIntrCallback);
EXPORT_SYMBOL(kerSysMocaHostIntrEnable);
EXPORT_SYMBOL(kerSysMocaHostIntrDisable);
#endif
EXPORT_SYMBOL(BpGetSimInterfaces);
EXPORT_SYMBOL(BpGetBoardId);
EXPORT_SYMBOL(BpGetBoardIds);
EXPORT_SYMBOL(BpGetGPIOverlays);
EXPORT_SYMBOL(BpGetFpgaResetGpio);
EXPORT_SYMBOL(BpGetEthernetMacInfo);
EXPORT_SYMBOL(BpGetDeviceOptions);
EXPORT_SYMBOL(BpGetPortConnectedToExtSwitch);
EXPORT_SYMBOL(BpGetRj11InnerOuterPairGpios);
EXPORT_SYMBOL(BpGetRtsCtsUartGpios);
EXPORT_SYMBOL(BpGetAdslLedGpio);
EXPORT_SYMBOL(BpGetAdslFailLedGpio);
EXPORT_SYMBOL(BpGetWanDataLedGpio);
EXPORT_SYMBOL(BpGetWanErrorLedGpio);
EXPORT_SYMBOL(BpGetVoipLedGpio);
EXPORT_SYMBOL(BpGetPotsLedGpio);
EXPORT_SYMBOL(BpGetVoip2FailLedGpio);
EXPORT_SYMBOL(BpGetVoip2LedGpio);
EXPORT_SYMBOL(BpGetVoip1FailLedGpio);
EXPORT_SYMBOL(BpGetVoip1LedGpio);
EXPORT_SYMBOL(BpGetDectLedGpio);
EXPORT_SYMBOL(BpGetMoCALedGpio);
EXPORT_SYMBOL(BpGetMoCAFailLedGpio);
EXPORT_SYMBOL(BpGetWirelessSesExtIntr);
EXPORT_SYMBOL(BpGetWirelessSesLedGpio);
EXPORT_SYMBOL(BpGetWirelessFlags);
EXPORT_SYMBOL(BpGetWirelessPowerDownGpio);
EXPORT_SYMBOL(BpUpdateWirelessSromMap);
EXPORT_SYMBOL(BpGetSecAdslLedGpio);
EXPORT_SYMBOL(BpGetSecAdslFailLedGpio);
EXPORT_SYMBOL(BpGetDslPhyAfeIds);
EXPORT_SYMBOL(BpGetExtAFEResetGpio);
EXPORT_SYMBOL(BpGetExtAFELDPwrGpio);
EXPORT_SYMBOL(BpGetExtAFELDModeGpio);
EXPORT_SYMBOL(BpGetIntAFELDPwrGpio);
EXPORT_SYMBOL(BpGetIntAFELDModeGpio);
EXPORT_SYMBOL(BpGetAFELDRelayGpio);
EXPORT_SYMBOL(BpGetExtAFELDDataGpio);
EXPORT_SYMBOL(BpGetExtAFELDClkGpio);
EXPORT_SYMBOL(BpGetUart2SdoutGpio);
EXPORT_SYMBOL(BpGetUart2SdinGpio);
EXPORT_SYMBOL(BpGet6829PortInfo);
EXPORT_SYMBOL(BpGetEthSpdLedGpio);
EXPORT_SYMBOL(BpGetLaserDisGpio);
EXPORT_SYMBOL(BpGetLaserTxPwrEnGpio);
EXPORT_SYMBOL(BpGetVregSel1P2);
EXPORT_SYMBOL(BpGetVregAvsMin);
EXPORT_SYMBOL(BpGetGponOpticsType);
EXPORT_SYMBOL(BpGetDefaultOpticalParams);
EXPORT_SYMBOL(BpGetI2cGpios);
EXPORT_SYMBOL(BpGetMiiOverGpioFlag);
#if defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
EXPORT_SYMBOL(restartHWWatchDog);
#endif //defined(SKY) && defined(CONFIG_SKY_FEATURE_WATCHDOG)
EXPORT_SYMBOL(BpGetSwitchPortMap);
EXPORT_SYMBOL(BpGetMocaInfo);
EXPORT_SYMBOL(BpGetPhyResetGpio);
EXPORT_SYMBOL(BpGetPhyAddr);
#if defined (CONFIG_BCM_ENDPOINT_MODULE)
EXPORT_SYMBOL(BpGetVoiceBoardId);
EXPORT_SYMBOL(BpGetVoiceBoardIds);
EXPORT_SYMBOL(BpGetVoiceParms);
#endif

#if defined(CONFIG_EPON_SDK)
EXPORT_SYMBOL(BpGetNumFePorts);
EXPORT_SYMBOL(BpGetNumGePorts);
EXPORT_SYMBOL(BpGetNumVoipPorts);
#endif
#if defined(CONFIG_BCM96838)
EXPORT_SYMBOL(BpGetPonTxEnGpio);
EXPORT_SYMBOL(BpGetPonRxEnGpio);
#endif

EXPORT_SYMBOL(BpGetOpticalWan);
EXPORT_SYMBOL(BpGetRogueOnuEn);
EXPORT_SYMBOL(BpGetGpioLedSim);
EXPORT_SYMBOL(BpGetGpioLedSimITMS);

MODULE_LICENSE("GPL");
