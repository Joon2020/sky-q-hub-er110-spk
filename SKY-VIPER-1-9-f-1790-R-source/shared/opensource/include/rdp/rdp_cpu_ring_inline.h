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

#ifndef _RDP_CPU_RING_INLINE_H_
#define _RDP_CPU_RING_INLINE_H_

#include "rdp_cpu_ring.h"

#define D_WLAN_CHAINID_OFFSET	8
#if defined(__KERNEL__)
static inline void *rdp_databuf_alloc( RING_DESCTIPTOR* 	pDescriptor)
{
	if(likely(pDescriptor->buff_cache_cnt))
	{
		return (void *)(pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt]);
	}
	else
	{
		uint32_t alloc_cnt;
		/* refill the local cache from global pool */
		alloc_cnt = bdmf_sysb_databuf_alloc(pDescriptor->buff_cache, MAX_BUFS_IN_CACHE, 0);
		if(alloc_cnt)
		{
			pDescriptor->buff_cache_cnt = alloc_cnt;
			return (void *)(pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt]);
		}
	}
	return NULL;
}

static inline void rdp_databuf_free(void* pBuf, uint32_t context)
{
	bdmf_sysb_databuf_free(pBuf, context);
}
#elif defined(_CFE_)

static inline void *rdp_databuf_alloc( RING_DESCTIPTOR* 	pDescriptor)
{
	void *pBuf = KMALLOC(BCM_PKTBUF_SIZE,16);

	if(pBuf)
	{
		INV_RANGE(pBuf, BCM_PKTBUF_SIZE);
		return pBuf;		
	}
	return NULL;
}

static inline void rdp_databuf_free(void* pBuf, uint32_t context)
{

	KFREE(pBuf);
}

#endif


#if defined(__KERNEL__) || defined(_CFE_)
extern RING_DESCTIPTOR host_ring[D_NUM_OF_RING_DESCRIPTORS];

static inline void AssignPacketBuffertoRing(RING_DESCTIPTOR*	pDescriptor,volatile RING_DESC_UNION* pTravel, void *pBuf)
{
        /* assign the buffer address to ring and set the ownership to runner
         * by clearing  bit 31 which is used as ownership flag */
        pTravel->cpu_rx.word2   = ( (VIRT_TO_PHYS(pBuf)) & 0x7fffffff );

        /* advance the head ptr, wrap around if needed*/
        if ( ++pDescriptor->head == pDescriptor->end )
            pDescriptor->head = pDescriptor->base;
}

static inline int ReadPacketFromRing(RING_DESCTIPTOR*	pDescriptor ,volatile RING_DESC_UNION*	pTravel	, CPU_RX_PARAMS* rxParams)
{
    /* pTravel is in uncached mem so reading 32bits at a time into
       cached mem improves performance*/
    CPU_RX_DESCRIPTOR	    rxDesc;

    rxDesc.word2 = pTravel->cpu_rx.word2;

    if((rxDesc.ownership == OWNERSHIP_HOST))
    {
        rxParams->data_ptr = (uint8_t *) PHYS_TO_CACHED(rxDesc.word2);

        rxDesc.word0 = pTravel->cpu_rx.word0 ;
        rxParams->packet_size = rxDesc.packet_length;
        rxParams->src_bridge_port = (BL_LILAC_RDD_BRIDGE_PORT_DTE)rxDesc.source_port;
        rxParams->flow_id = rxDesc.flow_id;

        rxDesc.word1 = pTravel->cpu_rx.word1 ;
        rxParams->reason = (BL_LILAC_RDD_CPU_RX_REASON_DTE)rxDesc.reason;
        rxParams->dst_ssid = rxDesc.dst_ssid;
        rxDesc.word3 = pTravel->cpu_rx.word3 ;
    	rxParams->wlan_chain_prio = (rxDesc.wl_chain_id << D_WLAN_CHAINID_OFFSET)|rxDesc.wl_tx_priority;

	    return 0;
    }

    return BL_LILAC_RDD_ERROR_CPU_RX_QUEUE_EMPTY;
}

#ifndef _CFE_
/*this API get the pointer of the next available packet and reallocate buffer in ring
 * in the descriptor is optimized to 16 bytes cache line, 6838 has 16 bytes cache line
 * while 68500 has 32 bytes cache line, so we don't prefetch the descriptor to cache
 * Also on ARM platform we are not sure of how to skip L2 cache, and use only L1 cache
 * so for now  always use uncached accesses to Packet Descriptor(pTravel)
 */

static inline int rdp_cpu_ring_read_packet_refill(uint32_t ring_id, CPU_RX_PARAMS* rxParams)
{
	uint32_t 					ret;
	RING_DESCTIPTOR* 			pDescriptor		= &host_ring[ ring_id  ];
	RING_DESC_UNION*			pTravel 		= pDescriptor->head;
	void*						pNewBuf;


#ifdef __KERNEL__
	if(unlikely(pDescriptor->admin_status == 0))
		return BL_LILAC_RDD_ERROR_CPU_RX_QUEUE_EMPTY;
#endif

	ret = ReadPacketFromRing(pDescriptor,pTravel, rxParams);
	if(ret)
	{
		return  ret;
	}

	/* A valid packet is recieved try to allocate a new data buffer and
	 * refill the ring before giving the packet to upper layers
	 */

	pNewBuf	= rdp_databuf_alloc(pDescriptor);

	/*validate allocation*/
	if(unlikely(!pNewBuf))
	{
		//printk("ERROR:system buffer allocation failed!\n");
		/*assign old data buffer back to ring*/
		pNewBuf	= rxParams->data_ptr;
		rxParams->data_ptr = NULL;
		ret=1;
	}

	AssignPacketBuffertoRing(pDescriptor, pTravel, pNewBuf);

	return ret;
}

#endif
#endif /* defined(__KERNEL__) || defined(_CFE_) */
#endif /* _RDP_CPU_RING_INLINE_H_ */
