/*****************************************************************************/
/* Copyright (c) 2009 NXP B.V.                                               */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, using version 2 of the License.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307       */
/* USA.                                                                      */
/*                                                                           */
/*****************************************************************************/
#ifndef _TIMEDOCTOR_H
#define _TIMEDOCTOR_H

#include <linux/types.h>

/* These types describe the command to log */
#define TD_CMD_STATE            7
#define TD_CMD_VALUE            5
#define TD_CMD_DELETE           4
#define TD_CMD_CREATE           3
#define TD_CMD_GENERIC          2
#define TD_CMD_START            1
#define TD_CMD_STOP             0

/* These types describe the kind of object to log */
#define TD_TYPE_TASK            0
#define TD_TYPE_ISR             1
#define TD_TYPE_SEM             2
#define TD_TYPE_QUEUE           3
#define TD_TYPE_EVENT           4
#define TD_TYPE_VALUE_COUNT     5
#define TD_TYPE_AGENT           8
#define TD_TYPE_INVALID         15

#define TDI_NO_STOP_AFTER                ((unsigned int)-1)

#define TIMEDOCTOR_INFO_DATASIZE         (3)

#define TIMEDOCTOR_IOCTL_RESET           _IO( 'T', 0)
#define TIMEDOCTOR_IOCTL_START           _IO( 'T', 1)
#define TIMEDOCTOR_IOCTL_STOP            _IO( 'T', 2)
#define TIMEDOCTOR_IOCTL_GET_ENTRIES     _IO( 'T', 3)
#define TIMEDOCTOR_IOCTL_GET_MAX_ENTRIES _IO( 'T', 4)
#define TIMEDOCTOR_IOCTL_INFO            _IO( 'T', 5)


#define CONFIG_TIMEDOCTOR_TASKS
#define CONFIG_TIMEDOCTOR_INTERRUPTS


/* Bitmask filter of interrupts that should be filtered out */
extern uint32_t td_irq_filter[];


/* This function records an event in the event buffer. An event is composed of
 * a 32 bit time stamp and 3 data of 32 bits length.
 */
extern void timedoctor_info(uint32_t data1, uint32_t data2, uint32_t data3);

/* This function starts of stops a trace trace. */
extern int timedoctor_start_stop(int start);

/* This function stop a trace after n number of samples have been stored. */
extern int timedoctor_stop_after(unsigned int n);

/* This function reset the event buffer. */
extern int timedoctor_reset(void);

/* This function enables/disables buffer wrapping */
extern int timedoctor_wrap_ctrl(int enable);




/* This macro is used to build the first argument of the main log API in a way
 * that is compliant with time doctor,
 */
#define TDI_COMMAND(cmd, type) \
    (cmd << 24 | type << 16)

/* This macro is used to convert a 4 char array into a 32 bits word.
 * It is typically used to convert task names into a 32 bit word.
 */
#define CHAR_ARRAY_TO_U32(array) \
    ((unsigned int)((array[0]<<24)|(array[1]<<16)|(array[2]<<8)|array[3]))

#define CHAR2_ARRAY_TO_U32(array) \
    ((unsigned int)((array[4]<<24)|(array[5]<<16)|(array[6]<<8)|array[7]))

#define CHAR3_ARRAY_TO_U32(array) \
    ((unsigned int)((array[4]<<24)|(array[5]<<16)|(array[6]<<8)))





#if defined(CONFIG_TIMEDOCTOR_TASKS)

/* This macro is used to log the creation of a task. */
#define timeDoctor_task_create(tid, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_CREATE, TD_TYPE_TASK) | ((tid) & 0xffff), \
	                CHAR_ARRAY_TO_U32(name), CHAR2_ARRAY_TO_U32(name))

/* This macro is used to log the deletion of a task. */
#define timeDoctor_task_delete(tid, name) \
    timedoctor_info(TDI_COMMAND(TD_CMD_DELETE, TD_TYPE_TASK) | ((tid) & 0xffff), \
	                CHAR_ARRAY_TO_U32(name), CHAR2_ARRAY_TO_U32(name))

/* This macro is used to log the entry of a task. */
#define timeDoctor_task_enter(tid, name) \
    timedoctor_info(TDI_COMMAND(TD_CMD_START, TD_TYPE_TASK) | ((tid) & 0xffff), \
	                CHAR_ARRAY_TO_U32(name), CHAR2_ARRAY_TO_U32(name))

/* This macro is used to log the exit of a task. */
#define timeDoctor_task_exit(tid, name) \
    timedoctor_info(TDI_COMMAND(TD_CMD_STOP, TD_TYPE_TASK) | ((tid) & 0xffff), \
	                CHAR_ARRAY_TO_U32(name), CHAR2_ARRAY_TO_U32(name))

/* This macro is used to log the switch of tasks */
#define timeDoctor_task_switch(newtid, newname, oldtid, oldname) \
	do { \
		timeDoctor_task_exit(oldtid, oldname); \
		timeDoctor_task_enter(newtid, newname); \
	} while(0)

#else  /* !defined(CONFIG_TIMEDOCTOR_TASKS) */

#define timeDoctor_task_create(tid, name)
#define timeDoctor_task_delete(tid, name)
#define timeDoctor_task_enter(tid, name)
#define timeDoctor_task_exit(tid, name)
#define timeDoctor_task_switch(newtid, newname, oldtid, oldname)

#endif




#if defined(CONFIG_TIMEDOCTOR_INTERRUPTS)

/* This macro is used to log the start of an ISR */
#define timeDoctor_interrupt_enter(isr_id) \
	do { \
		if (!(td_irq_filter[((isr_id) >> 5)] & (1 << ((isr_id) & 0x1f)))) \
			timedoctor_info(TDI_COMMAND(TD_CMD_START, TD_TYPE_ISR) | (isr_id), \
			                0, 0); \
	} while(0)

/* This macro is used to log the exit of an ISR */
#define timeDoctor_interrupt_exit(isr_id) \
	do { \
		if(!(td_irq_filter[((isr_id) >> 5)] & (1 << ((isr_id) & 0x1f)))) \
			timedoctor_info(TDI_COMMAND(TD_CMD_STOP, TD_TYPE_ISR) | (isr_id), \
			                0, 0); \
    } while(0)

#else /* !defined(CONFIG_TIMEDOCTOR_INTERRUPTS) */

#define timeDoctor_interrupt_enter(isr_id)
#define timeDoctor_interrupt_exit(isr_id)

#endif




/* This macro is used to log the start of a user defined event */
#define timeDoctor_event_enter(evt_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_START, TD_TYPE_EVENT) | \
	                ((unsigned int)(evt_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))

/* This macro is used to log the exit of a user defined event */
#define timeDoctor_event_exit(evt_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_STOP, TD_TYPE_EVENT) | \
	                ((unsigned int)(evt_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))



/* This macro is used to log a single user defined event */
#define timeDoctor_event_single(evt_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_GENERIC, TD_TYPE_EVENT) | \
	                ((unsigned int)(evt_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))

/* This macro is used to log a single user defined event with a data value */
#define timeDoctor_event_single_data(evt_id, name, data) \
	timedoctor_info(TDI_COMMAND(TD_CMD_GENERIC, TD_TYPE_VALUE_COUNT) | \
	                ((unsigned int)(evt_id) & 0xffff), \
	                CHAR_ARRAY_TO_U32(name), data)

/* This macro is used to add a single event flag with a 32-bit value */
#define timeDoctor_event_value(evt_id, name, val) \
	timedoctor_info(TDI_COMMAND(TD_CMD_VALUE, TD_TYPE_VALUE_COUNT) | \
	                ((unsigned int)(evt_id) & 0xffff), \
	                CHAR_ARRAY_TO_U32(name), val)



/* This macro is used to log the start of an agent */
#define timeDoctor_agent_start(agent_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_START, TD_TYPE_AGENT) | \
	                ((unsigned int)(agent_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))

/* This macro is used to log the end of an agent */
#define timeDoctor_agent_stop(agent_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_STOP, TD_TYPE_AGENT) | \
	                ((unsigned int)(agent_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))



/* This macro is used to increment the count for a given queue */
#define timeDoctor_event_queue_add(queue_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_START, TD_TYPE_QUEUE) | \
	                ((unsigned int)(queue_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))

/* This macro is used to decrement the count for a given queue */
#define timeDoctor_event_queue_del(queue_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_STOP, TD_TYPE_QUEUE) | \
	                ((unsigned int)(queue_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))



/* This macro is used to log a semaphore acquire event */
#define timeDoctor_semaphore_acquire(sem_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_STOP, TD_TYPE_SEM) | \
	                ((unsigned int)(sem_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))

/* This macro is used to log the release of a semaphore */
#define timeDoctor_semaphore_release(sem_id, name) \
	timedoctor_info(TDI_COMMAND(TD_CMD_START, TD_TYPE_SEM) | \
	                ((unsigned int)(sem_id) & 0xffff), \
	                CHAR2_ARRAY_TO_U32(name), CHAR_ARRAY_TO_U32(name))


#endif /* _TIMEDOCTOR_H */
