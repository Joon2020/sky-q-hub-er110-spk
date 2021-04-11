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

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
#include "rdp_cpu_ring.h"
#include "rdp_cpu_ring_inline.h"

#if defined(__KERNEL__)
static uint32_t		init_shell = 0;
#endif

#if !defined(__KERNEL__) && !defined(_CFE_)
	#error "rdp_cpu_ring is supported only in CFE and Kernel modules"
#endif

RING_DESCTIPTOR host_ring[D_NUM_OF_RING_DESCRIPTORS] = {};
EXPORT_SYMBOL(host_ring);

/*bdmf shell is compiling only for RDPA and not CFE*/
#ifdef __KERNEL__
#define MAKE_BDMF_SHELL_CMD_NOPARM(dir, cmd, help, cb) \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, NULL)

#define MAKE_BDMF_SHELL_CMD(dir, cmd, help, cb, parms...)   \
{                                                           \
    static bdmfmon_cmd_parm_t cmd_parms[]={                 \
        parms,                                              \
        BDMFMON_PARM_LIST_TERMINATOR                        \
    };                                                      \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, cmd_parms); \
}




static int cpu_ring_shell_list_rings ( bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms )
{
    uint32_t cntr;
	uint32_t first = 0 ,last = D_NUM_OF_RING_DESCRIPTORS;


    bdmf_session_print ( session, "CPU RX Ring Descriptors \n" );
    bdmf_session_print ( session, "------------------------------\n" );

    if (n_parms == 1 )
    {
    	first = ( uint32_t )parm[ 0 ].value.unumber;
    	last = first + 1;
    }

    for( cntr=first;cntr < last;cntr++ )
    {
    	if(!host_ring[cntr].num_of_entries) continue;

    	bdmf_session_print ( session, "CPU RX Ring Queue = %d:\n",cntr );
    	bdmf_session_print ( session, "\tCPU RX Ring Queue = %d \n",cntr );
    	bdmf_session_print ( session, "\tNumber of entries = %d\n",host_ring[cntr].num_of_entries );
    	bdmf_session_print ( session, "\tSize of entry = %d bytes\n",host_ring[cntr].size_of_entry );
    	bdmf_session_print ( session, "\tAllocated Packet size = %d bytes\n",host_ring[cntr].packet_size );
    	bdmf_session_print ( session, "\tSystem Buffer Type = %s\n",host_ring[cntr].buff_type==sysb_type_skb ?
    						"skb":host_ring[cntr].buff_type==sysb_type_fkb?"fkb":"raw");
    	bdmf_session_print ( session, "\tRing Base address = 0x%pK \n",host_ring[cntr].base );
    	bdmf_session_print ( session, "\tRing Head address = 0x%pK \n",host_ring[cntr].head );
    	bdmf_session_print ( session, "\tRing Head position = %d \n",host_ring[cntr].head - host_ring[cntr].base);
    	bdmf_session_print ( session, "\tCurrently Queued = %d \n",rdp_cpu_ring_get_queued(cntr));
    	bdmf_session_print ( session, "-------------------------------\n" );
    	bdmf_session_print ( session, "\n\n" );
    }

    return ( 0 );
}
static int cpu_ring_shell_print_pd ( bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms )
{
	uint32_t 					ring_id;
	uint32_t					pdIndex;
	volatile RING_DESC_UNION*	pdPtr;

	ring_id  = ( uint32_t )parm[ 0 ].value.unumber;
	pdIndex = ( uint32_t )parm[ 1].value.unumber;
	pdPtr = host_ring[ring_id].base + pdIndex;

	bdmf_session_print ( session, "descriptor ownership is:%s\n",pdPtr->cpu_rx.ownership ? "Host":"Runner");
	bdmf_session_print ( session, "descriptor packet DDR uncached address is:0x%pK\n",(void*)PHYS_TO_UNCACHED(pdPtr->cpu_rx.host_buffer_data_pointer));
	bdmf_session_print ( session, "descriptor packet len is:%d\n",pdPtr->cpu_rx.packet_length);
	if(pdPtr->cpu_rx.ownership == OWNERSHIP_HOST)
	{
		bdmf_session_print ( session, "descriptor type is: %d\n",pdPtr->cpu_rx.descriptor_type );
		bdmf_session_print ( session, "descriptor source port is: %d\n",pdPtr->cpu_rx.source_port );
		bdmf_session_print ( session, "descriptor packet reason is: %s\n",bdmf_attr_get_enum_text_hlp(&rdpa_cpu_reason_enum_table, pdPtr->cpu_rx.reason) );
		bdmf_session_print ( session, "descriptor packet flow_id is: %d\n",pdPtr->cpu_rx.flow_id );
		bdmf_session_print ( session, "descriptor packet wl chain id is: %d\n",pdPtr->cpu_rx.wl_chain_id );
		bdmf_session_print ( session, "descriptor packet wl priority is: %d\n",pdPtr->cpu_rx.wl_tx_priority);
	}
	return 0;
}

static int cpu_ring_shell_admin_ring ( bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms )
{
	uint32_t 					ring_id;
	ring_id  						= ( uint32_t )parm[ 0 ].value.unumber;
	host_ring[ring_id].admin_status 	= ( uint32_t )parm[ 1 ].value.unumber;

	bdmf_session_print ( session, "ring_id %d admin status is set to :%s\n",ring_id,host_ring[ring_id].admin_status ? "Up" : "Down");

	return 0;

}
#define MAKE_BDMF_SHELL_CMD_NOPARM(dir, cmd, help, cb) \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, NULL)

#define MAKE_BDMF_SHELL_CMD(dir, cmd, help, cb, parms...)   \
{                                                           \
    static bdmfmon_cmd_parm_t cmd_parms[]={                 \
        parms,                                              \
        BDMFMON_PARM_LIST_TERMINATOR                        \
    };                                                      \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, cmd_parms); \
}
int ring_make_shell_commands ( void )
{
    bdmfmon_handle_t driver_dir, cpu_dir;

    if ( !( driver_dir = bdmfmon_dir_find ( NULL, "driver" ) ) )
    {
        driver_dir = bdmfmon_dir_add ( NULL, "driver", "Device Drivers", BDMF_ACCESS_ADMIN, NULL );

        if ( !driver_dir )
            return ( 1 );
    }

    cpu_dir = bdmfmon_dir_add ( driver_dir, "cpur", "CPU Ring Interface Driver", BDMF_ACCESS_GUEST, NULL );

    if ( !cpu_dir )
        return ( 1 );


    MAKE_BDMF_SHELL_CMD( cpu_dir, "sar",   "Show available rings", cpu_ring_shell_list_rings,
        					 BDMFMON_MAKE_PARM_RANGE( "ring_id", "ring id", BDMFMON_PARM_NUMBER, BDMFMON_PARM_FLAG_OPTIONAL, 0, D_NUM_OF_RING_DESCRIPTORS) );

    MAKE_BDMF_SHELL_CMD( cpu_dir, "vrpd",     "View Ring packet descriptor", cpu_ring_shell_print_pd,
    					 BDMFMON_MAKE_PARM_RANGE( "ring_id", "ring id", BDMFMON_PARM_NUMBER, 0, 0, D_NUM_OF_RING_DESCRIPTORS ),
                         BDMFMON_MAKE_PARM_RANGE( "descriptor", "packet descriptor index ", BDMFMON_PARM_NUMBER, 0, 0, RDPA_CPU_QUEUE_MAX_SIZE) );

    MAKE_BDMF_SHELL_CMD( cpu_dir, "cras",     "configure ring admin status", cpu_ring_shell_admin_ring,
        					 BDMFMON_MAKE_PARM_RANGE( "ring_id", "ring id", BDMFMON_PARM_NUMBER, 0, 0, D_NUM_OF_RING_DESCRIPTORS ),
                             BDMFMON_MAKE_PARM_RANGE( "admin", "ring admin status ", BDMFMON_PARM_NUMBER, 0, 0, 1) );

    return 0;
}
#endif /*__KERNEL__*/


static void *alignedAlloc(uint32_t size)
{
    void **ptr;
    uint32_t	cacheLine = DMA_CACHE_LINE;
    void *mem = (void*)CACHED_MALLOC(size + cacheLine + sizeof(void*) -1 );
    /*Invalidate this allocation*/
    FLUSH_RANGE((uint32_t)mem,size + cacheLine + sizeof(void*) -1);
    ptr = (void**)((long)(mem+cacheLine+sizeof(void*)) & ~(cacheLine-1));
    ptr[-1] = mem;
    return ptr;
}

static void alignedFree(void *ptr) {
	CACHED_FREE(((void**)ptr)[-1]);
}

/*delete a preallocated ring*/
int	rdp_cpu_ring_delete_ring(uint32_t ring_id)
{
	RING_DESCTIPTOR*			pDescriptor;
	uint32_t					entry;
	volatile RING_DESC_UNION*	pTravel;

	pDescriptor = &host_ring[ ring_id  ];
	if(!pDescriptor->num_of_entries)
	{
		printk("ERROR:deleting ring_id %d which does not exists!",ring_id);
		return -1;
	}

	/*first delete the ring from runner*/
	rdd_ring_destroy(ring_id);



	/*free the data buffers in ring */
	for (pTravel =(volatile RING_DESC_UNION*)pDescriptor->base, entry = 0 ; entry < pDescriptor->num_of_entries; pTravel++ ,entry++ )
	{
		if(pTravel->cpu_rx.host_buffer_data_pointer)
		{
			pTravel->cpu_rx.ownership = OWNERSHIP_HOST;
			rdp_databuf_free((void *)PHYS_TO_CACHED(pTravel->cpu_rx.host_buffer_data_pointer), 0);
			pTravel->cpu_rx.word2 = 0;
		}
	}

#ifndef _CFE_
	/* free any buffers in buff_cache */
	while(pDescriptor->buff_cache_cnt) 
	{
		rdp_databuf_free((void *)pDescriptor->buff_cache[--pDescriptor->buff_cache_cnt], 0);
	}

	/*free buff_cache */
	if(pDescriptor->buff_cache)
		CACHED_FREE(pDescriptor->buff_cache);
#endif
	/*delete the ring of descriptors*/
	if(pDescriptor->base)
		alignedFree((void*)NONCACHE_TO_CACHE(pDescriptor->base));

	pDescriptor->num_of_entries = 0;

	return 0;
}

int	rdp_cpu_ring_create_ring(uint32_t ring_id,uint32_t entries,cpu_ring_sysb_type buffType)
{
	RING_DESCTIPTOR*			pDescriptor;
	volatile RING_DESC_UNION*	pTravel;
	uint32_t					entry;
	void*					dataPtr 	= 0;

	if( ring_id > RDP_CPU_RING_MAX_QUEUES)
	{
		printk("ERROR: ring_id %d out of range(%d)",ring_id,RDP_CPU_RING_MAX_QUEUES);
		return -1;
	}

	pDescriptor = &host_ring[ ring_id ];
	if(pDescriptor->num_of_entries)
	{
		printk("ERROR: ring_id %d already exists! must be deleted first",ring_id);
		return -1;
	}

	if(!entries)
	{
		printk("ERROR: can't create ring with 0 packets\n");
		return -1;
	}

	printk("Creating CPU ring for queue number %d with %d packets descriptor=0x%p\n ",ring_id,entries,pDescriptor);

	/*set ring parameters*/
	pDescriptor->ring_id 			= ring_id;
	pDescriptor->admin_status		= 1;
	pDescriptor->num_of_entries		= entries;
	pDescriptor->packet_size 		= BCM_PKTBUF_SIZE;
	pDescriptor->size_of_entry		= sizeof(RING_DESC_UNION);
	pDescriptor->buff_type			= buffType;
	pDescriptor->buff_cache_cnt = 0;

#ifndef _CFE_
    /*TODO:update the comment  allocate buff_cache which helps to reduce the overhead of when
       * allocating data buffers to ring descriptor */
      pDescriptor->buff_cache = (uint32_t *)(CACHED_MALLOC(sizeof(uint32_t) * MAX_BUFS_IN_CACHE));
      if( pDescriptor->buff_cache == NULL )
      {
              printk("failed to allocate memory for cache of data buffers \n");
              return -1;
      }
#endif
    /*allocate ring descriptors - must be non-cacheable memory*/
        pDescriptor->base       =       (RING_DESC_UNION*)CACHE_TO_NONCACHE(alignedAlloc(sizeof(RING_DESC_UNION) * entries));
        if( pDescriptor->base == NULL)
        {
                printk("failed to allocate memory for ring descriptor\n");
                rdp_cpu_ring_delete_ring(ring_id);
                return -1;
        }




	/*initialize descriptors*/
	for (pTravel =(volatile RING_DESC_UNION*)pDescriptor->base, entry = 0 ; entry < entries; pTravel++ ,entry++ )
	{
		memset((void*)pTravel,0,sizeof(*pTravel));
		pTravel->cpu_rx.ownership 					= OWNERSHIP_RUNNER;

		/*allocate actual packet in DDR*/
		dataPtr = rdp_databuf_alloc(pDescriptor);
		if(dataPtr)
		{
			pTravel->cpu_rx.host_buffer_data_pointer    = VIRT_TO_PHYS(dataPtr);
		}
		else
		{
			pTravel->cpu_rx.host_buffer_data_pointer = (uint32_t)NULL;
			printk("failed to allocate packet map entry=%d\n",entry);
			rdp_cpu_ring_delete_ring(ring_id);
			return -1;
		}
	}

	/*set the ring header to the first entry*/
	pDescriptor->head = pDescriptor->base;

	/*using pointer arithmetics calculate the end of the ring*/
	pDescriptor->end  = pDescriptor->base + entries;

	/*call the runner ring initialization*/
	rdd_ring_init(ring_id, (uint32_t*)VIRT_TO_PHYS(pDescriptor->base), entries, sizeof(*pTravel), ring_id );
	printk("Done initializing Ring %d Base=0x%pK End=0x%pK calculated entries= %d RDD Base=0x%pK descriptor=0x%p\n",ring_id,
			pDescriptor->base,pDescriptor->end,( pDescriptor->end - pDescriptor->base)  ,(uint32_t*)VIRT_TO_PHYS(pDescriptor->base),pDescriptor);

#ifdef __KERNEL__
	if(!init_shell)
	{
		if(ring_make_shell_commands())
		{	printk("Failed to create ring bdmf shell commands\n");
			return 1;
		}

		init_shell = 1;
	}
#endif
	return 0;
}


/*this API copies the next available packet from ring to given pointer*/
int rdp_cpu_ring_read_packet_copy( uint32_t ring_id,CPU_RX_PARAMS* rxParams)
{
	RING_DESCTIPTOR* 				pDescriptor		= &host_ring[ ring_id ];
	volatile RING_DESC_UNION*			pTravel = (volatile RING_DESC_UNION*)pDescriptor->head;
	void* 	 					client_pdata;
	uint32_t 					ret = 0;


	client_pdata 		= (t_sysb_ptr)rxParams->data_ptr;
	ret = ReadPacketFromRing(pDescriptor,pTravel, rxParams);
	if ( ret )
		goto exit;
	/*copy the data to user buffer*/
	memcpy(client_pdata,(void*)rxParams->data_ptr,rxParams->packet_size);


	/*Assign the data buffer back to ring*/
	INV_RANGE((uint32_t)rxParams->data_ptr,rxParams->packet_size);
	AssignPacketBuffertoRing(pDescriptor,pTravel, rxParams->data_ptr);

exit:
	return ret;
}

/*this function if for debug purposes*/
int	rdp_cpu_ring_get_queue_size(uint32_t ring_id)
{
	return host_ring[ ring_id ].num_of_entries;
}


/*this function if for debug purposes and should not be called during runtime*/
/*TODO:Add mutex to protect when reading while packets read from another context*/
int	rdp_cpu_ring_get_queued(uint32_t ring_id)
{
	RING_DESCTIPTOR*			pDescriptor = &host_ring[ ring_id ];
	volatile RING_DESC_UNION*	pTravel		= pDescriptor->base;
	volatile RING_DESC_UNION*	pEnd		= pDescriptor->end;
	uint32_t					packets		= 0;

	if(!pDescriptor->num_of_entries)
		return 0;

	while (pTravel != pEnd)
	{
		packets += (pTravel->cpu_rx.ownership == OWNERSHIP_HOST) ? 1 : 0;
		pTravel++;
	}

	return packets;
}

int	rdp_cpu_ring_flush(uint32_t ring_id)
{
	RING_DESCTIPTOR*			pDescriptor = &host_ring[ ring_id ];
	volatile RING_DESC_UNION*	pTravel		= pDescriptor->base;
	volatile RING_DESC_UNION*	pEnd		= pDescriptor->end;
	uint32_t					rc			= 0;

	/*stop packets from runner to ring*/

	rc = rdd_ring_destroy(ring_id);
	if(rc)
		goto failed;

	while (pTravel != pEnd)
	{
		pTravel->cpu_rx.ownership = OWNERSHIP_RUNNER;
		pTravel++;
	}

	pDescriptor->head = pDescriptor->base;

	/*rearm runner ring*/
	rc = rdd_ring_init(ring_id, (uint32_t*)VIRT_TO_PHYS(pDescriptor->base), pDescriptor->num_of_entries, sizeof(*pTravel), ring_id );
	if(!rc)
		goto exit;

failed:
	printk("failed in rdd_ring %s line %d\n",__FUNCTION__,__LINE__);

exit:
	printk("cpu Ring %d has been flushed\n",ring_id);
	return rc;
}

int	rdp_cpu_ring_not_empty(uint32_t ring_id)
{
	RING_DESCTIPTOR*			pDescriptor = &host_ring[ ring_id ];
	RING_DESC_UNION*	pTravel		= (pDescriptor->head );

	return (pTravel->cpu_rx.ownership==OWNERSHIP_HOST);

}
