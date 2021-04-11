/*
<:proprietary-BRCM::DUAL/GPL:standard 
:> 
*/

//****************************************************************************
//  Description:
//      This contains the 6838 PMC driver.
//****************************************************************************

#ifndef PMC_DRV_H
#define PMC_DRV_H

#include <bcmtypes.h>

#define BPCM_SRESET_CNTL_REG_BYTE_OFFSET        0x20
#define BPCM_SRESET_CNTL_REG_WORD_OFFSET        8

//--------- BPCM device addressing and structure of zones in the BPCM devices ------------------------
#define   PMC_ADDR_APM                  0       // apm_fpm_top-apm_top - No FS CTL registers
    enum {
        APM_Zone_Main,
        APM_Zone_Audio,
        APM_Zone_PCM,
        APM_Zone_HVG,
        APM_Zone_BMU,
    };
    
//--------- SOFT Reset bits for APM ------------------------
#define   BPCM_APM_SRESET_HARDRST_N   0x00000040	    
#define   BPCM_APM_SRESET_AUDIO_N     0x00000020	
#define   BPCM_APM_SRESET_PCM_N       0x00000010
#define   BPCM_APM_SRESET_HVGA_N      0x00000008
#define   BPCM_APM_SRESET_HVGB_N      0x00000004
#define   BPCM_APM_SRESET_BMU_N       0x00000002
#define   BPCM_APM_SRESET_200_N       0x00000001

    
#define   PMB_ADDR_FPM                  1      // apm_fpm_top-fpm_top
    enum {
        FPM_Zone_Main,
    };
	
#define   PMB_ADDR_CHIP_CLKRST          2      // chip_clkrst
#define CLKRST_APM_200_SEL_REG_BYTE_OFFSET      0x48
#define CLKRST_APM_200_SEL_REG_WORD_OFFSET      0x12

#define   PMB_ADDR_CPU4355_BCM_MIPS0    3      // cpu4355_bcm_mips0 - No FS CTL register
    enum {
        Viper_Zone_Main,
    };
	
#define   PMB_ADDR_WAN            		4		// wan_top_wrapper
    enum {
        WAN_Zone_0,
        WAN_Zone_1,
        WAN_Zone_2,
        WAN_Zone_3,
    };
	
#define   PMB_ADDR_RDP		            5      // rdp_top_wrapper
    enum {
        RDP_Zone_0,
        RDP_Zone_1,
   };
   
#define   PMB_ADDR_MEMC       14     // memc23_sj_300_16b_fc_40g
    enum {
        MEMC_Zone_Main,
    };
	
#define   PMB_ADDR_PERIPH               16     // periph_top -  no FS CTL registers
    enum {
        PERIPH_Zone_Main,
        PERIPH_Zone_HS_SPIM,
        PERIPH_Zone_TBUS,
    };
	
#define   PMB_ADDR_UGB                  17     // ubus_gisb_bridge_top - FS CTL register
    enum {
        UGB_Zone_0,
    };
	
#define   PMB_ADDR_SYSPLL0              23     // clkrst_testif-pmb_syspll0

#define   PMB_ADDR_SYSPLL1              24     // clkrst_testif-pmb_syspll1

#define   PMB_ADDR_SYSPLL2              25     // clkrst_testif-pmb_syspll2

#define   PMB_ADDR_LCPLL0				26     // clkrst_testif-pmb_syspll3

#define   PMB_ADDR_LCPLL1				27     // clkrst_testif-pmb_syspll4

#define   PMB_ADDR_UNIMAC_MBDMA         30     // unimac_mbdma_top - FS CTL register in all zones
    enum {
        UNIMAC_MBDMA_Zone_Top,
        UNIMAC_MBDMA_Zone_Unimac0,
        UNIMAC_MBDMA_Zone_Unimac1,
    };
	
#define   PMB_ADDR_PCIE_UBUS_0          33     // pcie_2x_top-pcie_ubus_top_0 - FS CTL register
    enum {
        PCIE_UBUS_0_Zone_0,
    };
	
#define   PMB_ADDR_PCIE_UBUS_1          34     // pcie_2x_top-pcie_ubus_top_1 - FS CTL register
    enum {
        PCIE_UBUS_1_Zone_0,
    };
	
#define   PMB_ADDR_USB30_2X             35     // usb_top - no FS CTL registers
    enum {
        USB30_2X_Zone_Common,
        USB30_2X_Zone_USB_Host,
        USB30_2X_Zone_USB_Device,
    };
	
#define   PMB_ADDR_PSAB                 38     // ubus2_pipestage_A_B_top
    enum {
        PSAB_Zone_0,
    };
	
#define   PMB_ADDR_PSBC                 39     // ubus2_pipestage_B_C_top
    enum {
        PSBC_Zone_0,
    };
	
#define   PMB_ADDR_EGPHY                40     // egphy_wrapper
    enum {
        EGPHY_Zone_0,
    };

// ---------------------------- Returned error codes --------------------------
enum {
    // 0..15 may come from either the interface or from the PMC command handler
    // 256 or greater only come from the interface
    kPMC_NO_ERROR,
    kPMC_INVALID_ISLAND,
    kPMC_INVALID_DEVICE,
    kPMC_INVALID_ZONE,
    kPMC_INVALID_STATE,
    kPMC_INVALID_COMMAND,
    kPMC_LOG_EMPTY,
    kPMC_INVALID_PARAM,
    kPMC_BPCM_READ_TIMEOUT,
    kPMC_INVALID_BUS,
    kPMC_INVALID_QUEUE_NUMBER,
    kPMC_QUEUE_NOT_AVAILABLE,
    kPMC_INVALID_TOKEN_SIZE,
    kPMC_INVALID_WATERMARKS,
    kPMC_INSUFFICIENT_QSM_MEMORY,
	kPMC_INVALID_BOOT_COMMAND,
    kPMC_COMMAND_TIMEOUT = 256,
	kPMC_MESSAGE_ID_MISMATCH,
};

// ---------------------------- Returned log entry structure --------------------------
typedef struct {
    uint8 reserved;
    uint8 logMsgID;
    uint8 errorCode;
    uint8 logCmdID;
    uint8 srcPort;
    uint8 e_msgID;
    uint8 e_errorCode;
    uint8 e_cmdID;
    struct {
        uint32 logReplyNum : 8;
        uint32 e_Island    : 4;
        uint32 e_Bus       : 2;
        uint32 e_DevAddr   : 8;
        uint32 e_Zone      : 10;
    } s;
    uint32  e_Data0;
} TErrorLogEntry;

// ---------------------------- Power states --------------------------
enum {
    kPMCPowerState_Unknown,
    kPMCPowerState_NoPower,
    kPMCPowerState_LowPower,
    kPMCPowerState_FullPower,
};

// PMC run-state:
enum {
	kPMCRunStateExecutingBootROM = 0,
	kPMCRunStateWaitingBMUComplete,
	kPMCRunStateAVSCompleteWaitingForImage,
	kPMCRunStateAuthenticatingImage,
	kPMCRunStateAuthenticationFailed,
	kPMCRunStateReserved,
	kPMCRunStateFatalError,
	kPMCRunStateRunning
};

// the only valid "gear" values for "SetClockGear" function
enum {
    kClockGearLow,
    kClockGearHigh,
    kClockGearDynamic,
    kClockGearBypass
};

int GetDevPresence(int devAddr, int *value);
int GetSWStrap(int devAddr, int *value);
int GetHWRev(int devAddr, int *value);
int GetNumZones(int devAddr, int *value);
int GetErrorLogEntry(TErrorLogEntry *logEntry);
int SetClockHighGear(int devAddr, int zone, int clkN);
int SetClockLowGear(int devAddr, int zone, int clkN);
int SetClockGear(int devAddr, int zone, int gear);
int ReadBPCMRegister(int devAddr, int wordOffset, uint32 *value);
int ReadZoneRegister(int devAddr, int zone, int wordOffset, uint32 *value);
int WriteBPCMRegister(int devAddr, int wordOffset, uint32 value);
int WriteZoneRegister(int devAddr, int zone, int wordOffset, uint32 value);
int SetRunState(int island, int state);
int SetPowerState(int island, int state);
int GetDieTemperature(void);
int GetVoltage(void);
int PowerOnDevice(int devAddr);
int PowerOffDevice(int devAddr, int repower);
int PowerOnZone(int devAddr, int zone);
int PowerOffZone(int devAddr, int zone);
int ResetDevice(int devAddr);
int ResetZone(int devAddr, int zone);
int Ping(void);
void WaitPmc(int runState);
void BootPmcNoRom(unsigned long physAddr);

#endif // PMC_DRV_H

