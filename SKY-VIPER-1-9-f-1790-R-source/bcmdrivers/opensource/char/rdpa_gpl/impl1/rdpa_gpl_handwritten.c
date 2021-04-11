/*
* <:copyright-BRCM:2013:GPL/GPL:standard
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
 * CPU interface
 */
#include <bdmf_interface.h>
#include <rdpa_types.h>

/** Enable CPU queue interrupt */
void (*f_rdpa_cpu_int_enable)(rdpa_cpu_port port, int queue);
EXPORT_SYMBOL(f_rdpa_cpu_int_enable);
void rdpa_cpu_int_enable(rdpa_cpu_port port, int queue)
{
    if (!f_rdpa_cpu_int_enable)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return;
    }
    f_rdpa_cpu_int_enable(port, queue);
}
EXPORT_SYMBOL(rdpa_cpu_int_enable);

/** Disable CPU queue interrupt */
void (*f_rdpa_cpu_int_disable)(rdpa_cpu_port port, int queue);
EXPORT_SYMBOL(f_rdpa_cpu_int_disable);
void rdpa_cpu_int_disable(rdpa_cpu_port port, int queue)
{
    if (!f_rdpa_cpu_int_disable)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return;
    }
    f_rdpa_cpu_int_disable(port, queue);
}
EXPORT_SYMBOL(rdpa_cpu_int_disable);

/** Clear CPU queue interrupt */
void (*f_rdpa_cpu_int_clear)(rdpa_cpu_port port, int queue);
EXPORT_SYMBOL(f_rdpa_cpu_int_clear);
void rdpa_cpu_int_clear(rdpa_cpu_port port, int queue)
{
    if (!f_rdpa_cpu_int_clear)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return;
    }
    f_rdpa_cpu_int_clear(port, queue);
}
EXPORT_SYMBOL(rdpa_cpu_int_clear);

/** Pull a single received packet from host queue. */
int (*f_rdpa_cpu_packet_get)(rdpa_cpu_port port, bdmf_index queue,
    bdmf_sysb *sysb, rdpa_cpu_rx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_packet_get);
int rdpa_cpu_packet_get(rdpa_cpu_port port, bdmf_index queue,
    bdmf_sysb *sysb, rdpa_cpu_rx_info_t *info)
{
    if (!f_rdpa_cpu_packet_get)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_packet_get(port, queue, sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_packet_get);

/** Send system buffer */
int (*f_rdpa_cpu_send_sysb)(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_sysb);
int rdpa_cpu_send_sysb(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info)
{
    if (!f_rdpa_cpu_send_sysb)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_sysb(sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_sysb);

/** Send raw packet */
int (*f_rdpa_cpu_send_raw)(void *data, uint32_t length, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_raw);
int rdpa_cpu_send_raw(void *data, uint32_t length, const rdpa_cpu_tx_info_t *info)
{
    if (!f_rdpa_cpu_send_raw)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_raw(data, length, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_raw);

/** Map from HW port to rdpa_if */
rdpa_if (*f_rdpa_port_map_from_hw_port)(int hw_port, bdmf_boolean emac_only);
EXPORT_SYMBOL(f_rdpa_port_map_from_hw_port);
rdpa_if rdpa_port_map_from_hw_port(int hw_port, bdmf_boolean emac_only)
{
    if (!f_rdpa_port_map_from_hw_port)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return rdpa_if_none;
    }
    return f_rdpa_port_map_from_hw_port(hw_port, emac_only);
}
EXPORT_SYMBOL(rdpa_port_map_from_hw_port);


int (*f_rdpa_cpu_queue_not_empty)(rdpa_cpu_port port, bdmf_index queue);
EXPORT_SYMBOL(f_rdpa_cpu_queue_not_empty);
int rdpa_cpu_queue_not_empty(rdpa_cpu_port port, bdmf_index queue)
{
	if (!f_rdpa_cpu_queue_not_empty)
	{
	   BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
	   return BDMF_ERR_STATE;
	}
	return f_rdpa_cpu_queue_not_empty(port,queue);
}
EXPORT_SYMBOL(rdpa_cpu_queue_not_empty);

/** Send EPON Dgasp */
int (*f_rdpa_cpu_send_epon_dying_gasp)(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);
EXPORT_SYMBOL(f_rdpa_cpu_send_epon_dying_gasp);
int rdpa_cpu_send_epon_dying_gasp(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info)
{
    if (!f_rdpa_cpu_send_epon_dying_gasp)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return BDMF_ERR_STATE;
    }
    return f_rdpa_cpu_send_epon_dying_gasp(sysb, info);
}
EXPORT_SYMBOL(rdpa_cpu_send_epon_dying_gasp);

/** Run time mapping from HW port to rdpa_if using array */
rdpa_if (*f_rdpa_physical_port_to_rdpa_if)(rdpa_physical_port port);
EXPORT_SYMBOL(f_rdpa_physical_port_to_rdpa_if);
rdpa_if rdpa_physical_port_to_rdpa_if(rdpa_physical_port port)
{
    if (!f_rdpa_physical_port_to_rdpa_if)
    {
        BDMF_TRACE_ERR("rdpa.ko is not loaded\n");
        return rdpa_if_none;
    }
    return f_rdpa_physical_port_to_rdpa_if(port);
}
EXPORT_SYMBOL(rdpa_physical_port_to_rdpa_if);
