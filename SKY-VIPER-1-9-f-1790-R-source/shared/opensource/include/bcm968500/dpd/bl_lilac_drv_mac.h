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

/***************************************************************************/
/*                                                                         */
/* MAC driver for Lilac Project                                            */
/*                                                                         */
/* Title                                                                   */
/*                                                                         */
/*   MAC driver H File                                                     */
/*                                                                         */
/* Abstract                                                                */
/*                                                                         */
/*   The driver provides API to access Mentor Graphics device. Driver      */
/*   supports intialization of the device.                                 */
/*   Mode of the device is specified in the configuration structure        */
/*   Initialization function resets the MAC, releases the MAC from reset,  */
/*   setups required by the mode and specified configuration registers     */
/*   This API is not reentrant. Typically application calls the            */
/*   initialization function only once.                                    */
/*   Make sure to reset Ethernet PHY before calling the initialization     */
/*   function.                                                             */
/***************************************************************************/

/*  Revision history                                                  */
/*  01/01/10: Creation, Asi Bieda (Version 1.0.0)*/
/*  2/03/10: Fixes after review, Asi Bieda (Version 1.0.1)*/
/*  22/07/10: add API set_emacs_mode, Asi Bieda (Version 1.0.2)*/


#ifndef DRV_MAC_H_
#define DRV_MAC_H_


#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
 DRV_MAC_NO_ERROR  ,                   
 DRV_MAC_ERROR_WRITE_MDIO_FAILED ,     
 DRV_MAC_ERROR_READ_MDIO_FAILED  ,
 DRV_MAC_ERROR_VPB_DRIVER_ERROR ,   
 DRV_MAC_INVALID_MAC_MODE
} 
DRV_MAC_ERROR ;

typedef enum
{
    DRV_MAC_DISABLE = 0,
    DRV_MAC_ENABLE = 1
}
E_DRV_MAC_ENABLE ;


#define DRV_MAC_EMAC_MIN    ( 0 )

typedef enum
{
    DRV_MAC_EMAC_0 = DRV_MAC_EMAC_MIN,
    DRV_MAC_EMAC_1,
    DRV_MAC_EMAC_2,
    DRV_MAC_EMAC_3,
    DRV_MAC_EMAC_4,
    DRV_MAC_EMAC_5,
    DRV_MAC_NUMBER_OF_EMACS
}
DRV_MAC_NUMBER ;

#define DRV_MAC_MGMT_CLOCK_DIVIDER_MIN ( 0 )

typedef enum
{
    DRV_MAC_MGMT_CLOCK_DIVIDER_4 = DRV_MAC_MGMT_CLOCK_DIVIDER_MIN ,
    DRV_MAC_MGMT_CLOCK_DIVIDER_6 ,
    DRV_MAC_MGMT_CLOCK_DIVIDER_8 ,
    DRV_MAC_MGMT_CLOCK_DIVIDER_10 ,
    DRV_MAC_MGMT_CLOCK_DIVIDER_14 ,
    DRV_MAC_MGMT_CLOCK_DIVIDER_20 ,
    DRV_MAC_MGMT_CLOCK_DIVIDER_28 ,
    DRV_MAC_MGMT_CLOCK_DIVIDER_LAST
}
DRV_MAC_MGMT_CLOCK_DIVIDER ;

#define DRV_MAC_RATE_MIN ( 0 )
#define DRV_MAC_PHY_RATE_MIN   ( DRV_MAC_RATE_MIN )

typedef enum
{
	DRV_MAC_PHY_RATE_10_FULL = DRV_MAC_PHY_RATE_MIN,
	DRV_MAC_PHY_RATE_10_HALF,
	DRV_MAC_PHY_RATE_100_FULL,
	DRV_MAC_PHY_RATE_100_HALF,
	DRV_MAC_PHY_RATE_1000_FULL,
	DRV_MAC_PHY_RATE_1000_HALF,

	DRV_MAC_PHY_RATE_LINK_DOWN,
	DRV_MAC_PHY_RATE_ERR,

	DRV_MAC_PHY_RATE_LAST

} DRV_MAC_PHY_RATE;

typedef enum
{
	DRV_MAC_RATE_10	= 0,	/* 10	Mb/s */
	DRV_MAC_RATE_100	= 2,	/* 100	Mb/s */
	DRV_MAC_RATE_1000	= 4,	/* 1000	Mb/s */
	DRV_MAC_RATE_LAST
}
DRV_MAC_RATE ;

typedef enum
{
	DRV_MAC_DUPLEX_MODE_FULL = 0,	/* Full Duplex */
	DRV_MAC_DUPLEX_MODE_HALF = 1,	/* Half Duplex */
	DRV_MAC_DUPLEX_MODE_LAST
}
DRV_MAC_DUPLEX_MODE ;

/****************************************************************************************************/
/* set drop packet DRV_MAC_DROP_PACKET_MASK in the EMAC config with "OR" type DRV_MAC_DROP_PACKET_MASK                                      */
/*                                                                                                  */
/* This enum is uded in order to create a DRV_MAC_DROP_PACKET_MASK, for example (1 << DRV_MAC_DROP_PACKET_MASK_RX_LONG_EVENT)               */
/* comments are taken from LILAC Registers spec (see EMAC0 section register at address FF08058)  */
/* the comments are long, but it is better to keep them in the source code, the reg spec is tricky  */
/****************************************************************************************************/
typedef enum
{

   /* 32 Receive Frame Truncated (16) */
   DRV_MAC_DROP_PACKET_MASK_RX_FRAME_TRUNCATED = 1 << 16,

   /* 31 Receive Long Event (15) */
   DRV_MAC_DROP_PACKET_MASK_RX_LONG_EVENT = 1 << 15,

   /* 30 Receive VLAN Tag Detected: Frame?s length/type field contained 0x8100 which is the VLAN Protocol Identifier (14) */
   DRV_MAC_DROP_PACKET_MASK_RX_VLAN_TAG_DETECTED = 1 << 14,

   /* 29 Receive Unsupported Op-code: Current Frame was recognized as a Control frame by the PEMCS, but it contained an Unknown
      Op-code.Customer may wish to qualify with inverse of CRCERR (~RSV[20]), and with length (641518) to verify frame
      was a valid Control Frame. (13) */
   DRV_MAC_DROP_PACKET_MASK_RX_UNSUP_OPCODE = 1 << 13,

   /* 28 Receive PAUSE Control Frame: Current frame was recognized as a Control frame containing a valid PAUSE Frame Op-code and a valid
     address. Customer may wish to qualify with inverse of CRCERR (~RSV[20]), and with length (641518) to verify frame was a valid Control
     Frame. (12) */
   DRV_MAC_DROP_PACKET_MASK_RX_PAUSE = 1 << 12,

   /* 27 Receive Control Frame: Current Frame was recognized as a Control frame for having a valid Type-Length designation.
      Customer may wish to qualify with inverse of CRCERR (~RSV[20]), and with length (641518) to verify frame
      was a valid Control Frame.) (11) */
   DRV_MAC_DROP_PACKET_MASK_RX_VALID_CONTROL = 1 << 11,

   /* 26 Receive Dribble Nibble: Indicates that after the end of the packet an additional 1 to 7 bits were received.
     A single nibble, called the dribble nibble, is formed but not sent to the system (10/100 Mb/s only) (10) */
   DRV_MAC_DROP_PACKET_MASK_RX_DRIBBLE_NIBBLE = 1 << 10,

   /* 25 Receive Broadcast: Packet?s destination address contained the broadcast address (9) */
   DRV_MAC_DROP_PACKET_MASK_RX_BROADCAST = 1 << 9,

   /* 24 Receive Multicast: Packet?s destination address contained a multicast address (8) */
   DRV_MAC_DROP_PACKET_MASK_RX_MULTICAST = 1 << 8,

   /* 23 Receive OK: Frame contained a valid CRC and did not have a code error (7) */
   DRV_MAC_DROP_PACKET_MASK_RX_OK = 1 << 7,

   /* 22 Receive Length Out of Range: Indicates that frame?s Length was larger than 1518 bytes but smaller than the Host?s Maximum Frame
      Length Value (Type Field) (6) */
   DRV_MAC_DROP_PACKET_MASK_RX_LEN_OUT_OF_RANGE = 1 << 6,

   /* 21 Receive Length Check Error: Indicates that frame length field value in the packet does not match the actual
      data byte length and is not a Type Field (5) */
   DRV_MAC_DROP_PACKET_MASK_RX_LEN_CHECK_ERROR = 1 << 5,

   /* 20 Receive CRC Error: The packet?s CRC did not match the internally generated CRC (4) */
   DRV_MAC_DROP_PACKET_MASK_RX_CRC_ERROR = 1 << 4,

   /* 19 Receive Code Error: One or more nibbles were signaled as errors during the reception of the packet (3) */
   DRV_MAC_DROP_PACKET_MASK_RX_CODE_ERROR = 1 << 3,

   /* 18 Receive False Carrier: Indicates that at some time since the last receive statistics vector,
     a false carrier was detected, noted and reported with this the next receive statistics. The false carrier is not associated with this
     packet. False carrier is activity on the receive channel that does not result in a packet receive attempt being made. Defined to
     be RX_ER = 1, RX_DV = 0, RXD[3:0] = 0xE (RXD[7:0] = 0x0E) (2) */
   DRV_MAC_DROP_PACKET_MASK_RX_FALSE_CARRIER = 1 << 2,

   /* 17 Receive RX_DV Event: Indicates that the last receive event seen was not long enough to be a valid packet (1) */
   DRV_MAC_DROP_PACKET_MASK_RX_DV_EVENT = 1 << 1,

   /* 16 Receive Previous Packet Dropped: Indicates that a packet since the last RSV was dropped (i.e. IFG too small) (0) */
   DRV_MAC_DROP_PACKET_MASK_RX_PREV_PACKET_DROPPED = 1 << 0

} DRV_MAC_DROP_PACKET_MASK;



typedef enum
{
    DRV_MAC_LOOPBACK_OFF ,
    DRV_MAC_LOOPBACK_ON 
}
DRV_MAC_LOOPBACK ;


#define DRV_MAC_MODE_MIN   ( 0 )

typedef enum
{      
    DRV_MAC_MODE_SGMII   = DRV_MAC_MODE_MIN      ,
    DRV_MAC_MODE_HISGMII    ,
    DRV_MAC_MODE_QSGMII     ,
    DRV_MAC_MODE_SMII       ,
    DRV_MAC_MODE_SS_SMII    ,
    DRV_MAC_MODE_RGMII      ,
    DRV_MAC_MODE_MII        , 
    DRV_MAC_MODE_NOT_APPLICABLE  ,/* not in use */
    DRV_MAC_MODE_LAST
}
DRV_MAC_MODE;


#define DRV_MAC_STORE_MODE_MIN   ( 0 )

typedef enum
{
    DRV_MAC_STORE_MODE_STORE_FORWARD = DRV_MAC_STORE_MODE_MIN ,
    DRV_MAC_STORE_MODE_CUT_THRU       ,
    DRV_MAC_STORE_MODE_LAST
}
DRV_MAC_STORE_MODE ;

/* MAC configuration structure */
typedef struct
{
   /* SGMII, HiSGMII, QSGMII,   */
   DRV_MAC_MODE mode ;

   /* preamble length. 7 is a recommended setting */
   int32_t preamble ;

   /* set rate */
   DRV_MAC_RATE rate ;

   /* set to non-zero for full-duplex mode */
   DRV_MAC_DUPLEX_MODE full_duplex ;

   /* if non-zero do not add CRC in Tx. if disable_crc is
     non-zero disable_pading field should be zero */
   E_DRV_MAC_ENABLE disable_crc ;

   /* set max frame length */
   uint32_t max_frame_length ;

   /* set fw mode */
    int32_t ctfws ;

   /* set minimum 4 bytes */
    int32_t min4bytes ;

   /* set maximum 4 bytes */
    int32_t max4bytes ;

/*
    These configuration bits are used to signal the drop frame
    conditions internal to the A-MCXFIF. The bits correspond to the
    Receive Statistics Vector on a one per one basis. For example
    bit 0 corresponds to RSV[16], and bit 1 corresponds to RSV[17].
    When these bits compare and the dont care is not asserted the
    frame will be dropped.
    The setting of this bits along with their dont care values in the
    hstfltrfrmdc configuration registers, create the filter that will assert
    the srdrpfrm output if the receive frame does not pass the
    acceptable conditions and should be dropped by the System. For
    example if its desired to drop a frame that contains a FCS Error,
    bit 4 would be set, and all receive frames that have RSV[20]
    asserted would be dropped.
    RECEIVE STATISTICS VECTOR
    Bit(s) Vector Bit / Description (error vector location)
    -- Ucad Unicast Address Detected (17)
    32 Receive Frame Truncated (16)
    31 Receive Long Event (15)
    30 Receive VLAN Tag Detected: Frames length/type field contained 0x8100 which is the VLAN Protocol Identifier (14)
    29 Receive Unsupported Op-code: Current Frame was recognized as a Control frame by the PEMCS, but it contained an Unknown Op-code. Customer may wish to qualify with inverse of CRCERR (~RSV[20]), and with length (64?1518) to verify frame was a valid Control Frame. (13)
    28 Receive PAUSE Control Frame: Current frame was recognized as a Control frame containing a valid PAUSE Frame Op-code and a valid address. Customer may wish to qualify with inverse of CRCERR (~RSV[20]), and with length (64?1518) to verify frame was a valid Control Frame. (12)
    27 Receive Control Frame: Current Frame was recognized as a Control frame for having a valid Type-Length designation. Customer may wish to qualify with inverse of CRCERR (~RSV[20]), and with length (64?1518) to verify frame was a valid Control Frame.) (11)
    26 Receive Dribble Nibble: Indicates that after the end of the packet an additional 1 to 7 bits were received. A single nibble, called the dribble nibble, is formed but not sent to the system (10/100 Mb/s only) (10)
    25 Receive Broadcast: Packets destination address contained the broadcast address (9)
    24 Receive Multicast: Packets destination address contained a multicast address (8)
    23 Receive OK: Frame contained a valid CRC and did not have a code error (7)
    22 Receive Length Out of Range: Indicates that frames Length was larger than 1518 (6)
    bytes but smaller than the Hosts Maximum Frame Length Value (Type Field)
    21 Receive Length Check Error: Indicates that frame length field value in the packet does not match the actual data byte length and is not a Type Field (5)
    20 Receive CRC Error: The packets CRC did not match the internally generated CRC (4)
    19 Receive Code Error: One or more nibbles were signaled as errors during the reception of the packet (3)
    18 Receive False Carrier: Indicates that at some time since the last receive statistics vector, a false carrier was detected, noted and reported with this the next receive statistics. The false carrier is not associated with this packet. False carrier is activity on the receive channel that does not result in a packet receive attempt being made. Defined to be RX_ER = 1, RX_DV = 0, RXD[3:0] = 0xE (RXD[7:0] = 0x0E) (2)
    17 Receive RX_DV Event: Indicates that the last receive event seen was not long enough to be a valid packet (1)
    16 Receive Previous Packet Dropped: Indicates that a packet since the last RSV was dropped (i.e. IFG too small) (0)
    15-0 Receive Byte Count: Total bytes in receive frame not counting collided bytes */

   uint32_t drop_mask ;

   /* xoff control */
   int32_t xoff_periodic ;
   uint32_t xoff_timer ;

   /* set to non-zero to disable padding of short packets */
   E_DRV_MAC_ENABLE disable_pading ;

   /* set to non-zero to disable check frame length */
   E_DRV_MAC_ENABLE disable_check_frame_length ;

   /* use non-zero to enforce preamble */
   E_DRV_MAC_ENABLE enable_preamble_length ;

   /* preamble length. 7 is a recommended setting */
   uint32_t preamble_length ;

   /* set to non-zero to allow huge frames */
   E_DRV_MAC_ENABLE allow_huge_frame ;

   /* non-zero if to set back2back_gap */
   E_DRV_MAC_ENABLE set_back2back_gap ;

   /* set gap value - back 2 back (set_back2back_gap should be non-zero) */
   uint32_t back2back_gap ;

   /* non-zero if to set non_back2back_gap */
   E_DRV_MAC_ENABLE set_non_back2back_gap ;

   /* set gap value - non back 2 back (set_non_back2back_gap should be non-zero) */
   uint32_t non_back2back_gap ;

   /* non-zero if to set min_interframe_gap */
   E_DRV_MAC_ENABLE set_min_interframe_gap ;

   /* set gap value - minimum interframe gap (set_min_interframe_gap should be non-zero) */
   uint32_t min_interframe_gap ;

   /* set flow control Rx, 0 - disable, 1 - enable */
   E_DRV_MAC_ENABLE flow_control_rx ;

   /* set flow control Tx, 0 - disable, 1 - enable */
   E_DRV_MAC_ENABLE flow_control_tx ;

   /* management clock divider */
   DRV_MAC_MGMT_CLOCK_DIVIDER clock_divider ;

   /* set to non-zero to leave MAC in Rx/Tx disabled          */
   /* application                                             */
   /* has to call function fi_drv_mac_rx_enable()      */
   /* after intialization. if this flag is zero -             */
   /* fi_drv_mac_init() will enable Rx/Tx              */
   E_DRV_MAC_ENABLE init_disabled ;

   /* rh_thdf_en, enables half duplex flow control (back pressure), 
   to be initiated by runner handler ,0 - disable, 1 - enable */
   E_DRV_MAC_ENABLE flow_half_duplex_en;
   
   /* rh_tcrq_en, enables full duplex flow control (pause generation), 
   to be initiated by runner handler,0 - disable, 1 - enable */
   E_DRV_MAC_ENABLE flow_full_duplex_en;

}
DRV_MAC_CONF ;

/* MAC transmit counters - read from NET/IF block*/
typedef struct
{
     uint32_t bytes;                /*Transmit Byte Counter*/
     uint32_t packets;              /*Transmit Packet Counter */
     uint32_t multicast_packets;    /*Transmit Multicast Packet Counter*/
     uint32_t broadcat_packets;     /*Transmit Broadcast Packet Counter  */
     uint32_t pause_packets;        /*Transmit PAUSE Control Frame Counter*/
     uint32_t deferral_packets;     /*Transmit Deferral Packet Counter*/
     uint32_t excessive_packets;    /*Transmit Excessive Deferral Packet Counter*/
     uint32_t single_collision;     /*Transmit Single Collision Packet Counter*/
     uint32_t multiple_collision;   /*Transmit Multiple Collision Packet Counter*/
     uint32_t late_collision;       /*Transmit Late Collision Packet Counter*/
     uint32_t excessive_collision;  /*Transmit Excessive Collision Packet Counter*/
     uint32_t total_collision;      /*Transmit Total Collision Counter*/
     uint32_t jabber_frames;        /*Transmit Jabber Frame Counter*/
     uint32_t fcs_errors;           /*Transmit FCS Error Counter*/
     uint32_t control_frames;       /*Transmit Control Frame Counter*/
     uint32_t oversize_frames;      /*Transmit Oversize Frame Counter*/
     uint32_t undersize_frames;     /*Transmit Undersize Frame Counter*/
     uint32_t fragmets;             /*Transmit Fragments Frame Counter*/
     uint32_t underrun;             /*Transmit underrun counter*/
     uint32_t transmission_errors;  /*Transmit error counter*/
     uint32_t frame_64_bytes;       /*Transmit 64 Byte Frame Counter*/
     uint32_t frame_65_127_bytes;   /*Transmit 65 to 127 Byte Frame Counter*/
     uint32_t frame_128_255_bytes;  /*Transmit 128 to 255 Byte Frame Counter*/
     uint32_t frame_256_511_bytes;  /*Transmit 256 to 511 Byte Frame Counter*/
     uint32_t frame_512_1023_bytes; /*Transmit 512 to 1023 Byte Frame Counter*/
     uint32_t frame_1024_1518_bytes;/*Transmit 1024 to 1518 Byte Frame Counter*/
     uint32_t frame_1519_mtu_bytes; /*Transmit 1519 to MTU Byte Frame Counter*/
}
DRV_MAC_TX_COUNTERS ;

/* MAC recieve counters - read from NET/IF block*/
typedef struct
{
     uint32_t bytes;                /*Receive Byte Counter*/
     uint32_t packets;              /*Receive Packet Counter */
     uint32_t fcs_errors;           /*Receive FCS Error Counter*/
     uint32_t multicast_packets;    /*Receive Multicast Packet Counter */
     uint32_t broadcat_packets;     /*Receive Broadcast Packet Counter */
     uint32_t control_packets;      /*Receive Control Frame Packet Counter*/
     uint32_t pause_packets;        /*Receive PAUSE Frame Packet Counter*/
     uint32_t unknown_op;           /*Receive Unknown Opcode Counter*/
     uint32_t alignment_error;      /*Receive Alignment Error Counter*/
     uint32_t length_error;         /*Receive Frame Length Error Counter*/
     uint32_t code_error;           /*Receive Code Error Counter*/
     uint32_t carrier_sense_error;  /*Receive Carrier Sense Error Counter*/
     uint32_t oversize_frames;      /*Receive Oversize Packet Counter*/
     uint32_t undersize_frames;     /*Receive Undersize Packet Counter*/
     uint32_t fragmets;             /*Receive Fragments Counter*/
     uint32_t jabber_frames;        /*Receive Jabber Counter*/
     uint32_t overflow;             /*Recieve overflow counter*/
     uint32_t frame_64_bytes;       /*Receive 64 Byte Frame Counter*/
     uint32_t frame_65_127_bytes;   /*Receive 65 to 127 Byte Frame Counter*/
     uint32_t frame_128_255_bytes;  /*Receive 128 to 255 Byte Frame Counter*/
     uint32_t frame_256_511_bytes;  /*Receive 256 to 511 Byte Frame Counter*/
     uint32_t frame_512_1023_bytes; /*Receive 512 to 1023 Byte Frame Counter*/
     uint32_t frame_1024_1518_bytes;/*Receive 1024 to 1518 Byte Frame Counter*/
     uint32_t frame_1519_mtu_bytes; /*Receive 1519 to MTU Byte Frame Counter*/
}
DRV_MAC_RX_COUNTERS ;

/* MAC eth address array*/
typedef struct {
    uint8_t b[6];
} DRV_MAC_ADDRESS;

/*********************************************************************/
/*                    Function Declarations                          */
/*********************************************************************/

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_configuration                                    */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Set Configuration                                  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function     sets the configuration for specific EMAC               */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2 , IPG_IFG , FIFO_REG_0 , FIFO_REG_1, FIFO_REG_3, FIFO_REG_4,   */
/*     FIFO_REG_5, GR_0, GR_1 , GR_2                                          */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_mac_conf - EMAC configuration (struct)                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_configuration ( DRV_MAC_NUMBER xi_index, 
                                             const DRV_MAC_CONF * xi_mac_conf ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_configuration                                    */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get Configuration                                  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the configuration for specific EMAC                   */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2 , IPG_IFG , FIFO_REG_0 , FIFO_REG_1, FIFO_REG_3, FIFO_REG_4,   */
/*     FIFO_REG_5, GR_0, GR_1 , GR_2                                          */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_mac_conf - EMAC configuration (struct)                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_configuration ( DRV_MAC_NUMBER xi_index, 
                                             DRV_MAC_CONF * const xo_mac_conf ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_status                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get Emac Status                                    */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the RX and TX status for specific EMAC                */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_tx_enable - TX enable : 0 (Disable) | 1 Enable                        */
/*                                                                            */
/*   xo_rx_enable - RX enable : 0 (Disable) | 1 Enable                        */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_status ( DRV_MAC_NUMBER xi_index, 
                                      E_DRV_MAC_ENABLE * const xo_tx_enable, 
                                      E_DRV_MAC_ENABLE * const xo_rx_enable ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_enable                                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Enable MAC                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function Enables specific EMAC                                      */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_enable - MAC enable : 0 (Disable) | 1 Enable                          */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_enable ( DRV_MAC_NUMBER xi_index, 
                                  E_DRV_MAC_ENABLE xi_enable ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_sw_reset                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Software reset                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function set the SW reset bit of specific EMAC                      */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_reset - MAC reset : 0 (out of reset) | 1 (reset )                     */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_sw_reset ( DRV_MAC_NUMBER xi_index,
                                    E_DRV_MAC_ENABLE xi_reset ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_reset_bit                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get reset bit                                      */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the SW reset bit of specific EMAC                  */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_reset - MAC reset : 0 (out of reset) | 1 (reset )                     */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_reset_bit ( DRV_MAC_NUMBER xi_index,
                                         E_DRV_MAC_ENABLE * const xo_reset_bit ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_mac_mgmt_read                                            */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - MDIO Read                                          */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function reads data from the MDIO controling the PHY                */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MII_MGMT_ADDR , MII_MGMT_IND , MII_MGMT_CMD                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_phy_address - represents the 5-bit PHY Address of Mgmt cycles         */
/*                                                                            */
/*   xi_reg_address - 5-bit Register Address of Mgmt cycles                   */
/*                                                                            */
/*   xo_value - 16 bits value read from the connected PHY                     */
/*                                                                            */
/* Output:   DRV_MAC_ERROR - Return code                                  */
/*                                                                            */
/******************************************************************************/
DRV_MAC_ERROR  fi_bl_drv_mac_mgmt_read  ( DRV_MAC_NUMBER xi_index, 
                                                             int32_t xi_phy_address , 
                                                             int32_t xi_reg_address , 
                                                             uint32_t * const xo_value ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_mac_mgmt_write                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - MDIO Write                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function write data to the MDIO controling the PHY                  */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MII_MGMT_ADDR , MII_MGMT_IND , MII_MGMT_CMD                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_phy_address - represents the 5-bit PHY Address of Mgmt cycles         */
/*                                                                            */
/*   xi_reg_address - 5-bit Register Address of Mgmt cycles                   */
/*                                                                            */
/*   xi_value - 16 bits value to write to the connected PHY                   */
/*                                                                            */
/* Output:   DRV_MAC_ERROR - Return code                                  */
/*                                                                            */
/******************************************************************************/
DRV_MAC_ERROR fi_bl_drv_mac_mgmt_write ( DRV_MAC_NUMBER xi_index, 
                                                            uint32_t xi_phy_address , 
                                                            uint32_t xi_reg_address , 
                                                            uint32_t xi_value ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_read_rx_counters                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Read RX Counters                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets all RX counters                                       */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_counters - RX counters (struct)                                       */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_read_rx_counters ( DRV_MAC_NUMBER xi_index, 
                                            DRV_MAC_RX_COUNTERS * const xo_counters ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_read_tx_counters                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Read TX Counters                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets all TX counters                                       */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_counters - TX counters (struct)                                       */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_read_tx_counters ( DRV_MAC_NUMBER xi_index, 
                                            DRV_MAC_TX_COUNTERS * const xo_counters ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_rate                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Set MAC Rate                                       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the rate for specific EMAC                            */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_rate - 0 10Mbps | 1 100bps | 2 1Gbps                                  */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_rate (DRV_MAC_NUMBER xi_index, 
                                   DRV_MAC_RATE xi_rate );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_rate                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get MAC Rate                                       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the rate for specific EMAC                            */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_rate - 0 10Mbps | 1 100bps | 2 1Gbps                                  */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_rate (DRV_MAC_NUMBER xi_index, 
                                   DRV_MAC_RATE * const xo_rate );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_duplex                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Set MAC Duplex                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the duplex mode for specific EMAC                     */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_duplex - 0 Half-Duplex | 1 Full-Duplex                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_duplex (DRV_MAC_NUMBER xi_index, 
                                     DRV_MAC_DUPLEX_MODE xi_duplex);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_duplex                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get MAC Duplex                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the duplex mode for specific EMAC                     */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_duplex - 0 Half-Duplex | 1 Full-Duplex                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_duplex (DRV_MAC_NUMBER xi_index, 
                                     DRV_MAC_DUPLEX_MODE * const xo_duplex);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_max_frame_length                                 */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Set MAC MTU                                        */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the MTU for specific EMAC                             */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MAX_FRM_LEN ,  MAX_CNT_VAL                                             */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_length - MTU : decimal                                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_max_frame_length ( DRV_MAC_NUMBER xi_index, 
                                                uint32_t xi_length ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_max_frame_length                                 */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get MAC MTU                                        */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the MTU for specific EMAC                             */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MAX_FRM_LEN ,  MAX_CNT_VAL                                             */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_length - MTU : decimal                                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_max_frame_length ( DRV_MAC_NUMBER xi_index, 
                                                uint32_t * const xo_length ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_loopback                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Enable MAC Loopback                                */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function Enables loopback on specific EMAC                          */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_loopback - MAC loopback : 0 (Disable) | 1 Enable                      */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_loopback ( DRV_MAC_NUMBER xi_index, 
                                        E_DRV_MAC_ENABLE xi_loopback ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_loopback                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get MAC Loopback                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the loopback mode for specific EMAC                */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_loopback - MAC loopback : 0 (Disable) | 1 Enable                      */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_loopback ( DRV_MAC_NUMBER xi_index , 
                                        E_DRV_MAC_ENABLE * const xo_loopback ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_flow_control                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Set MAC Flow Control                               */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the flow control mode for specific EMAC               */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_tx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/*   xi_rx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_flow_control ( DRV_MAC_NUMBER xi_index, 
                                            E_DRV_MAC_ENABLE xi_tx_flow_control, 
                                            E_DRV_MAC_ENABLE xi_rx_flow_control )  ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_flow_control                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Get MAC Flow Control                               */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the flow control mode for specific EMAC               */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_tx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/*   xo_rx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_flow_control ( DRV_MAC_NUMBER xi_index, 
                                            E_DRV_MAC_ENABLE * const xo_tx_flow_control, 
                                            E_DRV_MAC_ENABLE * const xo_rx_flow_control ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_pause_packets_transmision_parameters             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Lilac_Driver_MAC - Set MAC pause packets transmition parameters       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the transmision of pause packets :enable and interval */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     XOFF_CTRL                                                              */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_enable_tranmision - 0 Disable| 1 Enable                               */
/*                                                                            */
/*   xi_timer_interval - interval for sending pause packets                   */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_pause_packets_transmision_parameters ( DRV_MAC_NUMBER xi_index, 
                                                                    E_DRV_MAC_ENABLE xi_enable_trasnmision ,
                                                                    uint32_t xi_timer_interval ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_ctrl_pause_packets_generation_bit                          */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set MAC pause packets generation bit.              */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the generation bit of pause packets                   */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     NET_IF_CORE_GENERAL_ETH_GR_0_DTE                                       */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_bbh_tcrq_en_bit - 0 Disable| 1 Enable                                 */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_ctrl_pause_packets_generation_bit ( DRV_MAC_NUMBER xi_index,
                                                       E_DRV_MAC_ENABLE xi_bbh_tcrq_en_bit );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_modify_flow_control_pause_pkt_addr                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Modify the Flow Control pause pkt source address   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function modifies the flow control pause pkt source address         */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_1                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_2                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_3                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_4                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_5_TO_6_STATION_ADDR_5                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_5_TO_6_STATION_ADDR_6                  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_mac_addr - Flow Control mac address                                   */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_modify_flow_control_pause_pkt_addr ( DRV_MAC_NUMBER xi_index,
    DRV_MAC_ADDRESS xi_mac_addr);

#ifdef __cplusplus
}
#endif

#endif /* DRV_MAC_H_*/
