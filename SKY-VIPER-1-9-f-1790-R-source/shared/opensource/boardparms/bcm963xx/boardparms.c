/*
    Copyright 2000-2010 Broadcom Corporation

    <:label-BRCM:2011:DUAL/GPL:standard
    
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

/**************************************************************************
* File Name  : boardparms.c
*
* Description: This file contains the implementation for the BCM63xx board
*              parameter access functions.
*
* Updates    : 07/14/2003  Created.
***************************************************************************/

/* Includes. */
#include "boardparms.h"
#include "bcmSpiRes.h"
#if !defined(_CFE_)
#include "6802_map_part.h"
#endif

/* Typedefs */

/*
You can add a new enum entry anywhere in the enum list below.
Then, you can use the enums in the board parm arrays, with the following restrictions:
-An API must be created for accessing any data from the array except for bp_elemTemplate.
-bp_cpBoardId  must be the first element of the array.
-bp_elemTemplate if used should be the last or second to last (just before bp_last) element
 of the array
-Most enums that are used only once in the array can be placed anywhere after bp_cpBoardId.
-These enums are read with the BpGetUc/BpGetUs/BpGetUl private functions.
-If there is a board parm array that is very similar to one you are adding, then you
 can use the bp_elemTemplate to point to that the board parm array BUT the restriction
 below still applies - that is do not split up the "groups"
 (see g_bcm96362advngr and g_bcm96362advngr2 or g_bcm963168vx and g_bcm963168vx_p300)
-Certain enums can appear multiple times in the board parm arrays
-These enums have special APIs that understand how to loop through each occurence
 They are:
 *packet switch related parameters (from bp_ucPhyAddress to bp_ulPhyId7) which can
  appear once per packet switch (bp_ucPhyType0 and bp_ucPhyType1)
 *led info related parameters (bp_usDuplexLed, bp_usSpeedLed100 and bp_usSpeedLed1000) which can
  appear once per internal led (bp_ulPhyId0 and bp_ulPhyId1)
 *add a new id bp_usPhyConnType to specify the phy connection type as we are running out phy id flag
 *bits. use this id initially for PLC MAC connection type, but will use it for MOCA, EPON, GPON in the
 *futures and remove the phy id connecton type flags.
 *voip dsp related parameters (from bp_ucDspAddress to bp_usGpioDectLed) which can
  appear once per dsp (bp_ucDspType0 and bp_ucDspType1)
*/

#define bp_usGpioOverlay     bp_ulGpioOverlay

enum bp_id {
  bp_cpBoardId,
  bp_cpComment,
  bp_ulGpioOverlay,
  bp_usGpioLedAdsl,
  bp_usGpioLedAdslFail,
  bp_usGpioSecLedAdsl,
  bp_usGpioSecLedAdslFail,
  bp_usGpioLedSesWireless,
#ifdef SKY
  bp_usGpioLedSesWirelessFail,    /* GPIO pin or not defined */
  bp_usGpioLedWireless,  // This is for wireless white LED
  bp_usGpioLedWirelessFail,    /* GPIO pin or not defined */
  bp_usGpioLedskyHD, /* This is newly added id for sky+hd led */
  bp_usGpioLedskyHDFail,
#endif /* SKY */
  bp_usGpioLedWanData,
  bp_usGpioLedWanError,
  bp_usGpioLedBlPowerOn,
  bp_usGpioLedBlStop,
  bp_usGpioFpgaReset,
  bp_usGpioLedGpon,
  bp_usGpioLedGponFail,
  bp_usGpioLedMoCA,
  bp_usGpioLedMoCAFail,
  bp_usGpioLedEpon,
  bp_usGpioLedEponFail,
  bp_usExtIntrResetToDefault,
  bp_usExtIntrSesBtnWireless,
  bp_usAntInUseWireless,
  bp_usWirelessFlags,
  bp_usGpioWirelessPowerDown,
  bp_ucPhyType0,
  bp_ucPhyType1,
  bp_ucPhyAddress,
  bp_usConfigType,
  bp_ulPortMap,
  bp_ulPhyId0,
  bp_ulPhyId1,
  bp_ulPhyId2,
  bp_ulPhyId3,
  bp_ulPhyId4,
  bp_ulPhyId5,
  bp_ulPhyId6,
  bp_ulPhyId7,
  bp_usDuplexLed,
  bp_usSpeedLed100,
  bp_usSpeedLed1000,
  bp_usPhyConnType,
  bp_usGpioPhyReset,    // used if each phy has a different gpio pin for reset
  bp_ucPhyDevName,
  bp_ucDspType0,
  bp_ucDspType1,
  bp_ucDspAddress,
  bp_usGpioLedVoip,
  bp_usGpioVoip1Led,
  bp_usGpioVoip1LedFail,
  bp_usGpioVoip2Led,
  bp_usGpioVoip2LedFail,
  bp_usGpioPotsLed,
  bp_usGpioDectLed,
  bp_usGpioPassDyingGasp,
  bp_ulAfeId0,
  bp_ulAfeId1,
  bp_usGpioExtAFEReset,
  bp_usGpioExtAFELDPwr,
  bp_usGpioExtAFELDMode,
  bp_usGpioIntAFELDPwr,
  bp_usGpioIntAFELDMode,
  bp_usGpioAFELDRelay,
  bp_usGpioUart2Sdin,
  bp_usGpioUart2Sdout,
  bp_usGpioLaserDis,
  bp_usGpioLaserTxPwrEn,
  bp_usGpioLaserRxPwrEn,
  bp_usGpioEponOpticalSD,
  bp_usVregSel1P2,
  bp_ucVreg1P8,
  bp_usVregAvsMin,
  bp_usGponOpticsType,
  bp_usGpioFemtoReset,
  bp_cpDefaultOpticalParams,
  bp_usEphyBaseAddress,
  bp_usGpioSpiSlaveReset,  
  bp_usSpiSlaveBusNum,  
  bp_usSpiSlaveSelectNum,
  bp_usSpiSlaveMode,
  bp_ulSpiSlaveCtrlState,
  bp_ulSpiSlaveMaxFreq,
  bp_usSpiSlaveProtoRev,
  bp_usGpioExtAFELDData,
  bp_usGpioExtAFELDClk,
  bp_ulNumFePorts,
  bp_ulNumGePorts,
  bp_ulNumVoipPorts,
  bp_usGpioI2cScl,
  bp_usGpioI2cSda,
  bp_elemTemplate,
  bp_usSerialLEDMuxSel,
  bp_ulDeviceOptions,
  bp_pPhyInit,
  bp_usGpioLaserReset,  
  bp_usGpio_Intr,
  bp_usGphyBaseAddress,
  bp_cpPersonalityName,
  bp_usExtIntrMocaHostIntr,
  bp_usExtIntrMocaSBIntr0,
  bp_usExtIntrMocaSBIntr1,
  bp_usExtIntrMocaSBIntr2,
  bp_usExtIntrMocaSBIntr3,
  bp_usExtIntrMocaSBIntr4,
  bp_usExtIntrMocaSBIntrAll,
  bp_usMocaType0,
  bp_usMocaType1,
  bp_pMocaInit,
  bp_ulDyingGaspIntrPin,
  bp_usGpioPLCPwrEn,
  bp_usGpioPLCReset,
  bp_usExtIntrPLCStandBy,
  bp_ulPortFlags,
  bp_usMocaRfBand,
  bp_usGpioLedOpticalLinkFail,
  bp_usGpioLedLan,
  bp_usGpioLedWL2_4GHz,
  bp_usGpioLedWL5GHz, 
  bp_usGpioLedUSB,
  bp_usGpioBoardReset, // used to reset the entire board
  bp_usGpioUsb0,  	   // used for USB 0	
  bp_usGpioUsb1,       // used for USB 0
  bp_ulOpticalWan,
  bp_ulSimInterfaces,
  bp_ulSlicInterfaces,
  bp_usGpioPonTxEn,
  bp_usGpioPonRxEn,
  bp_usRogueOnuEn,
  bp_usGpioLedSim,
  bp_usGpioLedSim_ITMS,
  bp_usPinMux,
  bp_us1ppsPin,
  bp_last
};

/* Include all the led id in this list. The cfe/setup initilization code
 * use this list to perform any necessary setup such pinmux selection on these
 * LED gpio pins
 */
static enum bp_id bpLedList[] = {
  bp_usGpioLedAdsl,
  bp_usGpioLedAdslFail,
  bp_usGpioSecLedAdsl,
  bp_usGpioSecLedAdslFail,
  bp_usGpioLedSesWireless,
  bp_usGpioLedWanData,
  bp_usGpioLedWanError,
  bp_usGpioLedBlPowerOn,
  bp_usGpioLedBlStop,
  bp_usGpioFpgaReset,
  bp_usGpioLedGpon,
  bp_usGpioLedGponFail,
  bp_usGpioLedMoCA,
  bp_usGpioLedMoCAFail,
  bp_usGpioLedEpon,
  bp_usGpioLedEponFail,
  bp_usDuplexLed,
  bp_usSpeedLed100,
  bp_usSpeedLed1000,
  bp_usGpioLedVoip,
  bp_usGpioVoip1Led,
  bp_usGpioVoip1LedFail,
  bp_usGpioVoip2Led,
  bp_usGpioVoip2LedFail,
  bp_usGpioPotsLed,
  bp_usGpioDectLed,
  bp_usGpioLedOpticalLinkFail,
  bp_usGpioLedLan,
  bp_usGpioLedWL2_4GHz,
  bp_usGpioLedWL5GHz, 
  bp_usGpioLedUSB,
  bp_usGpioLedSim,
  bp_usGpioLedSim_ITMS,
#ifdef SKY  
  bp_usGpioLedSesWirelessFail,    /* GPIO pin or not defined */
  bp_usGpioLedWireless,  // This is for wireless white LED
  bp_usGpioLedWirelessFail,    /* GPIO pin or not defined */
  bp_usGpioLedskyHD, /* This is newly added id for sky+hd led */
  bp_usGpioLedskyHDFail,
#endif //SKY  
};

/* Include all the gpio id in this list. The cfe/setup initilization code
 * use this list to perform any necessary setup such pinmux selection on these
 * gpio pins
 */
static enum bp_id bpGpioList[] = {
  bp_usGpioWirelessPowerDown,
  bp_usGpioPassDyingGasp,
  bp_usGpioExtAFEReset,
  bp_usGpioExtAFELDPwr,
  bp_usGpioExtAFELDMode,
  bp_usGpioIntAFELDPwr,
  bp_usGpioIntAFELDMode,
  bp_usGpioAFELDRelay,
  bp_usGpioUart2Sdin,
  bp_usGpioUart2Sdout,
  bp_usGpioLaserDis,
  bp_usGpioLaserTxPwrEn,
  bp_usGpioSpiSlaveReset,
  bp_usGpioI2cScl,
  bp_usGpioI2cSda,
  bp_usGpioLaserReset,
  bp_usGpio_Intr,
  bp_usGpioEponOpticalSD,
  bp_usGpioPLCPwrEn,
  bp_usGpioPLCReset,
  bp_usGpioPhyReset,
  bp_usGpioBoardReset,
  bp_usGpioUsb0,  
  bp_usGpioUsb1, 
  bp_usGpioPonTxEn,
  bp_usGpioPonRxEn,
};

/* Include all the gpio and led id in this list for EPON MAC controlled
 * gpio pins on 6828 device.
 */
static enum bp_id bpEponGpioList[] = {
  bp_usGpioLedEpon,
  bp_usGpioLedEponFail,
  bp_usGpioLedWanData,
  bp_usGpioI2cScl,
  bp_usGpioI2cSda,
  bp_usGpioEponOpticalSD
};

typedef struct bp_elem {
  enum bp_id id;
  union {
    char * cp;
    unsigned char * ucp;
    unsigned char uc;
    unsigned short us;
    unsigned long ul;
    void * ptr;
    struct bp_elem * bp_elemp;
  } u;
} bp_elem_t;

/* Variables */
#if 0
/* Sample structure with all elements present in a valid order */
/* Indentation is used to illustrate the groupings where parameters can be repeated */
/* use bp_elemTemplate to use another structure but do not split the groups */   
static bp_elem_t g_sample[] = {
  {bp_cpBoardId,               .u.cp = "SAMPLE"},
  {bp_ulGpioOverlay,           .u.ul = 0;
  {bp_usGpioLedAdsl,           .u.us = 0},
  {bp_usGpioLedAdslFail,       .u.us = 0},
  {bp_usGpioSecLedAdsl,        .u.us = 0},
  {bp_usGpioSecLedAdslFail,    .u.us = 0},
  {bp_usGpioLedSesWireless,    .u.us = 0},
  {bp_usGpioLedWanData,        .u.us = 0},
  {bp_usGpioLedWanError,       .u.us = 0},
  {bp_usGpioLedBlPowerOn,      .u.us = 0},
  {bp_usGpioLedBlStop,         .u.us = 0},
  {bp_usGpioFpgaReset,         .u.us = 0},
  {bp_usGpioLedGpon,           .u.us = 0},
  {bp_usGpioLedGponFail,       .u.us = 0},
  {bp_usGpioLedMoCA,           .u.us = 0},
  {bp_usGpioLedMoCAFail,       .u.us = 0},
  {bp_usGpioLedEpon,           .u.us = 0},
  {bp_usGpioLedEponFail,       .u.us = 0},
  {bp_usExtIntrResetToDefault, .u.us = 0},
  {bp_usExtIntrSesBtnWireless, .u.us = 0},
  {bp_usMocaType0,             .u.us = 0}, // first MoCA chip always for WAN
    {bp_usExtIntrMocaHostIntr, .u.us = 0},
    {bp_usGpio_Intr,           .u.us = 0},
    {bp_usExtIntrMocaSBIntr0,  .u.us = 0},
    {bp_usGpio_Intr,           .u.us = 0},
    {bp_usExtIntrMocaSBIntr1,  .u.us = 0},
    {bp_usGpio_Intr,           .u.us = 0},
    {bp_pMocaInit,             .u.ptr = (void*)mocaCmdSeq;
  {bp_usMocaType1,             .u.us = 0}, // MoCA LAN
    {bp_usExtIntrMocaHostIntr, .u.us = 0},
    {bp_usGpio_Intr,           .u.us = 0},
    {bp_usExtIntrMocaSBIntr0,  .u.us = 0},
    {bp_usGpio_Intr,           .u.us = 0},
    {bp_usExtIntrMocaSBIntr1,  .u.us = 0},
    {bp_usGpio_Intr,           .u.us = 0},
    {bp_pMocaInit,             .u.ptr = (void*)mocaCmdSeq;
  {bp_usAntInUseWireless,      .u.us = 0},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_usGpioWirelessPowerDown, .u.us = 0},
  {bp_ucPhyType0,              .u.uc = 0}, // First switch
    {bp_ucPhyAddress,          .u.uc = 0},
    {bp_usConfigType,          .u.us = 0},
    {bp_ulPortMap,             .u.ul = 0},
    {bp_ulPhyId0,              .u.ul = 0},
      {bp_usDuplexLed,         .u.us = 0},
      {bp_usSpeedLed100,       .u.us = 0},
      {bp_usSpeedLed1000,      .u.us = 0},
      {bp_pPhyInit             .u.ptr = (void *)g_phyinit},
    {bp_ulPhyId1,              .u.ul = 0},
      {bp_usDuplexLed,         .u.us = 0},
      {bp_usSpeedLed100,       .u.us = 0},
      {bp_usSpeedLed1000,      .u.us = 0},
      {bp_pPhyInit             .u.ptr = (void *)g_phyinit},
    {bp_ulPhyId2,              .u.ul = 0},
    {bp_ulPhyId3,              .u.ul = 0},
    {bp_ulPhyId4,              .u.ul = 0},
    {bp_ulPhyId5,              .u.ul = 0},
      {bp_usPhyConnType        .u.us = 0},
      {bp_pPhyInit             .u.ptr = (void *)g_phyinit},
    {bp_ulPhyId6,              .u.ul = 0},
    {bp_ulPhyId7,              .u.ul = 0},
  {bp_ucPhyType1,              .u.uc = 0}, // Second switch
    {bp_ucPhyAddress,          .u.uc = 0},
    {bp_usConfigType,          .u.us = 0},
    {bp_ulPortMap,             .u.ul = 0},
    {bp_ulPhyId0,              .u.ul = 0},
      {bp_usDuplexLed,         .u.us = 0},
      {bp_usSpeedLed100,       .u.us = 0},
      {bp_usSpeedLed1000,      .u.us = 0},
    {bp_ulPhyId1,              .u.ul = 0},
      {bp_usDuplexLed,         .u.us = 0},
      {bp_usSpeedLed100,       .u.us = 0},
      {bp_usSpeedLed1000,      .u.us = 0},
    {bp_ulPhyId2,              .u.ul = 0},
    {bp_ulPhyId3,              .u.ul = 0},
    {bp_ulPhyId4,              .u.ul = 0},
    {bp_ulPhyId5,              .u.ul = 0},
    {bp_ulPhyId6,              .u.ul = 0},
    {bp_ulPhyId7,              .u.ul = 0},
  {bp_ucDspType0,              .u.uc = 0}, // First VOIP DSP
    {bp_ucDspAddress,          .u.uc = 0},
    {bp_usGpioLedVoip,         .u.us = 0},
    {bp_usGpioVoip1Led,        .u.us = 0},
    {bp_usGpioVoip1LedFail,    .u.us = 0},
    {bp_usGpioVoip2Led,        .u.us = 0},
    {bp_usGpioVoip2LedFail,    .u.us = 0},
    {bp_usGpioPotsLed,         .u.us = 0},
    {bp_usGpioDectLed,         .u.us = 0},
  {bp_ucDspType1,              .u.uc = 0}, // Second VOIP DSP
    {bp_ucDspAddress,          .u.uc = 0},
    {bp_usGpioLedVoip,         .u.us = 0},
    {bp_usGpioVoip1Led,        .u.us = 0},
    {bp_usGpioVoip1LedFail,    .u.us = 0},
    {bp_usGpioVoip2Led,        .u.us = 0},
    {bp_usGpioVoip2LedFail,    .u.us = 0},
    {bp_usGpioPotsLed,         .u.us = 0},
    {bp_usGpioDectLed,         .u.us = 0},
  {bp_ulAfeId0,                .u.ul = 0},
  {bp_ulAfeId1,                .u.ul = 0},
  {bp_usGpioExtAFEReset,       .u.us = 0},
  {bp_usGpioExtAFELDPwr,       .u.us = 0},
  {bp_usGpioExtAFELDMode,      .u.us = 0},
  {bp_usGpioLaserDis,          .u.us = 0},
  {bp_usGpioLaserTxPwrEn,      .u.us = 0},
  {bp_usGpioEponOpticalSD,     .u.us = 0},
  {bp_usVregSel1P2,            .u.us = 0},
  {bp_ucVreg1P8                .u.uc = 0},
  {bp_usVregAvsMin             .u.us = 0},
  {bp_usGpioFemtoReset,        .u.us = 0},
  {bp_usSerialLEDMuxSel        .u.us = 0},
  {bp_elemTemplate             .u.bp_elemp = g_sample2},
  {bp_last}
};

#endif

#if defined(_BCM968500_) || defined(CONFIG_BCM968500)
static bp_elem_t g_bcm968500cherry[] = {
  {bp_cpBoardId,               .u.cp = "968500CHERRY"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_usGpioBoardReset,        .u.us = BP_GPIO_47_AH},
  {bp_usGpioUsb0,              .u.us = ((BP_GPIO_3_AH << 8) | BP_GPIO_2_AH)},
  {bp_usGpioUsb1, 			   .u.us = ((BP_GPIO_1_AH << 8) | BP_GPIO_0_AH)},	 
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x07 | MAC_IF_QSGMII }, // Interface modes of ports are NOT INDEPENDENT
  {bp_usGpioLedLan,            .u.us = BP_GPIO_48_AH},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_37_AH}, 
  {bp_ulPortFlags,             .u.ul = PORT_FLAG_MGMT }, // Managment port
  {bp_ulPhyId1,                .u.ul = 0x08 | MAC_IF_QSGMII }, // the combination must select a supported
  {bp_usGpioLedLan,            .u.us = BP_GPIO_5_AH},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_37_AH}, 
  {bp_ulPhyId2,                .u.ul = 0x09 | MAC_IF_QSGMII }, // configuration
  {bp_usGpioLedLan,            .u.us = BP_GPIO_39_AH},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_37_AH}, 
  {bp_ulPhyId3,                .u.ul = 0x0a | MAC_IF_QSGMII },
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_37_AH}, 
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioLedLan,            .u.us = BP_GPIO_32_AH},
  {bp_ulDyingGaspIntrPin,      .u.ul = BP_GPIO_NONE},
  /* There are not enough LEDs on Cherry board as required by E8C so some LEDs are mapped to the same GPIO */
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_38_AH},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_38_AH},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_36_AH},
  {bp_usGpioVoip2Led,          .u.us = BP_GPIO_48_AH},  
  {bp_usGpioLedOpticalLinkFail, .u.us = BP_GPIO_38_AH},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_36_AH}, /* SesWireless is the old name of WPS*/ 
  {bp_usGpioLedUSB,            .u.us = BP_GPIO_48_AH},  
  {bp_last}
};

/* Special Dying GASP demo/test cherry board - inherits from cherry, but GPIO 36 is connected to a button that sends
 * dying GASP */
static bp_elem_t g_bcm968500dg_test[] = {
  {bp_cpBoardId,               .u.cp = "968500DG_TEST"},
  /* bp_ulDyingGaspIntrPin and bp_usGpioVoip1Led, bp_usGpioLedSesWireless are mutually exclusive - they are all
   * connected to GPIO 36. */
  {bp_ulDyingGaspIntrPin,      .u.ul = BP_GPIO_36_AH},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_NONE},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_NONE},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968500cherry},
  {bp_last}
};

static bp_elem_t g_bcm968751Lupine2[] = {
  {bp_cpBoardId,               .u.cp = "968751LUPINE2"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_usGpioBoardReset,        .u.us = BP_GPIO_47_AH},
  {bp_usGpioUsb0,              .u.us = ((BP_GPIO_3_AH << 8) | BP_GPIO_2_AH)},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_NO_PHY},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ucPhyAddress,            .u.uc = 0x1e},
  {bp_ulPortMap,               .u.ul = 0x10},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_39_AH}, 
  {bp_ulPortFlags,             .u.ul = PORT_FLAG_MGMT }, // Managment port is on switch
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MDIO},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_39_AH}, 
  {bp_ulPhyId1,                .u.ul = 0x01},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_39_AH}, 
  {bp_ulPhyId2,                .u.ul = 0x02},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_39_AH}, 
  {bp_ulPhyId3,                .u.ul = 0x03},
  {bp_usGpioPhyReset,		   .u.us = BP_GPIO_39_AH},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0}, 
  {bp_ulDyingGaspIntrPin,	   .u.ul = BP_GPIO_NONE},
  /* There are not enough LEDs on Cherry board as required by E8C so some LEDs are mapped to the same GPIO */
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_38_AH},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_38_AH},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_36_AH},
  {bp_usGpioVoip2Led,          .u.us = BP_GPIO_48_AH},  
  {bp_usGpioLedOpticalLinkFail, .u.us = BP_GPIO_38_AH},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_36_AH}, /* SesWireless is the old name of WPS*/ 
  {bp_usGpioLedUSB,            .u.us = BP_GPIO_48_AH},  
  {bp_last}
};

static bp_elem_t g_bcm968500cedar[] = {
  {bp_cpBoardId,               .u.cp = "968500CEDAR"},
  {bp_usGpioBoardReset,        .u.us = BP_GPIO_47_AH},
  {bp_usGpioUsb0,              .u.us = ((BP_GPIO_3_AH << 8) | BP_GPIO_2_AH)},
  {bp_usGpioUsb1,              .u.us = ((BP_GPIO_1_AH << 8) | BP_GPIO_0_AH)},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x07 | MAC_IF_QSGMII },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_37_AH},
  {bp_ulPhyId1,                .u.ul = 0x08 | MAC_IF_QSGMII },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_37_AH},
  {bp_ulPhyId2,                .u.ul = 0x09 | MAC_IF_QSGMII },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_37_AH},
  {bp_ulPhyId3,                .u.ul = 0x0a | MAC_IF_QSGMII },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_37_AH},
  {bp_ulPhyId4,                .u.ul = 0x06 | MAC_IF_RGMII  }, /* WAN */
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_39_AH},
  {bp_ulDyingGaspIntrPin,      .u.ul = BP_GPIO_NONE},
  {bp_last}
};

static bp_elem_t g_bcm968500tulip_wo0[] = {
  {bp_cpBoardId,               .u.cp = "968500TULIP_WO0"},
  {bp_usGpioBoardReset,        .u.us = BP_GPIO_47_AH},
  {bp_usGpioUsb0,              .u.us = ((BP_GPIO_3_AH << 8) | BP_GPIO_2_AH)},
  {bp_usGpioUsb1,              .u.us = ((BP_GPIO_1_AH << 8) | BP_GPIO_0_AH)},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x11},
  {bp_ulPhyId0,                .u.ul = 0x10 | MAC_IF_SGMII },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_48_AH},
  {bp_ulPhyId4,                .u.ul = 0x04 | MAC_IF_RGMII  },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_48_AH},
  {bp_ulPortFlags,             .u.ul = PORT_FLAG_MGMT }, // Managment port is on wan
  {bp_ulDyingGaspIntrPin,      .u.ul = BP_GPIO_NONE},
  {bp_last}
};

static bp_elem_t g_bcm968500tulip_wo5[] = {
  {bp_cpBoardId,               .u.cp = "968500TULIP_WO5"},
  {bp_usGpioBoardReset,        .u.us = BP_GPIO_47_AH},
  {bp_usGpioUsb0,              .u.us = ((BP_GPIO_3_AH << 8) | BP_GPIO_2_AH)},
  {bp_usGpioUsb1,              .u.us = ((BP_GPIO_1_AH << 8) | BP_GPIO_0_AH)},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x30},
  {bp_ulPhyId5,                .u.ul = 0x03 | MAC_IF_SGMII },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_48_AH},
  {bp_ulPhyId4,                .u.ul = 0x04 | MAC_IF_RGMII  },
  {bp_usGpioPhyReset,          .u.us = BP_GPIO_48_AH},
  {bp_ulPortFlags,             .u.ul = PORT_FLAG_MGMT }, // Managment port is on wan
  {bp_ulDyingGaspIntrPin,      .u.ul = BP_GPIO_NONE},
  {bp_last}
};

static bp_elem_t * g_BoardParms[] = {g_bcm968500cherry, g_bcm968751Lupine2, g_bcm968500cedar, g_bcm968500dg_test, g_bcm968500tulip_wo0, g_bcm968500tulip_wo5, 0};
#endif

#if defined(_BCM96838_) || defined(CONFIG_BCM96838)
//E8C reference design , 2L PCB, 4FE, 1xUSB, 1xWiFi, GPON, SIM CARD, NAND
static bp_elem_t g_bcm968380fhgu[] = {
  {bp_cpBoardId,               .u.cp = "968380FHGU"},  
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_72_AL},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_47_AL},
  {bp_usGpioWirelessPowerDown, .u.us = BP_GPIO_71_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_4_AL},
  {bp_usGpioVoip1Led,          .u.us = BP_LED_2_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_10_AL},
  {bp_usGpioLedGpon,           .u.us = BP_LED_5_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_LED_6_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_17_AL},
  {bp_usGpioLedOpticalLinkFail,.u.us = BP_LED_0_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_33_AL},
  {bp_usGpioLedUSB,            .u.us = BP_GPIO_53_AL},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_MII},
  {bp_ulPhyId1,                .u.ul = 0x02 | MAC_IF_MII},
  {bp_ulPhyId2,                .u.ul = 0x03 | MAC_IF_MII},
  {bp_ulPhyId3,                .u.ul = 0x04 | MAC_IF_MII},
  {bp_ulSimInterfaces,         .u.us = BP_SIMCARD_GROUPA},
  {bp_ulSlicInterfaces,        .u.us = BP_SLIC_GROUPD},
  {bp_usGpioPonTxEn,           .u.us = BP_GPIO_13_AH},
  {bp_usGpioPonRxEn,           .u.us = BP_GPIO_13_AH},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0}, 
  {bp_usRogueOnuEn,            .u.us = 1},
  {bp_last}
};
//E8C reference design , 2L PCB, 4FE, 1xUSB, 1xWiFi, EPON, SIM CARD, NAND
static bp_elem_t g_bcm968380fehg[] = {
  {bp_cpBoardId,               .u.cp = "968380FEHG"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_NONE},
  {bp_usGpioLedEpon,           .u.us = BP_LED_5_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fhgu},
  {bp_last}
};

//Check with EDI 
//E8C reference design, 2L PCB,  4GE, 1xUSB, 1xWiFi, GPON, SIM CARD, NAND
static bp_elem_t g_bcm968380fggu[] = {
  {bp_cpBoardId,               .u.cp = "968380FGGU"},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_GMII},
  {bp_ulPhyId1,                .u.ul = 0x02 | MAC_IF_GMII},
  {bp_ulPhyId2,                .u.ul = 0x03 | MAC_IF_GMII},
  {bp_ulPhyId3,                .u.ul = 0x04 | MAC_IF_GMII},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fhgu},
  {bp_last}
};

//E8C reference design, 2L PCB, 4xGE, 1xUSB, 1xWiFi, EPON , SIM CARD, NAND
static bp_elem_t g_bcm968380fegu[] = {
  {bp_cpBoardId,               .u.cp = "968380FEGU"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_NONE},
  {bp_usGpioLedEpon,           .u.us = BP_LED_5_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fggu},
  {bp_last}
};

//E8C reference design, 4L PCB, 4GbE, 1xUSB, 1xWiFi, GPON, SIM CARD, NAND
static bp_elem_t g_bcm968380ffhg[] = {
  {bp_cpBoardId,               .u.cp = "968380FFHG"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fggu},
  {bp_last}
};

//E8C reference design, 4L PCB, 4GbE, 1xUSB, 1xWiFi, EPON, SIM CARD, NAND
static bp_elem_t g_bcm968380fesfu[] = {
  {bp_cpBoardId,               .u.cp = "968380FESFU"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_NONE},
  {bp_usGpioLedEpon,           .u.us = BP_LED_5_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fggu},
  {bp_last}
};


//E8C Reference design - 4L PCB, 4xGbE, 2xUSB, 2xWiFi, GPON, SIM CARD, 2xFXS
static bp_elem_t g_bcm968380gerg[] = {
  {bp_cpBoardId,               .u.cp = "968380GERG"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_47_AL},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_39_AL},
  {bp_usGpioWirelessPowerDown, .u.us = BP_GPIO_48_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_4_AL},
  {bp_usGpioVoip1Led,          .u.us = BP_LED_2_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_10_AL},
  {bp_usGpioLedGpon,           .u.us = BP_LED_5_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_LED_6_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_17_AL},
  {bp_usGpioLedOpticalLinkFail,.u.us = BP_LED_0_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_33_AL},
  {bp_usGpioLedUSB,            .u.us = BP_LED_3_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_11_AL},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_GMII},
  {bp_ulPhyId1,                .u.ul = 0x02 | MAC_IF_GMII},
  {bp_ulPhyId2,                .u.ul = 0x03 | MAC_IF_GMII},
  {bp_ulPhyId3,                .u.ul = 0x04 | MAC_IF_GMII},
  {bp_ulSimInterfaces,         .u.us = BP_SIMCARD_GROUPA},
  {bp_ulSlicInterfaces,        .u.us = BP_SLIC_GROUPD},
  {bp_usGpioPonTxEn,           .u.us = BP_GPIO_13_AH},
  {bp_usGpioPonRxEn,           .u.us = BP_GPIO_13_AH},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usRogueOnuEn,            .u.us = 1},
  {bp_last}
};

//E8C Reference design, 4L PCB 4xGbE, 2xUSB, 2xWiFi, EPON , SIM CARD, 2xFXS
static bp_elem_t g_bcm968380eprg[] = {
  {bp_cpBoardId,               .u.cp = "968380EPRG"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_NONE},
  {bp_usGpioLedEpon,           .u.us = BP_LED_5_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_GMII},
  {bp_ulPhyId1,                .u.ul = 0x02 | MAC_IF_GMII},
  {bp_ulPhyId2,                .u.ul = 0x03 | MAC_IF_GMII},
  {bp_ulPhyId3,                .u.ul = 0x04 | MAC_IF_GMII},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380gerg},
  {bp_last}
};


//BCM968380F SV Board, big 6838SV board , GPON
static bp_elem_t g_bcm968380fsv_g[] = {
  {bp_cpBoardId,               .u.cp = "968380FSV_G"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
//  {bp_ulPortMap,               .u.ul = 0x1f}, DAN: temporary comment out
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_GMII},
  {bp_ulPhyId1,                .u.ul = 0x02 | MAC_IF_GMII},
  {bp_ulPhyId2,                .u.ul = 0x03 | MAC_IF_GMII},
  {bp_ulPhyId3,                .u.ul = 0x04 | MAC_IF_GMII},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_GMII|PHY_EXTERNAL|PHY_INTEGRATED_VALID},
//  {bp_ulPhyId4,                .u.ul = 0x00 | PHYCFG_VALID |MAC_IF_GMII | PHY_EXTERNAL}, DAN: temporary comment out
  {bp_usGpioPonTxEn,           .u.us = BP_GPIO_1_AH},
  {bp_usGpioPonRxEn,           .u.us = BP_GPIO_0_AH},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_us1ppsPin,               .u.us = BP_PIN_1PPS_6},
//{bp_usSerialLEDMuxSel,       .u.us = BP_SERIAL_LED_MUX_GROUPA}, an example of us eof shift register to output LEDs
  {bp_usGpioUart2Sdin,         .u.us = BP_GPIO_14_AH}, // uncomment to enable second UART
  {bp_usGpioUart2Sdout,        .u.us = BP_GPIO_15_AH}, // uncomment to enable second UART       
  {bp_last}
};

//BCM968380F SV Board, big 6838SV board , EPON , same as 968380FSV_G
static bp_elem_t g_bcm968380fsv_e[] = {
  {bp_cpBoardId,               .u.cp = "968380FSV_E"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fsv_g},
  {bp_last}
};

//BCM968380F SV Board, big 6838SV board , EPON_GPON, , same as 968380FSV_G
static bp_elem_t g_bcm968380fsv_ge[] = {
  {bp_cpBoardId,               .u.cp = "968380FSV_GE"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON_EPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fsv_g},
  {bp_last}
};


//TBD 
static bp_elem_t g_bcm968380sv_g[] = {
  {bp_cpBoardId,               .u.cp = "968380SV_G"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380fsv_g}, //?????
  {bp_last}
};

static bp_elem_t g_bcm968380sv_e[] = {
  {bp_cpBoardId,               .u.cp = "968380SV_E"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380sv_g}, 
  {bp_last}
};

static bp_elem_t g_bcm968380sv_ge[] = {
  {bp_cpBoardId,               .u.cp = "968380SV_GE"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON_EPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968380sv_g},
  {bp_last}
};

//E8C Reference design  2L PCB, GPON, 2xFE, 1x FXS, SIM CARD, NAND, SPI NOR
static bp_elem_t g_bcm968385sfu[] = {
  {bp_cpBoardId,               .u.cp = "968385SFU"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_48_AL},
  {bp_usGpioVoip1Led,          .u.us = BP_LED_2_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_10_AL},
  {bp_usGpioLedGpon,           .u.us = BP_LED_5_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_usGpioLedOpticalLinkFail,.u.us = BP_LED_0_AL},
  {bp_usPinMux,                .u.us = BP_GPIO_33_AL},
  {bp_usGpioLedSim,            .u.us = BP_GPIO_4_AL},
  {bp_usGpioLedSim_ITMS,       .u.us = BP_GPIO_9_AL},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x03},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_MII},
  {bp_ulPhyId1,                .u.ul = 0x02 | MAC_IF_MII},
  {bp_ulSimInterfaces,         .u.us = BP_SIMCARD_GROUPA},
  {bp_ulSlicInterfaces,        .u.us = BP_SLIC_GROUPD},
  {bp_usGpioPonTxEn,           .u.us = BP_GPIO_13_AH},
  {bp_usGpioPonRxEn,           .u.us = BP_GPIO_13_AH},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_usRogueOnuEn,            .u.us = 1},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_last}
};

//E8C Reference design  2L PCB, EPON, 2xFE, 1x FXS, SIM CARD, NAND, SPI NOR
static bp_elem_t g_bcm968385esfu[] = {
  {bp_cpBoardId,               .u.cp = "968385ESFU"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968385sfu},
  {bp_last}
};

//E8C Reference design  2L PCB, GPON, 1x GbE, SIM CARD, NAND, SPI NOR
static bp_elem_t g_bcm968385gsp[] = {
  {bp_cpBoardId,               .u.cp = "968385GSP"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_NONE},
  {bp_ucDspType0,              .u.uc = BP_VOIP_NO_DSP},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x01},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_GMII},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968385sfu},
  {bp_last}
};

//E8C Reference design  2L PCB, EPON, 1x GbE, SIM CARD, NAND, SPI NOR
static bp_elem_t g_bcm968385esp[] = {
  {bp_cpBoardId,               .u.cp = "968385ESP"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_NONE},  
  {bp_usGpioLedEpon,           .u.us = BP_LED_5_AL}, 
  {bp_usPinMux,                .u.us = BP_GPIO_15_AL},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968385gsp},
   {bp_last}
};

//R&D only
static bp_elem_t g_bcm968385sv_g[] = {
  {bp_cpBoardId,               .u.cp = "968385SV_G"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_GPON},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_INTERNAL_PHY},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3},
  {bp_ulPhyId0,                .u.ul = 0x01 | MAC_IF_GMII},
  {bp_ulPhyId1,                .u.ul = 0x02 | MAC_IF_GMII},
//  {bp_ulPhyId2,                .u.ul = 0x00 | PHYCFG_VALID |MAC_IF_GMII | PHY_EXTERNAL},
  {bp_usGpioPonTxEn,           .u.us = BP_GPIO_4_AH},
  {bp_usGpioPonRxEn,           .u.us = BP_GPIO_12_AH},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_last}
};

static bp_elem_t g_bcm968385sv_e[] = {
  {bp_cpBoardId,               .u.cp = "968385SV_E"},
  {bp_ulOpticalWan,            .u.ul = BP_OPTICAL_WAN_EPON},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm968385sv_g},
  {bp_last}
};


static bp_elem_t * g_BoardParms[] = {g_bcm968380fhgu, g_bcm968380ffhg, g_bcm968380gerg, g_bcm968380fsv_g, g_bcm968380fsv_e, g_bcm968380fsv_ge, g_bcm968380sv_g, g_bcm968380sv_e, g_bcm968380sv_ge, g_bcm968380eprg, g_bcm968380fesfu, g_bcm968380fehg, g_bcm968380fggu, g_bcm968380fegu, g_bcm968385sfu, g_bcm968385esfu, g_bcm968385gsp, g_bcm968385esp, g_bcm968385sv_g, g_bcm968385sv_e, 0};
#endif

#if defined(_BCM96362_) || defined(CONFIG_BCM96362)

static bp_elem_t g_bcm96362advnx[] = {
  {bp_cpBoardId,               .u.cp = "96362ADVNX"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm96362advngr[] = {
  {bp_cpBoardId,               .u.cp = "96362ADVNgr"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_SPI_SSB3_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ulPhyId5,                .u.ul = 0x19},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm96362advngr2[] = {
  {bp_cpBoardId,               .u.cp = "96362ADVNgr2"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_SPI_SSB3_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_11_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_2},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96362advngr},
  {bp_last}
};

static bp_elem_t g_bcm96362advn2xh[] = {
  {bp_cpBoardId,               .u.cp = "96362ADVN2xh"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_SPI_SSB2_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x10},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_SPI_SSB_2},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x00 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId1,                .u.ul = 0x01 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId2,                .u.ul = 0x02 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId3,                .u.ul = 0x03 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId4,                .u.ul = 0x04 | CONNECTED_TO_EXTERN_SW},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm96362advn2xm[] = {
  {bp_cpBoardId,               .u.cp = "96362ADVN2XM"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_SPI_SSB2_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x10},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_SPI_SSB_2},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x00 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId1,                .u.ul = 0x01 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId2,                .u.ul = 0x02 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId3,                .u.ul = 0x03 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId4,                .u.ul = 0x04 | CONNECTED_TO_EXTERN_SW},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};


static bp_elem_t g_bcm96361XF[] = {
  {bp_cpBoardId,               .u.cp = "96361XF"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = MII_DIRECT},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_2},
  {bp_usVregSel1P2,            .u.us = 0x13},
  {bp_usGpioFemtoReset,        .u.us = BP_GPIO_35_AH},
  {bp_last}
};

static bp_elem_t g_bcm96361I2[] = {
  {bp_cpBoardId ,              .u.cp = "96361I2"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_24 | MAC_IF_RGMII},
  {bp_ulPhyId5,                .u.ul = BP_PHY_ID_25 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm96362rpvt[] = {
  {bp_cpBoardId ,              .u.cp = "96362RPVT"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII},
  {bp_ulPhyId5,                .u.ul = 0x19 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_2}, 
  {bp_last}
};

static bp_elem_t g_bcm96362rpvt_2u[] = {
  {bp_cpBoardId ,              .u.cp = "96362RPVT_2U"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII},
  {bp_ulPhyId5,                .u.ul = 0x19 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_2}, 
  {bp_last}
};

static bp_elem_t g_bcm96362ravngr2[] = {
  {bp_cpBoardId,               .u.cp = "96362RAVNGR2"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_11_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_2},
  {bp_last}
};

static bp_elem_t g_bcm96362radvnxh5[] = {
  {bp_cpBoardId,               .u.cp = "96362RADVNXH5"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_SPI_SSB2_EXT_CS |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_11_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_2},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96362advn2xh},
  {bp_last}
};

static bp_elem_t g_bcm96362radvnxh5_p203[] = {
  {bp_cpBoardId,               .u.cp = "96362RADVNXH503"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_SPI_SSB2_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x11},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_SPI_SSB_2},
  {bp_ulPortMap,               .u.ul = 0x1e},
  {bp_ulPhyId1,                .u.ul = 0x01 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId2,                .u.ul = 0x02 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId3,                .u.ul = 0x03 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId4,                .u.ul = 0x04 | CONNECTED_TO_EXTERN_SW},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_2},
  {bp_last}
};


static bp_elem_t g_bcm96362radvn2xh[] = {
  {bp_cpBoardId,               .u.cp = "96362RADVN2XH"},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96362radvnxh5},
  {bp_last}
};

#ifndef SKY
static bp_elem_t g_bcm96362rwfar[] = {
  {bp_cpBoardId,               .u.cp = "96362RWFAR"},
  {bp_ulGpioOverlay,           .u.ul = BP_OVERLAY_EPHY_LED_0},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_5_AL},  
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_19_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},  
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_0_AL},  
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_28_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_29_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_2},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_0},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x31},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ulPhyId5,                .u.ul = 0x19},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_16_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_GPIO_17_AL},
  {bp_usGpioPotsLed,           .u.us = BP_GPIO_18_AL},	
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301| BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_2},
  {bp_last}
};
#endif

#ifdef SKY
static bp_elem_t g_bcm96362meerkat[] = {
  {bp_cpBoardId ,              .u.cp = "Meerkat"},
  {bp_usGpioOverlay,           .u.ul =(BP_OVERLAY_SPI_SSB3_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3)
                                       },
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION}, /* Murthy This option is added as part on 4.12.08 merge, check for any side effects */
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_12_AH}, /* Change for IHR-Nk BP_SERIAL_GPIO_12_AH */
  {bp_usGpioLedAdslFail,           .u.us = BP_GPIO_14_AL }, /* Change for IHR-Nk BP_SERIAL_GPIO_14_AL*/
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_16_AL},  /* Change for IHR-Nk BP_SERIAL_GPIO_10_AL */
  {bp_usGpioLedSesWirelessFail,    .u.us = BP_GPIO_16_AL},/* Change for IHR-Nk BP_SERIAL_GPIO_16_AL*/
   {bp_usGpioLedWireless,    .u.us = BP_GPIO_10_AH},/* Change for IHR-Nk BP_SERIAL_GPIO_10_AH*/
  {bp_usGpioLedskyHD,    .u.us = BP_GPIO_18_AH},/* Change for IHR-Nk BP_SERIAL_GPIO_16_AL*/
  {bp_usGpioLedskyHDFail,    .u.us = BP_GPIO_17_AL},/* Change for IHR-Nk BP_SERIAL_GPIO_16_AL*/
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_12_AH},/* Change for IHR-Nk */
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_14_AL},/* Change for IHR-Nk BP_SERIAL_GPIO_14_AL */
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_27_AH}, /* Change for IHR-Nk BP_SERIAL_GPIO_12_AL*/
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_26_AL}, /* Change for IHR-Nk BP_SERIAL_GPIO_13_AL*/
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ulPhyId5,                .u.ul = 0x19},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_2},
  {bp_last}
};

static bp_elem_t g_bcm96362meerkatB[] = {
  {bp_cpBoardId ,              .u.cp = "MeerkatB"},
  {bp_usGpioOverlay,           .u.ul =(BP_OVERLAY_SPI_SSB3_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usWirelessFlags,         .u.us = BP_WLAN_EXCLUDE_ONBOARD},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ulPhyId5,                .u.ul = 0x19},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t * g_BoardParms[] = {g_bcm96362advnx, g_bcm96362advngr, g_bcm96362advngr2, g_bcm96362advn2xh, g_bcm96361XF, g_bcm96361I2, g_bcm96362rpvt, g_bcm96362rpvt_2u, g_bcm96362ravngr2, g_bcm96362advn2xm, g_bcm96362radvnxh5, g_bcm96362radvn2xh, g_bcm96362radvnxh5_p203, g_bcm96362meerkat, g_bcm96362meerkatB, 0};
#else /* SKY */
static bp_elem_t * g_BoardParms[] = {g_bcm96362advnx, g_bcm96362advngr, g_bcm96362advngr2, g_bcm96362advn2xh, g_bcm96361XF, g_bcm96361I2, g_bcm96362rpvt, g_bcm96362rpvt_2u, g_bcm96362ravngr2, g_bcm96362advn2xm, g_bcm96362radvnxh5, g_bcm96362radvn2xh, g_bcm96362radvnxh5_p203, g_bcm96362rwfar, 0};
#endif /* SKY */
#endif

#if defined(_BCM96368_) || defined(CONFIG_BCM96368)

static bp_elem_t g_bcm96368vvw[] = {
  {bp_cpBoardId,               .u.cp = "96368VVW"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_24_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_33_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_0_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_1_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_ISIL1556 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_ISIL_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368gpolt[] = {
  {bp_cpBoardId,               .u.cp = "96368GPOLT"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_SERIAL_LEDS)},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_0_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_1_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x33}, 
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId4,                .u.ul = 0x10},
  {bp_ulPhyId5,                .u.ul = GMII_DIRECT},
  {bp_last}
};

static bp_elem_t g_bcm96368vvwb[] = {
  {bp_cpBoardId,               .u.cp = "96368VVWB"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_24_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_33_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_0_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_1_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_ISIL1556 | BP_AFE_FE_ANNEXB | BP_AFE_FE_REV_ISIL_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368ntr[] = {
  {bp_cpBoardId,               .u.cp = "96368NTR"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_33_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_0_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_1_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_25_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_GPIO_26_AL},
  {bp_usGpioPotsLed,           .u.us = BP_GPIO_27_AL},
  {bp_last}
};

static bp_elem_t g_bcm96368sv2[] = {
  {bp_cpBoardId,               .u.cp = "96368SV2"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_33_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_30_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_31_AH},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x12},
  {bp_ulPhyId5,                .u.ul = 0x11},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_25_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_GPIO_26_AL},
  {bp_usGpioPotsLed,           .u.us = BP_GPIO_27_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_ISIL1556 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_ISIL_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368mvwg[] = {
  {bp_cpBoardId,               .u.cp = "96368MVWG"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_22_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x36},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId4,                .u.ul = 0x12},
  {bp_ulPhyId5,                .u.ul = 0x11},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368mvwgb[] = {
  {bp_cpBoardId,               .u.cp = "96368MVWGB"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_22_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x36},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId4,                .u.ul = 0x12},
  {bp_ulPhyId5,                .u.ul = 0x11},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXB | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368mvwgj[] = {
  {bp_cpBoardId,               .u.cp = "96368MVWGJ"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_22_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x36},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x00},
  {bp_ulPhyId4,                .u.ul = 0x12},
  {bp_ulPhyId5,                .u.ul = 0x11},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXJ | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368mb2g[] = {
  {bp_cpBoardId,               .u.cp = "96368MB2G"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_27_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_ulAfeId1,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_ISIL_REV1},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_35_AL},
  {bp_last}
};

static bp_elem_t g_bcm96368mbg6b[] = {
  {bp_cpBoardId,               .u.cp = "96368MBG6b"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_27_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_ISIL_REV1},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_35_AL},
  {bp_last}
};

static bp_elem_t g_bcm96368mbg6302[] = {
  {bp_cpBoardId,               .u.cp = "96368MBG6302"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_27_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_ulAfeId1,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_35_AL},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_37_AH},
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_36_AH},
  {bp_last}
};

static bp_elem_t g_bcm96368mng[] = {
  {bp_cpBoardId,               .u.cp = "96368MNG"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x20},
  {bp_ulPhyId5,                .u.ul = GMII_DIRECT | EXTSW_CONNECTED},
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_SPI_SSB_1},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x00 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId1,                .u.ul = 0x01 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId2,                .u.ul = 0x02 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId3,                .u.ul = 0x03 | CONNECTED_TO_EXTERN_SW},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_ISIL_REV1},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_37_AL},
  {bp_last}
};

static bp_elem_t g_bcm96367avng[] = {
  {bp_cpBoardId,               .u.cp = "96367AVNG"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_23_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_31_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x2f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId5,                .u.ul = 0x11},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368mvngr[] = {
  {bp_cpBoardId,               .u.cp = "96368MVNgr"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_SPI_EXT_CS |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_23_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_3_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368rmvng_nand[] = {
  {bp_cpBoardId,               .u.cp = "96368RMVNg NAND"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_SPI_EXT_CS |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_23_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_3_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x03},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368rmvng_nor[] = {
  {bp_cpBoardId,               .u.cp = "96368RMVNg NOR"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_SPI_EXT_CS |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_23_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_3_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x13},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368mvngrP2[] = {
  {bp_cpBoardId,               .u.cp = "96368MVNgrP2"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_23_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_3_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_22_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_24_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t g_bcm96368Ext[] = {
  {bp_cpBoardId,               .u.cp = "96368EXT"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_2_AL},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x03},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_last}
};

static bp_elem_t * g_BoardParms[] =
{g_bcm96368vvw, g_bcm96368mvwg, g_bcm96368sv2, g_bcm96368mb2g,
g_bcm96368ntr, g_bcm96368Ext, g_bcm96368vvwb, g_bcm96368mvwgb,
g_bcm96368mng, g_bcm96368mbg6302, g_bcm96368mvwgj, g_bcm96367avng,
g_bcm96368mvngr, g_bcm96368rmvng_nand, g_bcm96368rmvng_nor, g_bcm96368mvngrP2, g_bcm96368mbg6b, 
g_bcm96368gpolt, 0};
#endif

#if !defined(_CFE_)
// MoCA init command sequence
// Enable RGMII_0 to MOCA. 1Gbps
// Enable RGMII_1 to GPHY. 1Gbps
BpCmdElem moca6802ONTInitSeq[] = {
    { CMD_WRITE, SUN_TOP_CTRL_SW_INIT_0_CLEAR, 0x0FFFFFFF }, //clear sw_init signals

    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_0, 0x11110011 }, //pinmux, rgmii, 3450
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_1, 0x11111111 }, //rgmii
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_2, 0x11111111 }, //rgmii
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_3, 0x22221111 }, // enable sideband all,0,1,2, rgmii
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_4, 0x00000022 }, // enable sideband 3,4

    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_6, 0x00001100 }, // mdio, mdc

//    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_2, 0x00555510 }, // set gpio 38,39,40 to PULL_NONE
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_3, 0x2402 }, // Set GPIO 41 to PULL_UP

    { CMD_WRITE, SUN_TOP_CTRL_GENERAL_CTRL_NO_SCAN_0, 0x11 }, // , Use 2.5V for rgmii
    { CMD_WRITE, SUN_TOP_CTRL_GENERAL_CTRL_NO_SCAN_5, 0x47 }, // set test_drive_sel to 16mA

    { CMD_WRITE, EPORT_REG_EMUX_CNTRL, 0x02 }, //  Select port mode. Activate both GPHY and MOCA phys.

    { CMD_WRITE, EPORT_REG_RGMII_0_CNTRL,  0x1 }, //  Enable rgmiis.
    { CMD_WRITE, EPORT_REG_RGMII_1_CNTRL,  0x1 }, //

    { CMD_WRITE, EPORT_REG_GPHY_CNTRL, 0x02A4C000 | 0x200 }, //  Reset Gphy
    { CMD_WAIT,  100,                 0x0         },
    { CMD_WRITE, EPORT_REG_GPHY_CNTRL, 0x02A4C000 },

    { CMD_END,  0, 0 }
};

BpCmdElem moca6802BHRInitSeq[] = {
    { CMD_WRITE, SUN_TOP_CTRL_SW_INIT_0_CLEAR, 0x0FFFFFFF }, //clear sw_init signals

    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_0, 0x11110011 }, //pinmux, rgmii, 3450
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_1, 0x11111111 }, //rgmii
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_2, 0x11111111 }, //rgmii
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_3, 0x22221111 }, // enable sideband all,0,1,2, rgmii
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_4, 0x00000022 }, // enable sideband 3,4

    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_CTRL_6, 0x00001100 }, // mdio, mdc

//    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_2, 0x00555510 }, // set gpio 38,39,40 to PULL_NONE
    { CMD_WRITE, SUN_TOP_CTRL_PIN_MUX_PAD_CTRL_3, 0x2402 }, // Set GPIO 41 to PULL_UP

    { CMD_WRITE, SUN_TOP_CTRL_GENERAL_CTRL_NO_SCAN_0, 0x11 }, // , Use 2.5V for rgmii
    { CMD_WRITE, SUN_TOP_CTRL_GENERAL_CTRL_NO_SCAN_5, 0x47 }, // set test_drive_sel to 16mA

    { CMD_WRITE, EPORT_REG_EMUX_CNTRL, 0x02 }, //  Select port mode. Activate both GPHY and MOCA phys.

    { CMD_WRITE, EPORT_REG_RGMII_0_CNTRL,  0x1 }, //  Enable rgmiis.

    { CMD_WRITE, EPORT_REG_GPHY_CNTRL, 0x02A4C00F }, // Shutdown Gphy

    { CMD_END,  0, 0 }
};

#endif

#if defined(_BCM96818_) || defined(CONFIG_BCM96818)

#if !defined(_CFE_)
unsigned char _6818RgParams[BP_OPTICAL_PARAMS_LEN] =
   {0x28, 0x03, 0x02, 0x0b, 0x00, 0xd8, 0x00, 0x77,
    0x01, 0x2A, 0x18, 0x02, 0x02, 0x32, 0x00, 0x00,
    0x85, 0x95, 0xea, 0x00, 0x00, 0x09, 0x00, 0x00,
    0x20, 0x6f, 0xff, 0xff, 0xef, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01};

#endif

/* additional PHY INIT for AC201 LED on 6818EGG board */
static bp_mdio_init_t g_phyinit_ac201led[] = {
    { .u.update.op = BP_MDIO_INIT_OP_UPDATE, .u.update.reg = 0x1F, .u.update.mask = 0x80, .u.update.data = 0x80},	/* turn on show register access */
    { .u.update.op = BP_MDIO_INIT_OP_UPDATE, .u.update.reg = 0x1A, .u.update.mask = 0x3,  .u.update.data = 0x0},	/* set LED1 link/act, LED2 speed */
    { .u.update.op = BP_MDIO_INIT_OP_UPDATE, .u.update.reg = 0x1F, .u.update.mask = 0x80, .u.update.data = 0x0},	/* turn off show register access */
    { .u.op.op = BP_MDIO_INIT_OP_NULL}	/* end of list */
};

static bp_elem_t g_bcm96818sv[] = {
  {bp_cpBoardId,               .u.cp = "96818SV"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_1 )},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_3_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_4_AH},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_8_AH},
  {bp_usGpioLedGponFail,       .u.us = BP_GPIO_16_AH}, 
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_15_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_17_AH},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_ulPhyId1,                .u.ul = 0x01},
  {bp_ulPhyId2,                .u.ul = 0x14},
  {bp_ulPhyId3,                .u.ul = 0x12},
  {bp_ulPhyId4,                .u.ul = 0xff | PHYCFG_VALID}, /* WAN interface */
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_last}
};

static bp_elem_t g_bcm96818sv_miiOverGpio[] = {
  {bp_cpBoardId,               .u.cp = "96818SV_MII"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCI |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_1 )},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_3_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_4_AH},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_8_AH},
  {bp_usGpioLedGponFail,       .u.us = BP_GPIO_16_AH},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_15_AH},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_17_AH},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_ulPhyId1,                .u.ul = 0x01},
  {bp_ulPhyId2,                .u.ul = 0x14},
  {bp_ulPhyId3,                .u.ul = 0x12},
  {bp_ulPhyId4,                .u.ul = 0x10 | MII_OVER_GPIO_VALID },  
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_last}
};

static bp_elem_t g_bcm96818g_rg[] = {
  {bp_cpBoardId,               .u.cp = "96818G_RG"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_1 |
                                       BP_OVERLAY_SERIAL_LEDS)},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_11_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_19_AH},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_12_AH},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_0_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_1_AH},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_8_AH},
  {bp_usGpioLedGponFail,       .u.us = BP_SERIAL_GPIO_6_AH},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_usSpeedLed100,           .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usSpeedLed1000,          .u.us = BP_SERIAL_GPIO_3_AL},  
  {bp_ulPhyId1,                .u.ul = 0x01},
  {bp_usSpeedLed100,           .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usSpeedLed1000,          .u.us = BP_SERIAL_GPIO_5_AL},  
  {bp_ulPhyId2,                .u.ul = 0x18 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_ulPhyId3,                .u.ul = 0x19 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_ulPhyId4,                .u.ul = 0xff | PHYCFG_VALID}, /* WAN interface */
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0}, 
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_8_AL},  
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usGpioLaserDis,          .u.us = BP_GPIO_0_AL},
  {bp_usGpioLaserTxPwrEn,      .u.us = BP_GPIO_5_AL},  
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_last}
};

static bp_elem_t g_bcm96818g_rg_bosa[] = {
  {bp_cpBoardId,               .u.cp = "96818G_RG_BOSA"},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_BOSA},
#if !defined(_CFE_)
  {bp_cpDefaultOpticalParams,  .u.ucp = _6818RgParams},
#endif
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96818g_rg},
  {bp_last}
};

static bp_elem_t g_bcm96812brg[] = {
  {bp_cpBoardId,               .u.cp = "96812BRG"},
  {bp_ulGpioOverlay,           .u.ul = BP_OVERLAY_GPHY_LED_0},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_18_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_19_AH},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedGponFail,       .u.us = BP_GPIO_5_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_4},  
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x15},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_usSpeedLed100,           .u.us = BP_GPIO_15_AL},
  {bp_usSpeedLed1000,          .u.us = BP_GPIO_26_AL},
  {bp_ulPhyId2,                .u.ul = 0x18 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_ulPhyId4,                .u.ul = 0xff | PHYCFG_VALID}, /* WAN interface */
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_last}
};

static bp_elem_t g_bcm96812brg_bosa[] = {
  {bp_cpBoardId,               .u.cp = "96812BRG_BOSA"},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_BOSA},
#if !defined(_CFE_)
  {bp_cpDefaultOpticalParams,  .u.ucp = _6818RgParams},
#endif
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96812brg},
  {bp_last}
};

static bp_elem_t g_bcm96812r2g[] = {
  {bp_cpBoardId,               .u.cp = "BCM96812R2G"},
  {bp_ulGpioOverlay,           .u.ul = BP_OVERLAY_GPHY_LED_0},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_18_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_19_AH},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedGponFail,       .u.us = BP_GPIO_5_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_4},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x11},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_ulPhyId4,                .u.ul = 0xff | PHYCFG_VALID}, /* WAN interface */
  {bp_usSpeedLed100,           .u.us = BP_GPIO_15_AL},
  {bp_usSpeedLed1000,          .u.us = BP_GPIO_26_AL},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_BOSA},
#if !defined(_CFE_)
  {bp_cpDefaultOpticalParams,  .u.ucp = _6818RgParams},
#endif
  {bp_last}
};

static bp_elem_t g_bcm96818egg[] = {
  {bp_cpBoardId,               .u.cp = "96818EGG"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_1 |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_SERIAL_LEDS
                                       )},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedGponFail,       .u.us = BP_GPIO_10_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_11_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_3},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_usSpeedLed100,           .u.us = BP_GPIO_3_AL},
  {bp_usSpeedLed1000,          .u.us = BP_GPIO_30_AL},
  {bp_ulPhyId1,                .u.ul = 0x01},
  {bp_usSpeedLed100,           .u.us = BP_SERIAL_GPIO_1_AL},
  {bp_usSpeedLed1000,          .u.us = BP_SERIAL_GPIO_0_AL},
  {bp_ulPhyId2,                .u.ul = 0x08 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_MII},
  {bp_pPhyInit,                .u.ptr = (void *)g_phyinit_ac201led},
  {bp_ulPhyId3,                .u.ul = 0x10 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_MII},
  {bp_pPhyInit,                .u.ptr = (void *)g_phyinit_ac201led},
  {bp_ulPhyId4,                .u.ul = 0xff | PHYCFG_VALID}, /* WAN interface */
  {bp_ulPhyId5,                .u.ul = 0x18 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_last}
};

static bp_elem_t g_bcm96818egg_p202[] = {
  {bp_cpBoardId,               .u.cp = "96818EGG_P202"},
  {bp_usGpioLedGponFail,       .u.us = BP_GPIO_38_AL},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96818egg},
  {bp_last}
};

static bp_elem_t g_bcm96818grrg[] = {
  {bp_cpBoardId,               .u.cp = "96818GRRG"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_1 |
                                       BP_OVERLAY_SERIAL_LEDS)},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_11_AL},
  /*{bp_usGpioLedWanData,        .u.us = BP_GPIO_19_AH},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_12_AH},*/
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_0_AH},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_1_AH},
  {bp_usGpioLedGpon,           .u.us = BP_GPIO_8_AH},
  {bp_usGpioLedGponFail,       .u.us = BP_SERIAL_GPIO_6_AH},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x3f},
  {bp_ulPhyId0,                .u.ul = 0x00},
  {bp_usSpeedLed100,           .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usSpeedLed1000,          .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_ulPhyId1,                .u.ul = 0x01},
  {bp_usSpeedLed100,           .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usSpeedLed1000,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_ulPhyId2,                .u.ul = 0x19 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_ulPhyId3,                .u.ul = 0x1A | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_ulPhyId4,                .u.ul = 0xff | PHYCFG_VALID}, /* WAN interface */
  {bp_ulPhyId5,                .u.ul = 0x18 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_3},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_0},
  {bp_usGpioLaserReset,        .u.us = BP_GPIO_10_AL}, 
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_LEGACY},
  {bp_last}
};

static bp_elem_t g_bcm96818grrg_bosa[] = {
  {bp_cpBoardId,               .u.cp = "96818GRRG_BOSA"},
  {bp_usGponOpticsType,        .u.us = BP_GPON_OPTICS_TYPE_BOSA},
#if !defined(_CFE_)
  {bp_cpDefaultOpticalParams,  .u.ucp = _6818RgParams},
#endif
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96818grrg},
  {bp_last}
};

static bp_elem_t g_bcm96818grrg2l_bosa[] = {
  {bp_cpBoardId,               .u.cp = "96818GRRG2LBOSA"},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96818grrg_bosa},
  {bp_last}
};

static bp_elem_t g_bcm96818gr_ont[] = {
  {bp_cpBoardId,               .u.cp = "96818GR_ONT"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_1 |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_SPI_SSB3_EXT_CS |                                       
                                       BP_OVERLAY_SERIAL_LEDS
                                       )},
  {bp_usGpioLedBlPowerOn,          .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_usGpioLedBlStop,             .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedGpon,               .u.us = BP_GPIO_8_AL},
#if 1  
//  {bp_usExtIntrSesBtnWireless,     .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_3 | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL},
//  {bp_usGpio_Intr,                 .u.us = BP_GPIO_25_AL},
//  {bp_usExtIntrResetToDefault,     .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_3 | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL},
//  {bp_usGpio_Intr,                 .u.us = BP_GPIO_5_AL},
#if !defined(_CFE_)
  {bp_usMocaType0,                 .u.us = BP_MOCA_TYPE_LAN},
  {bp_usMocaRfBand,                .u.us = BP_MOCA_RF_BAND_EXT_D},
  {bp_usExtIntrMocaHostIntr,       .u.us = BP_EXT_INTR_3 | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL},
  {bp_pMocaInit,                   .u.ptr = (void*)moca6802ONTInitSeq},
  {bp_usGpioSpiSlaveReset,         .u.us = BP_GPIO_24_AL},
  {bp_usSpiSlaveBusNum,            .u.us = HS_SPI_BUS_NUM},
  {bp_usSpiSlaveSelectNum,         .u.us = 3},
  {bp_usSpiSlaveMode,              .u.us = SPI_MODE_3},
  {bp_ulSpiSlaveCtrlState,         .u.ul = SPI_CONTROLLER_STATE_GATE_CLK_SSOFF},
  {bp_ulSpiSlaveMaxFreq,           .u.ul = 12500000},// Move to higher freq when bring up is done.
#endif  
#endif
  {bp_usGpioLedSesWireless,        .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usWirelessFlags,             .u.us = 0},
  {bp_ucPhyType0,                  .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,                .u.uc = 0x0},
  {bp_usConfigType,                .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,                   .u.ul = 0x0F},
  {bp_ulPhyId0,                    .u.ul = 0x00},
  {bp_usSpeedLed100,               .u.us = BP_GPIO_3_AL},
  {bp_usSpeedLed1000,              .u.us = BP_GPIO_30_AL},
  {bp_ulPhyId1,                    .u.ul = 0x01},
  {bp_usSpeedLed100,               .u.us = BP_SERIAL_GPIO_1_AL},
  {bp_usSpeedLed1000,              .u.us = BP_SERIAL_GPIO_0_AL}, 
  {bp_ulPhyId2,                    .u.ul = 0xA | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_usPhyConnType,               .u.us = PHY_CONN_TYPE_MOCA_ETH},
  {bp_ucPhyDevName,                .u.cp = "eth%d"},
  {bp_ulPhyId3,                    .u.ul = RGMII_DIRECT},
  {bp_usPhyConnType,               .u.us = PHY_CONN_TYPE_MOCA}, 
  {bp_ucPhyDevName,                .u.cp = "moca%d"},
  {bp_ucDspType0,                  .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,                .u.uc = 0},
  {bp_usGpioVoip1Led,              .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioVoip2Led,              .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGponOpticsType,            .u.us = BP_GPON_OPTICS_TYPE_BOSA},
#if !defined(_CFE_)
  {bp_cpDefaultOpticalParams,  .u.ucp = _6818RgParams}, 
#endif  
  {bp_last}
};

static bp_elem_t g_bcm6802SV[] = {
  {bp_cpBoardId,               .u.cp = "96802SV"},    
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_1  |
                                       BP_OVERLAY_SPI_SSB3_EXT_CS) },
#if 1  
// Define these later. Introduce sharing after boards are reworked.
 // {bp_usExtIntrSesBtnWireless,     .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_3},
//  {bp_usGpio_Intr,                 .u.us = BP_GPIO_25_AL},
 // {bp_usExtIntrResetToDefault,     .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_3},
//  {bp_usGpio_Intr,                 .u.us = BP_GPIO_5_AL},
//  {bp_usExtIntrMocaHostIntr,       .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_3 | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL},
#if !defined(_CFE_)
    {bp_usMocaType0,                 .u.us = BP_MOCA_TYPE_LAN},
    {bp_usMocaRfBand,                .u.us = BP_MOCA_RF_BAND_EXT_D},
    {bp_usExtIntrMocaHostIntr,       .u.us = BP_EXT_INTR_0 | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL},
    {bp_pMocaInit,                   .u.ptr = (void*)moca6802ONTInitSeq},
    {bp_usGpioSpiSlaveReset,         .u.us = BP_GPIO_24_AL},
    {bp_usSpiSlaveBusNum,            .u.us = HS_SPI_BUS_NUM},
    {bp_usSpiSlaveSelectNum,         .u.us = 3},
    {bp_usSpiSlaveMode,              .u.us = SPI_MODE_3},
    {bp_ulSpiSlaveCtrlState,         .u.ul = SPI_CONTROLLER_STATE_GATE_CLK_SSOFF},
    {bp_ulSpiSlaveMaxFreq,           .u.ul = 12500000},
#endif
#endif  
  {bp_ucPhyType0,                  .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,                .u.uc = 0x0},
  {bp_usConfigType,                .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,                   .u.ul = 0x0F},
  {bp_ulPhyId0,                    .u.ul = 0x00},
  {bp_usSpeedLed100,               .u.us = BP_GPIO_0_AL},
  {bp_usSpeedLed1000,              .u.us = BP_GPIO_27_AL},
  {bp_ulPhyId1,                    .u.ul = 0x01},
  {bp_usSpeedLed100,               .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usSpeedLed1000,              .u.us = BP_SERIAL_GPIO_15_AL}, 
  {bp_ulPhyId2,                    .u.ul = RGMII_DIRECT},
  {bp_usPhyConnType,               .u.us = PHY_CONN_TYPE_MOCA},  
  {bp_ucPhyDevName,                .u.cp = "moca%d"},  
  {bp_ulPhyId3,                    .u.ul = 0xA | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
  {bp_usPhyConnType,               .u.us = PHY_CONN_TYPE_MOCA_ETH},  
  {bp_ucPhyDevName,                .u.cp = "eth%d"},  
  {bp_ucDspType0,                  .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,                .u.uc = 0},  

  {bp_last}
};

static bp_elem_t * g_BoardParms[] = {g_bcm96818sv, g_bcm96818sv_miiOverGpio, g_bcm96818g_rg, g_bcm96818g_rg_bosa, g_bcm96812brg, g_bcm96812brg_bosa, g_bcm96818egg, g_bcm96812r2g, g_bcm96818grrg, g_bcm96818grrg_bosa, g_bcm96818grrg2l_bosa, g_bcm96818gr_ont, g_bcm6802SV, g_bcm96818egg_p202, 0};

#endif

#if defined(_BCM96328_) || defined(CONFIG_BCM96328)

static bp_elem_t g_bcm96328avng[] = {
  {bp_cpBoardId,               .u.cp = "96328avng"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_0_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_4_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_8_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_6_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_GPIO_7_AL},
  {bp_usGpioPotsLed,           .u.us = BP_GPIO_5_AH},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm96328avngrP1[] = {
  {bp_cpBoardId,               .u.cp = "96328avngrP1"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SPI_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_11_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm96328avngr[] = {
  {bp_cpBoardId,               .u.cp = "96328avngr"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SPI_EXT_CS |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_10_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_11_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_13_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_15_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_12_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm963281TAN[] = {
  {bp_cpBoardId,               .u.cp = "963281TAN"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_11_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_7_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_4_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_8_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},
  {bp_last}
};

static bp_elem_t g_bcm963281TAN2[] = {
  {bp_cpBoardId,               .u.cp = "963281TAN2"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_11_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_7_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_4_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_8_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_2},
  {bp_last}
};

static bp_elem_t g_bcm963281TAN3[] = {
  {bp_cpBoardId,               .u.cp = "963281TAN3"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_11_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_7_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_4_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_8_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302| BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_2},
  {bp_last}
};

static bp_elem_t g_bcm963281TAN4[] = {
  {bp_cpBoardId,               .u.cp = "963281TAN4"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_11_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_1_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_7_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_4_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_8_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_2},
  {bp_last}
};

static bp_elem_t g_bcm963283_24tstbrd[] = {
  {bp_cpBoardId,               .u.cp = "963283_24tstbrd"},
  {bp_ulGpioOverlay,           .u.ul =( BP_OVERLAY_PCIE_CLKREQ)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_29_AL},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_last}
};

static bp_elem_t g_bcm963293_24sw_mb[] = {
  {bp_cpBoardId,               .u.cp = "963293_24SW_MB"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_29_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_31_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_1},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x03},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_last}
};

static bp_elem_t g_bcm93715rv[] = {
  {bp_cpBoardId,                 .u.cp = "93715rv"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SPI_EXT_CS)},

  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x01},
  {bp_ulPhyId0,                .u.ul = 0x01 | CONNECTED_TO_EPON_MAC},

  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_14_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_12_AL},
  
  {bp_usGpioPassDyingGasp,     .u.us = BP_SERIAL_GPIO_1_AL},
  
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_1},

  {bp_last}
 };


static bp_elem_t * g_BoardParms[] = {g_bcm96328avng, g_bcm96328avngrP1, g_bcm96328avngr, g_bcm963281TAN, g_bcm963281TAN2, g_bcm963281TAN3, g_bcm963281TAN4, g_bcm963283_24tstbrd, g_bcm963293_24sw_mb, g_bcm93715rv, 0};
#endif

#if defined(_BCM963268_) || defined(CONFIG_BCM963268)

static char g_obsoleteStr[] = "(obsolete)";

static bp_elem_t g_bcm963268sv1[] = {
  {bp_cpBoardId,               .u.cp = "963268SV1"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_17_AL},
  {bp_last}
};

static bp_elem_t g_bcm963168mbv_17a[] = {
  {bp_cpBoardId,               .u.cp = "963168MBV_17A"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0}, 
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x5f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII},
  {bp_ulPhyId6,                .u.ul = 0x19 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_ulAfeId1,                .u.ul = BP_AFE_CHIP_6306| BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_21},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_17_AL},
  {bp_usGpioAFELDRelay,      .u.us = BP_GPIO_39_AH},
  {bp_last}
};

static bp_elem_t g_bcm963168mbv_30a[] = {
  {bp_cpBoardId ,              .u.cp = "963168MBV_30A"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0}, 
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x5f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII},
  {bp_ulPhyId6,                .u.ul = 0x19 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_AVMODE_VDSL | BP_AFE_FE_REV_12_21 | BP_AFE_FE_ANNEXA },
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_17_AL},
  {bp_usGpioAFELDRelay,      .u.us = BP_GPIO_39_AH},
  {bp_last}
};

static bp_elem_t g_bcm963168mbv17a302[] = {
  {bp_cpBoardId,               .u.cp = "963168MBV17A302"},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_17_AL},
  {bp_usGpioIntAFELDMode,      .u.us = BP_PIN_DSL_CTRL_5},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_PIN_DSL_CTRL_4},
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_13_AH},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_12_AH},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_11_AL},
  {bp_usGpioAFELDRelay,        .u.us = BP_GPIO_NONE},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm963168mbv_17a},
  {bp_last}
};

static bp_elem_t g_bcm963168mbv30a302[] = {
  {bp_cpBoardId,               .u.cp = "963168MBV30A302"},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_17_AL},
  {bp_usGpioIntAFELDMode,      .u.us = BP_PIN_DSL_CTRL_5},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_PIN_DSL_CTRL_4},
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_13_AH},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_12_AH},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_11_AL},
  {bp_usGpioAFELDRelay,        .u.us = BP_GPIO_NONE},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm963168mbv_30a},
  {bp_last}
};

static bp_elem_t g_bcm963268mbv[] = {
  {bp_cpBoardId,                 .u.cp = "963268MBV"},
  {bp_cpComment,               .u.cp = g_obsoleteStr},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x5f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII},
  {bp_ulPhyId6,                .u.ul = 0x19 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_17_AL},
  {bp_usGpioAFELDRelay,      .u.us = BP_GPIO_39_AH},
  {bp_last}
};

static bp_elem_t g_bcm963168mbv3[] = {
  {bp_cpBoardId ,              .u.cp = "963168MBV3"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioSecLedAdsl,        .u.us = BP_GPIO_17_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6303 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6303_REV_12_3_30 }, 
  {bp_ulAfeId1,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_6303 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6303_REV_12_3_20 }, 
  // LDMode is set to NONE in case the board we are inheriting from set them
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_NONE},  
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_10_AH},
  // IntAFELDClk uses dedicated pin
  // IntAFELDData uses dedicated pin
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_NONE},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_9_AH},
  {bp_usGpioExtAFELDClk,       .u.us = BP_GPIO_8_AL},
  {bp_usGpioExtAFELDData,      .u.us = BP_GPIO_11_AH},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_13_AL},
  {bp_usGpioAFELDRelay,        .u.us = BP_GPIO_NONE},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm963168mbv_30a},
  {bp_last}
};

#if 0
/* Uncomment "#define BP_GET_INT_AFE_DEFINED" in Boardparams.h when these bp_ids are in used:
* bp_usGpioIntAFELDPwr
* bp_usGpioIntAFELDMode
* bp_usGpioAFELDRelay
*/

static bp_elem_t g_bcm963268mbv6b[] = {
  {bp_cpBoardId,               .u.cp = "963168MBV6b"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x5f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18},
  {bp_ulPhyId6,                .u.ul = 0x19},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_12_21},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_17_AL},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_13_AH},
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_12_AH},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_11_AH},
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_10_AH},
  {bp_usGpioAFELDRelay,      .u.us = BP_GPIO_39_AH},
  {bp_last}
};
#endif

static bp_elem_t g_bcm963168vx[] = {
  {bp_cpBoardId,               .u.cp = "963168VX"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_17_AL},
  {bp_last}
};

static bp_elem_t g_bcm963168vx_p300[] = {
  {bp_cpBoardId,               .u.cp = "963168VX_P300"},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm963168vx}, 
  {bp_last}
};

static bp_elem_t g_bcm963168vx_p400[] = {
  {bp_cpBoardId,               .u.cp = "963168VX_P400"},
  {bp_usGpioIntAFELDMode,      .u.us = BP_PIN_DSL_CTRL_5},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_PIN_DSL_CTRL_4},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm963168vx_p300},
  {bp_last}
};

static bp_elem_t g_bcm963168vx_ext1p8[] = {
  {bp_cpBoardId,               .u.cp = "963168VX_ext1p8"},
  {bp_ucVreg1P8,               .u.uc = BP_VREG_EXTERNAL},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm963168vx},
  {bp_last}
};

static bp_elem_t g_bcm963168xf[] = {
  {bp_cpBoardId,               .u.cp = "963168XF"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = MII_DIRECT},  
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_GPIO_14_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_GPIO_15_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_usGpioFemtoReset,        .u.us = BP_GPIO_8_AH},  
  {bp_last}
};

static bp_elem_t g_bcm963268sv2_extswitch[] = {
  {bp_cpBoardId,               .u.cp = "963268SV2_EXTSW"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_HS_SPI_SSB7_EXT_CS)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0xbf},
  {bp_ulPhyId0,                .u.ul = BP_PHY_ID_1},
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_2},
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_3},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_ulPhyId5,                .u.ul = 0x18 | MAC_IF_RGMII},
  {bp_ulPhyId7,                .u.ul = 0x19 | MAC_IF_RGMII},  
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MDIO},
  {bp_ulPortMap,               .u.ul = 0x03},
  {bp_ulPhyId0,                .u.ul = BP_PHY_ID_0 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_1 | CONNECTED_TO_EXTERN_SW},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_usGpioSpiSlaveReset,     .u.us = BP_GPIO_22_AH},
  {bp_usSpiSlaveBusNum,        .u.us = HS_SPI_BUS_NUM},
  {bp_usSpiSlaveSelectNum,     .u.us = 7},
  {bp_usSpiSlaveMode,          .u.us = SPI_MODE_3},
  {bp_ulSpiSlaveCtrlState,     .u.ul = SPI_CONTROLLER_STATE_GATE_CLK_SSOFF},
  {bp_ulSpiSlaveMaxFreq,       .u.ul = 781000},
  {bp_usSpiSlaveProtoRev,      .u.us = 0},   
  {bp_last}
};

static bp_elem_t g_bcm963268bu[] = {
  {bp_cpBoardId,               .u.cp = "963268BU"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  // {bp_usEphyBaseAddress,       .u.us = 0x10},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0xFC},
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_3 | PHY_INTERNAL | PHY_INTEGRATED_VALID },
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4 },
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_0 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},
  {bp_ulPhyId5,                .u.ul = BP_PHY_ID_1 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL },
  {bp_ulPhyId6,                .u.ul = BP_PHY_ID_24 | MAC_IF_RGMII},
  {bp_ulPhyId7,                .u.ul = BP_PHY_ID_25 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_10_AH},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_11_AH},
  {bp_last}
};

static bp_elem_t g_bcm963268bu_p300[] = {
  {bp_cpBoardId,               .u.cp = "963268BU_P300"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_INET_LED |
                                       BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_usEphyBaseAddress,       .u.us = 0x10},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0xF9},
  {bp_ulPhyId0,                .u.ul = BP_PHY_ID_17 | PHY_INTERNAL | PHY_INTEGRATED_VALID },
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4 | PHY_INTERNAL | PHY_INTEGRATED_VALID },
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_0 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},
  {bp_ulPhyId5,                .u.ul = BP_PHY_ID_1 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL },
  {bp_ulPhyId6,                .u.ul = BP_PHY_ID_24 | MAC_IF_RGMII},
  {bp_ulPhyId7,                .u.ul = BP_PHY_ID_25 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_10_AH},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_11_AH},
  {bp_last}
};

static bp_elem_t g_bcm963168xh[] = {
  {bp_cpBoardId,               .u.cp = "963168XH"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_PCIE_CLKREQ |
                                       BP_OVERLAY_HS_SPI_SSB5_EXT_CS)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_13_AH},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_10_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x58},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_ulPhyId6,                .u.ul = BP_PHY_ID_25 | MAC_IF_RGMII},
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_HS_SPI_SSB_5},// Remember to make MDIO HW changes(install resistors R540, R541 and R553) BP_ENET_CONFIG_HS_SPI_SSB_5},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = BP_PHY_ID_0 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_1 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_2 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_3 | CONNECTED_TO_EXTERN_SW},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_AVMODE_VDSL | BP_AFE_FE_REV_12_21 | BP_AFE_FE_ANNEXA },
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_11_AL},  
  {bp_last}
};

static bp_elem_t g_bcm963168xh5[] = {
    {bp_cpBoardId,               .u.cp = "963168XH5"},
    {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
    {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                      BP_OVERLAY_GPHY_LED_0|
                                      BP_OVERLAY_USB_DEVICE|
                                      BP_OVERLAY_USB_LED|
                                      BP_OVERLAY_PCIE_CLKREQ |
                                      BP_OVERLAY_HS_SPI_SSB5_EXT_CS)},
    {bp_usGpioLedAdsl,           .u.us = BP_GPIO_13_AH},
    {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
    {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
    {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
    {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_10_AL},
    {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_3_AL},
    {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
    {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
    {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
    {bp_usWirelessFlags,         .u.us = 0},
    {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
    {bp_ucPhyAddress,            .u.uc = 0x0},
    {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
    {bp_ulPortMap,               .u.ul = 0x58},
    {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
    {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
    {bp_ulPhyId6,                .u.ul = BP_PHY_ID_25 | MAC_IF_RGMII},
    {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
    {bp_ucPhyAddress,            .u.uc = 0x0},
    {bp_usConfigType,            .u.us = BP_ENET_CONFIG_HS_SPI_SSB_5},// Remember to make MDIO HW changes(install resistors R540, R541 and R553) BP_ENET_CONFIG_HS_SPI_SSB_5},
    {bp_ulPortMap,               .u.ul = 0x0f},
    {bp_ulPhyId0,                .u.ul = BP_PHY_ID_0 | CONNECTED_TO_EXTERN_SW},
    {bp_ulPhyId1,                .u.ul = BP_PHY_ID_1 | CONNECTED_TO_EXTERN_SW},
    {bp_ulPhyId2,                .u.ul = BP_PHY_ID_2 | CONNECTED_TO_EXTERN_SW},
    {bp_ulPhyId3,                .u.ul = BP_PHY_ID_3 | CONNECTED_TO_EXTERN_SW},
    {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
    {bp_ucDspAddress,            .u.uc = 0},
    {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
    {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
    {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
    {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_AVMODE_VDSL | BP_AFE_FE_REV_12_21 | BP_AFE_FE_ANNEXA },
    {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_11_AL},  
    {bp_last}
};

  static bp_elem_t g_bcm963168xm[] = {
    {bp_cpBoardId,               .u.cp = "963168XM"},
    {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
    {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                         BP_OVERLAY_USB_DEVICE|
                                         BP_OVERLAY_PCIE_CLKREQ |
                                         BP_OVERLAY_HS_SPI_SSB5_EXT_CS)},
    {bp_usGpioLedAdsl,           .u.us = BP_GPIO_13_AH},
    {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
    {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
    {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
    {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_10_AL},
    {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_3_AL},
    {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
    {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
    {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
    {bp_usWirelessFlags,         .u.us = 0},
    {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
    {bp_ucPhyAddress,            .u.uc = 0x0},
    {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
    {bp_ulPortMap,               .u.ul = 0x58},
    {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
    {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
    {bp_ulPhyId6,                .u.ul = BP_PHY_ID_25},
    {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
    {bp_ucPhyAddress,            .u.uc = 0x0},
    {bp_usConfigType,            .u.us = BP_ENET_CONFIG_HS_SPI_SSB_5},// Remember to make MDIO HW changes(install resistors R540, R541 and R553) BP_ENET_CONFIG_HS_SPI_SSB_5},
    {bp_ulPortMap,               .u.ul = 0x0f},
    {bp_ulPhyId0,                .u.ul = BP_PHY_ID_0 | CONNECTED_TO_EXTERN_SW},
    {bp_ulPhyId1,                .u.ul = BP_PHY_ID_1 | CONNECTED_TO_EXTERN_SW},
    {bp_ulPhyId2,                .u.ul = BP_PHY_ID_2 | CONNECTED_TO_EXTERN_SW},
    {bp_ulPhyId3,                .u.ul = BP_PHY_ID_3 | CONNECTED_TO_EXTERN_SW},
    {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
    {bp_ucDspAddress,            .u.uc = 0},
    {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
    {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
    {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
    {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_ISIL1556 | BP_AFE_FE_AVMODE_VDSL | BP_AFE_FE_REV_12_21 | BP_AFE_FE_ANNEXA },
    {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_11_AL},  
    {bp_last}
  };


  static bp_elem_t g_bcm963168mp[] = {
  {bp_cpBoardId,               .u.cp = "963168MP"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1F},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = MII_DIRECT},
  {bp_usPhyConnType,           .u.us = PHY_CONN_TYPE_PLC},
  {bp_ucPhyDevName,            .u.cp = "plc%d"},
  {bp_ulPortFlags,             .u.ul = PORT_FLAG_SOFT_SWITCHING},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_last}
};

  static bp_elem_t g_bcm963268v30a[] = {
  {bp_cpBoardId,               .u.cp = "963268V30A"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCIE_CLKREQ |
                                       BP_OVERLAY_PHY |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_SERIAL_LEDS )},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN}, // FIXME
  {bp_usWirelessFlags,         .u.us = 0}, // FIXME
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0xD8},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x00 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},
  {bp_ulPhyId6,                .u.ul = 0x18 | MAC_IF_RGMII},
  {bp_ulPhyId7,                .u.ul = 0x19 | MAC_IF_RGMII},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_6306 | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_6302_6306_REV_A_12_40},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_9_AL},  
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_10_AH},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_11_AH},
  {bp_last}
};

  static bp_elem_t g_bcm963168media[] = {
  {bp_cpBoardId,               .u.cp = "963168MEDIA"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PCIE_CLKREQ |
                                       BP_OVERLAY_PHY |                                       
                                       BP_OVERLAY_SERIAL_LEDS )},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_1_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN}, // FIXME
  {bp_usWirelessFlags,         .u.us = 0}, // FIXME
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x5F},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},
  {bp_ulPhyId6,                .u.ul = RGMII_DIRECT},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL}, 
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_10_AH},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_11_AH},
  {bp_usGpioUart2Sdin,         .u.us = BP_GPIO_12_AH},
  {bp_usGpioUart2Sdout,        .u.us = BP_GPIO_13_AH},        
  {bp_last}
};


static bp_elem_t g_bcm963268sv2[] = {
  {bp_cpBoardId,               .u.cp = "963268SV2"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE)},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = BP_PHY_ID_1},
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_2},
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_3},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},
  {bp_usGpioExtAFEReset,       .u.us = BP_GPIO_17_AL},
  {bp_last}
};

static bp_elem_t g_bcm963168xfg3[] = {
  {bp_cpBoardId,               .u.cp = "963168XFG3"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_DEVICE  |
                                       BP_OVERLAY_PHY         |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1F},
  {bp_ulPhyId0,                .u.ul = BP_PHY_ID_1}, 
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_2},  
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_3},         
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
  {bp_ulPhyId4,                .u.ul = MII_DIRECT},  
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioExtAFELDMode,      .u.us = BP_GPIO_10_AH},
  {bp_usGpioExtAFELDPwr,       .u.us = BP_GPIO_11_AH},  
  {bp_usGpioFemtoReset,        .u.us = BP_GPIO_8_AH},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_7_2_30}, 
  {bp_last}
};

static bp_elem_t g_bcm963269bhr[] = {
  {bp_cpBoardId,               .u.cp = "963269BHR"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_HS_SPI_SSB4_EXT_CS |
                                       BP_OVERLAY_HS_SPI_SSB5_EXT_CS |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_0},
  //{bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_NONE},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_42_AH},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_0},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_41_AH},
#if !defined(_CFE_)
  {bp_usMocaType0,             .u.us = BP_MOCA_TYPE_WAN},  //first MoCA always for WAN
  {bp_usMocaRfBand,            .u.us = BP_MOCA_RF_BAND_D_HIGH},
  {bp_usExtIntrMocaHostIntr,   .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_0},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_43_AH},
  {bp_usExtIntrMocaSBIntr0,    .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_3},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_45_AH},
  {bp_usExtIntrMocaSBIntr1,    .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_3},
  {bp_usGpio_Intr,             .u.us = BP_GPIO_44_AH},
  {bp_pMocaInit,               .u.ptr = (void*)moca6802BHRInitSeq},
  {bp_usGpioSpiSlaveReset,     .u.us = BP_GPIO_40_AL},
  {bp_usSpiSlaveBusNum,        .u.us = HS_SPI_BUS_NUM},
  {bp_usSpiSlaveSelectNum,     .u.us = 4},
  {bp_usSpiSlaveMode,          .u.us = SPI_MODE_3},
  {bp_ulSpiSlaveCtrlState,     .u.ul = SPI_CONTROLLER_STATE_GATE_CLK_SSOFF},
  {bp_ulSpiSlaveMaxFreq,       .u.ul = 12500000},
  {bp_usMocaType1,             .u.us = BP_MOCA_TYPE_LAN}, // LAN
  {bp_usMocaRfBand,            .u.us = BP_MOCA_RF_BAND_D_LOW},
  {bp_usExtIntrMocaHostIntr,   .u.us = BP_EXT_INTR_SHARED | BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_0},
  {bp_usExtIntrMocaSBIntr0,    .u.us = BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_1},
  {bp_usExtIntrMocaSBIntr1,    .u.us = BP_EXT_INTR_TYPE_IRQ_HIGH_LEVEL | BP_EXT_INTR_2},
  {bp_pMocaInit,               .u.ptr = (void*)moca6802BHRInitSeq},
  {bp_usGpioSpiSlaveReset,     .u.us = BP_GPIO_39_AL},
  {bp_usSpiSlaveBusNum,        .u.us = HS_SPI_BUS_NUM},
  {bp_usSpiSlaveSelectNum,     .u.us = 5},
  {bp_usSpiSlaveMode,          .u.us = SPI_MODE_3},
  {bp_ulSpiSlaveCtrlState,     .u.ul = SPI_CONTROLLER_STATE_GATE_CLK_SSOFF},
  {bp_ulSpiSlaveMaxFreq,       .u.ul = 12500000},
#endif
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0xd8},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_ulPhyId6,                .u.ul = RGMII_DIRECT},
  {bp_ucPhyDevName,            .u.cp = "moca%d"},
  {bp_ulPhyId7,                .u.ul = RGMII_DIRECT},
  {bp_ucPhyDevName,            .u.cp = "moca%d"},
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_HS_SPI_SSB_1},
  {bp_ulPortMap,               .u.ul = 0x1e},
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_1 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_2 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_3 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_4 | CONNECTED_TO_EXTERN_SW},
  {bp_last}
};

static bp_elem_t g_bcm963168ach5[] = {
  {bp_cpBoardId,               .u.cp = "963168ACH5"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_HS_SPI_SSB5_EXT_CS |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_13_AH},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x18},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = RGMII_DIRECT | EXTSW_CONNECTED},
  {bp_ucPhyType1,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_HS_SPI_SSB_5},
  {bp_ulPortMap,               .u.ul = 0x1e},
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_1 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_2 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_3 | CONNECTED_TO_EXTERN_SW},
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_4 | CONNECTED_TO_EXTERN_SW},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_11_AH},
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_10_AH},
  {bp_last}
};

static bp_elem_t g_bcm963168ac5[] = {
  {bp_cpBoardId,               .u.cp = "963168AC5"},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_PIN_DSL_CTRL_4},
  {bp_usGpioIntAFELDMode,      .u.us = BP_PIN_DSL_CTRL_5},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm963168ach5},
  {bp_last}
};

static bp_elem_t g_bcm963168xn5[] = {
  {bp_cpBoardId,               .u.cp = "963168XN5"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_SERIAL_LEDS |
                                       BP_OVERLAY_USB_LED |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_PCIE_CLKREQ)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_13_AH},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = BP_PHY_ID_1},
  {bp_ulPhyId1,                .u.ul = BP_PHY_ID_2},
  {bp_ulPhyId2,                .u.ul = BP_PHY_ID_3},
  {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_24 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_usGpioIntAFELDPwr,       .u.us = BP_PIN_DSL_CTRL_5},
  {bp_usGpioIntAFELDMode,      .u.us = BP_PIN_DSL_CTRL_4},
  {bp_last}
};

static bp_elem_t g_bcm963168xm5[] = {
    {bp_cpBoardId,               .u.cp = "963168XM5"},
    {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC},
    {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                         BP_OVERLAY_USB_DEVICE  |
                                         BP_OVERLAY_USB_LED     |
                                         BP_OVERLAY_EPHY_LED_0  |
                                         BP_OVERLAY_EPHY_LED_1  |
                                         BP_OVERLAY_EPHY_LED_2  |
                                         BP_OVERLAY_GPHY_LED_0  |
                                         BP_OVERLAY_PCIE_CLKREQ )},
    {bp_usGpioLedAdsl,           .u.us = BP_GPIO_20_AL},
    {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
    {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
    {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
    {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_18_AL},
    {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_3_AL},
    {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
    {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
    {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
    {bp_usWirelessFlags,         .u.us = 0},
    {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
    {bp_ucPhyAddress,            .u.uc = 0x0},
    {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
    {bp_ulPortMap,               .u.ul = 0x1f},
    {bp_ulPhyId0,                .u.ul = BP_PHY_ID_1},
    {bp_ulPhyId1,                .u.ul = BP_PHY_ID_2},
    {bp_ulPhyId2,                .u.ul = BP_PHY_ID_3},
    {bp_ulPhyId3,                .u.ul = BP_PHY_ID_4},
    {bp_ulPhyId4,                .u.ul = 0x18 | PHY_INTEGRATED_VALID | PHY_EXTERNAL | MAC_IF_RGMII},
    {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
    {bp_ucDspAddress,            .u.uc = 0},
    {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
    {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
    {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
    {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_4},
    {bp_last}
};

static bp_elem_t g_bcm963168xm5_6302[] = {
    {bp_cpBoardId,               .u.cp = "963168XM5_6302"},
    {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6302_REV_5_2_3},
    {bp_elemTemplate,            .u.bp_elemp = g_bcm963168xm5},
    {bp_last}
};

static bp_elem_t g_bcm963168wfar[] = {
  {bp_cpBoardId,               .u.cp = "963168WFAR"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC | BP_DEVICE_OPTION_DISABLE_LED_INVERSION},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_SERIAL_LEDS |
                                        BP_OVERLAY_GPHY_LED_0 |
                                        BP_OVERLAY_PCIE_CLKREQ )},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_3_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_SERIAL_GPIO_7_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_2_AL},
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_20_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_21_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0}, 
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},  
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x08},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},
  {bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},
  {bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30},
  {bp_last}
};

#ifdef SKY

//Murthy: Added New board for  SKY VDSL router
static bp_elem_t g_bcm963168SkyVdsl[] = {
	{bp_cpBoardId,               .u.cp = "BSKYB_63168"},
	/* Murthy: 28/03/2013: Fix for  SR-828 and SR-855
	   merging option BP_DEVICE_OPTION_DISABLE_LED_INVERSION from 4.12.08  is causing  inconsistant LED behaviours
	   1. Ethernet port Green LED stays on
	   2. Power LED goes out during soft reboot

	   Reverted back this option and have raised the issue with Broadcom for further information
	   */
//    {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_DISABLE_LED_INVERSION}, /* Murthy This option is added as part on 4.12.08 merge, check for any side effects */
	/*{bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC },*/ /* NJP removed not for use with FE interface */
	{bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
			/*BP_OVERLAY_SERIAL_LEDS |*/ /* NJP removed as serial shifted LED output not used */
			/*BP_OVERLAY_USB_DEVICE |*/ /* NJP removed as USB not used */
			/*BP_OVERLAY_USB_LED |*/ /* NJP removed as USB LED not used */
			BP_OVERLAY_EPHY_LED_0 | /* NJP added */
			BP_OVERLAY_EPHY_LED_1 | /* NJP added */
			BP_OVERLAY_EPHY_LED_2 | /* NJP added */
			BP_OVERLAY_GPHY_LED_0 ) }, /* NJP added */
	/*BP_OVERLAY_PCIE_CLKREQ)},*/ /* NJP removed as PCIe Clock Request signal not used */
	{bp_usGpioLedAdsl,           .u.us = BP_GPIO_5_AH}, /* ADSL Connection LED */
	{bp_usGpioLedSesWireless,    .u.us = BP_GPIO_3_AL}, /* WPS LED control over Wireless LED */
        {bp_usGpioLedSesWirelessFail,    .u.us = BP_GPIO_3_AL},  /* WPS LED control over Wireless */
	{bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH}, /* Internet OK Indicator LED */
	{bp_usGpioLedWanError,       .u.us = BP_GPIO_4_AL}, /* Internet Error LED */
	{bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_6_AH}, /* Power LED BP_GPIO_35_AH */
	{bp_usGpioLedBlStop,         .u.us = BP_GPIO_2_AL}, /* Power LED Orange */
{bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
{bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
{bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
{bp_usWirelessFlags,         .u.us = 0},
{bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
{bp_ucPhyAddress,            .u.uc = 0x0},
{bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
{bp_ulPortMap,               .u.ul = 0x0f},
{bp_ulPhyId0,                .u.ul = 0x01},
{bp_ulPhyId1,                .u.ul = 0x02},
{bp_ulPhyId2,                .u.ul = 0x03},
{bp_ulPhyId3,                .u.ul = 0x04 | FORCE_LINK_100FD},
	/*{bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},*/ /* NJP removed as only for 63168 VOIP applications */
	/*{bp_ucDspAddress,            .u.uc = 0},*/ /* NJP removed as only for 63168 VOIP applications */
	/*{bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},*/ /* NJP removed as not required */
	/*{bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},*/ /* NJP removed as not required */
	/*{bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},*/ /* NJP removed as not required */
	/*{bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},*/ /* NJP removed - wrong AFE_ID */
{bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30}, /* NJP added - correct AFE_ID */
{bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_24_AH}, /* NJP added */ 
{bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_25_AH},
{bp_usGpioLedWireless,       .u.us = BP_GPIO_8_AH}, /* Wireless LED BP_GPIO_23_AH */
{bp_usGpioLedskyHD,          .u.us = BP_GPIO_18_AH}, /* Sky HD LED */
{bp_last}
};



#define MAC_IFACE_VALID_M        1
#define MAC_IFACE_VALID_S        26
#define MAC_IFACE_VALID          (MAC_IFACE_VALID_M << MAC_IFACE_VALID_S)

static bp_elem_t g_bcm963168bskyb_jester[] = {
  {bp_cpBoardId,               .u.cp = "BSKYB_JESTER"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC },
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       /*BP_OVERLAY_SERIAL_LEDS |*/ /* NJP removed as serial shifted LED output not used */
                                       /* BP_OVERLAY_USB_DEVICE | */ /* NJP removed as USB not used */
                                       /*BP_OVERLAY_USB_LED |*/ /* NJP removed as USB LED not used */
                                       BP_OVERLAY_EPHY_LED_0 | /* NJP added */
                                       BP_OVERLAY_EPHY_LED_1 | /* NJP added */
                                       BP_OVERLAY_EPHY_LED_2 | /* NJP added */
                                       BP_OVERLAY_GPHY_LED_0 | /* NJP added */
                                       BP_OVERLAY_PCIE_CLKREQ )}, /* NJP added as PCIe Clock Request required */
	{bp_usGpioLedAdsl,           .u.us = BP_GPIO_5_AH}, /* ADSL Connection LED */
	{bp_usGpioLedSesWireless,    .u.us = BP_GPIO_3_AL}, /* WPS LED control over Wireless LED */
        {bp_usGpioLedSesWirelessFail,    .u.us = BP_GPIO_3_AL},  /* WPS LED control over Wireless */
	{bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AH}, /* Internet OK Indicator LED */
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_4_AL}, /* DSL Red */
	{bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_6_AH}, /* Power LED BP_GPIO_35_AH */
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_2_AL}, /* Power RED */
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x5e}, /* EPHY2 (10/100, EPHY3 (10/100), GPHY1 (Internal GbE Phy), RGMII_1 (External GbE Phy), RGMII_3 (External PLC Phy)*/
//  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ucPhyDevName,            .u.cp = "eth1"},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ucPhyDevName,            .u.cp = "eth0"},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucPhyDevName,            .u.cp = "eth2"},
//  {bp_ulPhyId4,                .u.ul = MII_DIRECT},
//  {bp_ucPhyDevName,            .u.cp = "eth4"},
//  {bp_usPhyConnType,           .u.us = PHY_CONN_TYPE_PLC},
  {bp_ulPhyId6,                .u.ul = 0x19 | MAC_IF_RGMII},	/* GW MII3 -> B50612E GbE PHY  */
  {bp_ucPhyDevName,            .u.cp = "eth3"},
  /*{bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},*/ /* NJP removed as only for 63168 VOIP applications */
  /*{bp_ucDspAddress,            .u.uc = 0},*/ /* NJP removed as only for 63168 VOIP applications */
  /*{bp_usGpioVoip1Led,          .u.us = BP_SERIAL_GPIO_4_AL},*/ /* NJP removed as not required */
  /*{bp_usGpioVoip2Led,          .u.us = BP_SERIAL_GPIO_5_AL},*/ /* NJP removed as not required */
  /*{bp_usGpioPotsLed,           .u.us = BP_SERIAL_GPIO_6_AL},*/ /* NJP removed as not required */
  /*{bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV1},*/ /* NJP removed - wrong AFE_ID */
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6302 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6302_REV_7_2_30}, /* NJP added - correct AFE_ID */
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_24_AH}, /* NJP added */  
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_25_AH},
  {bp_usGpioLedWirelessFail,  .u.us = BP_GPIO_6_AL}, /* Wireless Red */  
{bp_usGpioLedWireless,       .u.us = BP_GPIO_8_AH}, /* Wireless LED BP_GPIO_23_AH */
  {bp_usGpioLedskyHDFail,     .u.us = BP_GPIO_25_AL}, /* Sky HD Red */
{bp_usGpioLedskyHD,          .u.us = BP_GPIO_18_AH}, /* Sky HD LED */
  {bp_last}
};

/* GW/NJP added */
static bp_elem_t g_bcm963168bskyb_viper[] = {
  {bp_cpBoardId,               .u.cp = "BSKYB_VIPER"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC },
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_PCIE_CLKREQ )},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,			   .u.ul = 0x58}, /* GPHY1, RGMII_1 (External PLC), RGMII_3 (External Phy GbE) */
  {bp_ulPhyId3,                .u.ul = 0x04},
  // 06/03/14 - MRB - ER-108: Modification of PLC board parameters for Qualcomm AR7420.
  // The Qualcomm PLC chip is interfaced via an MII bus attached to port 4 of the
  // BCM63268 internal switch, a.k.a. RGMII_1. It is configured to behave as a PHY
  // device and, obviously, it is external. 
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_16 | /* Qualcomm AR7420 is PHY_ID 0x10 - Tony Bricknell */
                                       MAC_IFACE_VALID |
                                       MAC_IF_MII |
                                       PHY_EXTERNAL |
                                       MII_DIRECT /* Was RGMII_DIRECT */ /*PHY_INTEGRATED_VALID*/},
  {bp_usPhyConnType,           .u.us = PHY_CONN_TYPE_EXT_PHY /* Was PHY_CONN_TYPE_PLC */},
  {bp_ucPhyDevName,            .u.cp = "plc0"},
  {bp_ulPhyId6,                .u.ul = 0x19 | MAC_IF_RGMII}, /* GW MII3 -> B50612E GbE PHY  */
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6303 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6303_REV_12_3_30 }, /* NJP added for new 6303 LD */
  // LDMode is set to NONE in case the board we are inheriting from set them
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_NONE}, /* NJP added for new 6303 LD */ 
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_10_AH}, /* NJP added for new 6303 LD */
  // IntAFELDClk uses dedicated pin
  // IntAFELDData uses dedicated pin
  {bp_usGpioPLCPwrEn,          .u.us = BP_GPIO_16_AL},/* PLC power on/off GPIO */
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_2_AL}, /* Power Red */  
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_3_AL}, /* Power Green */
  {bp_usGpioLedAdslFail,       .u.us = BP_GPIO_4_AL}, /* DSL Red */
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_5_AL}, /* DSL Green */
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_4_AL}, /* DSL Red */ 
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AL}, /* DSL Green */
  {bp_usGpioLedWirelessFail,   .u.us = BP_GPIO_6_AL}, /* Wireless Red */  
  {bp_usGpioLedWireless,	   .u.us = BP_GPIO_7_AL}, /* Wireless Green */
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_14_AL}, /* WPS Green LED */ 
  {bp_usGpioLedSesWirelessFail,.u.us = BP_GPIO_15_AL}, /* WPS Red LED */ 
  {bp_usGpioLedskyHDFail,	   .u.us = BP_GPIO_8_AL}, /* Sky HD Red */
  {bp_usGpioLedskyHD,		   .u.us = BP_GPIO_9_AL}, /* Sky HD Green */
  {bp_last}
};

/* SN - 29/04/2014 - VIPER EXTENDER BoardParams */
static bp_elem_t g_bcm963168bskyb_viper_extender[] = {
  {bp_cpBoardId,               .u.cp = "BSKYB_VIPER_EXT"},
  {bp_ulDeviceOptions,         .u.ul = BP_DEVICE_OPTION_ENABLE_GMAC },
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_PHY |
                                       BP_OVERLAY_USB_DEVICE |
                                       BP_OVERLAY_GPHY_LED_0 |
                                       BP_OVERLAY_PCIE_CLKREQ )},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_0},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,			   .u.ul = 0x18}, /* GPHY1, RGMII_1 (External PLC) */
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = BP_PHY_ID_16 | /* Qualcomm AR7420 is PHY_ID 0x10 - Tony Bricknell */
                                       MAC_IFACE_VALID |
                                       MAC_IF_MII |
                                       PHY_EXTERNAL |
                                       MII_DIRECT /* Was RGMII_DIRECT */ /*PHY_INTEGRATED_VALID*/},
  {bp_usPhyConnType,           .u.us = PHY_CONN_TYPE_EXT_PHY /* Was PHY_CONN_TYPE_PLC */},
  {bp_ucPhyDevName,            .u.cp = "plc0"},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6303 | BP_AFE_FE_ANNEXA | BP_AFE_FE_REV_6303_REV_12_3_30 }, /* NJP added for new 6303 LD */
  // LDMode is set to NONE in case the board we are inheriting from set them
  {bp_usGpioIntAFELDMode,      .u.us = BP_GPIO_NONE}, /* NJP added for new 6303 LD */ 
  {bp_usGpioIntAFELDPwr,       .u.us = BP_GPIO_10_AH}, /* NJP added for new 6303 LD */
  // IntAFELDClk uses dedicated pin
  // IntAFELDData uses dedicated pin
  {bp_usGpioPLCPwrEn,          .u.us = BP_GPIO_16_AL},/* PLC power on/off GPIO */
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_2_AL}, /* Power Red */  
  {bp_usGpioLedBlPowerOn,      .u.us = BP_GPIO_3_AL}, /* Power Green */
  {bp_usGpioLedAdslFail,       .u.us = BP_GPIO_4_AL}, /* DSL Red */
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_5_AL}, /* DSL Green */
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_4_AL}, /* DSL Red */ 
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_5_AL}, /* DSL Green */
  {bp_usGpioLedWirelessFail,   .u.us = BP_GPIO_6_AL}, /* Wireless Red */  
  {bp_usGpioLedWireless,	   .u.us = BP_GPIO_7_AL}, /* Wireless Green */
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_14_AL}, /* WPS Green LED */ 
  {bp_usGpioLedSesWirelessFail,.u.us = BP_GPIO_15_AL}, /* WPS Red LED */ 
  {bp_usGpioLedskyHDFail,	   .u.us = BP_GPIO_8_AL}, /* Sky HD Red */
  {bp_usGpioLedskyHD,		   .u.us = BP_GPIO_9_AL}, /* Sky HD Green */
  {bp_last}
};
#endif


static bp_elem_t * g_BoardParms[] = {g_bcm963268sv1, g_bcm963268mbv, g_bcm963168vx, g_bcm963168vx_p300, g_bcm963268bu, g_bcm963268bu_p300,
		g_bcm963268sv2_extswitch, g_bcm963168mbv_17a, g_bcm963168mbv_30a, g_bcm963168xh, g_bcm963168xh5, g_bcm963168mp, g_bcm963268v30a,
		g_bcm963168media, g_bcm963268sv2, g_bcm963168xfg3, g_bcm963168xf, g_bcm963168xm, g_bcm963168mbv3, g_bcm963168mbv17a302, g_bcm963168mbv30a302,
		g_bcm963168vx_p400, g_bcm963168vx_ext1p8, g_bcm963269bhr, g_bcm963168ach5, g_bcm963168ac5, g_bcm963168xn5, g_bcm963168xm5, g_bcm963168xm5_6302, g_bcm963168wfar, 
#ifdef SKY
    g_bcm963168SkyVdsl,	
   	g_bcm963168bskyb_jester,
   	g_bcm963168bskyb_viper,
    g_bcm963168bskyb_viper_extender,
#endif
		0};

#endif


#if defined(_BCM96318_) || defined(CONFIG_BCM96318)

static bp_elem_t g_bcm96318sv[] = {
  {bp_cpBoardId,               .u.cp = "96318SV"},
  {bp_usGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 )},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_10_AL},
 // {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_12_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_11_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_3},
  {bp_last}
};

static bp_elem_t g_bcm96318sv_sersw[] = {
  {bp_cpBoardId,               .u.cp = "96318SV_SERSW"},
  {bp_ulGpioOverlay,           .u.ul = (BP_OVERLAY_SERIAL_LEDS)},
  {bp_usSerialLEDMuxSel,       .u.us = (BP_SERIAL_MUX_SEL_GROUP0|BP_SERIAL_MUX_SEL_GROUP2)},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_10_AL},
 // {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_12_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_11_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_3},
  {bp_last}
};

static bp_elem_t g_bcm96318sv_serled[] = {
  {bp_cpBoardId,               .u.cp = "96318SV_SERLED"},
  {bp_ulGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_SERIAL_LEDS)},
  {bp_usGpioLedAdsl,           .u.us = BP_SERIAL_GPIO_10_AL},
 // {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_12_AL},
  {bp_usGpioLedWanData,        .u.us = BP_SERIAL_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_SERIAL_GPIO_9_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_SERIAL_GPIO_11_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_1},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x1f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ulPhyId4,                .u.ul = 0x18 | MAC_IF_RGMII | PHY_INTEGRATED_VALID | PHY_EXTERNAL},  
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_3},
  {bp_last}
};

static bp_elem_t g_bcm96318ref[] = {
  {bp_cpBoardId,               .u.cp = "96318REF"},
  {bp_usGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 )},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_10_AL},
 // {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_12_AL},
  {bp_usGpioLedWanData,        .u.us = BP_GPIO_8_AL},
  {bp_usGpioLedWanError,       .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_11_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_1},
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_0},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x0f},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId1,                .u.ul = 0x02},
  {bp_ulPhyId2,                .u.ul = 0x03},
  {bp_ulPhyId3,                .u.ul = 0x04},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_3},
  {bp_last}
};

static bp_elem_t g_bcm96318ref_p300[] = {
  {bp_cpBoardId,               .u.cp = "96318REF_P300"},
  {bp_usGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_EPHY_LED_1 |
                                       BP_OVERLAY_EPHY_LED_2 |
                                       BP_OVERLAY_EPHY_LED_3 |
                                       BP_OVERLAY_PCIE_CLKREQ )}, /* PCIE CLK req use GPIO 10 input so make sure GPIO 10 is not used when this flag is set. The linux kernel setup code check this condition */
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_17_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_16_AL},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96318ref},
  {bp_last}
};

static bp_elem_t g_bcm96319plc[] = {
  {bp_cpBoardId,               .u.cp = "96319PLC"},
  {bp_usGpioOverlay,           .u.ul =(BP_OVERLAY_EPHY_LED_0 |
                                       BP_OVERLAY_PCIE_CLKREQ )},
  {bp_usGpioPLCPwrEn,          .u.us = BP_GPIO_8_AH},
  {bp_usGpioPLCReset,          .u.us = BP_GPIO_9_AL},
  {bp_usGpioLedSesWireless,    .u.us = BP_GPIO_12_AL},
  {bp_usExtIntrResetToDefault, .u.us = BP_EXT_INTR_1},  //use reset to def irq for now instead of standby irq
  {bp_usExtIntrSesBtnWireless, .u.us = BP_EXT_INTR_0},
  {bp_usAntInUseWireless,      .u.us = BP_WLAN_ANT_MAIN},
  {bp_usWirelessFlags,         .u.us = 0},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x11},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ulPhyId4,                .u.ul = MII_DIRECT},
  {bp_usPhyConnType,           .u.us = PHY_CONN_TYPE_PLC},
  {bp_ucPhyDevName,            .u.cp = "plc%d"},
  {bp_usVregAvsMin,            .u.us = 950},
  {bp_last}
};

static bp_elem_t g_bcm96319plc_ac[] = {
  {bp_cpBoardId,               .u.cp = "96319PLC_AC"},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96319plc},
  {bp_last}
};

static bp_elem_t g_bcm96318plc[] = {
  {bp_cpBoardId,               .u.cp = "96318PLC"},
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_7_AL},
  {bp_usGpioLedBlStop,         .u.us = BP_GPIO_11_AL},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_3},
  {bp_elemTemplate,            .u.bp_elemp = g_bcm96319plc},
  {bp_last}
};

static bp_elem_t g_bcm96318tiny[] = {
  {bp_cpBoardId,               .u.cp = "96318TINY"},
  {bp_usGpioOverlay,           .u.ul = BP_OVERLAY_EPHY_LED_0 },
  {bp_usGpioLedAdsl,           .u.us = BP_GPIO_10_AL},
  {bp_ucPhyType0,              .u.uc = BP_ENET_EXTERNAL_SWITCH},
  {bp_ucPhyAddress,            .u.uc = 0x0},
  {bp_usConfigType,            .u.us = BP_ENET_CONFIG_MMAP},
  {bp_ulPortMap,               .u.ul = 0x01},
  {bp_ulPhyId0,                .u.ul = 0x01},
  {bp_ucDspType0,              .u.uc = BP_VOIP_MIPS},
  {bp_ucDspAddress,            .u.uc = 0},
  {bp_ulAfeId0,                .u.ul = BP_AFE_CHIP_INT | BP_AFE_LD_6301 | BP_AFE_FE_ANNEXA | BP_AFE_FE_AVMODE_ADSL | BP_AFE_FE_REV_6301_REV_5_1_3},
  {bp_last}
};


static bp_elem_t * g_BoardParms[] = {g_bcm96318sv, g_bcm96318ref, g_bcm96318ref_p300, g_bcm96318sv_serled, g_bcm96318sv_sersw, g_bcm96319plc, g_bcm96319plc_ac, g_bcm96318plc, g_bcm96318tiny, 0};
#endif

#if defined(_BCM963381_) || defined(CONFIG_BCM963381)
static bp_elem_t g_bcm963381sv[] = {
  {bp_cpBoardId,               .u.cp = "963381SV"},
  {bp_last}
};

static bp_elem_t * g_BoardParms[] = {g_bcm963381sv, 0};
#endif

static bp_elem_t * g_pCurrentBp = 0;


/* Private function prototypes */
bp_elem_t * BpGetElem(enum bp_id id, bp_elem_t **pstartElem, enum bp_id stopAtId);
char *BpGetSubCp(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
void *BpGetSubPtr(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
unsigned char BpGetSubUc(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
unsigned short BpGetSubUs(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
unsigned long BpGetSubUl(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId );
int BpGetCp(enum bp_id id, char **pcpValue );
int BpGetUc(enum bp_id id, unsigned char *pucValue );
int BpGetUs(enum bp_id id, unsigned short *pusValue );
int BpGetUl(enum bp_id id, unsigned long *pulValue );
int BpGetGpio(enum bp_id id, unsigned short *pusValue );
int BpEnumUs(enum bp_id id, int* token, unsigned short *pusValue);
int BpGetExtIntrGpio(enum bp_id id, unsigned short *pusValue);

/**************************************************************************
* Name       : bpstrcmp
*
* Description: String compare for this file so it does not depend on an OS.
*              (Linux kernel and CFE share this source file.)
*
* Parameters : [IN] dest - destination string
*              [IN] src - source string
*
* Returns    : -1 - dest < src, 1 - dest > src, 0 dest == src
***************************************************************************/
int bpstrcmp(const char *dest,const char *src)
{
    while (*src && *dest) {
        if (*dest < *src) return -1;
        if (*dest > *src) return 1;
        dest++;
        src++;
    }

    if (*dest && !*src) return 1;
    if (!*dest && *src) return -1;
    return 0;
} /* bpstrcmp */


/**************************************************************************
* Name       : BpGetElem
*
* Description: Private function to walk through the profile
*              and find the desired entry
*
* Parameters : [IN] id             - id to search for
*              [IN/OUT] pstartElem - where to start and where it was found
*              [IN] stopAtId       - id to stop at if the searched id is not found
*                                    (allows grouping and repeated ids)
*
* Returns    : ptr to entry found or to last entry otherwise
***************************************************************************/
bp_elem_t * BpGetElem(enum bp_id id, bp_elem_t **pstartElem, enum bp_id stopAtId)
{
    bp_elem_t * pelem;
    
    // when compiling CFE, it does not like 'NULL' hence using 0
    if ( 0 == *pstartElem )
        *pstartElem = g_pCurrentBp;

    for (pelem = *pstartElem; 
         pelem->id != bp_last && pelem->id != id && pelem->id != stopAtId; 
         pelem++, (*pstartElem)++) 
    {
        // found template so jump to it
        // any entries after bp_elemTemplate will be ignored
        if ( bp_elemTemplate == pelem->id ) {
            *pstartElem = pelem->u.bp_elemp;
            pelem = *pstartElem;
            // ignoring the first element of this new array
            // because it is always bp_cpBoardId
        }
    }

    return pelem; 
}

/**************************************************************************
* Name       : BpGetSubCp
*
* Description: Private function to get an char * entry from the profile
*              can be used to search an id within a group by specifying stop id
*
* Parameters : [IN] id         - id to search for
*              [IN] pstartElem - where to start from
*              [IN] stopAtId   - id to stop at if the searched id is not found
*                                (allows grouping and repeated ids)
*
* Returns    : the unsigned char * from the entry
***************************************************************************/
char *BpGetSubCp(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId )
{
    bp_elem_t *pelem;

    pelem = BpGetElem(id, &pstartElem, stopAtId);
    if (id == pelem->id) { 
        return pelem->u.cp;
    } else { 
        return (char *)BP_NOT_DEFINED;
    }
}

/**************************************************************************
* Name       : BpGetSubPtr
*
* Description: Private function to get an void * entry from the profile
*              can be used to search an id within a group by specifying stop id
*
* Parameters : [IN] id         - id to search for
*              [IN] pstartElem - where to start from
*              [IN] stopAtId   - id to stop at if the searched id is not found
*                                (allows grouping and repeated ids)
*
* Returns    : the unsigned char * from the entry
***************************************************************************/
void *BpGetSubPtr(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId )
{
    bp_elem_t *pelem;

    pelem = BpGetElem(id, &pstartElem, stopAtId);
    if (id == pelem->id) {
        return pelem->u.ptr;
    } else {
        return (void *)0;
    }
}

/**************************************************************************
* Name       : BpGetSubUc
*
* Description: Private function to get an unsigned char entry from the profile
*              can be used to search an id within a group by specifying stop id
*
* Parameters : [IN] id         - id to search for
*              [IN] pstartElem - where to start from
*              [IN] stopAtId   - id to stop at if the searched id is not found
*                                (allows grouping and repeated ids)
*
* Returns    : the unsigned char from the entry
***************************************************************************/
unsigned char BpGetSubUc(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId )
{
    bp_elem_t *pelem;

    pelem = BpGetElem(id, &pstartElem, stopAtId);
    if (id == pelem->id) {
        return pelem->u.uc;
    } else {
        return (unsigned char)BP_NOT_DEFINED;
    }
}

/**************************************************************************
* Name       : BpGetSubUs
*
* Description: Private function to get an unsigned short entry from the profile
*              can be used to search an id within a group by specifying stop id
*
* Parameters : [IN] id         - id to search for
*              [IN] pstartElem - where to start from
*              [IN] stopAtId   - id to stop at if the searched id is not found
*                                (allows grouping and repeated ids)
*
* Returns    : the unsigned short from the entry
***************************************************************************/
unsigned short BpGetSubUs(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId )
{
    bp_elem_t *pelem;

    pelem = BpGetElem(id, &pstartElem, stopAtId);
    if (id == pelem->id) {
        return pelem->u.us;
    } else {
        return BP_NOT_DEFINED;
    }
}

/**************************************************************************
* Name       : BpGetSubUl
*
* Description: Private function to get an unsigned long entry from the profile
*              can be used to search an id within a group by specifying stop id
*
* Parameters : [IN] id         - id to search for
*              [IN] pstartElem - where to start from
*              [IN] stopAtId   - id to stop at if the searched id is not found
*                                (allows grouping and repeated ids)
*
* Returns    : the unsigned long from the entry
***************************************************************************/
unsigned long BpGetSubUl(enum bp_id id, bp_elem_t *pstartElem, enum bp_id stopAtId )
{
    bp_elem_t *pelem;

    pelem = BpGetElem(id, &pstartElem, stopAtId);
    if (id == pelem->id)
        return pelem->u.ul;
    else
        return BP_NOT_DEFINED;
}

/**************************************************************************
* Name       : BpGetCp
*
* Description: Private function to get an char * entry from the profile
*              can only be used to search an id which appears once in the profile
*
* Parameters : [IN] id       - id to search for
*              [IN] pulValue - char ** found
*
* Returns    : BP_SUCCESS or appropriate error
***************************************************************************/
int BpGetCp(enum bp_id id, char **pcpValue )
{
    int nRet;

    if( g_pCurrentBp ) {
        *pcpValue = BpGetSubCp(id, 0, bp_last);
        nRet = ((char *)BP_NOT_DEFINED != *pcpValue ? BP_SUCCESS : BP_VALUE_NOT_DEFINED);
    } else {     
        *pcpValue = (char *)BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
}

/**************************************************************************
* Name       : BpGetUc
*
* Description: Private function to get an unsigned char entry from the profile
*              can only be used to search an id which appears once in the profile
*
* Parameters : [IN] id       - id to search for
*              [IN] pucValue - unsigned char found
*
* Returns    : BP_SUCCESS or appropriate error
***************************************************************************/
int BpGetUc(enum bp_id id, unsigned char *pucValue )
{
    int nRet;

    if( g_pCurrentBp ) {
        *pucValue = BpGetSubUc(id, 0, bp_last);
        nRet = ((unsigned char)BP_NOT_DEFINED != *pucValue ? BP_SUCCESS : BP_VALUE_NOT_DEFINED);
    } else {
        *pucValue = (unsigned char)BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
}

/**************************************************************************
* Name       : BpGetUs
*
* Description: Private function to get an unsigned short entry from the profile
*              can only be used to search an id which appears once in the profile
*
* Parameters : [IN] id       - id to search for
*              [IN] pusValue - unsigned short found
*
* Returns    : BP_SUCCESS or appropriate error
***************************************************************************/
int BpGetUs(enum bp_id id, unsigned short *pusValue )
{
    int nRet;

    if( g_pCurrentBp ) {
        *pusValue = BpGetSubUs(id, 0, bp_last);
        nRet = (BP_NOT_DEFINED != *pusValue ? BP_SUCCESS : BP_VALUE_NOT_DEFINED);
    } else {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
}

/**************************************************************************
* Name       : BpGetUl
*
* Description: Private function to get an unsigned long entry from the profile
*              can only be used to search an id which appears once in the profile
*
* Parameters : [IN] id       - id to search for
*              [IN] pulValue - unsigned long found
*
* Returns    : BP_SUCCESS or appropriate error
***************************************************************************/
int BpGetUl(enum bp_id id, unsigned long *pulValue )
{
    int nRet;

    if( g_pCurrentBp ) {
        *pulValue = BpGetSubUl(id, 0, bp_last);
        nRet = (BP_NOT_DEFINED != *pulValue ? BP_SUCCESS : BP_VALUE_NOT_DEFINED);
    } else {
        *pulValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
}

/**************************************************************************
* Name       : BpEnumUs
*
* Description: Enumerate the board parameters for all the instance of Us boardparms id 
*
* Parameters : [IN] id - boardparm id to enumerate.
*              [IN] token - transparent to caller. Set a pointer to NULL when first time call
*              [OUT] pusValue - value of the id
*
* Returns    : BP_SUCCESS - Success, value is returned in pusValue
*              BP_SUCCESS_LAST - Last entry reached, no value is returened in pusValue
*              BP_VALUE_NOT_DEFINED - this Led id is not defined in the boardparm
***************************************************************************/
int BpEnumUs(enum bp_id id, int* token, unsigned short *pusValue)
{
    bp_elem_t *pelem, *pnext;
    int rc;

    pelem = (bp_elem_t*)(*token);
    pnext = BpGetElem(id, &pelem, bp_last);
    if (id == pnext->id) 
    {
        *pusValue =  pnext->u.us;
        pnext++;
        *token = (int)pnext;
        rc = BP_SUCCESS;
    }
    else 
    {
        if( *token == 0 )
        {
	    /* no such bp item at all */   
	    *pusValue = BP_NOT_DEFINED;
            rc = BP_VALUE_NOT_DEFINED;
	}
        else
        {
            /* previous instant exists but we reach to the end... */   
	    *pusValue = BP_NOT_DEFINED;
            rc = BP_SUCCESS_LAST;
	}
    }

    return rc;
}

/**************************************************************************
* Name       : BpGetVoipDspConfig
*
* Description: Gets the DSP configuration from the board parameter
*              structure for a given DSP index.
*
* Parameters : [IN] dspNum - DSP index (number)
*
***************************************************************************/
VOIP_DSP_INFO g_VoIPDspInfo[BP_MAX_VOIP_DSP] = {{0}};
VOIP_DSP_INFO *BpGetVoipDspConfig( unsigned char dspNum )
{
    VOIP_DSP_INFO *pDspConfig = 0;
    int i;
    bp_elem_t *pelem;
    bp_elem_t *pDspType;
    enum bp_id bp_aucDspType[BP_MAX_VOIP_DSP+1] = {bp_ucDspType0, bp_ucDspType1, bp_last};
    enum bp_id bp_current, bp_next;

    if( g_pCurrentBp ) {
        /* First initialize the structure to known values */
        for( i = 0 ; i < BP_MAX_VOIP_DSP ; i++ ) {
            g_VoIPDspInfo[i].ucDspType = BP_VOIP_NO_DSP;
        }

        /* Now populate it with what we have in the element array */
        for( i = 0 ; i < BP_MAX_VOIP_DSP ; i++ ) {
            pDspType = 0;
            bp_current = bp_aucDspType[i];
            bp_next    = bp_aucDspType[i+1];
            pelem = BpGetElem(bp_current, &pDspType, bp_next);
            if (bp_current != pelem->id) {
                continue;
            }
            g_VoIPDspInfo[i].ucDspType = pelem->u.uc;

            ++pDspType;
            g_VoIPDspInfo[i].ucDspAddress       = BpGetSubUc(bp_ucDspAddress, pDspType, bp_next);
            g_VoIPDspInfo[i].usGpioLedVoip      = BpGetSubUs(bp_usGpioLedVoip, pDspType, bp_next);
            g_VoIPDspInfo[i].usGpioVoip1Led     = BpGetSubUs(bp_usGpioVoip1Led, pDspType, bp_next);
            g_VoIPDspInfo[i].usGpioVoip1LedFail = BpGetSubUs(bp_usGpioVoip1LedFail, pDspType, bp_next);
            g_VoIPDspInfo[i].usGpioVoip2Led     = BpGetSubUs(bp_usGpioVoip2Led, pDspType, bp_next);
            g_VoIPDspInfo[i].usGpioVoip2LedFail = BpGetSubUs(bp_usGpioVoip2LedFail, pDspType, bp_next);
            g_VoIPDspInfo[i].usGpioPotsLed      = BpGetSubUs(bp_usGpioPotsLed, pDspType, bp_next);
            g_VoIPDspInfo[i].usGpioDectLed      = BpGetSubUs(bp_usGpioDectLed, pDspType, bp_next);
        }

        /* Transfer the requested results */
        for( i = 0 ; i < BP_MAX_VOIP_DSP ; i++ ) {
            if( g_VoIPDspInfo[i].ucDspType != BP_VOIP_NO_DSP &&
                g_VoIPDspInfo[i].ucDspAddress == dspNum ) {
                pDspConfig = &g_VoIPDspInfo[i];
                break;
            }
        }
    }

    return pDspConfig;
} /* BpGetVoipDspConfig */

/**************************************************************************
* Name       : BpSetBoardId
*
* Description: This function find the BOARD_PARAMETERS structure for the
*              specified board id string and assigns it to a global, static
*              variable.
*
* Parameters : [IN] pszBoardId - Board id string that is saved into NVRAM.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_FOUND - Error, board id input string does not
*                  have a board parameters configuration record.
***************************************************************************/
int BpSetBoardId( char *pszBoardId )
{
    int nRet = BP_BOARD_ID_NOT_FOUND;
    bp_elem_t **ppcBp;

    for( ppcBp = g_BoardParms; *ppcBp; ppcBp++ ) {
        if( !bpstrcmp((*ppcBp)[0].u.cp, pszBoardId) ) {
            g_pCurrentBp = *ppcBp;
            nRet = BP_SUCCESS;
            break;
        }
    }

    return( nRet );
} /* BpSetBoardId */

/**************************************************************************
* Name       : BpGetBoardId
*
* Description: This function returns the current board id strings.
*
* Parameters : [OUT] pszBoardIds - Address of a buffer that the board id
*                  string is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
***************************************************************************/
int BpGetBoardId( char *pszBoardId )
{
    int i;

    if (g_pCurrentBp == 0) {
        return -1;
    }

    for (i = 0; i < BP_BOARD_ID_LEN; i++) {
        pszBoardId[i] = g_pCurrentBp[0].u.cp[i];
    }

    return 0;
}

/**************************************************************************
* Name       : BpGetBoardIdNameByIndex
*
* Description: This function returns the pointer to the board id name indexed by i. 
*
* Parameters : [OUT] Address of a buffer that the board id
*                    string is returned in.
*              [IN]  Index of the board in the g_BoardParms array.            
*
* Returns    : BP_SUCCESS - Success, value is returned.
***************************************************************************/
char * BpGetBoardIdNameByIndex( int i )
{
    return g_BoardParms[i][0].u.cp;
}


/**************************************************************************
* Name       : BpGetBoardIds
*
* Description: This function returns all of the supported board id strings.
*
* Parameters : [OUT] pszBoardIds - Address of a buffer that the board id
*                  strings are returned in.  Each id starts at BP_BOARD_ID_LEN
*                  boundary.
*              [IN] nBoardIdsSize - Number of BP_BOARD_ID_LEN elements that
*                  were allocated in pszBoardIds.
*
* Returns    : Number of board id strings returned.
***************************************************************************/
int BpGetBoardIds( char *pszBoardIds, int nBoardIdsSize )
{
    int i;
    char *src;
    char *dest;
    bp_elem_t **ppcBp;

    for( i = 0, ppcBp = g_BoardParms; *ppcBp && nBoardIdsSize;
        i++, ppcBp++, nBoardIdsSize--, pszBoardIds += BP_BOARD_ID_LEN ) {
        dest = pszBoardIds;
        src = (*ppcBp)[0].u.cp;
        while( *src ) {
            *dest++ = *src++;
        }
        *dest = '\0';
    }

    return( i );
} /* BpGetBoardIds */

int BPGetNumBoardIds(void)
{
    int numElements = sizeof(g_BoardParms) / sizeof(bp_elem_t *);
    return ( numElements - 1);
}


/**************************************************************************
* Name       : BpGetComment
*
* Description: This function returns is used to get a comment for a board.
*
* Parameters : [OUT] pcpValue - comment string.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetComment( char **pcpValue )
{
    return( BpGetCp(bp_cpComment, pcpValue ) );
} /* BpGetComment */

/**************************************************************************
* Name       : BpGetGPIOverlays
*
* Description: This function GPIO overlay configuration
*
* Parameters : [OUT] pusValue - Address of short word that interfaces in use.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetGPIOverlays( unsigned long *pulValue )
{
    return( BpGetUl(bp_ulGpioOverlay, pulValue ) );
} /* BpGetGPIOverlays */

/**************************************************************************
* Name       : BpGetEthernetMacInfo
*
* Description: This function returns all of the supported board id strings.
*
* Parameters : [OUT] pEnetInfos - Address of an array of ETHERNET_MAC_INFO
*                  buffers.
*              [IN] nNumEnetInfos - Number of ETHERNET_MAC_INFO elements that
*                  are pointed to by pEnetInfos.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
***************************************************************************/
int BpGetEthernetMacInfo( PETHERNET_MAC_INFO pEnetInfos, int nNumEnetInfos )
{
    int i, j;
    bp_elem_t *pelem;
    bp_elem_t *pPhyType;
    bp_elem_t *pPhyId;
    int nRet = BP_BOARD_ID_NOT_SET;
    PETHERNET_MAC_INFO pE;
    enum bp_id bp_aucPhyType[BP_MAX_ENET_MACS+1] = {bp_ucPhyType0, bp_ucPhyType1, bp_last};
    enum bp_id bp_current, bp_next;
    enum bp_id bp_aulPhyId[BP_MAX_SWITCH_PORTS+1] = {bp_ulPhyId0, bp_ulPhyId1, bp_ulPhyId2,
                bp_ulPhyId3, bp_ulPhyId4, bp_ulPhyId5, bp_ulPhyId6, bp_ulPhyId7, bp_last};
    enum bp_id bp_current_phyid;

    /* First initialize the structure to known values */
    for( i = 0, pE = pEnetInfos; i < nNumEnetInfos; i++, pE++ ) {
        pE->ucPhyType = BP_ENET_NO_PHY;
        /* The old code only initialized the first set, let's tdo the same so the 2 compare without error */
        if (0 == i) {
	  for( j = 0; j < BP_MAX_SWITCH_PORTS; j++ ) {
                pE->sw.ledInfo[j].duplexLed = BP_NOT_DEFINED;
                pE->sw.ledInfo[j].speedLed100 = BP_NOT_DEFINED;
                pE->sw.ledInfo[j].speedLed1000 = BP_NOT_DEFINED;
                pE->sw.ledInfo[j].LedLan = BP_NOT_DEFINED;
            }
        }
    }

    if( g_pCurrentBp ) {
        /* Populate it with what we have in the element array */
        for( i = 0, pE = pEnetInfos; i < nNumEnetInfos; i++, pE++ ) {
            pPhyType = 0;
            bp_current = bp_aucPhyType[i];
            bp_next    = bp_aucPhyType[i+1];
            pelem = BpGetElem(bp_current, &pPhyType, bp_next);
            if (bp_current != pelem->id)
                continue;
            pE->ucPhyType = pelem->u.uc;

            ++pPhyType;
            pE->ucPhyAddress  = BpGetSubUc(bp_ucPhyAddress, pPhyType, bp_next);
            pE->usConfigType  = BpGetSubUs(bp_usConfigType, pPhyType, bp_next);
            pE->sw.port_map   = BpGetSubUl(bp_ulPortMap, pPhyType, bp_next);

            for( j = 0; j < BP_MAX_SWITCH_PORTS; j++ ) {
                pPhyId = pPhyType;
                bp_current_phyid = bp_aulPhyId[j];
                pelem = BpGetElem(bp_current_phyid, &pPhyId, bp_next);
                pE->sw.phyconn[j] = PHY_CONN_TYPE_NOT_DEFINED;
                pE->sw.phy_devName[j] = PHY_DEVNAME_NOT_DEFINED;
                pE->sw.phyinit[j] = (bp_mdio_init_t* )0;
                pE->sw.port_flags[j] = 0;
                pE->sw.ledInfo[j].duplexLed = BP_GPIO_NONE;
                pE->sw.ledInfo[j].LedLan = BP_GPIO_NONE;
                pE->sw.ledInfo[j].speedLed100 = BP_GPIO_NONE;
                pE->sw.ledInfo[j].speedLed1000 = BP_GPIO_NONE;
                pE->sw.phyReset[j] = BP_GPIO_NONE;
                if (bp_current_phyid == pelem->id) {
                    pE->sw.phy_id[j] = pelem->u.ul & ~MII_OVER_GPIO_VALID;
                    ++pPhyId;
                    while (pPhyId) {
                        switch (pPhyId->id) {
                        case bp_usDuplexLed:
                            pE->sw.ledInfo[j].duplexLed = pPhyId->u.us;
                            ++pPhyId;
                            break;
                        case bp_usSpeedLed100:
                            pE->sw.ledInfo[j].speedLed100 = pPhyId->u.us;
                            ++pPhyId;
                            break;
                        case bp_usSpeedLed1000:
                            pE->sw.ledInfo[j].speedLed1000 = pPhyId->u.us;
                            ++pPhyId;
                            break;
                        case bp_usPhyConnType:
                            pE->sw.phyconn[j] = pPhyId->u.us;
                            ++pPhyId;
                            break;
                        case bp_usGpioLedLan:
                            pE->sw.ledInfo[j].LedLan = pPhyId->u.us;
                            ++pPhyId;
                            break;
                        case bp_ucPhyDevName:
                            pE->sw.phy_devName[j] = pPhyId->u.cp;
                            ++pPhyId;
                            break;                            
                        case bp_pPhyInit:
                             pE->sw.phyinit[j] = (bp_mdio_init_t*)pPhyId->u.ptr;
                             ++pPhyId;
                            break;
                        case bp_ulPortFlags:
                             pE->sw.port_flags[j] = pPhyId->u.ul;
                             ++pPhyId;
                            break;
                        case bp_usGpioPhyReset:
                             pE->sw.phyReset[j] = pPhyId->u.us;
                             ++pPhyId;
                            break;
                        default:
                            pPhyId = 0;
                            break;
                        }
                    }
                 } else {
                    pE->sw.phy_id[j] = 0;
                }
            }
        }
        nRet = BP_SUCCESS;
    }
    return( nRet );

} /* BpGetEthernetMacInfo */

static int intToExtSwPort = -2;
int BpGetPortConnectedToExtSwitch(void)
{
    unsigned long phy_id, port_map;

    if (intToExtSwPort != -2)
    {
        return intToExtSwPort;
    }

    BpGetUl(bp_ulPortMap, &port_map);
    for (intToExtSwPort = 0; intToExtSwPort < BP_MAX_SWITCH_PORTS; intToExtSwPort++) {
        if ( port_map &  (1 << intToExtSwPort)) {
            BpGetUl(bp_ulPhyId0 + intToExtSwPort, &phy_id);
            if (phy_id & EXTSW_CONNECTED)
            {
                break;
            }
        }
            }

    if (intToExtSwPort == BP_MAX_SWITCH_PORTS)
    {
        intToExtSwPort = -1;
        }

    return intToExtSwPort;
    }

/**************************************************************************
* Name       : BpGetMiiOverGpioFlag
*
* Description: This function returns logical disjunction of MII over GPIO
*              flag over all PHY IDs.
*
* Parameters : [OUT] pMiiOverGpioFlag - MII over GPIO flag
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
***************************************************************************/
int BpGetMiiOverGpioFlag( unsigned long* pMiiOverGpioFlag )
{
    int i, j;
    bp_elem_t *pelem;
    bp_elem_t *pPhyType;
    bp_elem_t *pPhyId;
    int nRet;
    enum bp_id bp_aucPhyType[BP_MAX_ENET_MACS+1] = {bp_ucPhyType0, bp_ucPhyType1, bp_last};
    enum bp_id bp_current, bp_next;
    enum bp_id bp_aulPhyId[BP_MAX_SWITCH_PORTS+1] = {bp_ulPhyId0, bp_ulPhyId1, bp_ulPhyId2,
                bp_ulPhyId3, bp_ulPhyId4, bp_ulPhyId5, bp_ulPhyId6, bp_ulPhyId7, bp_last};
    enum bp_id bp_current_phyid;

    *pMiiOverGpioFlag = 0;

    if( g_pCurrentBp ) {
        for( i = 0; i < BP_MAX_ENET_MACS; i++ ) {
            pPhyType = 0;
            bp_current = bp_aucPhyType[i];
            bp_next    = bp_aucPhyType[i+1];
            pelem = BpGetElem(bp_current, &pPhyType, bp_next);
            if (bp_current != pelem->id)
                continue;

            ++pPhyType;
            for( j = 0; j < BP_MAX_SWITCH_PORTS; j++ ) {
                pPhyId = pPhyType;
                bp_current_phyid = bp_aulPhyId[j];
                pelem = BpGetElem(bp_current_phyid, &pPhyId, bp_next);
                if (bp_current_phyid == pelem->id) {
                    *pMiiOverGpioFlag |= pelem->u.ul & MII_OVER_GPIO_VALID;                
                    ++pPhyId;
                }
            }
        }
        // Normalize flag value by positioning in lsb position        
        *pMiiOverGpioFlag >>= MII_OVER_GPIO_S;
        nRet = BP_SUCCESS;
    }
    else
    {
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );

} /* BpGetMiiOverGpioFlag */


/**************************************************************************
* Name       : BpGetGpio
*
* Description: Wrapper function to get the Gpio pin number
*
* Parameters : [IN] id       - id to search for
*              [IN] pusValue - unsigned short found
*
* Returns    : BP_SUCCESS or appropriate error
***************************************************************************/
int BpGetGpio(enum bp_id id, unsigned short *pusValue )
{
    int nRet;

    nRet = BpGetUs(id, pusValue );
    if(nRet == BP_SUCCESS && *pusValue == BP_GPIO_NONE)
    {
    	*pusValue = BP_NOT_DEFINED;
    	nRet = BP_VALUE_NOT_DEFINED;
    }

    return( nRet );
}

/**************************************************************************
* Name       : BpGetExtIntrGpio
*
* Description: Wrapper function to get the Gpio pin number for shared external
*              interrupt
*
* Parameters : [IN] id       - id for the external interrupt. The bp_usGpio_Intr
*                              id must follow this id
*              [IN] pusValue - unsigned short found
*
* Returns    : BP_SUCCESS or appropriate error
***************************************************************************/
int  BpGetExtIntrGpio(enum bp_id id, unsigned short *pusValue)
{
    bp_elem_t *pelem;
    int ret;
    bp_elem_t *start_elem = 0;

    /* First go to elem with interrupt id */
    pelem = BpGetElem(id,  &start_elem, bp_last);
    if (id != pelem->id)
    {
        *pusValue = BP_NOT_DEFINED;
        ret = BP_VALUE_NOT_DEFINED;
    }
    else
    {
        pelem++;
        /* We should expect the id bp_usGpio_Intr in the next element */
        if (bp_usGpio_Intr != pelem->id)
        {
            *pusValue = BP_NOT_DEFINED;
            ret = BP_VALUE_NOT_DEFINED;
        }
        else
        {
            *pusValue = pelem->u.us;
            ret = BP_SUCCESS;
        }
    }

    return ret;
}

/**************************************************************************
* Name       : BpGetLedPinMuxGpio
*
* Description: Wrapper function to get the Gpio pin number for LED parallel
*              connected to the LED controller
*
* Parameters : [IN] idx       - index of the LED in the led table
*               
*              [IN] pusValue - unsigned short found
*
* Returns    : BP_SUCCESS or appropriate error
***************************************************************************/
int  BpGetLedPinMuxGpio(int idx, unsigned short *pusValue)
{
    bp_elem_t *pelem;
    int ret;
    bp_elem_t *start_elem = 0;
    enum bp_id id = bpLedList[idx];

    /* First go to elem with LED id */
    pelem = BpGetElem(id,  &start_elem, bp_last);
    if (id != pelem->id)
    {
        *pusValue = BP_NOT_DEFINED;
        ret = BP_VALUE_NOT_DEFINED;
    }
    else
    {
        pelem++;
        /* We should expect the id bp_usPinMux in the next element */
        if (bp_usPinMux != pelem->id)
        {
            *pusValue = BP_NOT_DEFINED;
            ret = BP_VALUE_NOT_DEFINED;
        }
        else
        {
            *pusValue = pelem->u.us;
            ret = BP_SUCCESS;
        }
    }

    return ret;
}


/**************************************************************************
* Name       : BpGetRj11InnerOuterPairGpios
*
* Description: This function returns the GPIO pin assignments for changing
*              between the RJ11 inner pair and RJ11 outer pair.
*
* Parameters : [OUT] pusInner - Address of short word that the RJ11 inner pair
*                  GPIO pin is returned in.
*              [OUT] pusOuter - Address of short word that the RJ11 outer pair
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, values are returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetRj11InnerOuterPairGpios( unsigned short *pusInner,
                                 unsigned short *pusOuter )
{
    *pusInner = BP_NOT_DEFINED;
    *pusOuter = BP_NOT_DEFINED;

    return( BP_VALUE_NOT_DEFINED );
} /* BpGetRj11InnerOuterPairGpios */

/**************************************************************************
* Name       : BpGetUartRtsCtsGpios
*
* Description: This function returns the GPIO pin assignments for RTS and CTS
*              UART signals.
*
* Parameters : [OUT] pusRts - Address of short word that the UART RTS GPIO
*                  pin is returned in.
*              [OUT] pusCts - Address of short word that the UART CTS GPIO
*                  pin is returned in.
*
* Returns    : BP_SUCCESS - Success, values are returned.
*              BP_BOARD_ID_NOT_SET - Error, board id input string does not
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetRtsCtsUartGpios( unsigned short *pusRts, unsigned short *pusCts )
{
    *pusRts = BP_NOT_DEFINED;
    *pusCts = BP_NOT_DEFINED;

    return( BP_VALUE_NOT_DEFINED );
} /* BpGetUartRtsCtsGpios */

/**************************************************************************
* Name       : BpGetAdslLedGpio
*
* Description: This function returns the GPIO pin assignment for the ADSL
*              LED.
*
* Parameters : [OUT] pusValue - Address of short word that the ADSL LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetAdslLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedAdsl, pusValue ) );
} /* BpGetAdslLedGpio */

/**************************************************************************
* Name       : BpGetAdslFailLedGpio
*
* Description: This function returns the GPIO pin assignment for the ADSL
*              LED that is used when there is a DSL connection failure.
*
* Parameters : [OUT] pusValue - Address of short word that the ADSL LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetAdslFailLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedAdslFail, pusValue ) );
} /* BpGetAdslFailLedGpio */

/**************************************************************************
* Name       : BpGetSecAdslLedGpio
*
* Description: This function returns the GPIO pin assignment for the ADSL
*              LED of the Secondary line, applicable more for bonding.
*
* Parameters : [OUT] pusValue - Address of short word that the ADSL LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSecAdslLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioSecLedAdsl, pusValue ) );
} /* BpGetSecAdslLedGpio */

/**************************************************************************
* Name       : BpGetSecAdslFailLedGpio
*
* Description: This function returns the GPIO pin assignment for the ADSL
*              LED of the Secondary ADSL line, that is used when there is
*              a DSL connection failure.
*
* Parameters : [OUT] pusValue - Address of short word that the ADSL LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSecAdslFailLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioSecLedAdslFail, pusValue ) );
} /* BpGetSecAdslFailLedGpio */

/**************************************************************************
* Name       : BpGetWirelessAntInUse
*
* Description: This function returns the antennas in use for wireless
*
* Parameters : [OUT] pusValue - Address of short word that the Wireless Antenna
*                  is in use.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWirelessAntInUse( unsigned short *pusValue )
{
    return( BpGetUs(bp_usAntInUseWireless, pusValue ) );
} /* BpGetWirelessAntInUse */

/**************************************************************************
* Name       : BpGetWirelessFlags
*
* Description: This function returns optional control flags for wireless
*
* Parameters : [OUT] pusValue - Address of short word control flags
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWirelessFlags( unsigned short *pusValue )
{
    return( BpGetUs(bp_usWirelessFlags, pusValue ) );
} /* BpGetWirelessAntInUse */

/**************************************************************************
* Name       : BpGetWirelessSesExtIntr
*
* Description: This function returns the external interrupt number for the
*              Wireless Ses Button.
*
* Parameters : [OUT] pusValue - Address of short word that the Wireless Ses
*                  external interrup is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWirelessSesExtIntr( unsigned short *pusValue )
{
    return( BpGetUs(bp_usExtIntrSesBtnWireless, pusValue ) );
} /* BpGetWirelessSesExtIntr */

/**************************************************************************
* Name       : BpGetWirelessSesExtIntrGpio
*
* Description: This function returns the external interrupt number for the
*              Wireless Ses Button.
*
* Parameters : [OUT] pusValue - Address of short word that the Wireless Ses
*                  external interrup is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWirelessSesExtIntrGpio( unsigned short *pusValue )
{
    int ret;

    if( !g_pCurrentBp ) 
    {
        *pusValue = BP_NOT_DEFINED;
        ret       = BP_BOARD_ID_NOT_SET;
        return ret;
    }
    
    ret = BpGetExtIntrGpio(bp_usExtIntrSesBtnWireless, pusValue);
    
    return( ret );  
} /* BpGetWirelessSesExtIntrGpio */


/**************************************************************************
* Name       : BpGetWirelessSesLedGpio
*
* Description: This function returns the GPIO pin assignment for the Wireless
*              Ses Led.
*
* Parameters : [OUT] pusValue - Address of short word that the Wireless Ses
*                  Led GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWirelessSesLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedSesWireless, pusValue ) );
} /* BpGetWirelessSesLedGpio */

#if !defined(_CFE_)
/**************************************************************************
* Name       : BpGetMocaInfo
*
* Description: This function returns all of information about the moca chips.
*
* Parameters : [OUT] pMocaInfos - Address of an array of BP_MOCA_INFO
*                  buffers.
*              [IN, OUT] pNumEntry - Set to the maximum Number of BP_MOCA_INFO
*              elements that are pointed to by pEnetInfos and return the actual
*              defined number of moca devices
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
***************************************************************************/
int BpGetMocaInfo( PBP_MOCA_INFO pMocaInfos, int* pNumEntry )
{
    int i, j, k;
    bp_elem_t *pelem;
    bp_elem_t *pMocaType, *pIntId;
    int nRet = BP_BOARD_ID_NOT_SET;
    PBP_MOCA_INFO pMoca;
    enum bp_id bp_ausMocaType[BP_MOCA_MAX_NUM+1] = {bp_usMocaType0, bp_usMocaType1, bp_last};
    enum bp_id bp_current, bp_next;
    enum bp_id bp_ausIntId[BP_MOCA_MAX_INTR_NUM+1] = {bp_usExtIntrMocaHostIntr, bp_usExtIntrMocaSBIntr0, bp_usExtIntrMocaSBIntr1, bp_usExtIntrMocaSBIntr2,
    		bp_usExtIntrMocaSBIntr3, bp_usExtIntrMocaSBIntr4, bp_usExtIntrMocaSBIntrAll, bp_last};
    enum bp_id bp_current_intid;


    if( pMocaInfos == 0 || *pNumEntry == 0 )
    {
    	*pNumEntry = 0;
    	return nRet;
    }

    if( g_pCurrentBp ) {
        /* Populate it with what we have in the element array */
        for( i = 0, j = 0, pMoca = pMocaInfos; i < BP_MOCA_MAX_NUM; i++ ) {
            pMocaType = 0;
            bp_current = bp_ausMocaType[i];
            bp_next    = bp_ausMocaType[i+1];
            pMoca->type = BP_NOT_DEFINED;
            pelem = BpGetElem(bp_current, &pMocaType, bp_next);
            if (bp_current != pelem->id)
                continue;
            pMoca->type = pelem->u.us;
            ++pMocaType;

            pMoca->rfBand   = BpGetSubUs(bp_usMocaRfBand, pMocaType, bp_next);
            pMoca->initCmd  = BpGetSubPtr(bp_pMocaInit, pMocaType, bp_next);
            pMoca->spiDevInfo.resetGpio = BpGetSubUs(bp_usGpioSpiSlaveReset, pMocaType, bp_next);
            pMoca->spiDevInfo.busNum = BpGetSubUs(bp_usSpiSlaveBusNum, pMocaType, bp_next);
            pMoca->spiDevInfo.select = BpGetSubUs(bp_usSpiSlaveSelectNum, pMocaType, bp_next);
            pMoca->spiDevInfo.mode = BpGetSubUs(bp_usSpiSlaveMode, pMocaType, bp_next);
            pMoca->spiDevInfo.ctrlState = BpGetSubUl(bp_ulSpiSlaveCtrlState, pMocaType, bp_next);
            pMoca->spiDevInfo.maxFreq = BpGetSubUl(bp_ulSpiSlaveMaxFreq, pMocaType, bp_next);

            for( k = 0; k < BP_MOCA_MAX_INTR_NUM; k++ ) {
                pIntId = pMocaType;
                bp_current_intid = bp_ausIntId[k];
			    pelem = BpGetElem(bp_current_intid, &pIntId, bp_next);
			    pMoca->intr[k] = BP_NOT_DEFINED;
			    pMoca->intrGpio[k] = BP_NOT_DEFINED;
			    if (bp_current_intid == pelem->id) {
				    pMoca->intr[k] = pelem->u.us;
				    ++pIntId;
				    if( pIntId->id == bp_usGpio_Intr )
				    	 pMoca->intrGpio[k] = pIntId->u.us;
			   }
		    }

            pMoca++;
            j++;
            if( j >= *pNumEntry )
            	break;
        }

        *pNumEntry = j;
        nRet = BP_SUCCESS;
    }
    return( nRet );

} /* BpGetMocaInfo */
#endif
#ifdef SKY

int BpGetwirelessLedGpio( unsigned short *pusValue )
{

   return( BpGetUs(bp_usGpioLedWireless, pusValue ) );

}
/**************************************************************************
* Name       : BpGetWirelessSesErrorLedGpio
*
* Description: This function returns the GPIO pin assignment for the Wireless
*              Ses Error Led.
*
* Parameters : [OUT] pusValue - Address of short word that the Wireless Ses
*                  Led GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
* History : Added for new requirement (Amber blink on wps error)-Nk
***************************************************************************/
int BpGetWirelessSesErrorLedGpio( unsigned short *pusValue )
{
    return( BpGetUs(bp_usGpioLedSesWirelessFail, pusValue ) );
} /* BpGetWirelessSesLedGpio */


int BpGetSkyHdLedGpio ( unsigned short *pusValue )
{
   return( BpGetUs(bp_usGpioLedskyHD, pusValue ) );
}


int BpGetSkyHdErrorLedGpio ( unsigned short *pusValue )
{

   return( BpGetUs(bp_usGpioLedskyHDFail, pusValue ) );
}

int BpGetwirelessErrorLedGpio( unsigned short *pusValue )
{
    return( BpGetUs(bp_usGpioLedWirelessFail, pusValue ) );
} /* BpGetWirelessSesLedGpio */

#endif /* SKY */

#if defined(_BCM96362_) || defined(CONFIG_BCM96362)
/* The unique part contains the subSytemId (boardId) and the BoardRev number */
static WLAN_SROM_ENTRY wlan_patch_unique_96362ADVNX[] = {
    {2,  0x536},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96362ADVNgr2[] = {
    {2,  0x580},
    {65, 0x1500},
    {0, 0}
};


static WLAN_SROM_ENTRY wlan_patch_unique_96362ADVN2xh[] = {
    {2,  0x5a6},
    {65, 0x1200},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96361XF[] = {
    {2,  0x5b8},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96362RAVNGR2[] = {
    {2,  0x60d},
    {65, 0x1500},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96362ADVN2XM[] = {
    {2,  0x60d},
    {65, 0x1500},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96362RADVNXH5[] = {
    {2,  0x63F},
    {48, 0x434f},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96362RADVN2XH[] = {
    {2,  0x63E},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96362RWFAR[] = {
    {2,  0x60d},
    {65, 0x1500},
    {0, 0}
};

/* The common part contains the entries that are valid for multiple boards */
static WLAN_SROM_ENTRY wlan_patch_common_96362ADVNX[] = {
    { 87, 0x0319},
    { 96, 0x2058},
    { 97, 0xfe6f},
    { 98, 0x1785},
    { 99, 0xfa21},
    {112, 0x2058},
    {113, 0xfe77},
    {114, 0x17e0},
    {115, 0xfa16},
    {161, 0x5555},
    {162, 0x5555},
    {169, 0x5555},
    {170, 0x5555},
    {171, 0x5555},
    {172, 0x5555},
    {173, 0x3333},
    {174, 0x3333},
    {175, 0x3333},
    {176, 0x3333},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_96362ADVNgr2[] = {
    { 96, 0x2046},
    {100, 0x3c46},
    {101, 0x4040},    
    {102, 0xFF3C},
    {103, 0x1381},
    {104, 0xFBF1},
    {108, 0xFF09},
    {109, 0x1310},
    {110, 0xFBD6},
    {112, 0x2046},
    {116, 0x3c46},  
    {117, 0x4040},
    {118, 0xFFE9},
    {119, 0x1383},
    {120, 0xFCBC},
    {124, 0xFF3F},
    {125, 0x13C9},
    {126, 0xFBEC},
    {160, 0x3333},
    {161, 0x0000},
    {162, 0x3333},
    {163, 0x0000},
    {164, 0x5555},
    {165, 0x0000},
    {166, 0x2222},
    {167, 0x0000},
    {168, 0x4444},
    {169, 0x3000},
    {170, 0x3333},
    {171, 0x3000},
    {172, 0x3333},
    {177, 0x5000},
    {178, 0x5555},
    {179, 0x5000},
    {180, 0x5555},
    {185, 0x2000},
    {186, 0x2222},
    {187, 0x2000},
    {188, 0x2222},
    {193, 0x4000},
    {194, 0x4444},
    {195, 0x4000},
    {196, 0x4444},
    {201, 0x1111},
    {202, 0x2222},
    {203, 0x6775},  
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_96361XF[] = {
    { 66, 0x0200},
    { 67, 0x0000},
    { 78, 0x0003},
    { 79, 0x0000},
    { 80, 0x0000},
    { 87, 0x0313},
    { 88, 0x0313},
    { 93, 0xffff},
    { 96, 0x2054},
    { 97, 0xfe80},
    { 98, 0x1fa2},
    { 99, 0xf897},
    {112, 0x2054},
    {113, 0xfe8c},
    {114, 0x1f13},
    {115, 0xf8c4},
    {161, 0x4444},
    {162, 0x4444},
    {169, 0x4444},
    {170, 0x4444},
    {171, 0x4444},
    {172, 0x4444},
    {203, 0x2222},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_96362ADVN2xh[] = {
    { 66, 0x3200},
    { 67, 0x8000},
    { 78, 0x0003},
    { 79, 0x0000},
    { 80, 0x0000},
    { 87, 0x031f},
    { 88, 0x031f},
    { 93, 0x0202},
    { 96, 0x2064},
    { 97, 0xfe80},
    { 98, 0x1fa2},
    { 99, 0xf897},
    {112, 0x2064},
    {113, 0xfe8c},
    {114, 0x1f13},
    {115, 0xf8c4},
    {203, 0x2222},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_96362ADVN2XM[] = {
    /* PA params not updated yet */
    { 65, 0x1200},                
    { 66, 0x3200},
    { 67, 0x8000},
    { 78, 0x0003},
    { 79, 0x0000},
    { 80, 0x0000},
    { 87, 0x031f},
    { 88, 0x031f},
    { 93, 0x0202},
    { 96, 0x205c},
    { 97, 0xfe80},
    { 98, 0x1fa2},
    { 99, 0xf897},
    {112, 0x205c},
    {113, 0xfe8c},
    {114, 0x1f13},
    {115, 0xf8c4},
    {203, 0x2222},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_96362RADVNXH5[] = {
    /* PA params not updated yet */
    { 66,  0x3200},
    { 67,  0x8000},
    { 78,  0x0300},
    { 87,  0x031f},
    { 88,  0x033f},
    { 93,  0x0202},
    { 100, 0x4060},
    { 101, 0x6060},
    { 102, 0xFE87},
    { 103, 0x1F06},
    { 104, 0xF8B0},
    { 108, 0xFE88},
    { 109, 0x1EC8},
    { 110, 0xF8C0},
    { 116, 0x4060},
    { 117, 0x6060},
    { 118, 0xFE87},
    { 119, 0x1F06},
    { 120, 0xF8B0},
    { 124, 0xFE8E},
    { 125, 0x1F0B},
    { 126, 0xF8BB},
    { 161, 0x0000},
    { 162, 0x0000},
    { 169, 0x0000},
    { 170, 0x0000},
    { 171, 0x0000},
    { 172, 0x0000},
    { 203, 0x2222},
    {0, 0}
};
#ifdef SKY
static WLAN_SROM_ENTRY wlan_patch_unique_Meerkat[] = {
/* Murthy Taken new values after broadcom 4.12.08 ibntegration
   assuming that it wifi will be calibrated 
    {65, 0x1500},
*/
    {2,  0x580},
    {65, 0x1500},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_Meerkat[] = {
/* Murthy Taken new values after broadcom 4.12.08 ibntegration
   assuming that it wifi will be calibrated 
    { 96, 0x2040},
    { 97, 0xff9a},
    { 98, 0x18e4},
    { 99, 0xfaa0},
    {100, 0x3c3c},
    {101, 0x3c38},    
    {112, 0x2040},
    {113, 0xff8c},
    {114, 0x1804},
    {115, 0xfac2},
    {116, 0x3c3c},  
    {117, 0x3c38},
    {203, 0x2222},   
*/	
	{ 96, 0x2046},
    {100, 0x3c46},
    {101, 0x4040},    
    {102, 0xFF3C},
    {103, 0x1381},
    {104, 0xFBF1},
    {108, 0xFF09},
    {109, 0x1310},
    {110, 0xFBD6},
    {112, 0x2046},
    {116, 0x3c46},  
    {117, 0x4040},
    {118, 0xFFE9},
    {119, 0x1383},
    {120, 0xFCBC},
    {124, 0xFF3F},
    {125, 0x13C9},
    {126, 0xFBEC},
    {160, 0x3333},
    {161, 0x0000},
    {162, 0x3333},
    {163, 0x0000},
    {164, 0x5555},
    {165, 0x0000},
    {166, 0x2222},
    {167, 0x0000},
    {168, 0x4444},
    {169, 0x3000},
    {170, 0x3333},
    {171, 0x3000},
    {172, 0x3333},
    {177, 0x5000},
    {178, 0x5555},
    {179, 0x5000},
    {180, 0x5555},
    {185, 0x2000},
    {186, 0x2222},
    {187, 0x2000},
    {188, 0x2222},
    {193, 0x4000},
    {194, 0x4444},
    {195, 0x4000},
    {196, 0x4444},
    {201, 0x1111},
    {202, 0x2222},
    {203, 0x6775},   
    {0, 0}
};
#endif /* SKY */
#endif

#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
/* The unique part contains the subSytemId (boardId) and the BoardRev number */
static WLAN_SROM_ENTRY wlan_patch_unique_963268MBV[] = {
    {2,  0x5BB},
    {65, 0x1204},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963268V30A[] = {
    {2,  0x5E7},
    {65, 0x1101},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963268BU[] = {
    {2,  0x5A7},
    {65, 0x1201},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963168XH[] = {
    {2,  0x5E2},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963168XM[] = {
    {2,  0x61F},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963168XH5[] = {
    {2,  0x640},
    {48, 0x434f},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963168XN5[] = {
    {2,  0x684},
    {48, 0x434f},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963168XM5[] = {
    {2,  0x685},
    {48, 0x434f},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963168XM5_6302[] = {
    {2,  0x686},
    {48, 0x434f},
    {65, 0x1100},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_963168MP[] = {
    {2,  0x5BB},
    {48, 0x433f},
    {65, 0x1100},
    {0, 0}
};

/* The common part contains the entries that are valid for multiple boards */
static WLAN_SROM_ENTRY wlan_patch_common_963268MBV[] = {
    { 78,  0x0303}, 
#ifndef CONFIG_SKY_ETHAN  
    { 79,  0x0202}, 
#else
	{ 79,  0x0404}, 
#endif    
    { 80,  0xff02},
    { 87,  0x0315}, 
    { 88,  0x0315},
    /* Murthy Broadcom CSP: 751987: Spurious Emissions tests fail*/
    { 94,  0x1e},
    { 96,  0x2048}, 
#ifndef CONFIG_SKY_ETHAN    
    { 97,  0xFFB5}, 
    { 98,  0x175F},
    { 99,  0xFB29},
#else
	{ 97,  0xFF1D}, 
	{ 98,  0x15CF},
	{ 99,  0xFADF},
#endif
    { 100, 0x3E40},
    { 101, 0x403C},
    { 102, 0xFF15},
    { 103, 0x1455},
    { 104, 0xFB95},
    { 108, 0xFF28},
    { 109, 0x1315},
    { 110, 0xFBF0},
    { 112, 0x2048},
#ifndef CONFIG_SKY_ETHAN
    { 113, 0xFFD7},
    { 114, 0x17D6},
    { 115, 0xFB67},
#else    
	{ 113, 0xFF03},
	{ 114, 0x1516},
	{ 115, 0xFAD4},
#endif    
    { 116, 0x3E40},
    { 117, 0x403C},
    { 118, 0xFE87},
    { 119, 0x110F},
    { 120, 0xFB4B},
    { 124, 0xFF8D},
    { 125, 0x1146},
    { 126, 0xFCD6},
#ifdef SKY
#ifdef CONFIG_SKY_ETHAN
    { 160, 0x0000},
#else
    { 160, 0x7777},
#endif // CONFIG_SKY_ETHAN
#endif
    { 161, 0x0000},
    { 162, 0x4444},
    { 163, 0x0000},
    { 164, 0x2222},
    { 165, 0x0000},
    { 166, 0x2222},
    { 167, 0x0000},
    { 168, 0x2222},
    { 169, 0x4000},
    { 170, 0x4444},
    { 171, 0x4000},
    { 172, 0x4444},
    { 177, 0x2000},
    { 178, 0x2222},
    { 179, 0x2000},
    { 180, 0x2222},
    { 185, 0x2000},
    { 186, 0x2222},
    { 187, 0x2000},
    { 188, 0x2222},
    { 193, 0x2000},
    { 194, 0x2222},
    { 195, 0x2000},
    { 196, 0x2222},
    { 201, 0x1111},
    { 202, 0x2222},
    { 203, 0x4446},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_963168XH[] = {
    { 66,  0x3200},
    { 67,  0x8000},
    { 87,  0x031f},
    { 88,  0x031f},
    { 93,  0x0202},
    { 96,  0x2064},
    { 97,  0xfe59},
    { 98,  0x1c81},
    { 99,  0xf911},
    { 112, 0x2064},
    { 113, 0xfe55},
    { 114, 0x1c00},
    { 115, 0xf92c},
    { 161, 0x0000},
    { 162, 0x0000},
    { 169, 0x0000},
    { 170, 0x0000},
    { 171, 0x0000},
    { 172, 0x0000},
    { 203, 0x2222},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_963168XM[] = {
    { 66,  0x3200},
    { 67,  0x8000},
    { 87,  0x031f},
    { 88,  0x031f},
    { 93,  0x0202},
    { 96,  0x205c},
    { 97,  0xfe94},
    { 98,  0x1c92},
    { 99,  0xf948},
    { 112, 0x205c},
    { 113, 0xfe81},
    { 114, 0x1be6},
    { 115, 0xf947},
    { 161, 0x0000},
    { 162, 0x0000},
    { 169, 0x0000},
    { 170, 0x0000},
    { 171, 0x0000},
    { 172, 0x0000},
    { 203, 0x2222},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_963168XH5[] = {
    { 66,  0x3200},
    { 67,  0x8000},
    { 78,  0x0300},
    { 87,  0x031f},
    { 88,  0x033f},
    { 93,  0x0202},
    { 97,  0xffb5},
    { 98,  0x175f},
    { 99,  0xfb29},
    { 100, 0x4060},
    { 101, 0x6060},
    { 102, 0xfee4},
    { 103, 0x1d2c},
    { 104, 0xfa0c},
    { 105, 0xfea2},
    { 106, 0x149a},
    { 107, 0xfafc},
    { 108, 0xfead},
    { 109, 0x1da8},
    { 110, 0xf986},
    { 113, 0xffd7},
    { 114, 0x17d6},
    { 115, 0xfb67},
    { 116, 0x4060},
    { 117, 0x6060},
    { 118, 0xfee4},
    { 119, 0x1d2c},
    { 120, 0xfa0c},
    { 121, 0xfea2},
    { 122, 0x149a},
    { 123, 0xfafc},
    { 124, 0xfead},
    { 125, 0x1da8},
    { 126, 0xf986},
    { 161, 0x0000},
    { 162, 0x0000},
    { 169, 0x0000},
    { 170, 0x0000},
    { 171, 0x0000},
    { 172, 0x0000},
    { 203, 0x2222},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_963168XN5[] = {
    { 66,  0x2200},
    { 67,  0x8000},
    { 78,  0x0300},
    { 87,  0x031f},
    { 88,  0x033f},
    { 96,  0x4054},
    { 97,  0xffb5},
    { 98,  0x175f},
    { 99,  0xfb29},
    { 100,  0x405c},
    { 101,  0x5c5c},
    { 102,  0xfedf},
    { 103,  0x1670},
    { 104,  0xfabf},
    { 105,  0xfeb2},
    { 106,  0x1691},
    { 107,  0xfa6a},
    { 108,  0xfec0},
    { 109,  0x16cd},
    { 110,  0xfaa5},
    { 112,  0x404e},
    { 113,  0xffd7},
    { 114,  0x17d6},
    { 115,  0xfb67},
    { 116,  0x405c},
    { 117,  0x5c5c},
    { 118,  0xfeb2},
    { 119,  0x1691},
    { 120,  0xfa6a},
    { 121,  0xfeb2},
    { 122,  0x1691},
    { 123,  0xfa6a},
    { 124,  0xfe60},
    { 125,  0x1685},
    { 126,  0xfa09},
    { 161,  0x0000},
    { 162,  0x0000},
    { 163,  0x8888},
    { 164,  0x8888},
    { 165,  0x8888},
    { 166,  0x8888},
    { 167,  0x8888},
    { 168,  0x8888},
    { 169,  0x0000},
    { 170,  0x0000},
    { 171,  0x0000},
    { 172,  0x0000},
    { 177,  0x0000},
    { 178,  0x8888},
    { 179,  0x8888},
    { 180,  0x8888},
    { 181,  0x8888},
    { 182,  0x8888},
    { 183,  0x8888},
    { 184,  0x8888},
    { 185,  0x8888},
    { 186,  0x8888},
    { 187,  0x0000},
    { 188,  0x8888},
    { 189,  0x8888},
    { 190,  0x8888},
    { 191,  0x8888},
    { 192,  0x8888},
    { 193,  0x8888},
    { 194,  0x8888},
    { 195,  0x8888},
    { 196,  0x8888},
    { 197,  0x0000},
    { 198,  0x8888},
    { 199,  0x8888},
    { 200,  0x8888},
    { 201,  0x2222},
    { 202,  0x2222},
    { 203,  0x2222},
    { 204,  0x2222},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_963168XM5[] = {
    { 66,  0x3200},
    { 67,  0x8000},
    { 78,  0x0300},
    { 87,  0x031f},
    { 88,  0x033f},
    { 93,  0x0202},
    { 96,  0x405c},
    { 97,  0xffb5},
    { 98,  0x175f},
    { 99,  0xfb29},
    { 100,  0x405c},
    { 101,  0x5c5c},
    { 102,  0xfedd},
    { 103,  0x1a8b},
    { 104,  0xfa70},
    { 105,  0xfea2},
    { 106,  0x149a},
    { 107,  0xfafc},
    { 108,  0xfee7},
    { 109,  0x1a87},
    { 110,  0xfa84},
    { 112,  0x405c},
    { 113,  0xffd7},
    { 114,  0x17d6},
    { 115,  0xfb67},
    { 116,  0x405c},
    { 117,  0x5c5c},
    { 118,  0xfedd},
    { 119,  0x1a8b},
    { 120,  0xfa70},
    { 121,  0xfea2},
    { 122,  0x149a},
    { 123,  0xfafc},
    { 124,  0xfee7},
    { 125,  0x1a87},
    { 126,  0xfa84},
    { 161, 0x0000},
    { 162, 0x0000},
    { 169, 0x0000},
    { 170, 0x0000},
    { 171, 0x0000},
    { 172, 0x0000},
    { 203, 0x1111},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_common_963168MP[] = {
    { 78,  0x0303}, 
    { 79,  0x0202}, 
    { 80,  0xff02},
    { 87,  0x0315}, 
    { 88,  0x0315},
    { 96,  0x2048}, 
    { 97,  0xffb5},
    { 98,  0x168c},
    { 99,  0xfb31},
    { 100, 0x3E40},
    { 101, 0x403C},
    { 102, 0xFF15},
    { 103, 0x1455},
    { 104, 0xFB95},
    { 108, 0xFF28},
    { 109, 0x1315},
    { 110, 0xFBF0},
    { 112, 0x2048},
    { 113, 0xffd7},
    { 114, 0x17b6},
    { 115, 0xfb68},
    { 116, 0x3E40},
    { 117, 0x403C},
    { 118, 0xFE87},
    { 119, 0x110F},
    { 120, 0xFB4B},
    { 124, 0xFF8D},
    { 125, 0x1146},
    { 126, 0xFCD6},
    { 161, 0x0000},
    { 162, 0x4444},
    { 163, 0x0000},
    { 164, 0x2222},
    { 165, 0x0000},
    { 166, 0x2222},
    { 167, 0x0000},
    { 168, 0x2222},
    { 169, 0x4000},
    { 170, 0x4444},
    { 171, 0x4000},
    { 172, 0x4444},
    { 177, 0x2000},
    { 178, 0x2222},
    { 179, 0x2000},
    { 180, 0x2222},
    { 185, 0x2000},
    { 186, 0x2222},
    { 187, 0x2000},
    { 188, 0x2222},
    { 193, 0x2000},
    { 194, 0x2222},
    { 195, 0x2000},
    { 196, 0x2222},
    { 201, 0x1111},
    { 202, 0x2222},
    { 203, 0x4446},
    {0, 0}
};

#endif

#if defined(_BCM96828_) || defined(CONFIG_BCM96828)
/* The unique part contains the subSytemId (boardId) and the BoardRev number */
static WLAN_SROM_ENTRY wlan_patch_unique_96828MBV[] = {
    {65, 0x1230},
    {0, 0}
};

static WLAN_SROM_ENTRY wlan_patch_unique_96828BU[] = {
    {65, 0x1230},
    {0, 0}
};

/* The common part contains the entries that are valid for multiple boards */
static WLAN_SROM_ENTRY wlan_patch_common_96828MBV[] = {
    { 78,  0x0303}, 
    { 79,  0x0202}, 
    { 80,  0xff02},
    { 87,  0x0315}, 
    { 88,  0x0315},
    { 96,  0x2040}, 
    { 97,  0xFFB5}, 
    { 98,  0x175F},
    { 99,  0xFB29},
    { 112, 0x2040},
    { 113, 0xFFD7},
    { 114, 0x17D6},
    { 115, 0xFB67},
    {0, 0}
};
#endif      

static WLAN_SROM_ENTRY wlan_patch_LAST[] = {
    {0, 0}
};

/* this data structure could be moved to boardparams structure in the future */
/* does not require to rebuild cfe here if more srom entries are needed */
static WLAN_SROM_PATCH_INFO wlanPaInfo[]={
#if defined(_BCM96362_) || defined(CONFIG_BCM96362)
    {"96362ADVNX",    0x6362, 220, wlan_patch_unique_96362ADVNX,    wlan_patch_common_96362ADVNX},
    {"96362ADVNgr2",  0x6362, 220, wlan_patch_unique_96362ADVNgr2,  wlan_patch_common_96362ADVNgr2},
    {"96361XF",       0x6362, 220, wlan_patch_unique_96361XF,       wlan_patch_common_96361XF},
    {"96362RAVNGR2",  0x6362, 220, wlan_patch_unique_96362RAVNGR2,  wlan_patch_common_96362ADVNgr2},
    {"96362ADVN2xh",  0x6362, 220, wlan_patch_unique_96362ADVN2xh,  wlan_patch_common_96362ADVN2xh},
    {"96362ADVN2XM",  0x6362, 220, wlan_patch_unique_96362ADVN2XM,  wlan_patch_common_96362ADVN2XM},
    {"96362RADVN2XH", 0x6362, 220, wlan_patch_unique_96362RADVN2XH, wlan_patch_common_96362ADVN2xh},
    {"96362RADVNXH5", 0x6362, 220, wlan_patch_unique_96362RADVNXH5, wlan_patch_common_96362RADVNXH5},
    {"96362RADVNXH503", 0x6362, 220, wlan_patch_unique_96362RADVNXH5, wlan_patch_common_96362RADVNXH5},
    {"96362RWFAR",    0x6362, 220, wlan_patch_unique_96362RWFAR,    wlan_patch_common_96362ADVNgr2},
#ifdef SKY	
    {"Meerkat",  0x6362, 220, wlan_patch_unique_Meerkat,  wlan_patch_common_Meerkat},
#endif /* SKY */
#endif

#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
    {"963268MBV",     0x6362, 220, wlan_patch_unique_963268MBV,     wlan_patch_common_963268MBV},
    {"963168MBV_17A", 0x6362, 220, wlan_patch_unique_963268MBV,     wlan_patch_common_963268MBV},
    {"963168MBV_30A", 0x6362, 220, wlan_patch_unique_963268MBV,     wlan_patch_common_963268MBV},
    {"963168MBV3",    0x6362, 220, wlan_patch_unique_963268MBV,     wlan_patch_common_963268MBV},
    {"963168MBV17A302",0x6362, 220, wlan_patch_unique_963268MBV,     wlan_patch_common_963268MBV},
    {"963168MBV30A302",0x6362, 220, wlan_patch_unique_963268MBV,     wlan_patch_common_963268MBV},
    {"963268V30A",    0x6362, 220, wlan_patch_unique_963268V30A,    wlan_patch_common_963268MBV},
    {"963268BU",      0x6362, 220, wlan_patch_unique_963268BU,      wlan_patch_common_963268MBV},
    {"963268BU_P300", 0x6362, 220, wlan_patch_unique_963268BU,      wlan_patch_common_963268MBV},
    {"963168AC5",    0x6362, 220, wlan_patch_unique_963268BU,      wlan_patch_common_963268MBV},
    {"963168ACH5",    0x6362, 220, wlan_patch_unique_963268BU,      wlan_patch_common_963268MBV},
    {"963168XH",      0x6362, 220, wlan_patch_unique_963168XH,      wlan_patch_common_963168XH},
    {"963168XM",      0x6362, 220, wlan_patch_unique_963168XM,      wlan_patch_common_963168XM},
    {"963168XH5",     0x6362, 220, wlan_patch_unique_963168XH5,     wlan_patch_common_963168XH5},
    {"963168XN5",     0x6362, 220, wlan_patch_unique_963168XN5,     wlan_patch_common_963168XN5},
    {"963168XM5",     0x6362, 220, wlan_patch_unique_963168XM5,     wlan_patch_common_963168XM5},
    {"963168XM5_6302", 0x6362, 220, wlan_patch_unique_963168XM5_6302,  wlan_patch_common_963168XM5},
    {"963168WFAR",    0x6362, 220, wlan_patch_unique_963268MBV,     wlan_patch_common_963268MBV},
    {"963168MP",    0x6362, 220, wlan_patch_unique_963168MP,     wlan_patch_common_963168MP},
#ifdef SKY	
    {"BSKYB_63168",     0x6362, 220, wlan_patch_unique_963268BU, wlan_patch_common_963268MBV}, /* NJP added */
    {"BSKYB_JESTER",    0x6362, 220, wlan_patch_unique_963268BU, wlan_patch_common_963268MBV},
    {"BSKYB_VIPER",     0x6362, 220, wlan_patch_unique_963268BU, wlan_patch_common_963268MBV},
    {"BSKYB_VIPER_EXT", 0x6362, 220, wlan_patch_unique_963268BU, wlan_patch_common_963268MBV},
#endif /* SKY */		
#endif

#if defined(_BCM96828_) || defined(CONFIG_BCM96828)
    {"96828MBV",     0x6362, 220, wlan_patch_unique_96828MBV,       wlan_patch_common_96828MBV},
    {"96828BU",      0x6362, 220, wlan_patch_unique_96828BU,        wlan_patch_common_96828MBV},
#endif
      
    {"", 0, 0, wlan_patch_LAST, wlan_patch_LAST}, /* last entry*/
};

/**************************************************************************
* Name       : BpUpdateWirelessSromMap
*
* Description: This function patch wireless PA values
*
* Parameters : [IN] unsigned short chipID
*              [IN/OUT] unsigned short* pBase - base of srom map
*              [IN/OUT] int size - size of srom map
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpUpdateWirelessSromMap(unsigned short chipID, unsigned short* pBase, int sizeInWords)
{
    int nRet = BP_BOARD_ID_NOT_FOUND;
    int i = 0;
    int j = 0;

    if(chipID == 0 || pBase == 0 || sizeInWords <= 0 )
        return nRet;

    i = 0;
    while ( wlanPaInfo[i].szboardId[0] != 0 ) {
        /* check boardId */
        if ( !bpstrcmp(g_pCurrentBp[0].u.cp, wlanPaInfo[i].szboardId) ) {
            /* check chipId */
            if ( (wlanPaInfo[i].usWirelessChipId == chipID) && (wlanPaInfo[i].usNeededSize <= sizeInWords) ){

                /* valid , patch common to multiple boards entry */
                while ( wlanPaInfo[i].commonEntries[j].wordOffset != 0) {
                    pBase[wlanPaInfo[i].commonEntries[j].wordOffset] = wlanPaInfo[i].commonEntries[j].value;
                    j++;
                }

                j = 0;
                /* valid , patch board specific entry */
                while ( wlanPaInfo[i].uniqueEntries[j].wordOffset != 0) {
                    pBase[wlanPaInfo[i].uniqueEntries[j].wordOffset] = wlanPaInfo[i].uniqueEntries[j].value;
                    j++;
                }

                nRet = BP_SUCCESS;
                goto srom_update_done;
            }
        }
        i++;
    }

srom_update_done:

    return( nRet );

} /* BpUpdateWirelessSromMap */


static WLAN_PCI_PATCH_INFO wlanPciInfo[]={
#if defined(_BCM96362_) || defined(CONFIG_BCM96362)
    /* this is the patch to boardtype(boardid) for internal PA */
    {"96362ADVNX", 0x435f14e4, 64,
    {{"subpciids", 11, 0x53614e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96362ADVNgr2 */
    {"96362ADVNgr2", 0x435f14e4, 64,
    {{"subpciids", 11, 0x58014e4},
    {"",       0,      0}}},
#ifdef SKY
    /* this is the patch to boardtype(boardid) for Meerkat */
    {"Meerkat", 0x435f14e4, 64,
    {{"subpciids", 11, 0x58014e4},
    {"",       0,      0}}},
#endif /* SKY */
    /* this is the patch to boardtype(boardid) for 96362ADVN2xh */
    {"96362ADVN2xh", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5a614e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96361XF */
    {"96361XF", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5b814e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96361I2 */
    {"96361I2", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5e514e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96362RAVNGR2 */
    {"96362RAVNGR2", 0x435f14e4, 64,
    {{"subpciids", 11, 0x60d14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96362ADVN2XM */
    {"96362ADVN2XM", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5d414e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96362RADVNXH5 */
    {"96362RADVNXH5", 0x435f14e4, 64,
    {{"deviceids", 0, 0x434f14e4},
    {"subpciids", 11, 0x63F14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96362RADVNXH503 */
    {"96362RADVNXH503", 0x435f14e4, 64,
    {{"deviceids", 0, 0x434f14e4},
    {"subpciids", 11, 0x63F14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96362RADVN2XH */
    {"96362RADVN2XH", 0x435f14e4, 64,
    {{"subpciids", 11, 0x63E14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 96362RWFAR */
    {"96362RWFAR", 0x435f14e4, 64,
    {{"subpciids", 11, 0x60d14e4},
    {"",       0,      0}}},
#endif
#if defined(_BCM963268_) || defined(CONFIG_BCM963268)
    /* this is the patch to boardtype(boardid) for 63268MBV */
    {"963268MBV", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168MBV_17A */
    {"963168MBV_17A", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168MBV_30A */
    {"963168MBV_30A", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168MBV3 */
    {"963168MBV3", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168MBV17A302 */
    {"963168MBV17A302", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168MBV30A302 */
    {"963168MBV30A302", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963268V30A */
    {"963268V30A", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5E714e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 63268BU */
    {"963268BU", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5A714e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 63168ACH5 */
    {"963168ACH5", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5A714e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168VX */
    {"963168VX", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5A814e4},
    {"",       0,      0}}},
#ifdef SKY
	/* this is the patch to boardtype(boardid) for 963168VX */
	{"BSKYB_63168", 0x435f14e4, 64,
	{{"subpciids", 11, 0x5A814e4},
	{"",	   0,	   0}}},
#endif /* SKY */
    /* this is the patch to boardtype(boardid) for 963168VX_P300 */
    {"963168VX_P300", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5A814e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168VX_P400 */
    {"963168VX_P400", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5A814e4},
    {"",       0,      0}}},    
    /* this is the patch to boardtype(boardid) for 63168XH */
    {"963168XH", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5E214e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 63168XM */
    {"963168XM", 0x435f14e4, 64,
    {{"subpciids", 11, 0x61f14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168XH5 */
    {"963168XH5", 0x435f14e4, 64,
    {{"deviceids", 0, 0x434f14e4},
    {"subpciids", 11, 0x64014e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168XN5 */
    {"963168XN5", 0x435f14e4, 64,
    {{"deviceids", 0, 0x434f14e4},
    {"subpciids", 11, 0x68414e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168XM5 */
    {"963168XM5", 0x435f14e4, 64,
    {{"deviceids", 0, 0x434f14e4},
    {"subpciids", 11, 0x68514e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168XM5_6302 */
    {"963168XM5_6302", 0x435f14e4, 64,
    {{"deviceids", 0, 0x434f14e4},
    {"subpciids", 11, 0x68614e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 963168MP */
    {"963168MP", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
    /* this is the patch to boardtype(boardid) for 63168WFAR */
    {"963168WFAR", 0x435f14e4, 64,
    {{"subpciids", 11, 0x5BB14e4},
    {"",       0,      0}}},
#endif
    {"",                 0, 0, {{"",       0,      0}}}, /* last entry*/
};

/**************************************************************************
* Name       : BpUpdateWirelessPciConfig
*
* Description: This function patch wireless PCI Config Header
*              This is not functional critial/necessary but for dvt database maintenance
*
* Parameters : [IN] unsigned int pciID
*              [IN/OUT] unsigned int* pBase - base of pci config header
*              [IN/OUT] int sizeInDWords - size of pci config header
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpUpdateWirelessPciConfig (unsigned long pciID, unsigned long* pBase, int sizeInDWords)
{
    int nRet = BP_BOARD_ID_NOT_FOUND;
    int i = 0;
    int j = 0;

    if(pciID == 0 || pBase == 0 || sizeInDWords <= 0 )
        return nRet;

    i = 0;
    while ( wlanPciInfo[i].szboardId[0] != 0 ) {
        /* check boardId */
        if ( !bpstrcmp(g_pCurrentBp[0].u.cp, wlanPciInfo[i].szboardId) ) {
            /* check pciId */
            if ( (wlanPciInfo[i].usWirelessPciId == pciID) && (wlanPciInfo[i].usNeededSize <= sizeInDWords) ){
                /* valid , patch entry */
                while ( wlanPciInfo[i].entries[j].name[0] ) {
                    pBase[wlanPciInfo[i].entries[j].dwordOffset] = wlanPciInfo[i].entries[j].value;
                    j++;
                }
                nRet = BP_SUCCESS;
                goto pciconfig_update_done;
            }
        }
        i++;
    }

pciconfig_update_done:

    return( nRet );

}

/**************************************************************************
* Name       : BpGetWanDataLedGpio
*
* Description: This function returns the GPIO pin assignment for the WAN Data
*              LED.
*
* Parameters : [OUT] pusValue - Address of short word that the WAN Data LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWanDataLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedWanData, pusValue ) );
} /* BpGetWanDataLedGpio */

/**************************************************************************
* Name       : BpGetWanErrorLedGpio
*
* Description: This function returns the GPIO pin assignment for the WAN
*              LED that is used when there is a WAN connection failure.
*
* Parameters : [OUT] pusValue - Address of short word that the WAN LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWanErrorLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedWanError, pusValue ) );
} /* BpGetWanErrorLedGpio */

/**************************************************************************
* Name       : BpGetBootloaderPowerOnLedGpio
*
* Description: This function returns the GPIO pin assignment for the power
*              on LED that is set by the bootloader.
*
* Parameters : [OUT] pusValue - Address of short word that the alarm LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetBootloaderPowerOnLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedBlPowerOn, pusValue ) );
} /* BpGetBootloaderPowerOn */

/**************************************************************************
* Name       : BpGetBootloaderStopLedGpio
*
* Description: This function returns the GPIO pin assignment for the break
*              into bootloader LED that is set by the bootloader.
*
* Parameters : [OUT] pusValue - Address of short word that the break into
*                  bootloader LED GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetBootloaderStopLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedBlStop, pusValue ) );
} /* BpGetBootloaderStopLedGpio */

/**************************************************************************
* Name       : BpGetVoipLedGpio
*
* Description: This function returns the GPIO pin assignment for the VOIP
*              LED.
*
* Parameters : [OUT] pusValue - Address of short word that the VOIP LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
*
* Note       : The VoIP structure would allow for having one LED per DSP
*              however, the board initialization function assumes only one
*              LED per functionality (ie one LED for VoIP).  Therefore in
*              order to keep this tidy and simple we do not make usage of the
*              one-LED-per-DSP function.  Instead, we assume that the LED for
*              VoIP is unique and associated with DSP 0 (always present on
*              any VoIP platform).  If changing this to a LED-per-DSP function
*              then one need to update the board initialization driver in
*              bcmdrivers\opensource\char\board\bcm963xx\impl1
***************************************************************************/
int BpGetVoipLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedVoip, pusValue ) );
} /* BpGetVoipLedGpio */

/**************************************************************************
* Name       : BpGetVoip1LedGpio
*
* Description: This function returns the GPIO pin assignment for the VoIP1.
*              LED which is used when FXS0 is active
* Parameters : [OUT] pusValue - Address of short word that the VoIP1
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetVoip1LedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioVoip1Led, pusValue ) );
} /* BpGetVoip1LedGpio */

/**************************************************************************
* Name       : BpGetVoip1FailLedGpio
*
* Description: This function returns the GPIO pin assignment for the VoIP1
*              Fail LED which is used when there's an error with FXS0
* Parameters : [OUT] pusValue - Address of short word that the VoIP1
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetVoip1FailLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioVoip1LedFail, pusValue ) );
} /* BpGetVoip1FailLedGpio */

/**************************************************************************
* Name       : BpGetVoip2LedGpio
*
* Description: This function returns the GPIO pin assignment for the VoIP2.
*              LED which is used when FXS1 is active
* Parameters : [OUT] pusValue - Address of short word that the VoIP2
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetVoip2LedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioVoip2Led, pusValue ) );
} /* BpGetVoip2LedGpio */

/**************************************************************************
* Name       : BpGetVoip2FailLedGpio
*
* Description: This function returns the GPIO pin assignment for the VoIP2
*              Fail LED which is used when there's an error with FXS1
* Parameters : [OUT] pusValue - Address of short word that the VoIP2
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetVoip2FailLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioVoip2LedFail, pusValue ) );
} /* BpGetVoip2FailLedGpio */

/**************************************************************************
* Name       : BpGetPotsLedGpio
*
* Description: This function returns the GPIO pin assignment for the POTS1.
*              LED which is used when DAA is active
* Parameters : [OUT] pusValue - Address of short word that the POTS11
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetPotsLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioPotsLed, pusValue ) );
} /* BpGetPotsLedGpio */

/**************************************************************************
* Name       : BpGetDectLedGpio
*
* Description: This function returns the GPIO pin assignment for the DECT.
*              LED which is used when DECT is active
* Parameters : [OUT] pusValue - Address of short word that the DECT
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetDectLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioDectLed, pusValue ) );
} /* BpGetDectLedGpio */

/**************************************************************************
* Name       : BpGetDyingGaspIntrPin
*
* Description: This function returns the intrerrupt pin used by dying gasp
* Parameters : [OUT] pusValue - Address of long word that the DECT
*                   pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetDyingGaspIntrPin( unsigned long *pusValue )
{
    return( BpGetUl(bp_ulDyingGaspIntrPin, pusValue ) );
} /* BpGetDyingGaspIntrPin */

/**************************************************************************
* Name       : BpGetPassDyingGaspGpio
*
* Description: This function returns the GPIO pin assignment used to pass
*                  a dying gasp interrupt to an external processor.
* Parameters : [OUT] pusValue - Address of short word that the DECT
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetPassDyingGaspGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioPassDyingGasp, pusValue ) );
} /* BpGetPassDyingGaspGpio */

/**************************************************************************
* Name       : BpGetFpgaResetGpio
*
* Description: This function returns the GPIO pin assignment for the FPGA
*              Reset signal.
*
* Parameters : [OUT] pusValue - Address of short word that the FPGA Reset
*                  signal GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetFpgaResetGpio( unsigned short *pusValue ) {
    return( BpGetGpio(bp_usGpioFpgaReset, pusValue ) );
} /*BpGetFpgaResetGpio*/

/**************************************************************************
* Name       : BpGetGponLedGpio
*
* Description: This function returns the GPIO pin assignment for the GPON
*              LED.
*
* Parameters : [OUT] pusValue - Address of short word that the GPON LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetGponLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedGpon, pusValue ) );
} /* BpGetGponLedGpio */

/**************************************************************************
* Name       : BpGetGponFailLedGpio
*
* Description: This function returns the GPIO pin assignment for the GPON
*              LED that is used when there is a GPON connection failure.
*
* Parameters : [OUT] pusValue - Address of short word that the GPON LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetGponFailLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedGponFail, pusValue ) );
} /* BpGetGponFailLedGpio */



 /**************************************************************************
* Name	   : BpGetOpticalLinkFailLedGpio
*
* Description: This function returns the GPIO pin assignment for the OpticalLink
*              Fail LED which is used when Receive power is Lower than the receive sensitivity.
* Parameters : [OUT] pusValue - Address of short word that the VoIP1
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
  ***************************************************************************/
  int BpGetOpticalLinkFailLedGpio( unsigned short *pusValue )
  {
	  return( BpGetGpio(bp_usGpioLedOpticalLinkFail, pusValue ) );
  } /* BpGetOpticalLinkFailLedGpio */



  /**************************************************************************
   * Name		: BpGetUSBLedGpio
   *
   * Description: This function returns the GPIO pin assignment for the USB
   *			  LED.
   *
   * Parameters : [OUT] pusValue - Address of short word that the USB LED
   *				  GPIO pin is returned in.
   *
   * Returns	: BP_SUCCESS - Success, value is returned.
   *			  BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
   *			  BP_VALUE_NOT_DEFINED - At least one return value is not defined
   *				  for the board.
   ***************************************************************************/
   int BpGetUSBLedGpio( unsigned short *pusValue )
   {
	   return( BpGetGpio(bp_usGpioLedUSB, pusValue ) );
   } /* BpGetUSBLedGpio */



/**************************************************************************
* Name       : BpGetMoCALedGpio
*
* Description: This function returns the GPIO pin assignment for the MoCA
*              LED.
*
* Parameters : [OUT] pusValue - Address of short word that the MoCA LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetMoCALedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedMoCA, pusValue ) );
} /* BpGetMoCALedGpio */

/**************************************************************************
* Name       : BpGetMoCAFailLedGpio
*
* Description: This function returns the GPIO pin assignment for the MoCA
*              LED that is used when there is a MoCA connection failure.
*
* Parameters : [OUT] pusValue - Address of short word that the MoCA LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetMoCAFailLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedMoCAFail, pusValue ) );
} /* BpGetMoCAFailLedGpio */

/**************************************************************************
* Name       : BpGetEponLedGpio
*
* Description: This function returns the GPIO pin assignment for the Epon
*              LED.
*
* Parameters : [OUT] pusValue - Address of short word that the Epon LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetEponLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedEpon, pusValue ) );
} /* BpGetMoCALedGpio */

/**************************************************************************
* Name       : BpGetEponFailLedGpio
*
* Description: This function returns the GPIO pin assignment for the Epon Fail
*              LED that is used when there is a MoCA connection failure.
*
* Parameters : [OUT] pusValue - Address of short word that the Epon Fail LED
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetEponFailLedGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedEponFail, pusValue ) );
} /* BpGetMoCAFailLedGpio */

/**************************************************************************
* Name       : BpGetResetToDefaultExtIntr
*
* Description: This function returns the external interrupt number for the
*              reset to default button.
*
* Parameters : [OUT] pusValue - Address of short word that reset to default
*                  external interrupt is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetResetToDefaultExtIntr( unsigned short *pusValue )
{
    return( BpGetUs(bp_usExtIntrResetToDefault, pusValue ) );
} /* BpGetResetToDefaultExtIntr */

/**************************************************************************
* Name       : BpGetResetToDefaultExtIntrGpio
*
* Description: This function returns the external interrupt number for the
*              reset to default button.
*
* Parameters : [OUT] pusValue - Address of short word that reset to default
*                  external interrupt is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetResetToDefaultExtIntrGpio( unsigned short *pusValue )
{
    int ret;

    if( !g_pCurrentBp ) 
    {
        *pusValue = BP_NOT_DEFINED;
        ret       = BP_BOARD_ID_NOT_SET;
        return ret;
    }
    
    ret = BpGetExtIntrGpio(bp_usExtIntrResetToDefault, pusValue);
    
    return( ret );  

} /* BpGetResetToDefaultExtIntrGpio */


/**************************************************************************
* Name       : BpGetWirelessPowerDownGpio
*
* Description: This function returns the GPIO pin assignment for WLAN_PD
*
*
* Parameters : [OUT] pusValue - Address of short word that the WLAN_PD
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetWirelessPowerDownGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioWirelessPowerDown, pusValue ) );
} /* usGpioWirelessPowerDown */

/**************************************************************************
* Name       : BpGetDslPhyAfeIds
*
* Description: This function returns the DSL PHY AFE ids for primary and
*              secondary PHYs.
*
* Parameters : [OUT] pulValues-Address of an array of two long words where
*              AFE Id for the primary and secondary PHYs are returned.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET-Error, BpSetBoardId hasn't been called.
*              BP_VALUE_NOT_DEFINED - No defined AFE Ids.
**************************************************************************/
int BpGetDslPhyAfeIds( unsigned long *pulValues )
{
    int nRet;

    if( g_pCurrentBp )
    {
        if (BpGetUl(bp_ulAfeId0, &pulValues[0]) != BP_SUCCESS) {
          pulValues[0] = BP_AFE_DEFAULT;
        }
        if (BpGetUl(bp_ulAfeId1, &pulValues[1]) != BP_SUCCESS) {
          pulValues[1] = BP_AFE_DEFAULT;
        }
        nRet = BP_SUCCESS;
    }
    else
    {
        pulValues[0] = BP_AFE_DEFAULT;
        pulValues[1] = BP_AFE_DEFAULT;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetDslPhyAfeIds */

/**************************************************************************
* Name       : BpGetUart2SdoutGpio
*
* Description: This function returns the GPIO pin assignment for UART2 SDOUT
*
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioUart2Sdout
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetUart2SdoutGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioUart2Sdout, pusValue ) );
} /* BpGetUart2SdoutGpio */

/**************************************************************************
* Name       : BpGetUart2SdinGpio
*
* Description: This function returns the GPIO pin assignment for UART2 SDIN
*
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioUart2Sdin
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetUart2SdinGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioUart2Sdin, pusValue ) );
} /* BpGetUart2SdinGpio */

/**************************************************************************
* Name       : BpGetExtAFEResetGpio
*
* Description: This function returns the GPIO pin assignment for resetting the external AFE chip
*
*
* Parameters : [OUT] pusValue - Address of short word that the ExtAFEReset
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetExtAFEResetGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioExtAFEReset, pusValue ) );
} /* BpGetExtAFEResetGpio */

/**************************************************************************
* Name       : BpGetAFELDRelayGpio
*
* Description: This function returns the GPIO pin assignment for switching LD relay
*
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioAFELDRelay
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetAFELDRelayGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioAFELDRelay, pusValue ) );
} /* BpGetAFELDRelayGpio */

/**************************************************************************
* Name       : BpGetIntAFELDModeGpio
*
* Description: This function returns the GPIO pin assignment for setting LD Mode to ADSL/VDSL
*                  for the internal path.
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioIntAFELDMode
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetIntAFELDModeGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioIntAFELDMode, pusValue ) );
} /* BpGetIntAFELDModeGpio */

/**************************************************************************
* Name       : BpGetIntAFELDPwrGpio
*
* Description: This function returns the GPIO pin assignment for turning on/off the internal AFE LD
*
*
* Parameters : [OUT] pusValue - Address of short word that the usGpioExtAFELDPwr
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetIntAFELDPwrGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioIntAFELDPwr, pusValue ) );
} /* BpGetIntAFELDPwrGpio */

/**************************************************************************
* Name       : BpGetExtAFELDModeGpio
*
* Description: This function returns the GPIO pin assignment for setting LD Mode to ADSL/VDSL
*                  for the external path.
*
* Parameters : [OUT] pusValue - Address of short word that the usGpioExtAFELDMode
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetExtAFELDModeGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioExtAFELDMode, pusValue ) );
} /* BpGetExtAFELDModeGpio */

/**************************************************************************
* Name       : BpGetExtAFELDPwrGpio
*
* Description: This function returns the GPIO pin assignment for turning on/off the external AFE LD
*
*
* Parameters : [OUT] pusValue - Address of short word that the usGpioExtAFELDPwr
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetExtAFELDPwrGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioExtAFELDPwr, pusValue ) );
} /* BpGetExtAFELDPwrGpio */

/**************************************************************************
* Name       : BpGetExtAFELDDataGpio
*
* Description: This function returns the GPIO pin assignment for sending config data to the external AFE LD
*
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioExtAFELDData
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetExtAFELDDataGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioExtAFELDData, pusValue ) );
} /* BpGetExtAFELDDataGpio */

/**************************************************************************
* Name       : BpGetExtAFELDClkGpio
*
* Description: This function returns the GPIO pin assignment for sending the clk to the external AFE LD
*
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioExtAFELDClk
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/

int BpGetExtAFELDClkGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioExtAFELDClk, pusValue ) );
} /* BpGetExtAFELDClkGpio */

/**************************************************************************
* Name       : BpGet6829PortInfo
*
* Description: This function checks the ENET MAC info to see if a 6829
*              is connected
*
* Parameters : [OUT] portInfo6829 - 0 if 6829 is not present
*                                 - 6829 port information otherwise
*
* Returns    : BP_SUCCESS           - Success, value is returned.
*              BP_BOARD_ID_NOT_SET  - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGet6829PortInfo( unsigned char *portInfo6829 )
{
   ETHERNET_MAC_INFO enetMacInfo;
   ETHERNET_SW_INFO *pSwInfo;
   int               retVal;
   int               i;

   *portInfo6829 = 0;
   retVal = BpGetEthernetMacInfo( &enetMacInfo, 1 );
   if ( BP_SUCCESS == retVal ) {
      pSwInfo = &enetMacInfo.sw;
      for (i = 0; i < BP_MAX_SWITCH_PORTS; i++) {
         if ( ((pSwInfo->phy_id[i] & PHYID_LSBYTE_M) != 0xFF) &&
              ((pSwInfo->phy_id[i] & PHYID_LSBYTE_M) &  0x80) ) {
            *portInfo6829 = pSwInfo->phy_id[i] & PHYID_LSBYTE_M;
            retVal        = BP_SUCCESS;
            break;
         }
      }
   }

   return retVal;

}

/**************************************************************************
* Name       : BpGetEthSpdLedGpio
*
* Description: This function returns the GPIO pin assignment for the
*              specified port and link speed 
*
* Parameters : [IN] port - Internal phy number
*              [IN] enetIdx - index for Ethernet MAC info
*              [IN] ledIdx - 0 -> duplex GPIO
*                          - 1 -> spd 100 GPIO
*                          - 2 -> spd 1000 GPIO
*              [OUT] pusValue - Address of a short word to store the GPIO
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetEthSpdLedGpio( unsigned short port, unsigned short enetIdx,
                        unsigned short ledIdx, unsigned short *pusValue )
{
    ETHERNET_MAC_INFO enetMacInfos[BP_MAX_ENET_MACS];
    unsigned short *pShort;
    int nRet;

    if( g_pCurrentBp ) {
        nRet = BpGetEthernetMacInfo( enetMacInfos, BP_MAX_ENET_MACS );

        if ((enetIdx >= BP_MAX_ENET_MACS) ||
            (port >= BP_MAX_ENET_INTERNAL) ||
            (enetMacInfos[enetIdx].ucPhyType == BP_ENET_NO_PHY)) {
           *pusValue = BP_NOT_DEFINED;
           nRet = BP_VALUE_NOT_DEFINED;
        } else {
           pShort   = &enetMacInfos[enetIdx].sw.ledInfo[port].duplexLed;
           pShort   += ledIdx;
           *pusValue = *pShort;
           if( *pShort == BP_NOT_DEFINED ) {
               nRet = BP_VALUE_NOT_DEFINED;
           } else {
               nRet = BP_SUCCESS;
           }
        }
    } else {
        *pusValue = BP_NOT_DEFINED;
        nRet = BP_BOARD_ID_NOT_SET;
    }

    return( nRet );
} /* BpGetEthSpdLedGpio */


/**************************************************************************
* Name       : BpGetLaserDisGpio
*
* Description: This function returns the GPIO pin assignment for disabling
*              the laser
*
* Parameters : [OUT] pusValue - Address of short word that the usGpioLaserDis
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetLaserDisGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLaserDis, pusValue ) );
} /* BpGetLaserDisGpio */


/**************************************************************************
* Name       : BpGetLaserTxPwrEnGpio
*
* Description: This function returns the GPIO pin assignment for enabling
*              the transmit power of the laser
*
* Parameters : [OUT] pusValue - Address of short word that the usGpioLaserTxPwrEn
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetLaserTxPwrEnGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLaserTxPwrEn, pusValue ) );
} /* BpGetLaserTxPwrEnGpio */

/**************************************************************************
* Name       : BpGetLaserResetGpio
*
* Description: This function returns the GPIO pin assignment for resetting
* the optical module
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioLaserReset
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetLaserResetGpio( unsigned short *pusValue )
{
    return( BpGetUs(bp_usGpioLaserReset, pusValue ) );
} /* BpGetLaserResetGpio */

/**************************************************************************
* Name       : BpGetEponOpticalSDGpio
*
* Description: This function returns the GPIO pin assignment for Epon optical
* signal dectect
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioEponOpticalSD
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetEponOpticalSDGpio( unsigned short *pusValue )
{
    return( BpGetUs(bp_usGpioEponOpticalSD, pusValue ) );
} /* BpGetLaserResetGpio */


/**************************************************************************
* Name       : BpGetPLCPwrEnGpio
*
* Description: This function returns the GPIO pin assignment for PLC Power enable pin
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioPLCPwrEn
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetPLCPwrEnGpio( unsigned short *pusValue )
{
    return( BpGetUs(bp_usGpioPLCPwrEn, pusValue ) );
} /* BpGetPLCPwrEnGpio */

/**************************************************************************
* Name       : BpGetPLCResetGpio
*
* Description: This function returns the GPIO pin assignment for PLC reset pin
*
* Parameters : [OUT] pusValue - Address of short word that the bp_usGpioPLCReset
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetPLCResetGpio( unsigned short *pusValue )
{
    return( BpGetUs(bp_usGpioPLCReset, pusValue ) );
} /* BpGetPLCResetGpio */

/**************************************************************************
* Name       : BpGetPLCStandByExtIntr
*
* Description: This function returns the external interrupt number for the
*              PLC Standby Button.
*
* Parameters : [OUT] pusValue - Address of short word that the PLC Standby
*                  external interrup is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetPLCStandByExtIntr( unsigned short *pusValue )
{
    return( BpGetUs(bp_usExtIntrPLCStandBy, pusValue ) );
} /* BpGetWirelessSesExtIntr */

/**************************************************************************
* Name       : BpGetVregSel1P2
*
* Description: This function returns the desired voltage level for 1V2
*
* Parameters : [OUT] pusValue - Address of short word that the 1V2 level
*                  is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetVregSel1P2( unsigned short *pusValue )
{
    return( BpGetUs(bp_usVregSel1P2, pusValue ) );
} /* BpGetVregSel1P2 */

/**************************************************************************
* Name       : BpGetVreg1P8
*
* Description: This function returns the desired state for 1P8 regulator
*
* Parameters : [OUT] pucValue - Address of unsigned char where to return
*                               whether 1P8 regulator is external or not
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetVreg1P8( unsigned char *pucValue )
{
    return( BpGetUc(bp_ucVreg1P8, pucValue ) );
} /* BpGetVreg1P8 */

/**************************************************************************
* Name       : BpGetVregAvsMin
*
* Description: This function returns the desired min voltage level for AVS
*
* Parameters : [OUT] pusValue - Address of short word that the min voltage
*                  is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetVregAvsMin( unsigned short *pusValue )
{
    return( BpGetUs(bp_usVregAvsMin, pusValue ) );
} /* BpGetVregAvsMin */

/**************************************************************************
* Name       : BpGetGponOpticsType
*
* Description: This function returns an indication of whether the current 
*              board type supports GPON legacy or BOSA optics.
*
* Parameters : [Out] pusValue
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetGponOpticsType( unsigned short *pusValue )
{
    return ( BpGetUs (bp_usGponOpticsType, pusValue));
} /* BpGetGponOpticsType */


#if !defined(_CFE_)
/**************************************************************************
* Name       : BpGetDefaultOpticalParams
*
* Description: This function returns the optical params for BOSA optics if
*              they exist.  These are only used if they do not exist in NVRAM.
*
* Parameters : [OUT] pOpticalParams - Address of a buffer that the optical
*              params are returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
***************************************************************************/
int BpGetDefaultOpticalParams( unsigned char *pOpticalParams )
{
    int Index, Ret;    
    unsigned char * pBpOpticalParams = (unsigned char *)0;


    if (BP_SUCCESS == (Ret = BpGetCp (bp_cpDefaultOpticalParams, (char**)&pBpOpticalParams)))
    {
        for (Index = 0; Index < BP_OPTICAL_PARAMS_LEN; Index++)
        {
            pOpticalParams[Index] = pBpOpticalParams[Index];
        }
    }

    return (Ret);

} /* BpGetDefaultOpticalParams */


/**************************************************************************
* Name       : BpGetI2cGpios
*
* Description: This function returns the GPIO pin assignments for the I2C
*              clock (SCL) and data (SDA).
*
* Parameters : [OUT] pusScl - Address of short word that the SCL GPIO pin
*                  is returned in.
*              [OUT] pusSda - Address of short word that the SDA GPIO pin
*                  pin is returned in.
*
* Returns    : BP_SUCCESS - Success, values are returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetI2cGpios( unsigned short *pusScl, unsigned short *pusSda )
{
    int ret;

    *pusScl = *pusSda = BP_NOT_DEFINED;
    if( (ret = BpGetGpio (bp_usGpioI2cScl, pusScl)) == BP_SUCCESS )
        ret = BpGetGpio (bp_usGpioI2cSda, pusSda);
    
    return(ret);
} /* BpGetRj11InnerOuterPairGpios */
#endif


/**************************************************************************
* Name       : BpGetFemtoResetGpio
*
* Description: This function returns the GPIO that needs to be toggled high
*              for 2 msec at least to reset the FEMTO chip
*
* Parameters : [OUT] pusValue - Address of short word that the GPIO for
*                  resetting FEMTO chip is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetFemtoResetGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioFemtoReset, pusValue ) );
} /*  BpGetFemtoResetGpio */



/**************************************************************************
* Name       : BpGetEphyBaseAddress
*
* Description: This function returns the base address requested for
*              the internal EPHYs
*
* Parameters : [OUT] pusValue - Address of short word for returned value.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetEphyBaseAddress( unsigned short *pusValue )
{
    return( BpGetUs(bp_usEphyBaseAddress, pusValue ) );
} /*  BpGetEphyBaseAddress */


/**************************************************************************
* Name       : BpGetGphyBaseAddress
*
* Description: This function returns the base address requested for
*              the internal GPHYs
*
* Parameters : [OUT] pusValue - Address of short word for returned value.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetGphyBaseAddress( unsigned short *pusValue )
{
    return( BpGetUs(bp_usGphyBaseAddress, pusValue ) );
} /*  BpGetGphyBaseAddress */

/**************************************************************************
* Name       : BpGetSpiSlaveResetGpio
*
* Description: This function returns the GPIO pin assignment for the resetting the 
*              the SPI slave.
*
* Parameters : [OUT] pusValue - Address of short word that the spi slave reset
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSpiSlaveResetGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioSpiSlaveReset, pusValue ) );
} /* BpGetSpiSlaveResetGpio */

/**************************************************************************
* Name       : BpGetSpiSlaveBusNum
*
* Description: This function returns the bus number of the SPI slave device.
*
* Parameters : [OUT] pusValue - Address of short word that the spi slave select number
*                    is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSpiSlaveBusNum( unsigned short *pusValue )
{
    return( BpGetUs(bp_usSpiSlaveBusNum, pusValue ) );
} /* BpGetSpiSlaveBusNum */

/**************************************************************************
* Name       : BpGetSpiSlaveSelectNum
*
* Description: This function returns the SPI slave select number connected  
*              to the slave device.
*
* Parameters : [OUT] pusValue - Address of short word that the spi slave select number
*                    is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSpiSlaveSelectNum( unsigned short *pusValue )
{
    return( BpGetUs(bp_usSpiSlaveSelectNum, pusValue ) );
} /* BpGetSpiSlaveSelectNum */

/**************************************************************************
* Name       : BpGetSpiSlaveMode
*
* Description: This function returns the SPI slave select number connected  
*              to the slave device.
*
* Parameters : [OUT] pusValue - Address of short word that the spi slave mode is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSpiSlaveMode( unsigned short *pusValue )
{
    return( BpGetUs(bp_usSpiSlaveMode, pusValue ) );
} /* BpGetSpiSlaveMode */

/**************************************************************************
* Name       : BpGetSpiSlaveCtrlState
*
* Description: This function returns the spi controller state that is needed to talk
*              to the spi slave device.
*
* Parameters : [OUT] pusValue - Address of long word that the spi controller state is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSpiSlaveCtrlState( unsigned long *pulValue )
{
    return( BpGetUl(bp_ulSpiSlaveCtrlState, pulValue ) );
} /* BpGetSpiSlaveCtrlState */

/**************************************************************************
* Name       : BpGetSpiSlaveMaxFreq
*
* Description: This function returns the SPI slaves max frequency for communication.
*
* Parameters : [OUT] pusValue - Address of long word that the max freq is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSpiSlaveMaxFreq( unsigned long *pulValue )
{
    return( BpGetUl(bp_ulSpiSlaveMaxFreq, pulValue ) );
} /* BpGetSpiSlaveMaxFreq */

/**************************************************************************
* Name       : BpGetSpiSlaveProtoRev
*
* Description: This function returns the protocol revision that the slave device uses.
*
* Parameters : [OUT] pusValue - Address of short word that the spi slave protocol revision
                                is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSpiSlaveProtoRev( unsigned short *pusValue )
{
    return( BpGetUs(bp_usSpiSlaveProtoRev, pusValue ) );
} /* BpGetSpiSlaveProtoRev */

/**************************************************************************
* Name       : BpGetSerialLEDMuxSel
*
* Description: This function returns the serial LED Mux selection.
*
* Parameters : [OUT] pusValue - Address of short word that the serial LED MUX Selection
                                is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSerialLEDMuxSel( unsigned short *pusValue )
{
    return( BpGetUs(bp_usSerialLEDMuxSel, pusValue ) );
} /* BpGetSerialLEDMuxSel */

/**************************************************************************
* Name       : BpGetSwitchPortMap
*
* Description: This function returns the switch port map.
*
* Parameters : [OUT] pulValue - Bitmap of switch ports enabled.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_VALUE_NOT_DEFINED - bp_ulPortMap is not defined
*                  for the board.
***************************************************************************/
int BpGetSwitchPortMap (unsigned long *pulValue)
{
    *pulValue = BpGetSubUl(bp_ulPortMap, 0, bp_last);
    if (*pulValue == BP_NOT_DEFINED ){
        *pulValue = 0;
        return BP_VALUE_NOT_DEFINED;
    } else {
        return BP_SUCCESS;
    }
}

/**************************************************************************
* Name       : BpGetDeviceOptions
*
* Description: This function returns the serial LED Mux selection.
*
* Parameters : [OUT] pulValue - Address of word that device options bitmap
                                is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetDeviceOptions( unsigned long *pulValue )
{
    return( BpGetUl(bp_ulDeviceOptions, pulValue ) );
} /* BpGetDeviceOptions */

#if defined(CONFIG_EPON_SDK)
/**************************************************************************
* Name       : BpGetPortMacType
*
* Description: This function returns the mac type of uni port
*
* Parameters : [IN ] port - port index which we are interested
*              [OUT] pulValue - return mac type of the uni port
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetPortMacType(unsigned short port, unsigned long *pulValue )
{
    unsigned long phy_id, port_map, mac_type;
 
    BpGetUl(bp_ulPortMap, &port_map);

    if((port < BP_MAX_SWITCH_PORTS) &&
       (port_map & (1 << port)))                 
    {
        BpGetUl(bp_ulPhyId0 + port, &phy_id);
        mac_type = phy_id & MAC_IFACE;
        *pulValue = mac_type;
        return BP_SUCCESS;
    }
    return BP_VALUE_NOT_DEFINED;
} /* BpGetPortMacType */

/**************************************************************************
* Name       : BpGetNumFePorts
*
* Description: This function returns the number of FE ports
*              Please note the code assumes any phy not marked as GMII will be 
*              treated as FE ports. 
* Parameters : [OUT] pulValue - number of FE UNI ports in this board design
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetNumFePorts( unsigned long *pulValue )
{
    unsigned long phy_id, port_map, mac_type;
    int i = 0, fe_ports = 0;

    BpGetUl(bp_ulPortMap, &port_map);
    
    for (i = 0; i < BP_MAX_SWITCH_PORTS; i++) {
        if ( port_map & (1 << i)) {
            BpGetUl(bp_ulPhyId0 + i, &phy_id);
            mac_type = phy_id & MAC_IFACE;
            switch(mac_type)
            {
                case MAC_IF_GMII:
                case MAC_IF_RGMII:
                case MAC_IF_RGMII_3P3V:
                case MAC_IF_QSGMII:
                case MAC_IF_SGMII:    
                     break;

                default:
                     fe_ports++;
                     break;
            }
        }
    }
    *pulValue = fe_ports;
    return BP_SUCCESS;
} /* BpGetNumFePorts */

/**************************************************************************
* Name       : BpGetNumGePorts
*
* Description: This function returns the number of GE ports
*              Please note the code assumes phy_id to be marked as MAC GMII
*              for GE ports. 
* Parameters : [OUT] pulValue - number of GE UNI ports in this board design
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetNumGePorts( unsigned long *pulValue )
{
    unsigned long phy_id, port_map, mac_type;
    int i = 0, ge_ports = 0;

    BpGetUl(bp_ulPortMap, &port_map);
    
    for (i = 0; i < BP_MAX_SWITCH_PORTS; i++) {
        if ( port_map & (1 << i)) {
            BpGetUl(bp_ulPhyId0 + i, &phy_id);
            mac_type = phy_id & MAC_IFACE;
            switch(mac_type)
            {
                case MAC_IF_GMII:
                case MAC_IF_RGMII:
                case MAC_IF_RGMII_3P3V:
                case MAC_IF_QSGMII:
                case MAC_IF_SGMII:    
                     ge_ports++;
                     break;

                default:
                     break;
            }
        }
    }
    *pulValue = ge_ports;
    return BP_SUCCESS;
} /* BpGetNumGePorts */

/**************************************************************************
* Name       : BpGetNumVoipPorts
*
* Description: This function returns the number of VOIP ports
*
* Parameters : [OUT] pulValue - number of VOIP ports in this board design
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetNumVoipPorts( unsigned long *pulValue )
{
    return( BpGetUl(bp_ulNumVoipPorts, pulValue ) );
} /* BpGetNumVoipPorts */


#endif //CONFIG_EPON_SDK

/**************************************************************************
* Name       : BpGetLedGpio
*
* Description: Iterate throught the board parameters and get the gpio pin assignment 
*              for a LED boardparm id in the bpLedList. User call this function multiple 
*              times to get all assignment for a boardparm id.
* Parameters : [IN] idx - LED boardparm id index to the bpLedList array.
*              [IN] token - transparent to caller. Set a pointer to NULL when first time call
*              [OUT] pusValue - the GPIO number for the LED or BP_VALUE_NOT_DEFINED
*              if that LED is not defined in the board parameter
*
* Returns    : BP_SUCCESS - Success, value is returned in pusValue
*              BP_SUCCESS_LAST - Last entry reached, no value is returened in pusValue
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - this Led id is not defined in the boardparm
*              BP_MAX_ITEM_EXCEEDED - idx exceed the maximum led id in the list
***************************************************************************/
int BpGetLedGpio(int idx, int* token,  unsigned short *pusValue)
{
    int total = sizeof(bpLedList)/sizeof(enum bp_id);

    if(idx >= total)
        return BP_MAX_ITEM_EXCEEDED;
    else
        return BpEnumUs(bpLedList[idx], token, pusValue);
}

/**************************************************************************
* Name       : BpGetGpioGpio
*
* Description: Iterate throught the board parameters and get the gpio pin assignment 
*              for a GPIO boardparm id in the bpGpioList. User call this function multiple 
*              times to get all assignment for a boardparm id.
* Parameters : [IN] idx - GPIO boardparm id idx to the bpGpioList array.
*              [IN] token - transparent to caller. Set a pointer to NULL when first time call
*              [OUT] pusValue - the GPIO number for the Gpio or BP_VALUE_NOT_DEFINED
*              if that GPIO is not defined in the board parameter
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_SUCCESS_LAST - Last entry reached, no value is returened in pusValue
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - this GPIO id is not defined in the boardparm
*              BP_MAX_ITEM_EXCEEDED - idx exceed the maximum led id in the list
***************************************************************************/
int BpGetGpioGpio(int idx, int* token, unsigned short *pusValue)
{
    int total = sizeof(bpGpioList)/sizeof(enum bp_id);

    if(idx >= total)
        return BP_MAX_ITEM_EXCEEDED;
    else
        return BpEnumUs(bpGpioList[idx], token, pusValue);
}

/**************************************************************************
* Name       : BpIsGpioInUse
*
* Description: Check if the gpio pin is used in boardparamter for LED and other GPIO function
*              Does not including any serial gpio usage as they don't take gpin pin
*
* Parameters : [IN]  GPIO number
*
* Returns    : 0 if it is not used.
*              Non-zero if it is used in the currrent boardparameter
***************************************************************************/
int BpIsGpioInUse(unsigned short gpio)
{
    int i = 0, rc = 0, checkLed = 0;
    int inUse = 0, token = 0;
    unsigned short gpio2;

    while( checkLed < 2 )
    {
        i = 0;
        token = 0;
        for(;;)
        {
            if( checkLed )
	        rc = BpGetLedGpio(i, &token, &gpio2);
            else
	        rc = BpGetGpioGpio(i, &token, &gpio2);

            if( rc == BP_MAX_ITEM_EXCEEDED )
                break;
            else if( rc == BP_SUCCESS )
            {
                /* serial led gpio does not take GPIO pin */
              	if( ((gpio2&BP_GPIO_SERIAL) == 0x0) && (gpio&BP_GPIO_NUM_MASK) == (gpio2&BP_GPIO_NUM_MASK) )
                {
                    inUse = 1;
                    break;
                }
            }
            else 
	    {
	        token = 0;
            i++;
        }
        }

        if( inUse )
            break;
        checkLed++;
	}

    return inUse;
}

/**************************************************************************
* Name       : BpGetEponGpio
*
* Description: Get the gpio pin assignment for all the EPON gpio related boardparm id in the
* bpEponGpioList
*
* Parameters : [IN] idx - GPIO boardparm id idx to the bpEponGpioList array.
*              [OUT] pusValue - the GPIO number for the Gpio or BP_VALUE_NOT_DEFINED
*              if that GPIO is not defined in the board parameter
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - this GPIO id is not defined in the boardparm
*              BP_MAX_ITEM_EXCEEDED - idx exceed the maximum led id in the list
***************************************************************************/
int BpGetEponGpio(int idx, unsigned short *pusValue)
{
    int total = sizeof(bpEponGpioList)/sizeof(enum bp_id);

	if(idx >= total)
        return BP_MAX_ITEM_EXCEEDED;
    else
        return BpGetGpio(bpEponGpioList[idx], pusValue);
}

/**************************************************************************
* Name       : BpGetPhyResetGpio
*
* Description: This function returns the GPIO pin assignment for PHYs reset pin
*
* Parameters : [IN] port - phy index
*              [OUT] pucValue - Address of a short that the bp_ucsGpioPhyReset
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetPhyResetGpio(int port, unsigned short *pusValue)
{
	ETHERNET_MAC_INFO enetMacInfo;
	int               retVal;

	retVal = BpGetEthernetMacInfo( &enetMacInfo, 1 );
	*pusValue = enetMacInfo.sw.phyReset[port];
	
	return retVal;
}
/**************************************************************************
* Name       : BpGetBoardResetGpio
*
* Description: This function returns the GPIO pin assignment for board reset pin
*
* Parameters : [OUT] pucValue - Address of a short that the bp_usGpioBoardReset
*                  GPIO pin is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetBoardResetGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioBoardReset, pusValue ) );
} 
/**************************************************************************
* Name       : BpGetUSBGpio
*
* Description: This function returns the GPIO pin assignment for USB overcurrent GPIOs
*
* Parameters :  [IN] usb - usb number on board
* 				[OUT] pusValue - Address of a GPIO_USB_INFO struct that the bp_usGpioUsbX
*                  GPIO pins is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetUSBGpio( int usb, GPIO_USB_INFO *gpios )
{
	unsigned short pusValue;
    int nRet;

	switch (usb)
	{
	   case 0:
		   nRet = BpGetGpio(bp_usGpioUsb0, &pusValue);
		   break;
	   case 1:
		   nRet = BpGetGpio(bp_usGpioUsb1, &pusValue);
		   break;
	   default:
		   return BP_VALUE_NOT_DEFINED;
	}
	if (nRet == BP_SUCCESS)
	{
		gpios->gpio_for_oc_detect = (pusValue & 0xff00) >> 8; // input pin
		gpios->gpio_for_oc_output = pusValue & 0xff;
	}
    return nRet;
} 
/**************************************************************************
* Name       : BpGetPhyAddr
*
* Description: This function returns the phy address for the required port
*
* Parameters :  [IN] unit - punit number
*               [IN] port - port number
* 				[OUT] phyId - value of the phy address to be used with any MDC/MDIO command.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - Not defined
***************************************************************************/
int BpGetPhyAddr(int unit, int port)
{
   int phyId = -1;
   ETHERNET_MAC_INFO pE[BP_MAX_ENET_MACS];

   BpGetEthernetMacInfo(pE, BP_MAX_ENET_MACS);

   if ( unit == 0 && (pE[0].ucPhyType == BP_ENET_NO_PHY) && BpGetPortConnectedToExtSwitch() >=0)  /* switch virtual PHY Address */
                   phyId = pE[0].ucPhyAddress; 
   else if (unit < BP_MAX_ENET_MACS  && port < BP_MAX_SWITCH_PORTS)
         phyId = pE[unit].sw.phy_id[port] & BCM_PHY_ID_M;

   return phyId;
}
/**************************************************************************
* Name       : BpGetOpticalWan
*
* Description: This function returns the optical transceiver type
*
* Parameters : [OUT] pulValue - tranceiver type.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetOpticalWan( unsigned long *pulValue )
{
    return( BpGetUl(bp_ulOpticalWan, pulValue ) );
} 

/**************************************************************************
* Name       : BpGetSimInterfaces
*
* Description: This function returns the simcard interface number.
*
* Parameters : [OUT] pusValue - Address of short word that simcard
*                  interface is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSimInterfaces( unsigned short *pusValue )
{
    return( BpGetUs(bp_ulSimInterfaces, pusValue ) );
} /* BpGetSimInterfaces */

/**************************************************************************
* Name       : BpGetSlicInterfaces
*
* Description: This function returns the slic interface number.
*
* Parameters : [OUT] pusValue - Address of short word that slic
*                  interface is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetSlicInterfaces( unsigned short *pusValue )
{
    return( BpGetUs(bp_ulSlicInterfaces, pusValue ) );
} /* BpGetSlicInterfaces */

/**************************************************************************
* Name       : BpGetPonTxEnGpio
*
* Description: This function returns the pon tx enable gpio number.
*
* Parameters : [OUT] pusValue - Address of short word that the
*                  gpio number is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetPonTxEnGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioPonTxEn, pusValue ) );
} /* BpGetPonTxEnGpio */

/**************************************************************************
* Name       : BpGetPonRxEnGpio
*
* Description: This function returns the gpon rx enable gpio number.
*
* Parameters : [OUT] pusValue - Address of short word that the
*                  gpio number is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetPonRxEnGpio( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioPonRxEn, pusValue ) );
} /* BpGetPonRxEnGpio */

/**************************************************************************
* Name       : BpGetRogueOnuEn
*
* Description: This function returns true if internal rogue onu is connected.
*
* Parameters : [OUT] pusValue - Address of short word that the
*                  value is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetRogueOnuEn( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usRogueOnuEn, pusValue ) );
} /* BpGetRogueOnuEn */

/**************************************************************************
* Name       : BpGetGpioLedSim
*
* Description: This function returns simcard led pin.
*
* Parameters : [OUT] pusValue - Address of short word that the
*                  value is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetGpioLedSim( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedSim, pusValue ) );
} /* BpGetGpioLedSim */

/**************************************************************************
* Name       : BpGetGpioLedSimITMS
*
* Description: This function returns simcard ITMS led pin.
*
* Parameters : [OUT] pusValue - Address of short word that the
*                  value is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGetGpioLedSimITMS( unsigned short *pusValue )
{
    return( BpGetGpio(bp_usGpioLedSim_ITMS, pusValue ) );
} /* BpGetGpioLedSimITMS */


/**************************************************************************
* Name       : BpGet1ppsPin
*
* Description: This function returns the 1pps pin number.
*
* Parameters : [OUT] pusValue - Address of short word that 1pps
*                  pin number is returned in.
*
* Returns    : BP_SUCCESS - Success, value is returned.
*              BP_BOARD_ID_NOT_SET - Error, BpSetBoardId has not been called.
*              BP_VALUE_NOT_DEFINED - At least one return value is not defined
*                  for the board.
***************************************************************************/
int BpGet1ppsPin( unsigned short *pusValue )
{
    return( BpGetUs(bp_us1ppsPin, pusValue ) );
} /* BpGet1ppsPin */


