// <:copyright-BRCM:2013:DUAL/GPL:standard
// 
//    Copyright (c) 2013 Broadcom Corporation
//    All Rights Reserved
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2, as published by
// the Free Software Foundation (the "GPL").
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// 
// A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
// writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
// 
// :>
/*
 * egress_tm object header file.
 * This header file is generated automatically. Do not edit!
 */
#ifndef _RDPA_AG_EGRESS_TM_H_
#define _RDPA_AG_EGRESS_TM_H_

/** \addtogroup egress_tm
 * @{
 */


/** Get egress_tm type handle.
 *
 * This handle should be passed to bdmf_new_and_set() function in
 * order to create an egress_tm object.
 * \return egress_tm type handle
 */
bdmf_type_handle rdpa_egress_tm_drv(void);

/* egress_tm: Attribute types */
typedef enum {
    rdpa_egress_tm_attr_dir = 0, /* dir : MKRI : enum/4 : Traffic Direction */
    rdpa_egress_tm_attr_index = 1, /* index : KRI : number/4 : Egress-TM Index */
    rdpa_egress_tm_attr_level = 2, /* level : RI : enum/4 : Egress-TM Next Level */
    rdpa_egress_tm_attr_mode = 3, /* mode : RI : enum/4 : Scheduler Operating Mode */
    rdpa_egress_tm_attr_overall_rl = 4, /* overall_rl : RI : bool/1 : Overall Rate Limiter */
    rdpa_egress_tm_attr_rl = 6, /* rl : RW : aggregate/4 tm_rl_cfg(rdpa_tm_rl_cfg_t) : Rate Configuration */
    rdpa_egress_tm_attr_queue_cfg = 7, /* queue_cfg : RWF : aggregate/24[8] tm_queue_cfg(rdpa_tm_queue_cfg_t) : Queue Parameters Configuration */
    rdpa_egress_tm_attr_queue_flush = 8, /* queue_flush : W : bool/1[8(tm_queue_index)] : Flush Egress Queue */
    rdpa_egress_tm_attr_queue_stat = 9, /* queue_stat : RF : number/4[8(tm_queue_index)] : Retrive Egress Queue Statistics */
    rdpa_egress_tm_attr_subsidiary = 10, /* subsidiary : RWF : object/4[32] : Next Level Egress-TM */
    rdpa_egress_tm_attr_weight = 11, /* weight : RW : number/4 : Weight for WRR scheduling (0 for unset) */
} rdpa_egress_tm_attr_types;

/** egress_tm object key. */
typedef struct {
    rdpa_traffic_dir dir; /**< egress_tm: Traffic Direction */
    bdmf_number index; /**< egress_tm: Egress-TM Index */
} rdpa_egress_tm_key_t;


extern int (*f_rdpa_egress_tm_get)(const rdpa_egress_tm_key_t * key_, bdmf_object_handle *pmo);

/** Get egress_tm object by key.

 * This function returns egress_tm object instance by key.
 * \param[in] key_    Object key
 * \param[out] egress_tm_obj    Object handle
 * \return    0=OK or error <0
 */
int rdpa_egress_tm_get(const rdpa_egress_tm_key_t * key_, bdmf_object_handle *egress_tm_obj);

/** Get egress_tm/dir attribute.
 *
 * Get Traffic Direction.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[out]  dir_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_dir_get(bdmf_object_handle mo_, rdpa_traffic_dir *dir_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_egress_tm_attr_dir, &_nn_);
    *dir_ = (rdpa_traffic_dir)_nn_;
    return _rc_;
}


/** Set egress_tm/dir attribute.
 *
 * Set Traffic Direction.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   dir_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_dir_set(bdmf_object_handle mo_, rdpa_traffic_dir dir_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_egress_tm_attr_dir, dir_);
}


/** Get egress_tm/index attribute.
 *
 * Get Egress-TM Index.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[out]  index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_index_get(bdmf_object_handle mo_, bdmf_number *index_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_egress_tm_attr_index, &_nn_);
    *index_ = (bdmf_number)_nn_;
    return _rc_;
}


/** Set egress_tm/index attribute.
 *
 * Set Egress-TM Index.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_index_set(bdmf_object_handle mo_, bdmf_number index_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_egress_tm_attr_index, index_);
}


/** Get egress_tm/level attribute.
 *
 * Get Egress-TM Next Level.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[out]  level_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_level_get(bdmf_object_handle mo_, rdpa_tm_level_type *level_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_egress_tm_attr_level, &_nn_);
    *level_ = (rdpa_tm_level_type)_nn_;
    return _rc_;
}


/** Set egress_tm/level attribute.
 *
 * Set Egress-TM Next Level.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   level_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_level_set(bdmf_object_handle mo_, rdpa_tm_level_type level_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_egress_tm_attr_level, level_);
}


/** Get egress_tm/mode attribute.
 *
 * Get Scheduler Operating Mode.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[out]  mode_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_mode_get(bdmf_object_handle mo_, rdpa_tm_sched_mode *mode_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_egress_tm_attr_mode, &_nn_);
    *mode_ = (rdpa_tm_sched_mode)_nn_;
    return _rc_;
}


/** Set egress_tm/mode attribute.
 *
 * Set Scheduler Operating Mode.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   mode_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_mode_set(bdmf_object_handle mo_, rdpa_tm_sched_mode mode_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_egress_tm_attr_mode, mode_);
}


/** Get egress_tm/overall_rl attribute.
 *
 * Get Overall Rate Limiter.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[out]  overall_rl_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_overall_rl_get(bdmf_object_handle mo_, bdmf_boolean *overall_rl_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_egress_tm_attr_overall_rl, &_nn_);
    *overall_rl_ = (bdmf_boolean)_nn_;
    return _rc_;
}


/** Set egress_tm/overall_rl attribute.
 *
 * Set Overall Rate Limiter.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   overall_rl_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_egress_tm_overall_rl_set(bdmf_object_handle mo_, bdmf_boolean overall_rl_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_egress_tm_attr_overall_rl, overall_rl_);
}


/** Get egress_tm/rl attribute.
 *
 * Get Rate Configuration.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[out]  rl_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_rl_get(bdmf_object_handle mo_, rdpa_tm_rl_cfg_t * rl_)
{
    return bdmf_attr_get_as_buf(mo_, rdpa_egress_tm_attr_rl, rl_, sizeof(*rl_));
}


/** Set egress_tm/rl attribute.
 *
 * Set Rate Configuration.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   rl_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_rl_set(bdmf_object_handle mo_, const rdpa_tm_rl_cfg_t * rl_)
{
    return bdmf_attr_set_as_buf(mo_, rdpa_egress_tm_attr_rl, rl_, sizeof(*rl_));
}


/** Get egress_tm/queue_cfg attribute entry.
 *
 * Get Queue Parameters Configuration.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[out]  queue_cfg_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_queue_cfg_get(bdmf_object_handle mo_, bdmf_index ai_, rdpa_tm_queue_cfg_t * queue_cfg_)
{
    return bdmf_attrelem_get_as_buf(mo_, rdpa_egress_tm_attr_queue_cfg, (bdmf_index)ai_, queue_cfg_, sizeof(*queue_cfg_));
}


/** Set egress_tm/queue_cfg attribute entry.
 *
 * Set Queue Parameters Configuration.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[in]   queue_cfg_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_queue_cfg_set(bdmf_object_handle mo_, bdmf_index ai_, const rdpa_tm_queue_cfg_t * queue_cfg_)
{
    return bdmf_attrelem_set_as_buf(mo_, rdpa_egress_tm_attr_queue_cfg, (bdmf_index)ai_, queue_cfg_, sizeof(*queue_cfg_));
}


/** Set egress_tm/queue_flush attribute entry.
 *
 * Set Flush Egress Queue.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[in]   queue_flush_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_queue_flush_set(bdmf_object_handle mo_, rdpa_tm_queue_index_t * ai_, bdmf_boolean queue_flush_)
{
    return bdmf_attrelem_set_as_num(mo_, rdpa_egress_tm_attr_queue_flush, (bdmf_index)ai_, queue_flush_);
}


/** Get egress_tm/queue_stat attribute entry.
 *
 * Get Retrive Egress Queue Statistics.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[out]  queue_stat_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_queue_stat_get(bdmf_object_handle mo_, rdpa_tm_queue_index_t * ai_, uint32_t *queue_stat_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attrelem_get_as_num(mo_, rdpa_egress_tm_attr_queue_stat, (bdmf_index)ai_, &_nn_);
    *queue_stat_ = (uint32_t)_nn_;
    return _rc_;
}


/** Get next egress_tm/queue_stat attribute entry.
 *
 * Get next Retrive Egress Queue Statistics.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in,out]   ai_ Attribute array index
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_queue_stat_get_next(bdmf_object_handle mo_, rdpa_tm_queue_index_t * ai_)
{
    return bdmf_attrelem_get_next(mo_, rdpa_egress_tm_attr_queue_stat, (bdmf_index *)ai_);
}


/** Get egress_tm/subsidiary attribute entry.
 *
 * Get Next Level Egress-TM.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[out]  subsidiary_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_subsidiary_get(bdmf_object_handle mo_, bdmf_index ai_, bdmf_object_handle *subsidiary_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attrelem_get_as_num(mo_, rdpa_egress_tm_attr_subsidiary, (bdmf_index)ai_, &_nn_);
    *subsidiary_ = (bdmf_object_handle)(long)_nn_;
    return _rc_;
}


/** Set egress_tm/subsidiary attribute entry.
 *
 * Set Next Level Egress-TM.
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[in]   subsidiary_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_subsidiary_set(bdmf_object_handle mo_, bdmf_index ai_, bdmf_object_handle subsidiary_)
{
    return bdmf_attrelem_set_as_num(mo_, rdpa_egress_tm_attr_subsidiary, (bdmf_index)ai_, (long)subsidiary_);
}


/** Get egress_tm/weight attribute.
 *
 * Get Weight for WRR scheduling (0 for unset).
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[out]  weight_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_weight_get(bdmf_object_handle mo_, bdmf_number *weight_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_egress_tm_attr_weight, &_nn_);
    *weight_ = (bdmf_number)_nn_;
    return _rc_;
}


/** Set egress_tm/weight attribute.
 *
 * Set Weight for WRR scheduling (0 for unset).
 * \param[in]   mo_ egress_tm object handle or mattr transaction handle
 * \param[in]   weight_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_egress_tm_weight_set(bdmf_object_handle mo_, bdmf_number weight_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_egress_tm_attr_weight, weight_);
}

/** @} end of egress_tm Doxygen group */




#endif /* _RDPA_AG_EGRESS_TM_H_ */
