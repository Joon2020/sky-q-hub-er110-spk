/*
   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

    <:label-BRCM:2013:DUAL/GPL:standard
    
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

/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file contains the implementation of the Runner CPU ring interface     */
/*                                                                            */
/******************************************************************************/

#ifndef _RDP_CPU_RING_H_
#define _RDP_CPU_RING_H_

#if defined(__KERNEL__) || defined(_CFE_)

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#include<bcm_pkt_lengths.h>

#ifdef _CFE_
#include "bl_os_wraper.h"
#endif
#include "rdpa_types.h"
#include "rdd.h"



/*****************************************************************************/
/*                                                                           */
/* defines and structures                                                    */
/*                                                                           */
/*****************************************************************************/

#ifdef _CFE_
#include "lib_malloc.h"
#include "cfe_iocb.h"
#define RDP_CPU_RING_MAX_QUEUES	1
#define RDP_WLAN_MAX_QUEUES		0

#define CACHE_TO_NONCACHE(x)    			K0_TO_K1((uint32_t)x)
#define NONCACHE_TO_CACHE(x)    			K1_TO_K0((uint32_t)x)
#define CACHED_MALLOC(_size) 				NONCACHE_TO_CACHE(KMALLOC(_size,0))
#define NONCACHED_MALLOC(_size) 			CACHE_TO_NONCACHE(KMALLOC(_size,0))
#define NONCACHED_FREE(_ptr) 				KFREE(NONCACHE_TO_CACHE(_ptr))
#define CACHED_FREE(_ptr)					KFREE(_ptr)
#define VIRT_TO_PHYS(_addr)					K0_TO_PHYS((uint32_t)_addr)
#define PHYS_TO_CACHED(_addr)				PHYS_TO_K0((uint32_t)_addr)
#define PHYS_TO_UNCACHED(_addr)				PHYS_TO_K1((uint32_t)_addr)
#define DMA_CACHE_LINE						16
extern void _cfe_flushcache(int, uint8_t *, uint8_t *);
#define FLUSH_RANGE(s,l) 					_cfe_flushcache(CFE_CACHE_FLUSH_RANGE,((uint8_t *) (s)),((uint8_t *) (s))+(l))
#define INV_RANGE(s,l) 					    _cfe_flushcache(CFE_CACHE_INVAL_RANGE,((uint8_t *) (s)),((uint8_t *) (s))+(l))

#elif defined(__KERNEL__)
#include "rdpa_cpu.h"
#include "bdmf_system.h"
#include "bdmf_shell.h"
#include "bdmf_dev.h"

#define RDP_CPU_RING_MAX_QUEUES		RDPA_CPU_MAX_QUEUES
#define RDP_WLAN_MAX_QUEUES			RDPA_WLAN_MAX_QUEUES

#define CACHE_TO_NONCACHE(x)    			KSEG1ADDR(x)
#define NONCACHE_TO_CACHE(x)    			KSEG0ADDR(x)
#define CACHED_MALLOC(_size) 				kmalloc(_size,GFP_KERNEL)
#define CACHED_FREE(ptr) 					kfree((void*)NONCACHE_TO_CACHE(ptr))
#define NONCACHED_MALLOC(_size) 			CACHE_TO_NONCACHE(kmalloc(_size,GFP_ATOMIC|__GFP_DMA))
#define NONCACHED_FREE(_ptr) 				kfree((void*)NONCACHE_TO_CACHE(_ptr))
#define VIRT_TO_PHYS(_addr)					CPHYSADDR(_addr)
#define PHYS_TO_CACHED(_addr)				KSEG0ADDR(_addr)
#define PHYS_TO_UNCACHED(_addr)				KSEG1ADDR(_addr)
#define DMA_CACHE_LINE						dma_get_cache_alignment()
#define FLUSH_RANGE(s,l) 					bdmf_dcache_flush(s,l);
#define INV_RANGE(s,l) 					    bdmf_dcache_inv(s,l);

extern const bdmf_attr_enum_table_t rdpa_cpu_reason_enum_table;
#endif

typedef enum
{
	sysb_type_skb,
	sysb_type_fkb,
	sysb_type_raw,
}cpu_ring_sysb_type;

typedef enum
{
	type_cpu_rx,
	type_pci_tx
}cpu_ring_type;


typedef struct
{
	uint8_t*                        data_ptr;
	uint16_t                        packet_size;
	uint16_t                        flow_id;
	uint16_t						reason;
	uint16_t					    src_bridge_port;
    uint16_t                        dst_ssid;
    uint16_t						wlan_chain_prio; //chainId 8 bit|priority 4 bit
}
CPU_RX_PARAMS;

typedef void* (sysbuf_alloc_cb)(cpu_ring_sysb_type, uint32_t *pDataPtr);
typedef void (sysbuf_free_cb)(void* packetPtr);

typedef void* t_sysb_ptr;

typedef enum
{
	OWNERSHIP_RUNNER,
	OWNERSHIP_HOST
}E_DESCRIPTOR_OWNERSHIP;

typedef struct
{
	union{
		uint32_t word0;
		struct{
			uint32_t	flow_id:12;
			uint32_t	reserved0:1;
			uint32_t	source_port:5;
			uint32_t	packet_length:14;
		};
	};
	union{
		uint32_t word1;
		struct{
			uint32_t	payload_offset_flag:1;
			uint32_t	reason:6;
			uint32_t	dst_ssid:16;
			uint32_t	reserved1:5;
			uint32_t	descriptor_type:4;
			//uint32_t	abs_flag:1;
			//uint32_t	flow_id:8;

		};
	};
	union{
		uint32_t word2;
		struct{
			uint32_t	ownership:1;
			uint32_t	reserved2:2;
			uint32_t	host_buffer_data_pointer:29;
		};
	};
	union{
		uint32_t word3;
	struct{
			uint32_t	reserved4:16;
			uint32_t    ip_sync_1588_idx:4;
			uint32_t	wl_tx_priority:4;
			uint32_t	wl_chain_id:8;
			};
	};
}
CPU_RX_DESCRIPTOR;



typedef union
{
	CPU_RX_DESCRIPTOR cpu_rx;
	//PCI_TX_DESCRIPTOR pci_tx; not ready yet
}
RING_DESC_UNION;

#define MAX_BUFS_IN_CACHE 32 
typedef struct
{
	uint32_t				ring_id;
	uint32_t				admin_status;
	uint32_t				num_of_entries;
	uint32_t				size_of_entry;
	uint32_t				packet_size;
	cpu_ring_sysb_type		buff_type;
	RING_DESC_UNION*		head;
	RING_DESC_UNION*		base;
	RING_DESC_UNION*		end;
	uint32_t               buff_cache_cnt;
	uint32_t*              buff_cache;
}
RING_DESCTIPTOR;

/*array of possible rings private data*/
#define D_NUM_OF_RING_DESCRIPTORS (RDP_CPU_RING_MAX_QUEUES + RDP_WLAN_MAX_QUEUES)


int rdp_cpu_ring_create_ring(uint32_t ringId,uint32_t entries, cpu_ring_sysb_type buff_type);

int rdp_cpu_ring_delete_ring(uint32_t ringId);

int rdp_cpu_ring_read_packet_copy(uint32_t ringId, CPU_RX_PARAMS* rxParams);

int rdp_cpu_ring_get_queue_size(uint32_t ringId);

int rdp_cpu_ring_get_queued(uint32_t ringId);

int rdp_cpu_ring_flush(uint32_t ringId);

int rdp_cpu_ring_not_empty(uint32_t ringId);

#endif /* if defined(__KERNEL__) || defined(_CFE_) */

#endif /* _RDP_CPU_RING_H_ */
