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

#define _BCMENET_LOCAL_

#include "bcm_OS_Deps.h"
#include "board.h"
#include "spidevices.h"
#include <bcm_map_part.h>
#include "bcm_intr.h"
#include "bcmmii.h"
#include "ethsw_phy.h"
#include "bcmswdefs.h"
#include "bcmsw.h"
/* Exports for other drivers */
#include "bcmsw_api.h"
#include "bcmswshared.h"
#include "bcmPktDma_defines.h"
#include "pktCmf_public.h"
#include "boardparms.h"
#include "bcmenet.h"
#if defined(CONFIG_BCM_GMAC) 
#include "bcmgmac.h"
#endif
#include "bcmswaccess.h"

#ifndef SINGLE_CHANNEL_TX
/* for enet driver txdma channel selection logic */
extern int channel_for_queue[NUM_EGRESS_QUEUES];
/* for enet driver txdma channel selection logic */
extern int use_tx_dma_channel_for_priority;
#endif /*SINGLE_CHANNEL_TX*/

extern struct semaphore bcm_ethlock_switch_config;
extern BcmEnet_devctrl *pVnetDev0_g;
extern extsw_info_t extSwInfo;

#if defined(CONFIG_BCM_GMAC)
void gmac_hw_stats( int port,  struct net_device_stats *stats );
int gmac_dump_mib(int port, int type);
void gmac_reset_mib( void );
#endif

#define ALL_PORTS_MASK                     0x1FF
#define ONE_TO_ONE_MAP                     0x00FAC688
#define MOCA_QUEUE_MAP                     0x0091B492
#define DEFAULT_FC_CTRL_VAL                0x1F
/* Tx: 0->0, 1->1, 2->2, 3->3. */
#define DEFAULT_IUDMA_QUEUE_SEL            0x688

#if defined(CONFIG_BCM96828) && !defined(CONFIG_EPON_HGU)
extern int uni_to_uni_enabled;
#endif

/* Forward declarations */
uint8_t  port_in_loopback_mode[TOTAL_SWITCH_PORTS] = {0};

/************************/
/* Ethernet Switch APIs */
/************************/
#if defined(CONFIG_BCM96818)
int enet_ioctl_ethsw_port_tagreplace(struct ethswctl_data *e)
{
    uint32_t val32;

    BCM_ENET_DEBUG("Port: %d \n ", e->port);
    if (e->port >= TOTAL_SWITCH_PORTS) {
        printk("Invalid Switch Port \n");
        return BCM_E_ERROR;
    }

    down(&bcm_ethlock_switch_config);

    ethsw_rreg(PAGE_QOS, REG_QOS_VID_REMAP + (e->port * 4), (uint8_t *)&val32, 4);
    BCM_ENET_DEBUG("REG_QOS_VID_REMAP Read Val = 0x%08x", (unsigned int)val32);
    if (e->type == TYPE_GET) {
        if (copy_to_user((void*)(&e->vlan_tag), (void*)&val32, sizeof(int))) {
            up(&bcm_ethlock_switch_config);
            return -EFAULT;
        }
    } else {
        switch (e->replace_type) {
            case REPLACE_VLAN_TAG:
                val32 = e->vlan_tag;
                BCM_ENET_DEBUG("Given VLAN Tag: 0x%08x \n ", e->vlan_tag);
                break;
            case REPLACE_VLAN_TPID:
                BCM_ENET_DEBUG("Given TPID: 0x%04x \n ", e->vlan_param);
                val32 &= (~(BCM_NET_VLAN_TPID_M << BCM_NET_VLAN_TPID_S));
                val32 |= ((e->vlan_param & BCM_NET_VLAN_TPID_M)
                           << BCM_NET_VLAN_TPID_S);
                break;
            case REPLACE_VLAN_TCI:
                BCM_ENET_DEBUG("Given TCI: 0x%04x \n ", e->vlan_param);
                val32 &= (~(BCM_NET_VLAN_TCI_M << BCM_NET_VLAN_TCI_S));
                val32 |= ((e->vlan_param & BCM_NET_VLAN_TCI_M)
                           << BCM_NET_VLAN_TCI_S);
                break;
            case REPLACE_VLAN_VID:
                BCM_ENET_DEBUG("Given VID: 0x%04x \n ", e->vlan_param);
                val32 &= (~(BCM_NET_VLAN_VID_M << BCM_NET_VLAN_VID_S));
                val32 |= ((e->vlan_param & BCM_NET_VLAN_VID_M)
                           << BCM_NET_VLAN_VID_S);
                break;
            case REPLACE_VLAN_8021P:
                BCM_ENET_DEBUG("Given 8021P: 0x%04x \n ", e->vlan_param);
                val32 &= (~(BCM_NET_VLAN_8021P_M << BCM_NET_VLAN_8021P_S));
                val32 |= ((e->vlan_param & BCM_NET_VLAN_8021P_M)
                           << BCM_NET_VLAN_8021P_S);
                break;
            case REPLACE_VLAN_CFI:
                BCM_ENET_DEBUG("Given CFI: 0x%04x \n ", e->vlan_param);
                val32 &= (~(BCM_NET_VLAN_CFI_M << BCM_NET_VLAN_CFI_S));
                val32 |= ((e->vlan_param & BCM_NET_VLAN_CFI_M)
                           << BCM_NET_VLAN_CFI_S);
                break;
            default:
                break;
        }
        BCM_ENET_DEBUG("REG_QOS_VID_REMAP Val Written = 0x%08x",
                       (unsigned int)val32);
        ethsw_wreg(PAGE_QOS, REG_QOS_VID_REMAP + (e->port * 4),
                   (uint8_t *)&val32, 4);
    }

    up(&bcm_ethlock_switch_config);
    return 0;
}

#endif // #if defined(CONFIG_BCM96818)

/* Stats API */
typedef enum bcm_hw_stat_e {
    TXOCTETSr = 0,
    TXDROPPKTSr,
    TXQOSPKTSr,
    TXBROADCASTPKTSr,
    TXMULTICASTPKTSr,
    TXUNICASTPKTSr,
    TXCOLLISIONSr,
    TXSINGLECOLLISIONr,
    TXMULTIPLECOLLISIONr,
    TXDEFERREDTRANSMITr,
    TXLATECOLLISIONr,
    TXEXCESSIVECOLLISIONr,
    TXFRAMEINDISCr,
    TXPAUSEPKTSr,
    TXQOSOCTETSr,
    RXOCTETSr,
    RXUNDERSIZEPKTSr,
    RXPAUSEPKTSr,
    PKTS64OCTETSr,
    PKTS65TO127OCTETSr,
    PKTS128TO255OCTETSr,
    PKTS256TO511OCTETSr,
    PKTS512TO1023OCTETSr,
    PKTS1024TO1522OCTETSr,
    RXOVERSIZEPKTSr,
    RXJABBERSr,
    RXALIGNMENTERRORSr,
    RXFCSERRORSr,
    RXGOODOCTETSr,
    RXDROPPKTSr,
    RXUNICASTPKTSr,
    RXMULTICASTPKTSr,
    RXBROADCASTPKTSr,
    RXSACHANGESr,
    RXFRAGMENTSr,
    RXEXCESSSIZEDISCr,
    RXSYMBOLERRORr,
    RXQOSPKTSr,
    RXQOSOCTETSr,
    PKTS1523TO2047r,
    PKTS2048TO4095r,
    PKTS4096TO8191r,
    PKTS8192TO9728r,
    MAXNUMCNTRS,
} bcm_hw_stat_t;


typedef struct bcm_reg_info_t {
    uint8_t offset;
    uint8_t len;
} bcm_reg_info_t;

#ifdef NO_CFE
void ethsw_reset(void)
{
#if !defined(CONFIG_BCM96818)
#if defined(EPHY_RST_MASK)
#if defined(CONFIG_BCM96368)
    // Power up and reset EPHYs
    GPIO->RoboswEphyCtrl = 0;
#else
    // reset EPHYs without changing other bits
    GPIO->RoboswEphyCtrl &= ~(EPHY_RST_MASK);
#endif
    msleep(1);

    // Take EPHYs out of reset
    GPIO->RoboswEphyCtrl  |= (EPHY_RST_MASK);
    msleep(1);
#endif
#endif

#if defined(GPHY_EEE_1000BASE_T_DEF)
    // Enable EEE on internal GPHY
    GPIO->RoboswGphyCtrl |= ( GPHY_LPI_FEATURE_EN_DEF_MASK |
                              GPHY_EEE_1000BASE_T_DEF | GPHY_EEE_100BASE_TX_DEF |
                              GPHY_EEE_PCS_1000BASE_T_DEF | GPHY_EEE_PCS_100BASE_TX_DEF );
#endif

#if defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828)
    // Take GPHY out of low pwer and disable IDDQ
    GPIO->RoboswGphyCtrl &= ~( GPHY_IDDQ_BIAS | GPHY_LOW_PWR );
    msleep(1);

    // Bring internal GigPhy out of reset
    PERF->softResetB &= ~SOFT_RST_GPHY;
    msleep(1);
    PERF->softResetB |= SOFT_RST_GPHY;
    msleep(1);
#endif


#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96318)
    GPIO->RoboswSwitchCtrl |= (RSW_MII_DUMB_FWDG_EN | RSW_HW_FWDG_EN);
#endif
#if defined(CONFIG_BCM96368) || defined(CONFIG_BCM96818)
    GPIO->RoboswEphyCtrl |= (RSW_MII_DUMB_FWDG_EN | RSW_HW_FWDG_EN);
#endif

    // Enable Switch clock
    PERF->blkEnables |= ROBOSW_CLK_EN;
#if defined(CONFIG_BCM96368)
    PERF->blkEnables |= SWPKT_SAR_CLK_EN | SWPKT_USB_CLK_EN;
#endif
#if defined(CONFIG_BCM96818)
    PERF->blkEnables |= SWPKT_GPON_CLK_EN | SWPKT_USB_CLK_EN;
#endif
    msleep(1);

    PERF->softResetB &= ~SOFT_RST_SWITCH;
    msleep(1);
    PERF->softResetB |= SOFT_RST_SWITCH;
    msleep(1);
}
#endif
#if defined(CONFIG_BCM_EXT_SWITCH)
void extsw_rgmii_config(void)
{
    unsigned char data8;
    unsigned char unit = 1, port;
    uint16 v16;
    int phy_id, rgmii_ctrl;
    unsigned long port_map;

    port_map = enet_get_portmap(unit);

    for (port = 0; port <= MAX_SWITCH_PORTS; port++)
    {
         /* No phy id for MIPS port */
         if (port != MIPS_PORT_ID) {
             phy_id =  enet_logport_to_phyid(PHYSICAL_PORT_TO_LOGICAL_PORT(port, unit));
             if (!(port_map & (1<<port)) || !IsRGMII(phy_id)) {
                 continue;
             }
         }
    
         rgmii_ctrl = port == MIPS_PORT_ID? EXTSW_REG_RGMII_CTRL_IMP:
                      port == EXTSW_RGMII_PORT? REG_RGMII_CTRL_P5: -1;
          if (rgmii_ctrl < 0) continue;

         extsw_rreg( PAGE_CONTROL, rgmii_ctrl, &data8, sizeof(data8));
#ifdef CONFIG_BCM968500
         data8 &= ~EXTSW_RGMII_TXCLK_DELAY;
#else
         data8 |= EXTSW_RGMII_TXCLK_DELAY;
#endif
         data8 &= ~EXTSW_RGMII_RXCLK_DELAY;
         extsw_wreg(PAGE_CONTROL, rgmii_ctrl, &data8, sizeof(data8));
         if (port == MIPS_PORT_ID) {
             continue;
         }
         if (IsPortPhyConnected(unit, port)) {
    
          // GTXCLK to be  off (phy reg 0x1c, shadow 3 bit 9)  - phy defaults to ON 
          // RXC skew to be on (phy reg 0x18, shadow 7 bit 9)  - phy defaults to ON
    
             v16 = MII_1C_SHADOW_CLK_ALIGN_CTRL << MII_1C_SHADOW_REG_SEL_S;
             ethsw_phy_wreg(phy_id, MII_REGISTER_1C, &v16);
             ethsw_phy_rreg(phy_id, MII_REGISTER_1C, &v16);
             v16 &= (~GTXCLK_DELAY_BYPASS_DISABLE);
             v16 |= MII_1C_WRITE_ENABLE;
             v16 &= ~(MII_1C_SHADOW_REG_SEL_M << MII_1C_SHADOW_REG_SEL_S);
             v16 |= (MII_1C_SHADOW_CLK_ALIGN_CTRL << MII_1C_SHADOW_REG_SEL_S);
             ethsw_phy_wreg(phy_id, MII_REGISTER_1C, &v16);
    
              /*
               * Follow it up with any overrides to the above Defaults.
               * It may be done in two stages. 
               * 1. In the interim, customers may reprogramm the above RGMII defaults on the
               *    phy, by devising  a series of bp_mdio_init_t phy init data in boardparms.
               *    Ex.,  {bp_pPhyInit,                .u.ptr = (void *)g_phyinit_xxx},
               * 2. Eventually, if phyinit stuff above qualifies to become main stream,
               *    this is the place to code Rgmii phy config overrides.
               */
         }
    }//for all ports
}
void extsw_init_config(void)
{
    /* Retrieve external switch id - this can only be done after other globals have been initialized */
    if (extSwInfo.present) {
        uint8 val[4] = {0};

        extsw_rreg(PAGE_MANAGEMENT, REG_DEV_ID, (uint8 *)&val, 4);
        extSwInfo.switch_id = swab32(*(uint32 *)val);

        /* Assumption : External switch is always in MANAGED Mode w/ TAG enabled.
         * BRCM TAG enable in external switch is done via MDK as well
         * but it is not deterministic when the userspace app for external switch
         * will run. When it gets delayed and the device is already getting traffic,
         * all those packets are sent to CPU without external switch TAG.
         * To avoid the race condition - it is better to enable BRCM_TAG during driver init. */
        extsw_rreg(PAGE_MANAGEMENT, REG_BRCM_HDR_CTRL, val, sizeof(val[0]));
        val[0] &= (~(BRCM_HDR_EN_GMII_PORT_5|BRCM_HDR_EN_IMP_PORT)); /* Reset HDR_EN bit on both ports */
        val[0] |= BRCM_HDR_EN_IMP_PORT; /* Set only for IMP Port */
        extsw_wreg(PAGE_MANAGEMENT, REG_BRCM_HDR_CTRL, val, sizeof(val[0]));
        extsw_rgmii_config();

        /* Initialize EEE on external switch */
        extsw_eee_init();
    }
}
#endif

void extsw_rreg(int page, int reg, uint8 *data, int len)
{
    if (((len != 1) && (len % 2) != 0) || len > 8)
        panic("extsw_rreg: wrong length!\n");

    switch (pVnetDev0_g->extSwitch->accessType)
    {
      case MBUS_MDIO:
          bcmsw_pmdio_rreg(page, reg, data, len);
        break;

      case MBUS_SPI:
      case MBUS_HS_SPI:
        bcmsw_spi_rreg(pVnetDev0_g->extSwitch->bus_num, pVnetDev0_g->extSwitch->spi_ss,
                       pVnetDev0_g->extSwitch->spi_cid, page, reg, data, len);
        break;
      default:
        printk("Error Access Type %d: Neither SPI nor PMDIO access in %s\n", 
            pVnetDev0_g->extSwitch->accessType, __func__);
        break;
    }
}

void extsw_wreg(int page, int reg, uint8 *data, int len)
{
    if (((len != 1) && (len % 2) != 0) || len > 8)
        panic("extsw_wreg: wrong length!\n");

    switch (pVnetDev0_g->extSwitch->accessType)
    {
      case MBUS_MDIO:
        bcmsw_pmdio_wreg(page, reg, data, len);
        break;

      case MBUS_SPI:
      case MBUS_HS_SPI:
        bcmsw_spi_wreg(pVnetDev0_g->extSwitch->bus_num, pVnetDev0_g->extSwitch->spi_ss,
                       pVnetDev0_g->extSwitch->spi_cid, page, reg, data, len);
        break;
      default:
        printk("Error Access Type %d: Neither SPI nor PMDIO access in %s\n", 
            pVnetDev0_g->extSwitch->accessType, __func__);
        break;
    }
}

void extsw_fast_age_port(uint8 port, uint8 age_static)
{
    uint8 v8;
    uint8 timeout = 100;

    v8 = FAST_AGE_START_DONE | FAST_AGE_DYNAMIC | FAST_AGE_PORT;
    if (age_static) {
        v8 |= FAST_AGE_STATIC;
    }
    extsw_wreg(PAGE_CONTROL, REG_FAST_AGING_PORT, &port, 1);

    extsw_wreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, &v8, 1);
    extsw_rreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, &v8, 1);
    while (v8 & FAST_AGE_START_DONE) {
        mdelay(1);
        extsw_rreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, &v8, 1);
        if (!timeout--) {
            printk("Timeout of fast aging \n");
            break;
        }
    }

    v8 = 0;
    extsw_wreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, &v8, 1);
}

void extsw_set_wanoe_portmap(uint16 wan_port_map)
{
    int i;
    uint8 map[2] = {0};
    //BCM_ENET_DEBUG("wanoe port map = 0x%x", wan_port_map);
    /* Set WANoE port map */
#if defined(CONFIG_BCM_RDPA) || defined(CONFIG_BCM_RDPA_MODULE)
    /* Runner based devices do not support WANoE on External switch - bail out */
    return;
#endif /* RDPA */
    map[0] = (uint8)wan_port_map;

    extsw_wreg(PAGE_CONTROL, REG_WAN_PORT_MAP, map, 2);

    map[0] = (uint8)wan_port_map;
    /* MIPS port */
    map[1] = 1;
    /* Disable learning */
    extsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, map, 2);

    for(i=0; i < TOTAL_SWITCH_PORTS; i++) {
       if((wan_port_map >> i) & 0x1) {
            extsw_fast_age_port(i, 1);
       }
    }
}

void extsw_rreg_wrap(int page, int reg, uint8 *data, int len)
{
   uint8 val[4];

   extsw_rreg(page, reg, val, len);
   switch (len) {
   case 1:
      data[0] = val[0];
      break;
   case 2:
      *((uint16 *)data) = __le16_to_cpu(*((uint16 *)val));
      break;
   case 4:
      *((uint32 *)data) = __le32_to_cpu(*((uint32 *)val));
      break;
   default:
      printk("Length %d not supported\n", len);
      break;
   }
}

void extsw_wreg_wrap(int page, int reg, uint8 *data, int len)
{
   uint8 val[4];

   switch (len) {
   case 1:
      val[0] = data[0];
      break;
   case 2:
      *((uint16 *)val) = __cpu_to_le16(*((uint16 *)data));
      break;
   case 4:
      *((uint32 *)val) = __cpu_to_le32(*((uint32 *)data));
      break;
   default:
      printk("Length %d not supported\n", len);
      break;
   }
   extsw_wreg(page, reg, val, len);
}

static void fast_age_start_done_ext(uint8_t ctrl) {
    uint8_t timeout = 100;

    extsw_wreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, (uint8_t *)&ctrl, 1);
    extsw_rreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, (uint8_t *)&ctrl, 1);
    while (ctrl & FAST_AGE_START_DONE) {
        mdelay(1);
        extsw_rreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, (uint8_t *)&ctrl, 1);
        if (!timeout--) {
            printk("Timeout of fast aging \n");
            break;
        }
    }

    ctrl = 0;
    extsw_wreg(PAGE_CONTROL, REG_FAST_AGING_CTRL, (uint8_t *)&ctrl, 1);
}

void fast_age_all_ext(uint8_t age_static)
{
    uint8_t v8;

    v8 = FAST_AGE_START_DONE | FAST_AGE_DYNAMIC;
    if (age_static) {
        v8 |= FAST_AGE_STATIC;
    }

    fast_age_start_done_ext(v8);
}

void enet_arl_read_ext( uint8_t *mac, uint32_t *vid, uint32_t *val )
{
    int timeout, count = 0;
    uint32_t v32=0, cur_data_v32 = 0;
    uint16_t cur_vid_v16;
    uint8_t v8, mac_vid_v64[8];

    *val = 0;

    /* Setup ARL Search */
    v8 = ARL_SRCH_CTRL_START_DONE;
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
    extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
    BCM_ENET_DEBUG("ARL_SRCH_CTRL (0x%02x%02x) = 0x%x \n",PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, v8);
    while (v8 & ARL_SRCH_CTRL_START_DONE) {
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
        timeout = 1000;
        /* Now read the Search Ctrl Reg Until :
         * Found Valid ARL Entry --> ARL_SRCH_CTRL_SR_VALID, or
         * ARL Search done --> ARL_SRCH_CTRL_START_DONE */
        while((v8 & ARL_SRCH_CTRL_SR_VALID) == 0) {
            extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx,
                       (uint8_t *)&v8, 1);
            if (v8 & ARL_SRCH_CTRL_START_DONE) {
                mdelay(1);
                if (timeout-- <= 0) {
                    printk("ARL Search Timeout for Valid to be 1 \n");
                    return;
                }
            } else {
                BCM_ENET_DEBUG("ARL Search Done count %d\n", count);
                return;
            }
        }
        /* Found a valid entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_MAC_LO_ENTRY_531xx,&mac_vid_v64[0], 8);
        BCM_ENET_DEBUG("ARL_SRCH_result (%02x%02x%02x%02x%02x%02x%02x%02x) \n",
                       mac_vid_v64[0],mac_vid_v64[1],mac_vid_v64[2],mac_vid_v64[3],mac_vid_v64[4],
                       mac_vid_v64[5],mac_vid_v64[6],mac_vid_v64[7]);

        cur_vid_v16 = mac_vid_v64[6] | ((mac_vid_v64[7]&0xF)<<8);
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_DATA_ENTRY_531xx,(uint8_t *)&cur_data_v32, 4);
        cur_data_v32 = swab32(cur_data_v32);
        BCM_ENET_DEBUG("ARL_SRCH_result VID %d, DATA = 0x%08x \n", cur_vid_v16, cur_data_v32);
        
        /* ARL Search results are read; Mark it done(by reading the reg)
           so ARL will start searching the next entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_DATA_RESULT_DONE_531xx,
                   (uint8_t *)&v32, 4);
       
        /* Check if found the matching ARL entry */
        if ( 0 == memcmp(&mac[0], &mac_vid_v64[0], 6) ) {
            /* found the matching entry, mac already set correctly, update VID and val */
            BCM_ENET_DEBUG("Found matching ARL Entry\n");
            *vid = swab16(cur_vid_v16);
            *val = swab32(cur_data_v32);
            return;
        }
        
        if ((count++) > NUM_ARL_ENTRIES) {
            break;
        }
        /* Now read the ARL Search Ctrl reg. again for next entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
        BCM_ENET_DEBUG("ARL_SRCH_CTRL (0x%02x%02x) = 0x%x \n",PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, v8);
    }
    /* entry not found */
}

void enet_arl_write_ext( uint8_t *mac, uint16_t vid, uint32_t val )
{
    int timeout;
    uint16_t v16;
    uint32_t v32;
    uint8_t v8;
    unsigned char mac_vid_v64[8];


    /* try to find the entry
       write the MAC and VID */    
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_MAC_INDX_LO, (uint8_t *)&mac[0], 6);
    v16 = swab16(vid);
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_VLAN_INDX, (uint8_t *)&v16, 2);
    /* Initiate a read transaction */
    v8 = ARL_TBL_CTRL_START_DONE | ARL_TBL_CTRL_READ;
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
    extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
    timeout = 10;
    while(v8 & ARL_TBL_CTRL_START_DONE) {
        mdelay(1);
        if (timeout-- <= 0)  {
            printk("Error timeout - can't read/find the ARL entry\n");
            return;
        }
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
    }

    /* Read transaction complete - get the MAC + VID */
    extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_MAC_LO_ENTRY, &mac_vid_v64[0], 8);
    BCM_ENET_DEBUG("ARL_SRCH_result (%02x%02x%02x%02x%02x%02x%02x%02x) \n",
                   mac_vid_v64[0],mac_vid_v64[1],mac_vid_v64[2],mac_vid_v64[3],mac_vid_v64[4],mac_vid_v64[5],mac_vid_v64[6],mac_vid_v64[7]);

    mac_vid_v64[0] = mac[0];
    mac_vid_v64[1] = mac[1];
    mac_vid_v64[2] = mac[2];
    mac_vid_v64[3] = mac[3];
    mac_vid_v64[4] = mac[4];
    mac_vid_v64[5] = mac[5];
    mac_vid_v64[6] = vid & 0xFF;
    mac_vid_v64[7] = (vid >> 8) & 0xF;
  
    /* write MAC and VID */
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_MAC_LO_ENTRY, (uint8_t *)&mac_vid_v64, 8);

    /* write data */
    v32 = swab32(val);
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_DATA_ENTRY,(uint8_t *)&v32, 4);

    /* Initiate a write transaction */
    v8 = ARL_TBL_CTRL_START_DONE;
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
    timeout = 10;
    extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
    while(v8 & ARL_TBL_CTRL_START_DONE) {
        mdelay(1);
        if (timeout-- <= 0)  {
            printk("Error - can't write/find the ARL entry\n");
            break;
        }
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
    }

    return;
}

int enet_arl_access_dump_ext(void)
{
    int timeout, count = 0;
    uint32_t cur_mac_lo, cur_mac_hi, cur_vid, cur_data, v32;
    uint8_t v8, mac_vid[8];


    v8 = ARL_SRCH_CTRL_START_DONE;
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
    extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
    BCM_ENET_DEBUG("ARL_SRCH_CTRL (0x%02x%02x) = 0x%x \n",PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, v8);
    while (v8 & ARL_SRCH_CTRL_START_DONE) {
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
        timeout = 1000;
        /* Now read the Search Ctrl Reg Until :
         * Found Valid ARL Entry --> ARL_SRCH_CTRL_SR_VALID, or
         * ARL Search done --> ARL_SRCH_CTRL_START_DONE */
        while((v8 & ARL_SRCH_CTRL_SR_VALID) == 0) {
            extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx,
                       (uint8_t *)&v8, 1);
            if (v8 & ARL_SRCH_CTRL_START_DONE) {
                mdelay(1);
                if (timeout-- <= 0) {
                    printk("ARL Search Timeout for Valid to be 1 \n");
                    return BCM_E_NONE;
                }
            } else {
                printk("External Switch ARL Search Done count %d\n", count);
                return BCM_E_NONE;
            }
        }
        /* Found a valid entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_MAC_LO_ENTRY_531xx,&mac_vid[0], 8);
        BCM_ENET_DEBUG("ARL_SRCH_result (%02x%02x%02x%02x%02x%02x%02x%02x) \n",
                       mac_vid[0],mac_vid[1],mac_vid[2],mac_vid[3],mac_vid[4],mac_vid[5],mac_vid[6],mac_vid[7]);
        cur_mac_lo = mac_vid[0] | (mac_vid[1] << 8) | (mac_vid[2] << 16) | (mac_vid[3] << 24);
        cur_mac_hi = mac_vid[4] | (mac_vid[5] << 8);
        cur_vid = mac_vid[6] | ((mac_vid[7]&0xF)<<8);
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_DATA_ENTRY_531xx,(uint8_t *)&cur_data, 4);
        cur_data = swab32(cur_data);
        BCM_ENET_DEBUG("ARL_SRCH_result (0x%02x%08x : %d) \n",cur_mac_hi,cur_mac_lo,cur_vid);
        BCM_ENET_DEBUG("ARL_SRCH_DATA = 0x%08x \n", cur_data);

        /* ARL Search results are read; Mark it done(by reading the reg)
           so ARL will start searching the next entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_DATA_RESULT_DONE_531xx,
                   (uint8_t *)&v32, 4);
        if (count % 10 == 0) {
            printk(" VLAN  MAC          DATA <Switch : %x> ",extSwInfo.switch_id);
            printk("(static-15,age-14,pri-13:11,pmap-9:1)");
        }
        printk("\n %04d  %04x%08x 0x%04x", cur_vid,cur_mac_hi,cur_mac_lo, (cur_data&0xFE00)|((cur_data&0xFF)<<1));
        if ((count++) > NUM_ARL_ENTRIES) {
            printk("External Switch ARL Search Done \n");
            break;
        }
        /* Now read the ARL Search Ctrl reg. again for next entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
        BCM_ENET_DEBUG("ARL_SRCH_CTRL (0x%02x%02x) = 0x%x \n",PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, v8);
    }
    printk("\nExternal Switch ARL Search Done count %d\n", (int)count);
    return BCM_E_NONE;
}
void enet_arl_dump_ext_multiport_arl(void)
{
    uint16 v16;
    uint8 addr[8];
    int i, enabled;
    uint32 vect;

    extsw_rreg(PAGE_ARLCTRL, 0xe, (uint8 *)&v16, 2);
    v16=__cpu_to_le16(v16);
    enabled = v16 & 2;
    
    printk("Multiport Address: %s\n", enabled? "Enabled": "Disabled"); 

    if (enabled)
    {
        for (i=0; i<6; i++)
        {
            extsw_rreg(PAGE_ARLCTRL, REG_MULTIPORT_ADDR1_LO + i*16, (uint8 *)&addr, sizeof(addr));
            extsw_rreg(PAGE_ARLCTRL, REG_MULTIPORT_VECTOR1 + i*16, (uint8 *)&vect, sizeof(vect));
            vect = __cpu_to_le32(vect);
            printk("Mport Eth Type: 0x%04x, Mport Addrs: %02x:%02x:%02x:%02x:%02x:%02x, Port Map %04x\n",
                    *(uint16 *)(addr+6),
                    addr[5],
                    addr[4],
                    addr[3],
                    addr[2],
                    addr[1],
                    addr[0],
                    (int)vect);
        }
    }
}


int remove_arl_entry_ext(uint8_t *mac)
{
    int timeout, count = 0;
    uint32_t v32=0, cur_data_v32 = 0;
    uint8_t v8, mac_vid_v64[8];
    unsigned int extSwId;

    extSwId = bcm63xx_enet_extSwId();
    if (extSwId != 0x53115 && extSwId != 0x53125)
    {
        /* Currently only these two switches are supported */
        printk("Error - Ext-switch = 0x%x not supported\n", extSwId);
        return BCM_E_NONE;
    }

    BCM_ENET_DEBUG("mac: %02x %02x %02x %02x %02x %02x\n", mac[0],
                    mac[1], mac[2], mac[3], mac[4], (uint8_t)mac[5]);
    /* Setup ARL Search */
    v8 = ARL_SRCH_CTRL_START_DONE;
    extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
    extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
    BCM_ENET_DEBUG("ARL_SRCH_CTRL (0x%02x%02x) = 0x%x \n",PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, v8);
    while (v8 & ARL_SRCH_CTRL_START_DONE) {
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
        timeout = 1000;
        /* Now read the Search Ctrl Reg Until :
         * Found Valid ARL Entry --> ARL_SRCH_CTRL_SR_VALID, or
         * ARL Search done --> ARL_SRCH_CTRL_START_DONE */
        while((v8 & ARL_SRCH_CTRL_SR_VALID) == 0) {
            extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx,
                       (uint8_t *)&v8, 1);
            if (v8 & ARL_SRCH_CTRL_START_DONE) {
                mdelay(1);
                if (timeout-- <= 0) {
                    printk("ARL Search Timeout for Valid to be 1 \n");
                    return BCM_E_NONE;
                }
            } else {
                BCM_ENET_DEBUG("ARL Search Done count %d\n", count);
                return BCM_E_NONE;
            }
        }
        /* Found a valid entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_MAC_LO_ENTRY_531xx,&mac_vid_v64[0], 8);
        BCM_ENET_DEBUG("ARL_SRCH_result (%02x%02x%02x%02x%02x%02x%02x%02x) \n",
                       mac_vid_v64[0],mac_vid_v64[1],mac_vid_v64[2],mac_vid_v64[3],mac_vid_v64[4],mac_vid_v64[5],mac_vid_v64[6],mac_vid_v64[7]);
        /* ARL Search results are read; Mark it done(by reading the reg)
           so ARL will start searching the next entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_DATA_RESULT_DONE_531xx,
                   (uint8_t *)&v32, 4);
        /* Check if found the matching ARL entry */
        if ( mac_vid_v64[0] == mac[5] && 
             mac_vid_v64[1] == mac[4] &&
             mac_vid_v64[2] == mac[3] &&
             mac_vid_v64[3] == mac[2] &&
             mac_vid_v64[4] == mac[1] &&
             mac_vid_v64[5] == mac[0])
        { /* found the matching entry ; invalidate it */
            BCM_ENET_DEBUG("Found matching ARL Entry\n");
            /* Write the MAC Address */
            extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_MAC_INDX_LO, (uint8_t *)&mac_vid_v64[0], 8);
            /* Initiate a read transaction */
            v8 = ARL_TBL_CTRL_START_DONE | ARL_TBL_CTRL_READ;
            extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
            extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
            timeout = 10;
            while(v8 & ARL_TBL_CTRL_START_DONE) {
                mdelay(1);
                if (timeout-- <= 0)  {
                    printk("Error - can't read/find the ARL entry\n");
                    return 0;
                }
                extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
            }
            /* Read transaction complete - get the MAC + VID */
            extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_MAC_LO_ENTRY,&mac_vid_v64[0], 8);
            BCM_ENET_DEBUG("ARL_SRCH_result (%02x%02x%02x%02x%02x%02x%02x%02x) \n",
                           mac_vid_v64[0],mac_vid_v64[1],mac_vid_v64[2],mac_vid_v64[3],mac_vid_v64[4],mac_vid_v64[5],mac_vid_v64[6],mac_vid_v64[7]);
            /* Compare the MAC */
            if (!(mac_vid_v64[0] == mac[5] && 
                  mac_vid_v64[1] == mac[4] &&
                  mac_vid_v64[2] == mac[3] &&
                  mac_vid_v64[3] == mac[2] &&
                  mac_vid_v64[4] == mac[1] &&
                  mac_vid_v64[5] == mac[0]))
            {
                printk("Error - can't find the requested ARL entry\n");
                return 0;
            }
            /* Read the associated data entry */
            extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_DATA_ENTRY,(uint8_t *)&cur_data_v32, 4);
            BCM_ENET_DEBUG("ARL_SRCH_DATA = 0x%08x \n", cur_data_v32);
            cur_data_v32 = swab32(cur_data_v32);
            /* Invalidate the entry -> clear valid bit */
            cur_data_v32 &= 0xFFFF; 
            cur_data_v32 = swab32(cur_data_v32);
            /* Modify the data entry for this ARL */
            extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_DATA_ENTRY,(uint8_t *)&cur_data_v32, 4);
            /* Initiate a write transaction */
            v8 = ARL_TBL_CTRL_START_DONE;
            extsw_wreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
            timeout = 10;
            extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
            while(v8 & ARL_TBL_CTRL_START_DONE) {
                mdelay(1);
                if (timeout-- <= 0)  {
                    printk("Error - can't write/find the ARL entry\n");
                    break;
                }
                extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_TBL_CTRL, &v8, 1);
            }
            return 0;
        }
        if ((count++) > NUM_ARL_ENTRIES) {
            break;
        }
        /* Now read the ARL Search Ctrl reg. again for next entry */
        extsw_rreg(PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, (uint8_t *)&v8, 1);
        BCM_ENET_DEBUG("ARL_SRCH_CTRL (0x%02x%02x) = 0x%x \n",PAGE_AVTBL_ACCESS, REG_ARL_SRCH_CTRL_531xx, v8);
    }

    return 0; /* Actually this is error but no-one should care the return value */
}
int remove_arl_entry_wrapper(void *ptr)
{
    int ret = 0;
    ret = remove_arl_entry(ptr); /* remove entry from internal switch */
    if (bcm63xx_enet_isExtSwPresent()) {
        ret = remove_arl_entry_ext(ptr); /* remove entry from internal switch */
    }
    return ret;
}
void bcmeapi_reset_mib_ext(void)
{
#ifdef REPORT_HARDWARE_STATS
    uint8_t val8;
    if (pVnetDev0_g->extSwitch->present) {
        extsw_rreg(PAGE_MANAGEMENT, REG_GLOBAL_CONFIG, &val8, 1);
        val8 |= GLOBAL_CFG_RESET_MIB;
        extsw_wreg(PAGE_MANAGEMENT, REG_GLOBAL_CONFIG, &val8, 1);
        val8 &= (~GLOBAL_CFG_RESET_MIB);
        extsw_wreg(PAGE_MANAGEMENT, REG_GLOBAL_CONFIG, &val8, 1);
    }

#endif
    return ;
}

int bcmsw_dump_mib_ext(int port, int type)
{
    unsigned int v32;
    uint8 data[8] = {0};
    ETHERNET_MAC_INFO EnetInfo[BP_MAX_ENET_MACS];
    ETHERNET_MAC_INFO *info;

    if(BpGetEthernetMacInfo(&EnetInfo[0], BP_MAX_ENET_MACS) != BP_SUCCESS) 
    {
            printk(KERN_DEBUG " board id not set\n");
            return -ENODEV;
    }
    info = &EnetInfo[1];

    if (!((info->ucPhyType == BP_ENET_EXTERNAL_SWITCH) || 
         (info->ucPhyType == BP_ENET_SWITCH_VIA_INTERNAL_PHY)))
    {
        printk("No External switch connected\n");
        return 0;
    }

	info = &EnetInfo[1];
	if ((port != 8) && !(info->sw.port_map & (1<<port))) /* Only IMP port and ports in the port_map are allowed */
	{
		printk("Invalid/Unused External switch port %d\n",port);
		return -ENODEV;
	}
	
    /* Display Tx statistics */
    printk("External Switch Stats : Port# %d\n",port);
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXUPKTS, data, 4);  // Get TX unicast packet count
    v32 = swab32(*(uint32 *)data);                                
    printk("TxUnicastPkts:          %10u \n", v32);
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXMPKTS, data, 4);  // Get TX multicast packet count
    v32 = swab32(*(uint32 *)data);                                
    printk("TxMulticastPkts:        %10u \n",  v32);
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXBPKTS, data, 4);  // Get TX broadcast packet count
    v32 = swab32(*(uint32 *)data);                                
    printk("TxBroadcastPkts:        %10u \n", v32);
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXDROPS, data, 4);
    v32 = swab32(*(uint32 *)data);                                
    printk("TxDropPkts:             %10u \n", v32);

    if (type) 
    {
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXOCTETS, data, 8);
        v32 = swab32(((uint32 *)data)[0]);                                
        printk("TxOctetsLo:             %10u \n", v32);
        v32 = swab32(((uint32 *)data)[1]);                                
        printk("TxOctetsHi:             %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXQ0PKT, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxQ0Pkts:               %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXCOL, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxCol:                  %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXSINGLECOL, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxSingleCol:            %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXMULTICOL, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxMultipleCol:          %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXDEFERREDTX, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxDeferredTx:           %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXLATECOL, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxLateCol:              %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXEXCESSCOL, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxExcessiveCol:         %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXFRAMEINDISC, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxFrameInDisc:          %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXPAUSEPKTS, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("TxPausePkts:            %10u \n", v32);
    }
    /* Display Rx statistics */
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXUPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);  // Get RX unicast packet count
    v32 = swab32(*(uint32 *)data);                                
    printk("RxUnicastPkts:          %10u \n", v32);
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXMPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);  // Get RX multicast packet count
    v32 = swab32(*(uint32 *)data);                                
    printk("RxMulticastPkts:        %10u \n",v32);
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXBPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);  // Get RX broadcast packet count
    v32 = swab32(*(uint32 *)data);                                
    printk("RxBroadcastPkts:        %10u \n",v32);
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXDROPS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
    v32 = swab32(*(uint32 *)data);                                
    printk("RxDropPkts:             %10u \n",v32); 
    extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXDISCARD_EXT, data, 4);
    v32 = swab32(*(uint32 *)data);                                
    printk("RxDiscard:              %10u \n", v32);
   
    if (type) 
    {
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXJABBERS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxJabbers:              %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXALIGNERRORS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxAlignErrs:            %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXFCSERRORS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxFCSErrs:              %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXFRAGMENTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxFragments:            %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXOVERSIZE + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxOversizePkts:         %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXOUTOFRANGE_EXT, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxOutOfRange:           %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXUNDERSIZEPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxUndersizePkts:        %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXPAUSEPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxPausePkts:            %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXSACHANGES + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxSAChanges:            %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXSYMBOLERRORS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxSymbolError:          %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RX64OCTPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxPkts64Octets:         %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RX127OCTPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxPkts65to127Octets:    %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RX255OCTPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxPkts128to255Octets:   %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RX511OCTPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxPkts256to511Octets:   %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RX1023OCTPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxPkts512to1023Octets:  %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXMAXOCTPKTS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxPkts1024OrMoreOctets: %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXJUMBOPKT_EXT , data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxJumboPkts:            %10u \n", v32);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXOUTOFRANGE_EXT, data, 4);
        v32 = swab32(*(uint32 *)data);                                
        printk("RxOutOfRange:           %10u \n", v32);
    }

    return 0;
}

int bcmsw_set_multiport_address_ext(uint8_t* addr)
{
    uint32 v32;

    if (bcm63xx_enet_isExtSwPresent())
    {
        uint16 v16;
        uint8 v64[8];

        /* Enable multiport addresses 0 */
        v16 = swab16(0x2);
        extsw_wreg(PAGE_ARLCTRL, REG_MULTIPORT_CTRL, (uint8 *)&v16, 2);
        v64[5]=addr[0];	v64[4]=addr[1];	v64[3]=addr[2];	v64[2]=addr[3];	
        v64[1]=addr[4];	v64[0]=addr[5];	v64[6]=0;	v64[7]=0;	
        extsw_wreg(PAGE_ARLCTRL, REG_MULTIPORT_ADDR1_LO, (uint8 *)&v64, sizeof(v64));
        v32=__cpu_to_le32(PBMAP_MIPS);
        extsw_wreg(PAGE_ARLCTRL, REG_MULTIPORT_VECTOR1, (uint8 *)&v32, sizeof(v32));
    }

    return 0;
}

int bcmeapi_ioctl_set_multiport_address(struct ethswctl_data *e)
{
    if (e->unit == 0) {
       if (e->type == TYPE_GET) {
           return BCM_E_NONE;
       } else if (e->type == TYPE_SET) {
           bcmeapi_set_multiport_address(e->mac);
           bcmsw_set_multiport_address_ext(e->mac);
           return BCM_E_PARAM;
       }
    }    
    return BCM_E_NONE;
}

#ifdef REPORT_HARDWARE_STATS
int bcmsw_get_hw_stats(int port, int extswitch, struct net_device_stats *stats)
{
    uint64 ctr64 = 0;           // Running 64 bit counter
    uint8 data[8] = {0};
    uint64 tempctr64 = 0;       // Temp 64 bit counter

    if (extswitch) {
    
        // Track RX unicast, multicast, and broadcast packets
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXUPKTS 
                              + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);  // Get RX unicast packet count
        ctr64 = swab32(*(uint32 *)data);                                // Keep running count
            
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXMPKTS 
                              + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);  // Get RX multicast packet count
        tempctr64 = swab32(*(uint32 *)data);
        stats->multicast = tempctr64;                                   // Save away count
        ctr64 += tempctr64;                                             // Keep running count
        
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXBPKTS 
                              + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);  // Get RX broadcast packet count
        tempctr64 = swab32(*(uint32 *)data);
        stats->rx_broadcast_packets = tempctr64;                        // Save away count
        ctr64 += tempctr64;                                             // Keep running count
        stats->rx_packets = (unsigned long)ctr64;
        
        // Dump RX debug data if needed
        BCM_ENET_DEBUG("read data = %02x %02x %02x %02x \n",
            data[0], data[1], data[2], data[3]);
        BCM_ENET_DEBUG("ctr64 = %x \n", (unsigned int)ctr64);
        
        // Track RX byte count
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXOCTETS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 8);
        ((uint32 *)data)[0] = swab32(((uint32 *)data)[0]);
        ((uint32 *)data)[1] = swab32(((uint32 *)data)[1]);
        stats->rx_bytes = *(unsigned long *)data;
        
        // Track RX packet errors
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXDROPS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        stats->rx_dropped = swab32(*(uint32 *)data);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXFCSERRORS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        stats->rx_errors = swab32(*(uint32 *)data);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXSYMBOLERRORS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        stats->rx_errors += swab32(*(uint32 *)data);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_RXALIGNERRORS + REG_MIB_P0_EXTSWITCH_OFFSET, data, 4);
        stats->rx_errors += swab32(*(uint32 *)data);

        // Track TX unicast, multicast, and broadcast packets
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXUPKTS, data, 4);  // Get TX unicast packet count
        ctr64 = swab32(*(uint32 *)data);                                // Keep running count

        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXMPKTS, data, 4);  // Get TX multicast packet count
        tempctr64 = swab32(*(uint32 *)data);
        stats->tx_multicast_packets = tempctr64;                        // Save away count
        ctr64 += tempctr64;                                             // Keep running count

        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXBPKTS, data, 4);  // Get TX broadcast packet count
        tempctr64 = swab32(*(uint32 *)data);
        stats->tx_broadcast_packets = tempctr64;                        // Save away count
        ctr64 += tempctr64;                                             // Keep running count        
        stats->tx_packets = (unsigned long)ctr64;
        
        // Track TX byte count
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXOCTETS, data, 8);
        ((uint32 *)data)[0] = swab32(((uint32 *)data)[0]);
        ((uint32 *)data)[1] = swab32(((uint32 *)data)[1]);
        stats->tx_bytes = *(unsigned long *)data;
        
        // Track TX packet errors
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXDROPS, data, 4);
        stats->tx_dropped = swab32(*(uint32 *)data);
        extsw_rreg(PAGE_MIB_P0 + (port), REG_MIB_P0_TXFRAMEINDISC, data, 4);
        stats->tx_dropped += swab32(*(uint32 *)data);
    } else
    {
       ethsw_get_hw_stats(port, stats);
    }
    return 0;
}
#endif

#if defined(CONFIG_BCM_EXT_SWITCH)
void bcmsw_set_ext_switch_pbvlan(int port, uint16_t portMap)
{
    uint16_t v16 = swab16(portMap);

    extsw_wreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (port * 2), (uint8_t *)&v16, sizeof(v16));
}
#endif /* CONFIG_BCM_EXT_SWITCH */
int bcmeapi_ioctl_extsw_port_jumbo_control(struct ethswctl_data *e)
{
    uint32 val32;

    if (e->type == TYPE_GET) {
        // Read & log current JUMBO configuration control register.
        extsw_rreg(PAGE_JUMBO, REG_JUMBO_PORT_MASK, (uint8 *)&val32, 4);
        BCM_ENET_DEBUG("JUMBO_PORT_MASK = 0x%08X", (unsigned int)val32);

        // Attempt to transfer register read value to user space & test for success.
      if (copy_to_user((void*)(&e->ret_val), (void*)&val32, sizeof(int)))
      {
          // Report failure.
          return -EFAULT;
      }
    } else {
        // Read & log current JUMBO configuration control register.
        extsw_rreg(PAGE_JUMBO, REG_JUMBO_PORT_MASK, (uint8 *)&val32, 4);
        BCM_ENET_DEBUG("Old JUMBO_PORT_MASK = 0x%08X", (unsigned int)val32);
    
        // Setup JUMBO configuration control register.
        val32 = ConfigureJumboPort(val32, e->port, e->val);
        extsw_wreg(PAGE_JUMBO, REG_JUMBO_PORT_MASK, (uint8 *)&val32, 4);

      // Attempt to transfer register write value to user space & test for success.
      if (copy_to_user((void*)(&e->ret_val), (void*)&val32, sizeof(int)))
      {
          // Report failure.
          return -EFAULT;
      }
    }
    return BCM_E_NONE;
}

static uint16_t dis_learning_ext = 0x0100; /* This default value does not matter */
static uint8_t  port_fwd_ctrl_ext = 0xC1;
static uint16_t pbvlan_map_ext[TOTAL_SWITCH_PORTS];


int bcmsw_enable_hw_switching(void)
{
    u8 i;
    uint16 v16;

    /* restore pbvlan config */
    for (i = 0; i < TOTAL_SWITCH_PORTS; i++)
    {
        v16 = swab16(pbvlan_map_ext[i]);
        extsw_wreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (i * 2),
                   (uint8 *)&v16, 2);
    }
    /* restore disable learning register */
    v16 = swab16(dis_learning_ext);
    extsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&v16, 2);
    /* restore port forward control register */
    extsw_wreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&port_fwd_ctrl_ext, 1);

    i = 0;
    while (vnet_dev[i])
    {
        if (LOGICAL_PORT_TO_UNIT_NUMBER(VPORT_TO_LOGICAL_PORT(i)) != 1) /* Not External switch port */
        {
            i++;  /* Go to next port */
            continue;
        }
        /* When hardware switching is enabled, enable the Linux bridge to
          not to forward the bcast packets on hardware ports */
        vnet_dev[i++]->priv_flags |= IFF_HW_SWITCH;
    }

    return 0;
}

int bcmsw_disable_hw_switching(void)
{
    u8 i, v8;
    u16 reg_value;

    /* Save disable_learning_reg setting */
    extsw_rreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&reg_value, 2);
    dis_learning_ext = swab16(reg_value);
    /* disable learning on all ports */
    reg_value = swab16(PBMAP_ALL);
    extsw_wreg(PAGE_CONTROL, REG_DISABLE_LEARNING, (uint8 *)&reg_value, 2);

    for (i = 0; i < TOTAL_SWITCH_PORTS; i++)
    {
        extsw_rreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (i * 2),
                   (uint8 *)&reg_value, 2);
        pbvlan_map_ext[i] = swab16(reg_value);

        if (i == MIPS_PORT_ID)
        {
            reg_value = swab16(PBMAP_ALL);
        }
        else
        {
            reg_value = swab16(PBMAP_MIPS);
        }
        extsw_wreg(PAGE_PORT_BASED_VLAN, REG_VLAN_CTRL_P0 + (i * 2),
                   (uint8 *)&reg_value, 2);
    }
    /* Save port forward control setting */
    extsw_rreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&port_fwd_ctrl_ext, 1);
    /* flood unlearned packets */
    v8 = 0x00;
    extsw_wreg(PAGE_CONTROL, REG_PORT_FORWARD, (uint8 *)&v8, 1);

    i = 0;
    while (vnet_dev[i])
    {
        if (LOGICAL_PORT_TO_UNIT_NUMBER(VPORT_TO_LOGICAL_PORT(i)) != 1) /* Not External switch port */
        {
            i++;  /* Go to next port */
            continue;
        }
        /* When hardware switching is disabled, enable the Linux bridge to
          forward the bcast on hardware ports as well */
        vnet_dev[i++]->priv_flags &= ~IFF_HW_SWITCH;
    }

    /* Flush arl table dynamic entries */
    fast_age_all_ext(0);
    return 0;
}
