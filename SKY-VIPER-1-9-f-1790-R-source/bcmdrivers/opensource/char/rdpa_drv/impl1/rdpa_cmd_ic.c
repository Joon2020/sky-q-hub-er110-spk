
/*
* <:copyright-BRCM:2013:DUAL/GPL:standard
* 
*    Copyright (c) 2013 Broadcom Corporation
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

/*
 *******************************************************************************
 * File Name  : rdpa_cmd_tm.c
 *
 * Description: This file contains the FAP Traffic Manager configuration API.
 *
 * The FAP Driver maintains two sets of TM configuration parameters: One for the
 * AUTO mode, and another for the MANUAL mode. The AUTO mode settings are
 * managed by the Ethernet driver based on the auto-negotiated PHY rates.
 * The MANUAL settings should be used by the user to configure the FAP TM.
 *
 * The mode can be set dynamically. Changing the mode does not apply the
 * corresponding settings into the FAP, it simply selects the current mode.
 * The settings corresponding to the current mode will take effect only when
 * explicitly applied to the FAP(s). This allows the caller to have a complete
 * configuration before activating it.
 *
 *******************************************************************************
 */

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/bcm_log.h>
#include "bcmenet.h"
#include "bcmtypes.h"
#include "bcmnet.h"
#include "rdpa_types.h"
#include "rdpa_api.h"
#include "rdpa_ag_port.h"
#include "rdpa_drv.h"
#include "rdpa_cmd_ic.h"

#define __BDMF_LOG__

#define CMD_IC_LOG_ID_RDPA_CMD_DRV BCM_LOG_ID_RDPA_CMD_DRV

#if defined(__BDMF_LOG__)
#define CMD_IC_LOG_ERROR(fmt, args...) 										\
    do {                                                            				\
        if (bdmf_global_trace_level >= bdmf_trace_level_error)      				\
            bdmf_trace("ERR: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);	\
    } while(0)
#define CMD_IC_LOG_INFO(fmt, args...) 										\
    do {                                                            				\
        if (bdmf_global_trace_level >= bdmf_trace_level_info)      					\
            bdmf_trace("INF: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);	\
    } while(0)
#define CMD_IC_LOG_DEBUG(fmt, args...) 										\
    do {                                                            				\
        if (bdmf_global_trace_level >= bdmf_trace_level_debug)      					\
            bdmf_trace("DBG: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);	\
    } while(0)
#else
#define CMD_IC_LOG_ERROR(fmt, arg...) BCM_LOG_ERROR(fmt, arg...)
#define CMD_IC_LOG_INFO(fmt, arg...) BCM_LOG_INFO(fmt, arg...)
#define CMD_IC_LOG_DEBUG(fmt, arg...) BCM_LOG_DEBUG(fmt, arg...)
#endif

static rdpa_traffic_dir ic_get_direction(rdpactl_classification_rule_t *rule)
{
    if (rule->ingress_port_id >= RDPACTL_IF_LAN0 && rule->ingress_port_id <= RDPACTL_IF_LAN3)
        return rdpa_dir_us;
    else
        return rdpa_dir_ds;
}

static int ic_get_fieldmask(rdpactl_classification_rule_t *rule, uint32_t *fieldmask, int *size)
{
    *fieldmask = 0;
	
    if (rule->field_mask & RDPACTL_IC_MASK_SRC_IP)
        *fieldmask |= RDPA_IC_MASK_SRC_IP;
	
    if (rule->field_mask & RDPACTL_IC_MASK_DST_IP)
        *fieldmask |= RDPA_IC_MASK_DST_IP;
	
    if (rule->field_mask & RDPACTL_IC_MASK_SRC_PORT)
        *fieldmask |= RDPA_IC_MASK_SRC_PORT;
	
    if (rule->field_mask & RDPACTL_IC_MASK_DST_PORT)
        *fieldmask |= RDPA_IC_MASK_DST_PORT;

    if (rule->field_mask & RDPACTL_IC_MASK_OUTER_VID)
        *fieldmask |= RDPA_IC_MASK_OUTER_VID;

    if (rule->field_mask & RDPACTL_IC_MASK_INNER_VID)
        *fieldmask |= RDPA_IC_MASK_INNER_VID;
	
    if (rule->field_mask & RDPACTL_IC_MASK_DST_MAC)
        *fieldmask |= RDPA_IC_MASK_DST_MAC;

    if (rule->field_mask & RDPACTL_IC_MASK_SRC_MAC)
        *fieldmask |= RDPA_IC_MASK_SRC_MAC;

    if (rule->field_mask & RDPACTL_IC_MASK_ETHER_TYPE)
        *fieldmask |= RDPA_IC_MASK_ETHER_TYPE;

    if (rule->field_mask & RDPACTL_IC_MASK_IP_PROTOCOL)
        *fieldmask |= RDPA_IC_MASK_IP_PROTOCOL;

    if (rule->field_mask & RDPACTL_IC_MASK_DSCP)
        *fieldmask |= RDPA_IC_MASK_DSCP;

    if (rule->field_mask & RDPACTL_IC_MASK_SSID)
        *fieldmask |= RDPA_IC_MASK_SSID;

    if (rule->field_mask & RDPACTL_IC_MASK_OUTER_PBIT)
        *fieldmask |= RDPA_IC_MASK_OUTER_PBIT;

    if (rule->field_mask & RDPACTL_IC_MASK_INNER_PBIT)
        *fieldmask |= RDPA_IC_MASK_INNER_PBIT;

    if (rule->ingress_port_id >= RDPACTL_IF_LAN0 && rule->ingress_port_id < RDPACTL_IF_MAX)
        *fieldmask |= RDPA_IC_MASK_INGRESS_PORT;

    if (rule->field_mask & RDPACTL_IC_MASK_NUM_OF_VLANS)
        *fieldmask |= RDPA_IC_MASK_NUM_OF_VLANS;

    if (size) *size = 0;  //Sarah: TBD calculate mask size
    return 0;
}

static bdmf_index ic_get_ingress_port(rdpactl_classification_rule_t *rule)
{
    if (rule->ingress_port_id == RDPACTL_IF_LAN0)
       return rdpa_if_lan0;
	
    if (rule->ingress_port_id == RDPACTL_IF_LAN1)
       return rdpa_if_lan1;
	
    if (rule->ingress_port_id == RDPACTL_IF_LAN2)
       return rdpa_if_lan2;
	
    if (rule->ingress_port_id == RDPACTL_IF_LAN3)
       return rdpa_if_lan3;

    return rdpa_if_wan0;
}
	
static int delete_ic(bdmf_object_handle ingress_class_obj)
{
    bdmf_error_t rc;
	
    CMD_IC_LOG_INFO("delete_ic");
	
    if ((rc = bdmf_destroy(ingress_class_obj))) {
       CMD_IC_LOG_ERROR("bdmf_destroy() failed:rc(%d)", rc);
       return RDPA_DRV_SH_DESTROY;
    }
		
    return 0;
}

static int find_ic(rdpactl_classification_rule_t *rule, bdmf_object_handle *ingress_class_obj, bdmf_number *ic_idx)
{
    bdmf_object_handle obj = NULL;
    rdpa_ic_cfg_t  cfg;
    rdpa_traffic_dir dir, rule_dir;
    uint32_t field_mask;
	
    *ingress_class_obj = NULL;
    rule_dir = ic_get_direction(rule);
    ic_get_fieldmask(rule, &field_mask, NULL);
	
    while ((obj = bdmf_get_next(rdpa_ingress_class_drv(), obj, NULL)))
    {
        rdpa_ingress_class_dir_get(obj, &dir);
        if (dir != rule_dir)  continue;

        rdpa_ingress_class_cfg_get(obj, &cfg);
	 if (cfg.type != (rdpa_ic_type)rule->type || cfg.field_mask != field_mask) continue;

	 if (cfg.prty != rule->prty)
           CMD_IC_LOG_ERROR("rule priority %d ignored, using %d instead", rule->prty, cfg.prty);
	 
        rdpa_ingress_class_index_get(obj, ic_idx);
        *ingress_class_obj = obj;
         return 1;
    }
	
    return 0;
}

static int add_ic(rdpactl_classification_rule_t *rule, bdmf_object_handle *ingress_class_obj)
{
    rdpa_ic_cfg_t cfg;
    int rc;
    rdpa_traffic_dir dir = ic_get_direction(rule);
    BDMF_MATTR(ingress_class_attrs, rdpa_ingress_class_drv());

    CMD_IC_LOG_INFO("dir:%d, field_mask:0x%x", dir, rule->field_mask);

    cfg.type = (rdpa_ic_type)rule->type;
    ic_get_fieldmask(rule, &cfg.field_mask, NULL);
    cfg.prty = rule->prty;
    cfg.port_mask = 0;
    cfg.acl_mode = RDPA_ACL_MODE_BLACK;
	
    rc = rdpa_ingress_class_dir_set(ingress_class_attrs, dir);
    rc = rc ? : rdpa_ingress_class_cfg_set(ingress_class_attrs, &cfg);
    rc = rc ? : bdmf_new_and_set(rdpa_ingress_class_drv(), NULL, ingress_class_attrs, ingress_class_obj);
    if (rc < 0)
    {
       CMD_IC_LOG_ERROR("Failed to create ingress_class object, rc=%d", rc);
       return -1;
    }
			
    return 0;
}

static int create_ic_flow(rdpactl_classification_rule_t *rule, rdpa_ic_info_t *flow)
{
    memset(flow, 0, sizeof(rdpa_ic_info_t));
		
    if ((rule->field_mask & RDPACTL_IC_MASK_SRC_IP) && rule->ip_family == RDPACTL_IP_FAMILY_IPV4)
    	{
        flow->key.src_ip.family = bdmf_ip_family_ipv4;
        flow->key.src_ip.addr.ipv4 = rule->src_ip.ipv4;
   	}

    if ((rule->field_mask & RDPACTL_IC_MASK_SRC_IP) && rule->ip_family == RDPACTL_IP_FAMILY_IPV6)
    	{
        flow->key.src_ip.family = bdmf_ip_family_ipv6;
        memcpy(flow->key.src_ip.addr.ipv6.data, rule->src_ip.ipv6, sizeof(rule->src_ip.ipv6));
   	}

    if ((rule->field_mask & RDPACTL_IC_MASK_DST_IP) && rule->ip_family == RDPACTL_IP_FAMILY_IPV4)
    	{
        flow->key.dst_ip.family = bdmf_ip_family_ipv4;
        flow->key.dst_ip.addr.ipv4 = rule->dst_ip.ipv4;
   	}

    if ((rule->field_mask & RDPACTL_IC_MASK_DST_IP) && rule->ip_family == RDPACTL_IP_FAMILY_IPV6)
    	{
        flow->key.dst_ip.family = bdmf_ip_family_ipv6;
        memcpy(flow->key.dst_ip.addr.ipv6.data, rule->dst_ip.ipv6, sizeof(rule->dst_ip.ipv6));
   	}
		
    if (rule->field_mask & RDPACTL_IC_MASK_SRC_PORT)
        flow->key.src_port = rule->src_port;
	
    if (rule->field_mask & RDPACTL_IC_MASK_DST_PORT)
        flow->key.dst_port = rule->dst_port;

    if (rule->field_mask & RDPACTL_IC_MASK_OUTER_VID)
        flow->key.outer_vid =  rule->outer_vid;

    if (rule->field_mask & RDPACTL_IC_MASK_INNER_VID)
        flow->key.inner_vid = rule->inner_vid;
	
    if (rule->field_mask & RDPACTL_IC_MASK_DST_MAC)
        memcpy(&flow->key.dst_mac, rule->dst_mac, sizeof(flow->key.dst_mac));

    if (rule->field_mask & RDPACTL_IC_MASK_SRC_MAC)
        memcpy(&flow->key.src_mac, rule->src_mac, sizeof(flow->key.src_mac));

    if (rule->field_mask & RDPACTL_IC_MASK_ETHER_TYPE)
        flow->key.etype = rule->etype;

    if (rule->field_mask & RDPACTL_IC_MASK_IP_PROTOCOL)
        flow->key.protocol = rule->protocol;

    if (rule->field_mask & RDPACTL_IC_MASK_DSCP)
        flow->key.dscp = rule->dscp;

    if (rule->field_mask & RDPACTL_IC_MASK_OUTER_PBIT)
        flow->key.outer_pbits = rule->outer_pbits;

    if (rule->field_mask & RDPACTL_IC_MASK_INNER_PBIT)
        flow->key.inner_pbits = rule->inner_pbits;

    if (rule->field_mask & RDPACTL_IC_MASK_NUM_OF_VLANS)
        flow->key.number_of_vlans = rule->number_of_vlans;
    
    flow->key.ingress_port = ic_get_ingress_port(rule);
	
    flow->result.qos_method = rdpa_qos_method_flow;
    flow->result.queue_id = rule->queue_id & (~RDPACTL_WANFLOW_MASK); 

    flow->result.opbit_remark = (rule->opbit_remark != -1); 
    flow->result.opbit_val =(rule->opbit_remark == -1) ? 0 : rule->opbit_remark; 

    flow->result.ipbit_remark = (rule->ipbit_remark != -1); 
    flow->result.ipbit_val =(rule->ipbit_remark == -1) ? 0 : rule->ipbit_remark; 

    flow->result.dscp_remark = (rule->dscp_remark != -1); 
    flow->result.dscp_val =(rule->dscp_remark == -1) ? 0 : rule->dscp_remark; 
	
    flow->result.policer = NULL;  //Sarah: TBD
	
    //userspace qos rule should not aware of wanflow, but in epon MLLID for runner, it would
    flow->result.wan_flow = (rule->queue_id & RDPACTL_WANFLOW_MASK) >> RDPACTL_QUEUEID_BITS_NUMBER; 

    flow->result.action = rdpa_forward_action_forward;

    return 0;
}

/*******************************************************************************/
/* global routines                                                             */
/*******************************************************************************/

/*******************************************************************************
 *
 * Function: rdpa_cmd_ic_ioctl
 *
 * IOCTL interface to the RDPA INGRESS CLASSIFIER API.
 *
 *******************************************************************************/
int rdpa_cmd_ic_ioctl(unsigned long arg)
{
	rdpa_drv_ioctl_ic_t *userIc_p = (rdpa_drv_ioctl_ic_t *)arg;
	rdpa_drv_ioctl_ic_t ic;	
	int ret = 0;
	bdmf_error_t rc = BDMF_ERR_OK;

    copy_from_user(&ic, userIc_p, sizeof(rdpa_drv_ioctl_ic_t));

    CMD_IC_LOG_DEBUG("RDPA IC CMD(%d)", ic.cmd);

    bdmf_lock();

    switch(ic.cmd)
    {
    	case RDPA_IOCTL_IC_CMD_ADD_CLASSIFICATION_RULE: {
          bdmf_object_handle ingress_class_obj;
          int rc, icAdd = 0;
          bdmf_number ic_idx = 0;
          bdmf_index ic_flow_idx;
          rdpa_ic_info_t  ic_flow;
      
          CMD_IC_LOG_DEBUG("RDPA_IOCTL_IC_CMD_ADD_CLASSIFICATION_RULE: field(0x%x) port_id(%d)", ic.rule.field_mask, ic.rule.ingress_port_id);
   
          if (!find_ic(&ic.rule, &ingress_class_obj, &ic_idx)) 
          {
             rc = add_ic(&ic.rule, &ingress_class_obj);
             if (rc)
             {
                CMD_IC_LOG_ERROR("add  ic rule fail, rc=%d", rc);
                ret = RDPA_DRV_IC_ERROR;
                goto ioctl_exit;
             }
             icAdd = 1;
          }
          else
             CMD_IC_LOG_DEBUG("find exist ic:%d", (int)ic_idx);
  
          create_ic_flow(&ic.rule, &ic_flow);
          rc = rdpa_ingress_class_flow_add(ingress_class_obj, &ic_flow_idx, &ic_flow);
          if (rc)
          {
              CMD_IC_LOG_ERROR("add  ic flow error, rc=%d", rc);
              
              if (icAdd) 
                 delete_ic(ingress_class_obj);
              else
                 bdmf_put(ingress_class_obj);
			  
             ret = (rc == BDMF_ERR_ALREADY ? 0 : RDPA_DRV_IC_FLOW_ERROR);
             goto ioctl_exit;			  	
         }
          else
              CMD_IC_LOG_INFO("Created ic flow: ic_idx %d, flow_idx %d", (int)ic_idx, (int)ic_flow_idx);

          if (!icAdd) bdmf_put(ingress_class_obj);
          break;
    	}
    	case RDPA_IOCTL_IC_CMD_DEL_CLASSIFICATION_RULE: {
          bdmf_object_handle ingress_class_obj = NULL;
          bdmf_index ic_flow_idx;
          rdpa_ic_info_t  ic_flow;
          bdmf_number ic_idx;
          int rc;
          bdmf_number nflows;
          
          CMD_IC_LOG_DEBUG("RDPA_IOCTL_IC_CMD_DEL_CLASSIFICATION_RULE: field(0x%x) port_id(%d)", ic.rule.field_mask, ic.rule.ingress_port_id);
          
          if (!find_ic(&ic.rule, &ingress_class_obj, &ic_idx))
          {
             CMD_IC_LOG_ERROR("Not found qos ic");
             ret = RDPA_DRV_IC_NOT_FOUND;
             goto ioctl_exit;
          }
                    
          create_ic_flow(&ic.rule, &ic_flow);
          rc = rdpa_ingress_class_flow_find(ingress_class_obj, &ic_flow_idx, &ic_flow);
          if (rc)
          {
             CMD_IC_LOG_ERROR("Cannot find ic flow, rc=%d\n", rc);
             bdmf_put(ingress_class_obj);
             goto ioctl_exit;
          }
          
         CMD_IC_LOG_INFO("Delete flow: ic_idx %d, flow_idx %d",(int)ic_idx, (int)ic_flow_idx);          
          rc = rdpa_ingress_class_flow_delete(ingress_class_obj, ic_flow_idx);
          if (rc)
          {
              CMD_IC_LOG_ERROR("Cannot delete ingress_class flow: ic_idx %d, flow_idx %d", (int)ic_idx, (int)ic_flow_idx); 
              bdmf_put(ingress_class_obj);
              ret = RDPA_DRV_IC_FLOW_ERROR;
              goto ioctl_exit;
          }

          rdpa_ingress_class_nflow_get(ingress_class_obj, &nflows);
          if (nflows == 0) 
           {
              bdmf_put(ingress_class_obj);
              //Sarah: TBD: if qos ic is not created here			  
              delete_ic(ingress_class_obj);
           }
          else 
              bdmf_put(ingress_class_obj);
		  
          break;
    	}
        case RDPA_IOCTL_IC_CMD_ADD: {
            bdmf_object_handle ingress_class_obj;
            int rc, icAdd = 0;
            bdmf_number ic_idx = 0;

            CMD_IC_LOG_DEBUG("RDPA_IOCTL_IC_CMD_ADD: type(0x%x)field(0x%x) port_id(%d)", ic.rule.type,ic.rule.field_mask, ic.rule.ingress_port_id);

            if (!find_ic(&ic.rule, &ingress_class_obj, &ic_idx)) 
            {
                rc = add_ic(&ic.rule, &ingress_class_obj);
                if (rc)
                {
                    CMD_IC_LOG_ERROR("add  ic  fail, rc=%d", rc);
                    ret = RDPA_DRV_IC_ERROR;
                    goto ioctl_exit;
                }
                icAdd = 1;
            }
            else

            if (!icAdd) 
                bdmf_put(ingress_class_obj);
            break;
        }
        case RDPA_IOCTL_IC_CMD_DEL: {
            bdmf_object_handle ingress_class_obj = NULL;
            bdmf_number ic_idx;
            int rc;
            bdmf_number nflows;

            CMD_IC_LOG_DEBUG("RDPA_IOCTL_IC_CMD_DEL: type(0x%x)field(0x%x) port_id(%d)",ic.rule.type, ic.rule.field_mask, ic.rule.ingress_port_id);

            if (!find_ic(&ic.rule, &ingress_class_obj, &ic_idx))
            {
                CMD_IC_LOG_ERROR("Not found  ic");
                ret = RDPA_DRV_IC_NOT_FOUND;
                goto ioctl_exit;
            }

            rdpa_ingress_class_nflow_get(ingress_class_obj, &nflows);
            if (nflows == 0) 
            {
                bdmf_put(ingress_class_obj);
                rc = delete_ic(ingress_class_obj);
            }
            else 
                bdmf_put(ingress_class_obj);

            break;
        }

        default:
            CMD_IC_LOG_ERROR("Invalid IOCTL cmd %d", ic.cmd);
            ret = RDPA_DRV_ERROR;
    }

ioctl_exit:
	if (ret) {
		CMD_IC_LOG_ERROR("rdpa_cmd_ic_ioctl() OUT: FAILED: field(0x%x) rc(%d)", ic.rule.field_mask, rc);
	}

	bdmf_unlock();
       return ret;
}

/*******************************************************************************
 *
 * Function: rdpa_cmd_ic_init
 *
 * Initializes the RDPA IC API.
 *
 *******************************************************************************/
void rdpa_cmd_ic_init(void)
{
    CMD_IC_LOG_DEBUG("RDPA IC INIT");
}

EXPORT_SYMBOL(rdpa_cmd_ic_ioctl);
EXPORT_SYMBOL(rdpa_cmd_ic_init);
