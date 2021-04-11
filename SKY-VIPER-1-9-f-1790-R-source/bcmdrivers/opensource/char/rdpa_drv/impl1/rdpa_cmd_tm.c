
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
#include "rdpa_cmd_tm.h"

#define __BDMF_LOG__
//#define __DUMP_PORTS__

#define CMD_TM_LOG_ID_RDPA_CMD_DRV BCM_LOG_ID_RDPA_CMD_DRV

#if defined(__BDMF_LOG__)
#define CMD_TM_LOG_ERROR(fmt, args...) 										\
    do {                                                            				\
        if (bdmf_global_trace_level >= bdmf_trace_level_error)      				\
            bdmf_trace("ERR: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);	\
    } while(0)
#define CMD_TM_LOG_INFO(fmt, args...) 										\
    do {                                                            				\
        if (bdmf_global_trace_level >= bdmf_trace_level_info)      					\
            bdmf_trace("INF: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);	\
    } while(0)
#define CMD_TM_LOG_DEBUG(fmt, args...) 										\
    do {                                                            				\
        if (bdmf_global_trace_level >= bdmf_trace_level_debug)      					\
            bdmf_trace("DBG: %s#%d: " fmt "\n", __FUNCTION__, __LINE__, ## args);	\
    } while(0)
#else
#define CMD_TM_LOG_ERROR(fmt, arg...) BCM_LOG_ERROR(fmt, arg...)
#define CMD_TM_LOG_INFO(fmt, arg...) BCM_LOG_INFO(fmt, arg...)
#define CMD_TM_LOG_DEBUG(fmt, arg...) BCM_LOG_DEBUG(fmt, arg...)
#endif

#define MAX_TM_PER_PORT  32
#define MAX_RUT_QOS_PORTS 16 /* rdpa_if_lan_max is the maximum */
#define MAX_Q_PER_TM 8

typedef struct {
    uint32 tm_index;
} rdpa_cmd_tm_ctrl_t;

typedef struct {
	int		 	alloc;
	uint32_t 	tm_id;
	bdmf_number	qid;
} queue_st;

typedef struct {
	int		 alloc;
	int 	 root_tm_id;
	rdpa_if  port_id;
	rdpa_tm_sched_mode arbiter_mode;
	queue_st queue_list[MAX_Q_PER_TM];
	int		 queue_alloc_cnt;
} port_st;

static rdpa_cmd_tm_ctrl_t rdpa_cmd_tm_ctrl_g;
static port_st port_list[MAX_RUT_QOS_PORTS];
static bdmf_object_handle orig_sched = NULL; /* Original scheduling object - before configuration */

/*******************************************************************************/
/* static routines Functions                                                   */
/*******************************************************************************/

static int check_rdpa_mode(rdpa_wan_type mode)
{
    rdpa_system_init_cfg_t sys_init_cfg;
	bdmf_object_handle system_obj = NULL;

	rdpa_system_init_cfg_get(system_obj, &sys_init_cfg);

    return (sys_init_cfg.wan_type == (mode ? TRUE : FALSE));
}

static int add_root_tm_to_port(
				uint32_t port_id,
				uint32_t tm_id,
				uint32_t arbiter_mode)
{
	port_st	*pport = NULL;

	CMD_TM_LOG_DEBUG("IN: ADDED ROOT TM(%u) port_id(%u)", tm_id,  port_id);

	if (port_id >= MAX_RUT_QOS_PORTS) {
		CMD_TM_LOG_ERROR("FAIL: port_id(%u) is not valid", port_id);
		return RDPA_DRV_PORT_ID_NOT_VALID;
	}

	pport = &port_list[port_id];

	if (pport)
	{

	pport->alloc 		= TRUE;
	pport->root_tm_id 	= tm_id;
	pport->port_id 		= port_id;
	pport->arbiter_mode = arbiter_mode;
	}

	return 0;
}

static int remove_root_tm_from_port(
			uint32_t port_id)
{
	port_st	*pport = NULL;

	CMD_TM_LOG_DEBUG("IN: port_id(%u)",  port_id);

	if (port_id >= MAX_RUT_QOS_PORTS) {
		CMD_TM_LOG_ERROR("FAIL: port_id(%u) is not valid",  port_id);
		return RDPA_DRV_PORT_ID_NOT_VALID;
	}

	pport = &port_list[port_id];

	if (!pport->alloc) {
		CMD_TM_LOG_ERROR("FAIL: port_id(%u) is NOT ALLOCATED",  port_id);
		return RDPA_DRV_PORT_NOT_ALLOC;
	}

	pport->alloc 		= FALSE;
	pport->root_tm_id 	= 0;
	pport->port_id 		= 0;
	pport->arbiter_mode = 0;

	return 0;
}

static BOOL is_root_tm_allocated(
				uint32_t port_id)
{
	CMD_TM_LOG_DEBUG("IN: port_id(%u)",  port_id);

	if (port_id >= MAX_RUT_QOS_PORTS) {
		CMD_TM_LOG_ERROR("FAIL: port_id(%u) is out of range",  port_id);
		return FALSE;
	}

	return port_list[port_id].alloc;
}

/******
static int get_root_by_port_id(
				uint32_t port_id,
				uint32_t *proot_tm_id)
{
	port_st	*pport = NULL;

	CMD_TM_LOG_DEBUG("IN: port_id(%u)",  port_id);

	if (port_id >= MAX_RUT_QOS_PORTS) {
		CMD_TM_LOG_ERROR("FAIL: port_id(%u) is out of range",  port_id);
		return RDPA_DRV_PORT_ID_NOT_VALID;
	}

	pport = &port_list[port_id];

	if (!pport->alloc) {
//		CMD_TM_LOG_ERROR("FAIL: Port_id(%u) NOT ALLOCATED",  port_id);
		return RDPA_DRV_PORT_NOT_ALLOC;
	}

	*proot_tm_id = pport->root_tm_id;

	return 0;
}
***********/
static int add_q_to_port(
				uint32_t port_id,
				uint32_t tm_id,
				uint32_t q_id)
{
	port_st	*pport = NULL;
	queue_st *pq = NULL;

	CMD_TM_LOG_DEBUG("IN: TM(%u) port_id(%u) q_id(%u)",  tm_id,  port_id, q_id);

	if ((port_id >= MAX_RUT_QOS_PORTS) || (q_id >= MAX_Q_PER_TM))  {
		CMD_TM_LOG_ERROR("FAIL: TM(%u) port_id(%u) OR q_id(%u) are not valid",  tm_id,  port_id, q_id);
		return RDPA_DRV_PORT_ID_NOT_VALID;
	}

	pport = &port_list[port_id];
	pq = &port_list[port_id].queue_list[q_id];

	if (!pport->alloc || pq->alloc)  {
		CMD_TM_LOG_ERROR("FAIL: TM(%u) port_id(%u) ALC:[%s] is NOT ALLOCATED OR q_id(%u) ALC:[%s] is ALLOCATED",
			tm_id,  port_id, (pport->alloc?"TRUE":"FALSE"), q_id, (pq->alloc?"TRUE":"FALSE"));
		return RDPA_DRV_PORT_NOT_ALLOC;
	}

	pq->alloc 	= TRUE;
	pq->qid 	= q_id;
	pq->tm_id 	= tm_id;

	pport->queue_alloc_cnt++;

	return 0;
}

static int remove_q_from_port(
				uint32_t port_id,
				uint32_t tm_id,
				uint32_t q_id)
{
	port_st	*pport = NULL;
	queue_st *pq = NULL;

	CMD_TM_LOG_DEBUG("IN: TM(%u) port_id(%u) q_id(%u)",  tm_id,  port_id, q_id);

	if ((port_id >= MAX_RUT_QOS_PORTS) || (q_id >= MAX_Q_PER_TM))  {
		CMD_TM_LOG_ERROR("FAIL: TM(%u) port_id(%u) OR q_id(%u) are not valid",  tm_id,  port_id, q_id);
		return RDPA_DRV_PORT_ID_NOT_VALID;
	}

	pport = &port_list[port_id];
	pq = &port_list[port_id].queue_list[q_id];

	if (!pport->alloc || !pq->alloc)  {
		CMD_TM_LOG_ERROR("FAIL: TM(%u) port_id(%u) ALC:[%s] is NOT ALLOCATED OR q_id(%u) ALC:[%s] is ALLOCATED",
			tm_id,  port_id, (pport->alloc?"TRUE":"FALSE"), q_id, (pq->alloc?"TRUE":"FALSE"));
		return RDPA_DRV_PORT_NOT_ALLOC;
	}

	pq->alloc = FALSE;
	pq->qid   = q_id;
	pq->tm_id = tm_id;

	pport->queue_alloc_cnt--;

	return 0;
}

static int get_tm_by_q_qid(
		uint32_t port_id,
		uint32_t qid,
		uint32_t *ptm_id_ptr,
		BOOL     *pfound)
{
	port_st	*pport = NULL;
	int i;

	CMD_TM_LOG_DEBUG("IN: port_id(%u) qid(%u)", port_id, qid);

	*pfound = FALSE;

	if (port_id >= MAX_RUT_QOS_PORTS) {
		CMD_TM_LOG_ERROR("FAILED_1 port_id(%u) qid(%u)", port_id, qid);
		return RDPA_DRV_PORT_ID_NOT_VALID;
	}

	pport = &port_list[port_id];

	if (!pport->alloc) {
//		CMD_TM_LOG_ERROR("FAIL: PORT(%d) NOT ALLOCATED qid(%u)", port_id, qid);
		return RDPA_DRV_PORT_NOT_ALLOC;
	}

	for (i = 0; i < MAX_Q_PER_TM; i++) {
		if (pport->queue_list[i].alloc && (pport->queue_list[i].alloc) && (pport->queue_list[i].qid == qid)) {
			*ptm_id_ptr = pport->queue_list[i].tm_id;
			*pfound = TRUE;
			return 0;
		}
	}

	return 0;
}

#ifdef __DUMP_PORTS__
static int dump_ports(void)
{
	port_st	*pport = NULL;
	int i;

	for (i = 0; i < MAX_RUT_QOS_PORTS; i++) {
		pport = &port_list[i];
		CMD_TM_LOG_DEBUG("PORT(%u) STAT: alloc(%u) queue_cnt(%u) root_id(%u)",
			i, pport->alloc, pport->queue_alloc_cnt, pport->root_tm_id);
	}

	return 0;
}
#endif /* __DUMP_PORTS__ */

static int tm_set(
		bdmf_mattr_handle sched_attrs,
		rdpa_drv_ioctl_tm_t *pTm,
		uint32_t *ptm_id,
		bdmf_object_handle root_sched,
		bdmf_object_handle *psched)
{
	bdmf_error_t rc = BDMF_ERR_OK;
	bdmf_number new_tm_id;
	bdmf_object_handle tmp_psched = NULL;


	rdpa_egress_tm_dir_set(sched_attrs, pTm->dir);
	rdpa_egress_tm_level_set(sched_attrs, pTm->level); /* rdpa_tm_level_queue / rdpa_tm_level_egress_tm */
	rdpa_egress_tm_mode_set(sched_attrs, pTm->arbiter_mode);

	if ((rc = bdmf_new_and_set(rdpa_egress_tm_drv(), root_sched, sched_attrs, &tmp_psched))) {
		CMD_TM_LOG_ERROR("bdmf_new_and_set() failed: ROOT_tm(%u) rc(%d)", pTm->root_tm_id, rc);
		return RDPA_DRV_NEW_TM_ALLOC;
	}

	*psched = tmp_psched;

	if ((rc = rdpa_egress_tm_index_get(*psched, &new_tm_id))) {
		CMD_TM_LOG_ERROR("rdpa_egress_tm_index_get() failed: ROOT_tm(%u) rc(%d)", pTm->root_tm_id, rc);
		return RDPA_DRV_TM_INDEX_GET;
	}

	*ptm_id = (uint32_t) new_tm_id;

	return 0;
}

static int tm_replace(
		bdmf_object_handle port_obj,
		bdmf_object_handle sched)
{
	rdpa_port_tm_cfg_t orig_tm_cfg = {};
	bdmf_error_t rc = BDMF_ERR_OK;

	CMD_TM_LOG_DEBUG("IN");

	/* Removing an old TM from Port Object (and storing it in a cool & dark place) */
	if ((rc = rdpa_port_tm_cfg_get(port_obj, &orig_tm_cfg))) {
		CMD_TM_LOG_ERROR("rdpa_port_tm_cfg_get() failed: rc(%d)", rc);
		return RDPA_DRV_TM_CFG_GET;
	}

	orig_sched = orig_tm_cfg.sched;

//#ifdef __NO_ORIG_TM_DESTROY__
	if (orig_tm_cfg.sched) {
		if ((rc = bdmf_destroy(orig_tm_cfg.sched))) {
			CMD_TM_LOG_ERROR("bdmf_destroy() failed: rc(%d)", rc);
//			return RDPA_DRV_SH_DESTROY;
		}
	} else {
		CMD_TM_LOG_DEBUG("REPLACE-DESTROY avoided - sched == NULL");
	}
//#endif /* __NO_ORIG_TM_DESTROY__ */

	orig_tm_cfg.sched = sched;
	if ((rc = rdpa_port_tm_cfg_set(port_obj, &orig_tm_cfg))) {
		CMD_TM_LOG_ERROR("rdpa_port_tm_cfg_set() failed: rc(%d)", rc);
		return RDPA_DRV_TM_CFG_SET;
	}

	return 0;
}

static int root_tm_remove(
			uint32_t port_id,
			uint32_t root_tm_id,
			uint32_t dir)
{
	bdmf_error_t rc = BDMF_ERR_OK;
	bdmf_object_handle sched = NULL;
	rdpa_egress_tm_key_t tm_key = {};
	int ret = 0;

	CMD_TM_LOG_DEBUG("IN: port_id(%u) root_tm_id(%u) dir(%u)", port_id, root_tm_id, dir);

	tm_key.dir   = dir;
	tm_key.index = root_tm_id;

	if ((rc = rdpa_egress_tm_get(&tm_key, &sched))) {
		CMD_TM_LOG_ERROR("rdpa_egress_tm_get() failed: tm(%u) rc(%d)", root_tm_id, rc);
		return RDPA_DRV_TM_GET;
	}

	if (sched) {
		bdmf_put(sched);
		if ((rc = bdmf_destroy(sched))) {
			CMD_TM_LOG_ERROR("bdmf_destroy() failed: tm(%u) rc(%d)", root_tm_id, rc);
			return RDPA_DRV_SH_DESTROY;
		}
	}

	if (is_root_tm_allocated(port_id)) {
		if ((ret = remove_root_tm_from_port(port_id))) {
			CMD_TM_LOG_ERROR("remove_root_tm_from_port() failed: port_id(%u) rc(%d)", port_id, ret);
			return ret;
		}
	}

	return 0;
}

static int root_tm_config(
			rdpa_drv_ioctl_tm_t *ptm,
			bdmf_object_handle *psched)
{
	BDMF_MATTR(sched_attrs, rdpa_egress_tm_drv());
	bdmf_object_handle port_obj = NULL;
	bdmf_object_handle tmp_psched = NULL;
	bdmf_error_t rc = BDMF_ERR_OK;
	int ret = 0;

	CMD_TM_LOG_DEBUG("IN: port(%u)", ptm->port_id);

	/* ------------------- PORT Configuration: Acquire the port object ------------------- */
	if ((rc = rdpa_port_get(ptm->port_id, &port_obj))) {
		CMD_TM_LOG_ERROR("rdpa_port_get() failed: port(%d) rc(%d)", ptm->port_id, rc);
		return RDPA_DRV_PORT_GET;
	}

	if (!check_rdpa_mode(rdpa_wan_gbe)) { /* Perform the TCONT binding only in GPON or EPON mode */
		CMD_TM_LOG_DEBUG("TM to PORT bind removing");

		if ((ret = tm_replace(port_obj, NULL))) {
			CMD_TM_LOG_ERROR("tm_replace() failed: port(%d) rc(%d)", ptm->port_id, rc);
			bdmf_put(port_obj);
			return ret;
		}
	}

	if ((ret = tm_set(sched_attrs, ptm, &ptm->root_tm_id, port_obj, &tmp_psched))) {
		CMD_TM_LOG_ERROR("rdpa_egress_tm_index_set() failed: rc(%d)", rc);
		bdmf_put(port_obj);
		return ret;
	}

	CMD_TM_LOG_DEBUG("ADDING ROOT TM: ROOT_tm(%u) port_id(%d)", ptm->root_tm_id, ptm->port_id);

	bdmf_put(port_obj);

	*psched = tmp_psched;

	return 0;
}

static int q_config(
			rdpa_drv_ioctl_tm_t *ptm)
{
	bdmf_object_handle sched = NULL;
	rdpa_egress_tm_key_t tm_key = {};
	rdpa_tm_queue_cfg_t queue_cfg = {};
	rdpa_tm_rl_cfg_t rl_cfg = {};
	rdpa_tm_sched_mode arbiter_mode = rdpa_tm_sched_disabled;
	bdmf_error_t rc = BDMF_ERR_OK;

	CMD_TM_LOG_DEBUG("Q CONFIG: tm(%u) dir(%u) q_id(%u)", ptm->tm_id, ptm->dir, ptm->q_id);

	tm_key.dir 	 = ptm->dir;
	tm_key.index = ptm->tm_id;

	if ((rc = rdpa_egress_tm_get(&tm_key, &sched))) {
		CMD_TM_LOG_ERROR("rdpa_egress_tm_get() failed: tm(%u) rc(%d)", ptm->tm_id, rc);
		return RDPA_DRV_TM_GET;
	}

	arbiter_mode = port_list[ptm->port_id].arbiter_mode;

	//if (arbiter_mode == rdpa_tm_sched_sp)
	//{
	rl_cfg.af_rate = ptm->shaping_rate * 1000; 	/* Best Effort: shaping_rate is in kbit/s: 1 kilobit = 1000 bits */

	CMD_TM_LOG_DEBUG("rdpa_egress_tm_rl_set(): tm(%u) mode(%u) af_rate(%u)",
		ptm->tm_id, arbiter_mode, rl_cfg.af_rate);

	if ((rc = rdpa_egress_tm_rl_set(sched, &rl_cfg))) {
		CMD_TM_LOG_ERROR("rdpa_egress_tm_rl_set() failed: tm(%u) mode(%u) af(%u) rc(%d)",
			ptm->tm_id, arbiter_mode, rl_cfg.af_rate, rc);
		bdmf_put(sched);
		return RDPA_DRV_Q_RATE_SET;
		}
	//}
	if (arbiter_mode == rdpa_tm_sched_wrr)
	{
		CMD_TM_LOG_DEBUG("**: rdpa_egress_tm_weight_set(): tm(%u) mode(%u) weight(%u)", 	ptm->tm_id, arbiter_mode, ptm->weight);
		rc = rdpa_egress_tm_weight_set(sched, ptm->weight);
		if (rc != 0)
		{
			CMD_TM_LOG_ERROR("rdpa_egress_tm_weight_set() failed: tm(%u) mode(%u) weight(%u) rc(%d)",
				ptm->tm_id, arbiter_mode, ptm->weight, rc);
			bdmf_put(sched);
			return RDPA_DRV_Q_RATE_SET;
		}
	}

	queue_cfg.queue_id           = ptm->q_id;
	queue_cfg.drop_alg           = rdpa_tm_drop_alg_dt;
	queue_cfg.drop_threshold     = ptm->qsize;
	queue_cfg.weight             = ptm->weight;
	queue_cfg.red_high_threshold = 0; /**< Relevant for ::rdpa_tm_drop_alg_red/::rdpa_tm_drop_alg_wred : packet threshold */
	queue_cfg.red_low_threshold  = 0;

	CMD_TM_LOG_DEBUG("rdpa_egress_tm_queue_cfg_set(): tm(%u) q_id(%u) drop_alg(%u) drop_tresh(%u) weight(%u)",
		ptm->tm_id, queue_cfg.queue_id, queue_cfg.drop_alg, queue_cfg.drop_threshold, queue_cfg.weight);

	if ((rc = rdpa_egress_tm_queue_cfg_set(sched, ptm->index, &queue_cfg))) {
		CMD_TM_LOG_ERROR("rdpa_egress_tm_queue_cfg_set() failed: mode(%u) rc(%d)", arbiter_mode, rc);
		bdmf_put(sched);
		return RDPA_DRV_Q_CFG_SET;
	}

	bdmf_put(sched);

	return 0;
}

/*******************************************************************************/
/* global routines                                                             */
/*******************************************************************************/

/*******************************************************************************
 *
 * Function: rdpa_cmd_tm_ioctl
 *
 * IOCTL interface to the RDPA TM API.
 *
 *******************************************************************************/
int rdpa_cmd_tm_ioctl(unsigned long arg)
{
	rdpa_drv_ioctl_tm_t *userTm_p = (rdpa_drv_ioctl_tm_t *)arg;
	rdpa_drv_ioctl_tm_t tm;
	bdmf_object_handle sched = NULL;
	bdmf_object_handle root_sched = NULL;
    bdmf_object_handle    subsidiary=NULL;
	rdpa_egress_tm_key_t  egress_tm_key;
	bdmf_error_t rc = BDMF_ERR_OK;
    int ret = 0;

    copy_from_user(&tm, userTm_p, sizeof(rdpa_drv_ioctl_tm_t));

    CMD_TM_LOG_DEBUG("RDPA TM CMD(%d)", tm.cmd);

    bdmf_lock();

    switch(tm.cmd)
    {
    	case RDPA_IOCTL_TM_CMD_GET_ROOT_TM: 
        {
          bdmf_object_handle   port_obj = NULL;
	      bdmf_number          tr69_root_tm_id;
          rdpa_port_tm_cfg_t   tm_cfg;

    		
		  CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_GET_ROOT_TM: port_id(%d)", tm.port_id);

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif

	      tm.found = FALSE; //start looking
		  

          /* looking for RootTM set by TR69 */
	      ret = rdpa_port_get(tm.port_id, &port_obj);
	      if (ret != 0) 
	      {
		     CMD_TM_LOG_ERROR("rdpa_port_get FAILED");
		     break;
	      }

          ret = rdpa_port_tm_cfg_get(port_obj, &tm_cfg);
	      if (ret != 0) 
	      {
		     CMD_TM_LOG_ERROR("rdpa_port_tm_cfg_get FAILED ret=%d", ret);
		     break;
	      }

          ret = rdpa_egress_tm_index_get(tm_cfg.sched, &tr69_root_tm_id);
		  if (ret != 0)
		  {
		     CMD_TM_LOG_INFO("**: RDPA_IOCTL_TM_CMD_GET_ROOT_TM: ROOT NOT FOUND");
		  }
		  else 
		  {
            rdpa_tm_sched_mode mode;
            tm.found = TRUE;
		    tm.root_tm_id = tr69_root_tm_id;
            CMD_TM_LOG_INFO("**: TR69 ROOT TM ID=[%d]. New(GUI) Root TM id=[%d] ", (int)tr69_root_tm_id, tm.root_tm_id);
			rdpa_egress_tm_mode_get(tm_cfg.sched, &mode);
		    ret = add_root_tm_to_port(tm.port_id, tm.root_tm_id, mode);
	      }

	      break;
    	}
        case RDPA_IOCTL_TM_CMD_TM_Q_CONFIG: 
		{
			rdpa_tm_rl_cfg_t rl_cfg;
			rdpa_tm_sched_mode arbiter_mode;

			CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_TM_CONFIG: dir(%s)",	(tm.dir == rdpa_dir_us)?"US":"DS");

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif

			egress_tm_key.dir = tm.dir;
			egress_tm_key.index = tm.root_tm_id; //it's tr69_root_tm_id;

			if ((rc = rdpa_egress_tm_get(&egress_tm_key, &root_sched))) 
			{
				CMD_TM_LOG_ERROR("rdpa_egress_tm_get() failed: ROOT_tm(%u) rc(%d)", tm.root_tm_id, rc);
				ret = RDPA_DRV_TM_GET;
				goto ioctl_exit;
			}
			CMD_TM_LOG_INFO("Got ROOT SHCED = %p", root_sched);

			//get a subsidiary for choosen queue 
			rc = rdpa_egress_tm_subsidiary_get(root_sched, tm.priority, &subsidiary);
	        if (rc!=0)
			{
			    CMD_TM_LOG_ERROR("rdpa_egress_tm_subsidiary_get FAILED for subsidiary[%d]. ret=%d", tm.priority, rc);
				ret = RDPA_DRV_TM_GET;
				goto ioctl_exit;
			}

		    CMD_TM_LOG_DEBUG("*** Got ROOT.SUBSIDIARY[%d] = %p", tm.priority, subsidiary);

			//Check if it's the same mode.
			arbiter_mode = port_list[tm.port_id].arbiter_mode;
	        CMD_TM_LOG_DEBUG("**: mode(%u)[SP=1; WRR=2]", arbiter_mode);

			if (arbiter_mode == rdpa_tm_sched_sp)
			{
			    CMD_TM_LOG_DEBUG("Got ROOT.SUBSIDIARY[%d] mode is SP", tm.priority);
				rl_cfg.af_rate = tm.shaping_rate * 1000; 	/* Best Effort: shaping_rate is in kbit/s: 1 kilobit = 1000 bits */
				rc = rdpa_egress_tm_rl_set(subsidiary, &rl_cfg);
				if (rc != 0)
					CMD_TM_LOG_ERROR("rdpa_egress_tm_rl_set FAILED");
				else 
					CMD_TM_LOG_INFO("**: rdpa_egress_tm_rl_set: rl=%d", (int)rl_cfg.af_rate);
			
			}
			else
			if (arbiter_mode == rdpa_tm_sched_wrr)
			{
			    CMD_TM_LOG_DEBUG("***Got ROOT.SUBSIDIARY[%d] mode is WRR", tm.priority);
				rc = rdpa_egress_tm_weight_set(subsidiary, tm.weight);
				if (rc != 0)
					CMD_TM_LOG_ERROR("rdpa_egress_tm_weight_set() failed. rc(%d)", rc);
				
			}
			
			bdmf_put(root_sched);

            break;

		}
    	case RDPA_IOCTL_TM_GET_BY_QID: {
    		CMD_TM_LOG_INFO("RDPA_IOCTL_TM_GET_BY_QID: ROOT_tm(%u) port_id(%d)", tm.root_tm_id, tm.port_id);

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif
    		tm.found = FALSE;

			if ((ret = get_tm_by_q_qid(tm.port_id, tm.q_id, &tm.tm_id, &tm.found))) 
			{
				if (ret == RDPA_DRV_PORT_NOT_ALLOC) {
					CMD_TM_LOG_DEBUG("get_tm_by_q_qid(): no port for tm for port_id(%u) rc(%d)", tm.port_id, rc);
					ret = 0; /* Hide this error from upper layer */
					goto ioctl_exit;
				} else {
					CMD_TM_LOG_ERROR("get_tm_by_q_qid() failed: port_id(%u) rc(%d)", tm.port_id, rc);
					goto ioctl_exit;
				}
			}

			tm.found = TRUE;
    		break;
    	}
    	case RDPA_IOCTL_TM_CMD_ROOT_TM_CONFIG: {

			CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_ROOT_TM_CONFIG: port_id(%u) dir(%s)",
				tm.port_id, (tm.dir == rdpa_dir_us)?"US":"DS");

    		if ((ret = root_tm_config(&tm, &sched))) {
				CMD_TM_LOG_ERROR("root_tm_config() failed: ROOT_tm(%u) rc(%d)", tm.root_tm_id, rc);
				goto ioctl_exit;
    		}

			if ((ret = add_root_tm_to_port(tm.port_id, tm.root_tm_id, tm.arbiter_mode))) {
				CMD_TM_LOG_ERROR("add_root_tm_to_port() failed: ROOT_tm(%u) rc(%d)", tm.root_tm_id, rc);
				goto ioctl_exit;
			}

			break;
    	}
        case RDPA_IOCTL_TM_CMD_TM_CONFIG: {
			BDMF_MATTR(sched_attrs, rdpa_egress_tm_drv());
		    rdpa_egress_tm_key_t egress_tm_key;

			CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_TM_CONFIG: dir(%s)",	(tm.dir == rdpa_dir_us)?"US":"DS");

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif

			egress_tm_key.dir = tm.dir;
			egress_tm_key.index = tm.root_tm_id;

			if ((rc = rdpa_egress_tm_get(&egress_tm_key, &root_sched))) {
				CMD_TM_LOG_ERROR("rdpa_egress_tm_get() failed: ROOT_tm(%u) rc(%d)", tm.root_tm_id, rc);
				ret = RDPA_DRV_TM_GET;
				goto ioctl_exit;
			}

			if ((ret = tm_set(sched_attrs, &tm, &tm.tm_id, root_sched, &sched))) {
				CMD_TM_LOG_ERROR("tm_set() failed: ROOT_tm(%u) rc(%d)", tm.root_tm_id, rc);
				bdmf_put(root_sched);
				goto ioctl_exit;
			}

			rdpa_egress_tm_weight_set(sched, tm.weight);

			if ((rc = rdpa_egress_tm_subsidiary_set(root_sched,  tm.priority, sched))) {
				CMD_TM_LOG_ERROR("rdpa_egress_tm_subsidiary_set() failed: rootId(%u) rc(%d)", tm.root_tm_id, rc);
				bdmf_put(root_sched);
				bdmf_destroy(sched);
				ret = RDPA_DRV_SUBS_SET;
				goto ioctl_exit;
			}
			CMD_TM_LOG_DEBUG("rdpa_egress_tm_subsidiary[%d] SET!!!", tm.priority);

			bdmf_put(root_sched);

            break;
        }
        case RDPA_IOCTL_TM_CMD_ROOT_TM_REMOVE: {
        	bdmf_error_t rc = BDMF_ERR_OK;

			CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_ROOT_TM_REMOVE: tm(%u)", tm.tm_id);

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif

			if ((ret = root_tm_remove(tm.port_id, tm.root_tm_id, tm.dir))) {
				CMD_TM_LOG_ERROR("root_tm_remove() failed: ROOT_tm(%u) rc(%d)", tm.root_tm_id, rc);
				goto ioctl_exit;
			}

            break;
        }
        case RDPA_IOCTL_TM_CMD_TM_REMOVE: {
        	bdmf_error_t rc = BDMF_ERR_OK;
			rdpa_egress_tm_key_t tm_key = {};

			CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_TM_REMOVE: tm(%u)", tm.tm_id);

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif

			tm_key.dir = tm.dir;
			tm_key.index = tm.tm_id;

			if ((rc = rdpa_egress_tm_get(&tm_key, &sched))) {
				CMD_TM_LOG_ERROR("rdpa_egress_tm_get() failed: tm(%u) rc(%d)", tm.tm_id, rc);
				ret = RDPA_DRV_TM_GET;
				goto ioctl_exit;
			}

			if (sched) {
				bdmf_put(sched);
				if ((rc = bdmf_destroy(sched))) {
					CMD_TM_LOG_ERROR("bdmf_destroy() failed: tm(%u) rc(%d)", tm.tm_id, rc);
					ret = RDPA_DRV_SH_DESTROY;
					goto ioctl_exit;
				}
			}

			CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_TM_REMOVE: queue count(%u)", port_list[tm.port_id].queue_alloc_cnt);

			if (!port_list[tm.port_id].queue_alloc_cnt) {
#ifdef __NEW_REPLACE__
				bdmf_object_handle port_obj = NULL;

				if (!check_rdpa_mode(rdpa_wan_gbe)) { /* Perform the TCONT binding only in GPON or EPON mode */
					CMD_TM_LOG_DEBUG("TM to PORT binding replace");

					/* ------------------- Acquire the port object and set sched object back ------------------- */
					if ((rc = rdpa_port_get(tm.port_id, &port_obj))) {
						CMD_TM_LOG_ERROR("rdpa_port_get() failed: port(%d) rc(%d)", tm.port_id, rc);
						ret = RDPA_DRV_PORT_GET;
						goto ioctl_exit;
					}

					if ((ret = tm_replace(port_obj, orig_sched))) {
						CMD_TM_LOG_ERROR("tm_replace() failed: port(%d) rc(%d)", tm.port_id, rc);
						bdmf_put(port_obj);
						goto ioctl_exit;
					}

					bdmf_put(port_obj);
				}
#endif /* __NEW_REPLACE__ */

				/* ------------------- Remove root tm ------------------- */
				if ((ret = root_tm_remove(tm.port_id, port_list[tm.port_id].root_tm_id, tm.dir))) {
					CMD_TM_LOG_ERROR("root_tm_remove() failed: ROOT_tm(%u) rc(%d)", tm.root_tm_id, ret);
					goto ioctl_exit;
				}
			}

            break;
        }
        case RDPA_IOCTL_TM_CMD_QUEUE_CONFIG: {
            CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_QUEUE_CONFIG: port_id(%u) tm(%u) dir(%s) q_id(%u) index(%u)",
				tm.port_id, tm.tm_id, (tm.dir == rdpa_dir_us)?"US":"DS", tm.q_id, tm.index);

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif

    		if ((ret = q_config(&tm))) {
                CMD_TM_LOG_ERROR("q_config() failed: port_id(%u) tm_id(%u) q_id(%u) index(%u) ret(%d)", tm.port_id, tm.tm_id, tm.q_id, tm.index, ret);
				goto ioctl_exit;
    		}
            CMD_TM_LOG_DEBUG("**: q_config() Success!: port_id(%u) tm_id(%u) q_id(%u) index(%u) ret(%d)", tm.port_id, tm.tm_id, tm.q_id, tm.index, ret);

			if ((ret = add_q_to_port(tm.port_id, tm.tm_id, tm.q_id))) {
				CMD_TM_LOG_ERROR("add_q_to_port() failed: port_id(%u) tm_id(%u) q_id(%u) ret(%d)", tm.port_id, tm.tm_id, tm.q_id, ret);
				goto ioctl_exit;
			}

            break;
        }
        case RDPA_IOCTL_TM_CMD_QUEUE_REMOVE: {
            CMD_TM_LOG_INFO("RDPA_IOCTL_TM_CMD_QUEUE_REMOVE: port_id(%u) tm(%u) dir(%s) q_id(%u) index(%u)",
				tm.port_id, tm.tm_id, (tm.dir == rdpa_dir_us)?"US":"DS", tm.q_id, tm.index);

#ifdef __DUMP_PORTS__
    		dump_ports();
#endif

    		if ((ret = q_config(&tm))) {
				CMD_TM_LOG_ERROR("q_config() failed: port_id(%u) tm_id(%u) q_id(%u) index(%u) rc(%d)", tm.port_id, tm.tm_id, tm.q_id, tm.index, ret);
				goto ioctl_exit;
    		}

			if ((ret = remove_q_from_port(tm.port_id, tm.tm_id, tm.q_id))) {
				CMD_TM_LOG_ERROR("remove_root_tm_from_port() failed: port_id(%u) tm_id(%u) q_id(%u) rc(%d)", tm.port_id, tm.tm_id, tm.q_id, ret);
				goto ioctl_exit;
			}

            break;
        }
        default:
            CMD_TM_LOG_ERROR("Invalid IOCTL cmd %d", tm.cmd);
            ret = RDPA_DRV_ERROR;
    }

ioctl_exit:

	bdmf_unlock();

	copy_to_user(userTm_p, &tm, sizeof(rdpa_drv_ioctl_tm_t));

    return ret;
}

/*******************************************************************************
 *
 * Function: rdpa_cmd_tm_init
 *
 * Initializes the RDPA TM API.
 *
 *******************************************************************************/
void rdpa_cmd_tm_init(void)
{
#ifdef __EARLY_CONFIG__
	int q_id = 0;
	rdpa_drv_ioctl_tm_t tm;
	bdmf_object_handle root_sched = NULL;
	int ret = 0;
#endif

	CMD_TM_LOG_DEBUG("RDPA TM INIT");

    memset(&rdpa_cmd_tm_ctrl_g, 0, sizeof(rdpa_cmd_tm_ctrl_t));

    rdpa_cmd_tm_ctrl_g.tm_index = 0;

#ifdef __EARLY_CONFIG__
	tm.port_id 		= rdpa_if_wan0;
	tm.dir 			= rdpa_dir_us;
	tm.arbiter_mode = rdpa_tm_sched_sp;
	tm.level 		= rdpa_tm_level_egress_tm;

    if (!(ret = root_tm_config(&tm, &root_sched))) {
		CMD_TM_LOG_INFO("DEFAULT ROOT TM CONFIGURED OK");
		for (q_id = 0; q_id <= 7; q_id++) {
			tm.tm_id = tm.root_tm_id;
			tm.q_id = q_id;
            tm.index = q_id;
			if ((ret = q_config(&tm))) {
				CMD_TM_LOG_ERROR("q_config() failed: tm_id(%u) q_id(%u) rc(%d)", tm.tm_id, q_id, ret);
				return;
			}
		}
    } else {
    	CMD_TM_LOG_INFO("DEFAULT ROOT TM CONFIGURATION FAILED: rc(%d)", ret);
    }
#endif
}

EXPORT_SYMBOL(rdpa_cmd_tm_ioctl);
EXPORT_SYMBOL(rdpa_cmd_tm_init);
