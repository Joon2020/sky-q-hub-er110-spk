/*
* <:copyright-BRCM:2012:DUAL/GPL:standard
* 
*    Copyright (c) 2012 Broadcom Corporation
*    All Rights Reserved
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* 
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
* 
* :> 
*/

#ifndef BL_LILAC_PIN_MUXING_H_  
#define BL_LILAC_PIN_MUXING_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "bl_lilac_soc.h"


/*-------------------------------------------------
        Defines the chip pins
-------------------------------------------------*/
typedef enum {      
    GPIO_0          = 0, 		
    GPIO_1          = 1, 		
    GPIO_2          = 2, 		
    GPIO_3          = 3, 		
    GPIO_4          = 4, 		
    SYNC_CLK        = 5,		
    FSPI_CS_0       = 6,		
    FSPI_CLK        = 7,		
    FSPI_RXD        = 8,		
    FSPI_TX         = 9,		
    IIC_SCL         = 10,		
    IIC_SDA         = 11,		
    FMI_D_0         = 12,		
    FMI_D_1         = 13,		
    FMI_D_2         = 14,		
    FMI_D_3         = 15,		
    FMI_D_4         = 16,		
    FMI_D_5         = 17,		
    FMI_D_6         = 18,		
    FMI_D_7         = 19,		
    FMI_CLE         = 20,		
    FMI_ALE         = 21,		
    FMI_CS_0        = 22,		
    FMI_WE          = 23, 		
    FMI_RE          = 24, 		
    FMI_RDY_BSY_N   = 25, 	
    MDC             = 26,			
    MDIO            = 27,			
    MIPSC_UART_RXD  = 28,	
    MIPSC_UART_TXD  = 29,	
    TDM_RX_CLK      = 30, 	
    TDM_RXD         = 31,		
    TDM_RX_FSP      = 32,		
    TDM_TXD         = 33,		
    TDM_TX_FSP      = 34,		
    TDM_TX_CLK      = 35,		
    MII_CRS         = 36,		
    MII_COL         = 37,		
    MII_TX_CLK      = 38, 	
    MII_RX_ERR      = 39,		
    SPI_CS_0        = 40,		
    SPI_CS_1        = 41,		
    SPI_TXD         = 42,		
    SPI_CLK         = 43,		
    SPI_RXD         = 44,		
    CDR_TX_EN_P     = 45,	
    CDR_TX_EN_N     = 46,	
    GPIO_5          = 47,	
    GPIO_6          = 48,
    SKIP_PAD_49  	= 49,
    SKIP_PAD_50  	= 50,
    RGMII_RX_D_0    = 51,  
    RGMII_RX_D_1    = 52,  
    RGMII_RX_D_2    = 53,  
    RGMII_RX_D_3    = 54,  
    RGMII_RX_CLK    = 55,  
    RGMII_RX_CTL    = 56,  
    RGMII_TX_D_0    = 57,  
    RGMII_TX_D_1    = 58,  
    RGMII_TX_D_2    = 59,  
    RGMII_TX_D_3    = 60,  
    RGMII_TX_CLK    = 61,  
    RGMII_TX_CTL    = 62,  

    LAST_PHYSICAL_PIN
} LILAC_PHYSICAL_PIN ;

#define ONE_DIRECTION_PINS		RGMII_RX_D_0 /* 51 */
#define GPIO_ONLY_IN_MIN        RGMII_RX_D_0 /* 51 */
#define GPIO_ONLY_IN_MAX        RGMII_RX_CTL /* 56 */
#define GPIO_ONLY_OUT_MIN       RGMII_TX_D_0 /* 57 */
#define GPIO_ONLY_OUT_MAX       RGMII_TX_CTL /* 62 */


/*-------------------------------------------------
        Defines the chip pins' functions
-------------------------------------------------*/
typedef enum {
    I1588_ALARM,    /* 0  */
    I1588_1PPS,     /* 1  */
    I1588_CAPTURE,  /* 2  */
    I1588_VCXO,     /* 3  */
    I2C_SCL_OUT,    /* 4  */
    I2C_SDA_OUT,    /* 5  */
    I2C_SCL_IN,     /* 6  */
    I2C_SDA_IN,     /* 7  */
    RX_GPON_SYNC,   /* 8  */
    NFMC_CEN,  	    /* 9  */        
    NFMC_WPN, 	    /* 10 */        
    NFMC_BOOT_FAIL, /* 11 */
    SPI_FLASH_CSN,  /* 12 */	    
    SPI_FLASH_WPN,  /* 13 */      
    GRX_TDM_DATA,   /* 14 */
    GRX_TDM_CLK,    /* 15 */
    DSP_TDM_TXCLK,  /* 16 */
    DSP_TDM_RXCLK,  /* 17 */
    GTX_FROM_GPIO,  /* 18 */
    MIPSD_UART_TX,  /* 19 */
    MIPSD_UART_RX,  /* 20 */
    CLK_GPON_8KHz,  /* 21 */
    CLK_25_MHz,     /* 22 */
    CLK10_MHz_1588, /* 23 */
    CLK10_MHz_SYNC, /* 24 */
    SYNC_CLK_F,     /* 25 */
    CDR_TX_EN_P_F,  /* 26 */
    CDR_TX_EN_N_F,  /* 27 */
    NAND,            /* 28 */
    FSPI,           /* 29 */
    MDC_MDIO,       /* 30 */
    RGMII,          /* 31 */
    MII,            /* 32 */
    SPI,            /* 33 */
    TDM,            /* 34 */
    GPIO,           /* 35 */
    TIMER,          /* 36 */
    USB_PRT_WR,     /* 37 */
    USB_OVRCUR,     /* 38 */
    VOIP_TIMER_IRQ, /* 39 */
    VOIP_TIMER_PWM, /* 40 */
    INTERRUPT,      /* 41 */
    F_FSPI_CLK,     /* 42 */
    F_SPI_CLK,      /* 43 */

    ALWAYS_0,      /* 44 */
    ALWAYS_1,      /* 45 */
    LAST_PIN_FUNCTION,

} LILAC_PIN_FUNCTION ;

/*-------------------------------------------------
        Defines the return code from APIs
-------------------------------------------------*/
#define LILAC_PIN_MUXING_RETURN_CODE int32_t

#define CE_PIN_OUT_OF_RANGE                 1
#define CE_PIN_FUNCTION_OUT_OF_RANGE        2
#define CE_PIN_FUNCTION_ILEGAL_COMBITATION  3
#define CE_PIN_FUNCTION_ILEGAL_CONTROLLER   4
#define CE_PIN_FUNCTION_ILEGAL_NUMBER       5
#define CE_PIN_WRONG_STRENGTH               6
#define CE_PIN_WRONG_PULL                   7
#define CE_PIN_WRONG_SLEW                   8
#define CE_PIN_NULL_POINTER                 9
 
/*-------------------------------------------------
        Defines the pad sterngth
-------------------------------------------------*/
typedef enum {
    S6MA    = 0,
    S6_8MA  = 1,
    S8MA    = 2,
    S8_12MA = 3,
    S8_6MA  = 4,
    S8_6MA1 = 5, 
    S10_8MA = 6,
    S10MA   = 7,
    DS6MA   = 0,
    DS8MA   = 1
} PIN_STRENGTH;

/*-------------------------------------------------
        Defines the pad pull or slew
-------------------------------------------------*/
typedef enum {
    PULL_DOWN   = 0,
    PULL_UP     = 1,
    PULL_Z      = 2,
    SLOWEST     = 3,
    FASTEST     = 0
} PIN_PULL;

#define REAL_NUMBER_OF_FUNCTIONS 77

/*-------------------------------------------------
        Pad's electrical parameters structure
-------------------------------------------------*/
typedef struct
{
    PIN_STRENGTH  strength;
    PIN_PULL      pull_slew;
} PIN_CONFIGURATION;

/********************************************************
    Connect pin to function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_connect_pin (
                    /* actual pin, number between 0 - 62 */
                    LILAC_PHYSICAL_PIN pin,
                    /* function to connect the pin, number between 0 - 41 */
                    LILAC_PIN_FUNCTION function,
                    /* significant only for some functions (as timer, interrupt, etc) */
                    uint32_t number,
                    /* significant only for some functions (as timer, interrupt, etc) */
                    BL_LILAC_CONTROLLER controller
                    );

/*******************************************************
    Set 'mipsd-uart-txd_out' pin as output thru 'tx' pin
    Set 'UART_RXD' pin as input from 'rx' pin
*******************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_uartd (LILAC_PHYSICAL_PIN tx_pin, LILAC_PHYSICAL_PIN rx_pin);

/********************************************************
    Configure all the RGMII pins to rgmii function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_rgmii (void);

/********************************************************
    Configure all the MII pins to mii function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_mii (void);

/********************************************************
    Configure all the MDC_MDIO pins to mdc_mdio function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_mdc_mdio (void);

/********************************************************
    Configure all the TDM pins to tdm function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_tdm (void);

/********************************************************
    Configure all the SPI pins to spi function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_spi (void);

/********************************************************
    Configure all the FSPI pins to fspi function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_fspi (void);

/********************************************************
    Configure all the NAND pins to NAND function
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_nand (void);

/********************************************************
    Configure the electrical part of any given pin
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_config_pin_pull (LILAC_PHYSICAL_PIN pin, PIN_PULL parameters);

/********************************************************
    Return IO pin definition
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_get_config_pin_pull (LILAC_PHYSICAL_PIN pin, PIN_PULL *parameters, char *function, char *input_selected);

/********************************************************
    Configure the electrical part of any given pin
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_config_pin_strength (LILAC_PHYSICAL_PIN pin, PIN_STRENGTH parameters);

/********************************************************
    Return IO pin definition
********************************************************/
LILAC_PIN_MUXING_RETURN_CODE fi_bl_lilac_mux_get_config_pin_strength (LILAC_PHYSICAL_PIN pin, PIN_STRENGTH *parameters, char *function, char *input_selected);
void bl_lilac_mux_connect_25M_clock(int gpio);
void bl_lilac_mux_connect_ss_mii(void);


void fi_bl_lilac_mux_tx_en_constant(void);
void fi_bl_lilac_mux_tx_en_gpio(void);

#ifdef __cplusplus
}
#endif

#endif 
