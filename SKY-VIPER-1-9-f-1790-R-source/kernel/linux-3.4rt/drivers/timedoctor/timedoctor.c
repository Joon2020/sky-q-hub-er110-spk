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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/hash.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/timedoctor.h> 
#include <asm/uaccess.h>
#if defined(CONFIG_MIPS_BRCM)
#include <asm/time.h>
#endif




/* The strings describing the name of the module */
#define TIME_DOCTOR_DESCRIPTION "Time doctor event logger"
#define TIME_DOCTOR_DEVNAME     "timedoctor"
#define CONFIG_TIMEDOCTOR_DEFAULT_BUFSIZE 524288

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION(TIME_DOCTOR_DESCRIPTION);
MODULE_AUTHOR("NXP B.V.");


/* Legacy module parameter, perserved for backwards compatibility */
static int debug_level = 0;
module_param(debug_level, int, 0);
MODULE_PARM_DESC(debug_level, "Debug level (0-1)");

/* If this parameter is set the timedoctor buffer is allocated up front */
static int allocate = 1;
module_param(allocate, int, 0);
MODULE_PARM_DESC(allocate, "Allocate buffer up front (0-1)");

/* This parameter specifies the number of records in the buffer */
static int buffer_count = CONFIG_TIMEDOCTOR_DEFAULT_BUFSIZE;
module_param(buffer_count, int, 0);
MODULE_PARM_DESC(buffer_count, "Number of samples to buffer");




/* printk prefixes */
#define TIME_DOCTOR_PRINT       "[TimeDoctor] "
#define TIME_DOCTOR_ERR         KERN_ERR TIME_DOCTOR_PRINT
#define TIME_DOCTOR_NOTICE      KERN_ERR TIME_DOCTOR_PRINT
#define TIME_DOCTOR_WARNING     KERN_ERR TIME_DOCTOR_PRINT


extern unsigned int mips_hpt_frequency;
extern unsigned int mips_hpt_read(void);

/* If on a Broadcom MIPS platform we can use the MIPS high performance timer
 *   Sample clock freq    : mips_hpt_frequency
 *   Timer read function  : read using mips_hpt_read() func
 *   Tick IRQ number(s)   : BCM7335=97, BCM7401=67,69 BCM7340=97
 */



#define TD_SAMPLE_CLK_FREQ        mips_hpt_frequency
//#define TD_READ_SAMPLE_CLK()      mips_hpt_read()

#define TD_DEFAULT_IRQ_FILTER   { 0x00000000, 0x00000000, 0x00000000, 0x00000000 }


/* This is the number of bits for the hash table (i.e. 64 slots) */ 
#define TD_HASH_TBL_BITS       (6)

/* Macros to convert from 32-bit integer to 4 character string */
#define TD_INT32_TO_CHAR(i, c) \
	do {	(c)[0] = (((i) >> 24) & 0xff); \
			(c)[1] = (((i) >> 16) & 0xff); \
			(c)[2] = (((i) >> 8)  & 0xff); \
			(c)[3] = (((i) >> 0)  & 0xff); \
			(c)[4] = '\0'; \
	} while(0)

#define TD_TWO_INT32_TO_CHAR(i1, i2, c) \
	do { \
		TD_INT32_TO_CHAR((i1), &((c)[0])); \
		TD_INT32_TO_CHAR((i2), &((c)[4])); \
	} while(0)


/* The number of 32-bit words per timedoctor sample/record */
#define TD_WORDS_PER_RECORD (4)

/* Individual record consists of 4 32-bit values */
typedef uint32_t td_record[TD_WORDS_PER_RECORD];

/* A sample header stored in the hash table */
struct td_header {
	uint8_t  type;
	uint16_t id;
	uint32_t unique_id;
	char     name[12];
	uint32_t prev_value;
	
	int      written_name;
	
	struct hlist_node hlist;
};


/* This array stores records, each of which is 4 words */
static td_record *td_data;

/* Index pointing to the next available word in the array */
static unsigned int td_cur_index = 0;

/* The total number of entries that can be added to the array */
static unsigned int td_num_entries = 0;

/* Indicates whether we've wrapped around our buffer. */
static unsigned int td_has_wrapped = 0;

/* Once this time is reached timedoctor stops automatically */
static unsigned int td_stop_after = TDI_NO_STOP_AFTER;

/* Lock to protect access to the timedoctor state */ 
static struct mutex td_state_lock;

/* Lock to protect access to the timedoctor sample buffer */ 
static spinlock_t td_data_lock;

/* Indicates whether we're allowed to wrap around our buffer. Default value is on. */
static int td_wrap_allowed = 1;

/* Bitfield indicating the interrupts that should be filtered out. Typically
 * used to filter out the timer ticks.
 */
uint32_t td_irq_filter[((NR_IRQS + 0x1F) >> 5)] = TD_DEFAULT_IRQ_FILTER;


/* State transistions */
static enum _timedoctor_state {
	unallocated = 0,   /* no buffer allocated, initial state */
	stopped,           /* stopped recording */
	running,           /* currently recording */
	reading,           /* reading from the buffer, not recording */
	invalid,
} td_state = unallocated;

static const char * td_state_strings[] = {
	[unallocated] = "unallocated",
	[stopped] = "stopped",
	[running] = "running",
	[reading] = "reading",
	[invalid] = "invalid"
};


/* Context structure passed around when generating event strings */
struct timedoctor_data_ctx {
	int first;
	struct hlist_head *hash_tbl;
	uint32_t unadj_prev_time;
	uint32_t adj_prev_time;
	int32_t  adj_time_base;
	int32_t time_scalar;
};


/* The following arrays describe the order of name characters in the records */
#if defined(__LITTLE_ENDIAN)
static const int td_task_name_order[]    = { 3, 2, 1, 0, 7, 6, 5, 4, -1 };
static const int td_nontask_name_order[] = { 7, 6, 5, 4, 3, 2, 1, 0, -1 };
static const int td_value_name_order[]   = { 3, 2, 1, 0, -1 };
#elif defined(__BIG_ENDIAN)
static const int td_task_name_order[]    = { 0, 1, 2, 3, 4, 5, 6, 7, -1 };
static const int td_nontask_name_order[] = { 4, 5, 6, 7, 0, 1, 2, 3, -1 };
static const int td_value_name_order[]   = { 0, 1, 2, 3, -1 };
#else
#  error __LITTLE_ENDIAN or __BIG_ENDIAN must be defined
#endif


/* Declare funcions needed by the structures below */
static int timedoctor_buf_alloc(unsigned long nrecs);
static void timedoctor_buf_free(void);

static int __init timedoctor_init(void);
static void timedoctor_exit(void);



/* Path to a userspace application to call when the state of timedoctor changes */
static char td_usermode_helper[256];
#if 0

/* Function called in a workqueue to call the usermode helper */
static void timedoctor_helper(void *arg);
static DECLARE_WORK(td_helper_work, timedoctor_helper);




/**
 *	timedoctor_helper - Workqueue function called on a state change
 *	@work: pointer to the work structure
 *	
 *	Called by the kernel workqueue, scheduled when there is a state change
 *	in the driver.
 */
static void timedoctor_helper(void *arg)
{
	static enum _timedoctor_state prev_state = invalid;
	enum _timedoctor_state new_state;
	char str[16];
	char *envp[8];
	char *argv[8];

	/* Get the current state */
	mutex_lock(&td_state_lock);
	new_state = td_state;
	mutex_unlock(&td_state_lock);

	/* If changed called the usermode helper */
	if ((new_state != prev_state) && (td_usermode_helper[0] != '\0')) {

		snprintf(str, 16, "%d", new_state);

		envp[0] = "HOME=/";
		envp[1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
		envp[2] = NULL;

		argv[0] = td_usermode_helper;
		argv[1] = str;
		argv[2] = NULL;

		call_usermodehelper_keys(argv[0], argv, envp, NULL, 0);
	}

	/* Store the previous state we indicated */
	prev_state = new_state;
}


/**
 *	timedoctor_helper_write_proc - Called when user writes to proc entry
 *	@file: ignored
 *	@buffer: buffer containing the data written by the user
 *	@count: number of bytes in buffer
 *	@data: context data (ignored)
 *
 *	This function is called when a user writes to the 'timedoctor-helper'
 *	/proc file.
 *	
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	The number of bytes consumed on success, on failure a negative error code
 *	is returned.
 */
static int timedoctor_helper_write_proc(struct file *file,
                                        const char __user *buffer,
                                        unsigned long count, void *data)
{
	int len;

	/* Sanity check */
	if (!count) {
		memset(td_usermode_helper, 0x00, sizeof(td_usermode_helper));
		return 0;
	}

 	if (count > sizeof(td_usermode_helper)) {
 		printk(TIME_DOCTOR_ERR "Invalid /proc/timedoctor-helper string "
		                       "(string to large)\n");
		return -EFAULT;
	}

	/* Copy the command string*/
	if (copy_from_user(td_usermode_helper, buffer, count))
		return -EFAULT;
	td_usermode_helper[sizeof(td_usermode_helper) - 1] = '\0';

	/* Strip any trailing newlines */
	len = strlen(td_usermode_helper);
	if (len && (td_usermode_helper[len - 1] == '\n'))
		td_usermode_helper[len - 1] = '\0';
	
	return count;
}


/**
 *	timedoctor_helper_read_proc - Returns the string previous written
 *	@page: a buffer of PAGE_SIZE used to store the output string
 *	@start: ignored
 *	@off: ignored (assumed always read from the start)
 *	@count: ignored
 *	@eof: ignored
 *	@data: context data (ignored)
 *	
 *	This function simply prints the contents of the td_usermode_helper string.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	The number of bytes written into the page buffer.
 */
static int timedoctor_helper_read_proc(char *page, char **start, off_t off,
                                       int count, int *eof, void *data)
{
	return scnprintf(page, count, "%s\n", td_usermode_helper);
}


/**
 *	timedoctor_irq_filter_write_proc - Called when user writes to proc entry
 *	@file: ignored
 *	@buffer: buffer containing the data written by the user
 *	@count: number of bytes in buffer
 *	@data: context data (ignored)
 *
 *	This function is called when a user writes to the 'timedoctor_irq_filter'
 *	/proc file.  Here we interpret the strings to either add or remove an IRQs
 *	from the filter bitmask.
 *
 *	LOCKING:
 *	None. The data spinlock is held while modifying the IRQ filter mask.
 *
 *	RETURNS:
 *	The number of bytes consumed on success, on failure a negative error code
 *	is returned.
 */
static int timedoctor_irq_filter_write_proc(struct file *file,
                                            const char __user *buffer,
                                            unsigned long count, void *data)
{
	char command[32];
	char *strp;
	unsigned int irq;
	int add_irq = 1;
	unsigned long flags;

	/* Sanity check */
	if (!count)
		return 0;
	else if (count > 32) {
		printk(TIME_DOCTOR_ERR "Invalid /proc/timedoctor-irq-filter string "
		                       "(string to large)\n");
		return -EFAULT;
	}

	/* Copy the command string*/
	if (copy_from_user(&command, buffer, count))
		return -EFAULT;


	/* Process the command, the format is simply either a '+' or '-' followed
	 * by the number of the interrupt in question.
	 */
	if (command[0] == '+')
		strp = &command[1];
	else if (isdigit(command[0]))
		strp = &command[0];
	else if (command[0] == '-') {
		strp = &command[1];
		add_irq = 0;
	} else {
		printk(TIME_DOCTOR_ERR "Invalid /proc/timedoctor-irq-filter string "
		                       "(incorrectly formatted)\n");
		return -EFAULT;
	}
	
	/* Get the interrupt number */
	irq = simple_strtoul(strp, NULL, 0);
	if (irq >= NR_IRQS) {
		printk(TIME_DOCTOR_ERR "Invalid /proc/timedoctor-irq-filter string "
		                       "(irq number to large)\n");
		return -EFAULT;
	}
	
	/* Update the irq filter (there should be a unique spinlock for this) */
	spin_lock_irqsave(&td_data_lock, flags);
	
	if (add_irq)
		td_irq_filter[irq >> 5] |= (1 << (irq & 0x1F));
	else
		td_irq_filter[irq >> 5] &= ~(1 << (irq & 0x1F));
	
	spin_unlock_irqrestore(&td_data_lock, flags);
	
	return count;
}


/**
 *	timedoctor_irq_filter_read_proc - Returns a string listing filtered irqs
 *	@page: a buffer of PAGE_SIZE used to store the output string
 *	@start: ignored
 *	@off: ignored (assumed always read from the start)
 *	@count: ignored
 *	@eof: ignored
 *	@data: context data (ignored)
 *
 *	This function simply prints the IRQs that are currently filtered out by
 *	timedoctor.  Each IRQ is listed on it's own line with the name (if
 *	available) next to it.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	The number of bytes written into the page buffer.
 */
static int timedoctor_irq_filter_read_proc(char *page, char **start, off_t off,
                                           int count, int *eof, void *data)
{
	unsigned int i;
	unsigned int nnames;
	int len = 0;
	struct irq_desc *desc;
	struct irqaction *action;

	/* Loop through printing all the interrupts that are filtered */
	for (i = 0; i < (NR_IRQS); i++) {
		if (td_irq_filter[(i >> 5)] & (1 << (i & 0x1F))) {
		
			/* Print the number of the interrupt */
			len += scnprintf(page + len, PAGE_SIZE - len, "%3u  - ", i);
			
			/* Print the name of the interrupt */
			nnames = 0;
			desc = &(irq_desc[i]);
			for (action = desc->action; action; action = action->next) {
				if (action->name) {
					if (nnames++ > 0)
						len += scnprintf(page + len, PAGE_SIZE - len, ", ");
					len += scnprintf(page + len, PAGE_SIZE - len, "%s", action->name);
				}
			}
			
			len += scnprintf(page + len, PAGE_SIZE - len, "\n");
		}
	}

	/* If no interrupts are filter print out a message */
	if (len == 0)
		len += scnprintf(page + len, PAGE_SIZE - len, "No IRQs filtered\n");

	return len;
}



/**
 *	timedoctor_create_unique_id - Generates a unique id from a data entry
 *	@rec: record to calculate the unique ID of
 *
 *	This function accepts a record and calculates a unique ID by using the
 *	record's ID and type plus optionally a hash of the record name string.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	A signed 32-bit value that is the unique id of the record.
 */
#endif 

static inline int32_t timedoctor_create_unique_id(uint32_t *rec)
{
	unsigned int type = (rec[1] & 0x00ff0000) >> 16;
	unsigned int id   = (rec[1] & 0x0000ffff);
	unsigned int hash;

	/* For tasks we use the PID/TID value as the unique ID, this is because the
	 * name of a task can be changed and we don't want this to end up on
	 * another line.
	 */
	if (type == TD_TYPE_VALUE_COUNT)
		hash = hash_long(rec[2], 11);
	else if ((type == TD_TYPE_TASK) || (type == TD_TYPE_ISR))
		hash = 0;
	else {
		hash  = hash_long(rec[2], 5) << 6;
		hash |= hash_long(rec[3], 6);
	}

	/* The unique ID is made up of 4 bits of the type, 16 bits of the id and
	 * 11 bits of the hash.
	 */
	return (int32_t)(((type & 0xf) << 27) | (id << 11) | hash);
}


/**
 *	timedoctor_fixup_name -  Checks and fixes up the name of an entry
 *	@rec: record to check the name of
 *
 *	Because the name string of a event is usually generated by dumb macros that
 *	convert a string to a 32-bit value, the record may have names that appear
 *	managed when shown in the GUI.  Typical things that cause problems are
 *	newlines or linefeeds in the string, or names consisting of only a one or
 *	characters and the rest field with random bytes (which can break the hash
 *	table as that will typically use the entire name buffer to create a hash).
 *
 *	This function attempts to fix up the name strings by:
 *	  1. Replacing non-printable characters before the first valid character
 *	     with '_' characters.
 *	  2. Replacing non-printable characters after the first valid character
 *	     with '\0' characters.
 *	     
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Nothing.
 */
static inline void timedoctor_fixup_name(uint32_t *rec)
{
	char *name = NULL;
	const int *order;
	unsigned int type = ((rec[1] & 0x00ff0000) >> 16);

	/* The length and layout of the name is determined by the type of the
	 * record.
	 */
	switch (type) {
		case TD_TYPE_TASK:
			name = (char*)&(rec[2]);
			order = td_task_name_order;
			break;
		case TD_TYPE_EVENT:
		case TD_TYPE_SEM:
		case TD_TYPE_QUEUE:
		case TD_TYPE_AGENT:
			name = (char*)&(rec[2]);
			order = td_nontask_name_order;
			break;
		case TD_TYPE_VALUE_COUNT:
			name = (char*)&(rec[2]);
			order = td_value_name_order;
			break;
		default:
			return;
	}

	/* If any null bytes exist before the valid values replace with
	 * underscores.
	 */
	for (; *order != -1; order++) {
		if (!isprint(name[*order]))
			name[*order] = '_';
		else
			break;
	}
	
	/* Skip over the valid characters */
	for (; *order != -1; order++) {
		if (!isprint(name[*order]))
			break;
	}
		
	/* Fill the remainder with nulls */
	for (; *order != -1; order++)
		name[*order] = '\0';
}



/**
 *	timedoctor_alloc_hash_table - Creates a new empty hash table
 *
 *	Allocates memory for a new empty has table and returns a pointer to it.
 *	The hash table is used to store unique record types. A record type is
 *	determined by it's; ID, TYPE and NAME fields, or more precisely by it's
 *	unique ID which is calculated from the above fields in the
 *	timedoctor_create_unique_id() function.
 *	
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	A pointer to the newly created hash table, or NULL on failure.
 */
static struct hlist_head* timedoctor_alloc_hash_table(void)
{
	struct hlist_head *hash_tbl;
	unsigned int i;
	unsigned int nr = (1 << TD_HASH_TBL_BITS);

	/* Allocate memory for the table part */
	hash_tbl = (struct hlist_head*) kmalloc((sizeof(struct hlist_head) * nr),
	                                        GFP_KERNEL);
	if (hash_tbl == NULL) {
		printk(TIME_DOCTOR_ERR "Error allocating memory for hash table\n");
		return NULL;
	}
	
	/* Initialise all the entries */
	for (i = 0; i < nr; i++)
		INIT_HLIST_HEAD(&hash_tbl[i]);
	
	return hash_tbl;
}


/**
 *	timedoctor_free_hash_table - Frees memory allocated for a hash table
 *	@hash_tbl: Pointer to the hash table created by timedoctor_alloc_hash_table 
 *	
 *	Frees all memory allocated for the hash table including table entries
 *	inserted by timedoctor_lookup_hash_entry().
 *	
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Nothing.
 */
static void timedoctor_free_hash_table(struct hlist_head *hash_tbl)
{
	unsigned int i;
	unsigned int nr = (1 << TD_HASH_TBL_BITS);
	struct hlist_head *head;
	struct hlist_node *node, *tmp;
	struct td_header *header = NULL;

	/* Loop over all the top level hash table entries and remove all the entries
	 * in their lists.
	 */
	for (i = 0; i < nr; i++) {
		
		/* Get the head of the hash list for this entry */
		head = &hash_tbl[i];
		
		/* Check if the hash table entry contains at least one list entry */
		if (!hlist_empty(head)) {
			hlist_for_each_entry_safe(header, node, tmp, head, hlist) {
				/* Remove the entry from the list and free the memory */
				hlist_del(&header->hlist);
				kfree(header);
			}
		}

	}

	/* Finally free the table of hash lists */
	kfree(hash_tbl);
}


/**
 *	timedoctor_lookup_hash_entry - Looks up a record in the has table, creates
 *	                               an entry if it doesn't exist
 *	@hash_tbl: Pointer to the hash table created by timedoctor_alloc_hash_table
 *	@rec: The record to search for in the hash table
 *
 *	This function will add a new header entry to the hash table if one doesn't
 *	already exist and returns a pointer to the new entry. If an entry does exist
 *	it simply returns a pointer to the entry.
 *	
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	A pointer to the entry in the has table.
 */
static struct td_header *timedoctor_lookup_hash_entry(
	struct hlist_head *hash_tbl, uint32_t *rec)
{
	int32_t unique_id;
	unsigned int hash;
	struct hlist_head *head;
	struct hlist_node *node;
	struct td_header *header = NULL;
	
	
	/* Generate the unique ID for the entry and also a hash value for table
	 * lookup.  Note: the unique should be unique but it's 31 bits in size,
	 * meaning we need to take the hash of that to generate an index into the
	 * table.
	 *
	 * TODO: This is a bit inefficient as we could possible just use a limited
	 * number of bits from the unique_id as the hash lookup. But currently the
	 * unique ID is a bitfield, so would need to randomise the bits for this
	 * to work.
	 */
	unique_id = timedoctor_create_unique_id(rec);
	hash = hash_long(unique_id, TD_HASH_TBL_BITS);

	/* Get the head of the hash list for this entry and search for a match */
	head = &(hash_tbl[hash]);
	if (!hlist_empty(head)) {
		hlist_for_each_entry(header, node, head, hlist) {
			if (header->unique_id == unique_id) {
				return header;
			}
		}
	}
	
	
	/* Haven't found a matching entry so add a new one */
	header = (struct td_header*) kmalloc(sizeof(struct td_header), GFP_KERNEL);
	if (header == NULL)
		return NULL;
	
	/* Initialise the fields for the header */
	header->unique_id = unique_id;
	header->type = (rec[1] & 0x00ff0000) >> 16;
	header->id = (rec[1] & 0x0000ffff);
	
	header->written_name = 0;

	switch (header->type) {
		case TD_TYPE_TASK:
			TD_TWO_INT32_TO_CHAR(rec[2], rec[3], header->name);
			header->name[8] = '\0';
			break;
		case TD_TYPE_SEM:
		case TD_TYPE_QUEUE:
		case TD_TYPE_EVENT:
		case TD_TYPE_AGENT:
			TD_TWO_INT32_TO_CHAR(rec[3], rec[2], header->name);
			header->name[8] = '\0';
			break;
		case TD_TYPE_ISR:
			snprintf(header->name, 8, "%d", header->id);
			break;
		case TD_TYPE_VALUE_COUNT:
			/* For value types we need to set the initial value ... we use queue
			 * types to get around the fact the GUI doesn't correctly display
			 * value primatives.
			 */
			header->type = TD_TYPE_QUEUE;
			TD_INT32_TO_CHAR(rec[2], header->name);
			header->name[4] = '\0';
			header->prev_value = rec[3];
			break;
		default:
			header->type = TD_TYPE_INVALID;
			break;
	}
	
	/* Add the node to the hash list entry */
	INIT_HLIST_NODE(&header->hlist);
	hlist_add_head(&header->hlist, head);
	
	return header;
}


/**
 *	timedoctor_data_seq_start -  Start a seq read, convert initial record to pointer
 *	@s: The sequence file
 *	@pos: Upon entry contains the current logical position.
 *
 *	This function is called the first time an item is read from the sequence
 *	file. The function simply returns a pointer to the record at the start
 *	of the sequence.
 *	
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Pointer to the data at pos, or NULL if last entry.
 */
static void *timedoctor_data_seq_start(struct seq_file *s, loff_t *pos)
{
	loff_t local_pos = *pos;
	unsigned int data_limit = td_cur_index;

	if (td_has_wrapped) {
		local_pos = ((unsigned int)(td_cur_index + (*pos))) % td_num_entries;
		data_limit = td_num_entries;
	}

	if ((*pos) >= data_limit)
		return NULL;
	
	return &(td_data[local_pos]);
}

/**
 *	timedoctor_data_seq_next - Increment to next record and convert to pointer 
 *	@s: the sequence file
 *	@v: the previous value returned by this function (or timedoctor_seq_start)
 *	@pos: pointer to the current index, should be updated on return with the
 *	      new position
 *
 *	This function is called for every entry in the timedoctor buffer, it them
 *	formats that into a text line that can be loaded into the timdoctor GUI.
 *	
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Pointer to the data at pos, or NULL if last entry.
 */
static void *timedoctor_data_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	loff_t local_pos;
	unsigned int data_limit = td_cur_index;

	(*pos)++;
	local_pos = *pos;

	if (td_has_wrapped) {
		local_pos = ((unsigned int)(td_cur_index + (*pos))) % td_num_entries;
		data_limit = td_num_entries;
	}
	
	if ((*pos) >= data_limit)
		return NULL;
		
	return &(td_data[local_pos]);
}

/**
 *	timedoctor_data_seq_stop - Does nothing
 *
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Nothing.
 */
static void timedoctor_data_seq_stop(struct seq_file *f, void *v)
{
	/* Nothing to do */
}


/**
 *	timedoctor_data_seq_show - writes a single entry in the sequence file
 *	@s: the sequence file
 *	@v: pointer to the record to display
 *
 *	This function is called for every entry in the timedoctor buffer, it them
 *	formats that into a text line that can be loaded into the timdoctor GUI.
 *	
 *	Before converting the record into a text string the function tries to
 *	normalise the time values to account for timer wraps, it will also try
 *	to fix-up event name strings and insert record headers if needed.
 *
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure.
 */

static int timedoctor_data_seq_show(struct seq_file *s, void *v)
{
	struct timedoctor_data_ctx *ctx = (struct timedoctor_data_ctx*)s->private;
	uint32_t *rec = (uint32_t*)v;
	struct td_header *header = NULL;
	int err = 0;
	unsigned int cmd;
	uint32_t adj_time;
	uint32_t adj_diff;
	unsigned int pid;	

	/* Important:  Something I missed when I first wrote this function is that
	 * if seq_printf() fails the seq_next function won't be called and instead
	 * this function will be called with the same record again. Therefore we
	 * can't assume this function will be called once per record and the stored
	 * state information should only be updated if the seq_printf call
	 * succeeds.
	 *
	 * Note: There is a bigger problem with this implementation in that it
	 * doesn't support random seeks, e.g. the file has to be read from start
	 * to finish.  This is a known limitation, however given that currently all
	 * users only ever call 'cat' to read the file, this currently isn't a
	 * problem.
	 */

	/* If the first entry then we need to write the file header */
	if (unlikely(ctx->first)) {
		err += seq_printf(s, "CPU trace\n");
		err += seq_printf(s, "SPEED %d\n",
		                  (TD_SAMPLE_CLK_FREQ / ctx->time_scalar));
		err += seq_printf(s, "TIME %d\n",
		                  (TD_SAMPLE_CLK_FREQ / ctx->time_scalar));
		err += seq_printf(s, "MEMSPEED 200000000\n");

		/* If for any reason this failed we should give up */
		if (unlikely(err < 0))
			return -EINVAL;
	}
	

	/* Get the unique fields of the record */
	adj_time = rec[0] / ctx->time_scalar;
	cmd = (rec[1] & 0xff000000) >> 24;
	pid = (rec[1] & 0x0000ffff);	
	
	/* Next we need to normalise the record */
	if (unlikely(ctx->first)) {
		/* Get the un-adjusted time */
		ctx->unadj_prev_time = rec[0];
	
		/* Store the current adjusted time as the negative time base */
		ctx->adj_time_base = -(int32_t)adj_time;
		
		/* Set the adjust time to zero as this is the normalised first record */
		adj_time = 0;
		
		/* Clear the 'first' flag */
		ctx->first = 0;

	} else {
		/* Check if the time has wrapped */
		if (unlikely(ctx->unadj_prev_time > rec[0])) {
			/* Calculate the adjusted time between the previous entry and wrap */
			adj_diff = (~ctx->unadj_prev_time) / ctx->time_scalar;

			/* Store the unadjusted time for next loop */
			ctx->unadj_prev_time = rec[0];

			/* Add an entry to mark the timer wrap */
			err += seq_printf(s, "NAM 7 %u %s\n", 0x7fffffff, "TimeWrap");
			err += seq_printf(s, "OCC 7 %u %d\n", 0x7fffffff, ctx->adj_prev_time);

			/* Set the new adjustment value which is the time of the wrap */
			ctx->adj_time_base = (int32_t)(ctx->adj_prev_time + adj_diff);
		}
		/* Not a time wrap so just store the previous un-adjusted value */
		else {
			ctx->unadj_prev_time = rec[0];
		}
		
		/* Adjust the time value and normalise it */
		if (ctx->adj_time_base < 0)
			adj_time -= (uint32_t)(-ctx->adj_time_base);
		else
			adj_time += (uint32_t)ctx->adj_time_base;
	}

	/* Store the adjusted time */
	ctx->adj_prev_time = adj_time;


	/* Fixup the name field in the record if needed */
	timedoctor_fixup_name(rec);


	/* Lookup the record in the hash table */
	header = timedoctor_lookup_hash_entry(ctx->hash_tbl, rec);
	if (header == NULL) {
		printk(TIME_DOCTOR_WARNING "Hash lookup/create failed\n");
		return 0;
	}
	

	/* If it's a create command this must be before the name part */
	if (cmd == TD_CMD_CREATE)
		err += seq_printf(s, "CRE %u %u %u\n", header->type, header->unique_id,
		                  adj_time);


	/* Check for any write errors and get out early */
	if (unlikely(err < 0))
		return -EINVAL;


	/* If a new header was created we need to write that header into the output
	 * file as well.
	 */
	if (unlikely(header->written_name == 0)) {
		if (cmd == TD_CMD_GENERIC)
			err += seq_printf(s, "NAM 7 %u %s-%u\n", header->unique_id, header->name,pid);
		else
			err += seq_printf(s, "NAM %u %u %s-%u\n", header->type,
			                  header->unique_id, header->name,pid);
		
		/* For value types we need to set an initial start value (i.e. 0) */
		if ((err == 0) && (cmd == TD_CMD_VALUE)) {
			header->prev_value = rec[3];
			err += seq_printf(s, "STA %u %u 0 %u\n", header->type,
			                  header->unique_id, header->prev_value);
		}
		
		/* If the name was written out without issue then set the written_name
		 * flag.  If we failed to write then we might as well get out of here
		 * now.
		 */
		if (err == 0)
			header->written_name = 1;
		else
			return -EINVAL;
			
	}
	
	/* Finally write the actual record entry */
	switch (cmd) {
		case TD_CMD_START:
			err += seq_printf(s, "STA %u %u %u\n", header->type,
			                  header->unique_id, adj_time);
			break;
		case TD_CMD_STOP:
			err += seq_printf(s, "STO %u %u %u\n", header->type,
			                  header->unique_id, adj_time);
			break;
		case TD_CMD_CREATE:
			if (header->type == TD_TYPE_TASK)
				err += seq_printf(s, "STA %u %u %u\n", header->type,
				                  header->unique_id, adj_time);
			break;
		case TD_CMD_DELETE:
			if (header->type == TD_TYPE_TASK)
				err += seq_printf(s, "STO %u %u %u\n", header->type,
				                  header->unique_id, adj_time);
			err += seq_printf(s, "DEL %u %u %u\n", header->type,
			                  header->unique_id, adj_time);
			break;
		case TD_CMD_GENERIC:
			err += seq_printf(s, "OCC 7 %u %u\n", header->unique_id, adj_time);
			if ((err == 0) && ((header->type == TD_TYPE_VALUE_COUNT) || 
			                   (header->type == TD_TYPE_QUEUE)))
				err += seq_printf(s, "DSC 0 %u %s:%u(0x%x)\n", header->unique_id,
				                  header->name, rec[3], rec[3]);
			break;
			
		case TD_CMD_VALUE:
			/* For value types we cheat and use the GUI's ability to display
			 * queue values with increment/decrement by different amounts.
			 */
			if (rec[3] < header->prev_value)
				err += seq_printf(s, "STO %u %u %u %u\n", header->type,
				                  header->unique_id, adj_time,
				                  (header->prev_value - rec[3]));
			else if (rec[3] > header->prev_value)
				err += seq_printf(s, "STA %u %u %u %u\n", header->type,
				                  header->unique_id, adj_time,
				                  (rec[3] - header->prev_value));
			
			if ((err == 0) && (rec[3] != header->prev_value)) {
				/* To help reading values add a description tag */
				err += seq_printf(s, "DSC 0 %u Before=%u:After=%u\n",
				                  header->unique_id, header->prev_value, rec[3]);

				/* Save the previous value */
				header->prev_value = rec[3];
			}
			break;
	}
	
	return (err < 0) ? -EINVAL : 0;
}

/* Operations for reading /proc/timedoctor one record at a time */
static struct seq_operations timedoctor_data_seq_ops = {
	.start = timedoctor_data_seq_start,
	.next  = timedoctor_data_seq_next,
	.stop  = timedoctor_data_seq_stop,
	.show  = timedoctor_data_seq_show
};


/**
 *	timedoctor_calc_time_scalar - calculates the bit scalar for time samples
 *	                              to avoid 32-bit wrap.
 *
 *	This function simply calculates the number of times a wrap is detected in
 *	the log buffer.  It then creates a scale factor based on this, which
 *	should ensure that no wrap is present in the output trace.
 *
 *	Essentially if there were 6 timer wraps then the scalar would be 7
 *	multiplied by 2. 7 because this is the maximum number of timer cycles and
 *	multiplied by 2 because the time needs to be a signed 32-bit value.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	The number of bits to right shift the time value.
 */
static unsigned int timedoctor_calc_time_scalar(unsigned long start_idx,
                                                unsigned long end_idx)
{
	unsigned long idx = start_idx;
	unsigned long nwraps = 0;
	uint32_t cur_time, prev_time;
	
	prev_time = td_data[idx][0];

	while (idx != end_idx) {

		cur_time = td_data[idx][0];

		/* Check for a wrap */
		if (unlikely(prev_time > cur_time))
			nwraps++;
		
		prev_time = cur_time;
		
		/* Move to the next */
		idx++;
		if (unlikely(idx == td_num_entries))
			idx = 0;
	}
	
	return ((nwraps + 1) * 2);
}


/**
 *	timedoctor_data_open_proc - called when the timedoctor_data proc file is opened
 *
 *	Opens a sequence file for reading all the /proc/timedoctor_data entries from
 *	the device.  The actually work for reading the timedoctor entries is in
 *	the timedoctor_data_seq_? functions.
 *
 *	This function prepares the reading context for processing the data and
 *	allocates memory for the has table.  It also changes the state to 'reading'
 *	so as to lock out any other process from messing with the buffer while it
 *	is being processed.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure
 */
static int timedoctor_data_open_proc(struct inode* inode, struct file *file)
{
	int ret;
	struct seq_file *m;
	unsigned long flags;
	unsigned int start_idx, end_idx;
	struct timedoctor_data_ctx *ctx = NULL;


	/* Take the state mutex, protecting us from other modules starting/stopping
	 * the trace while we are reading/
	 */
	mutex_lock(&td_state_lock);
	
	/* Check the state is 'stopped', any other state is not valid */
	if (td_state != stopped) {
		mutex_unlock(&td_state_lock);
		return -EINVAL;
	}

	/* Move the state to reading */
	td_state = reading;

	mutex_unlock(&td_state_lock);
	


	/* Take the data spinlock which will ensure the timedoctor_info call has
	 * detected the state change.
	 */
	spin_lock_irqsave(&td_data_lock, flags);

	/* Get the start and end indices of the data */
	end_idx = td_cur_index;
	if (td_has_wrapped)
		start_idx = (end_idx == (td_num_entries - 1)) ? 0 : (end_idx + 1);
	else
		start_idx = 0;

	/* Sanity check there is anything in the buffer to read */
	if (start_idx == end_idx) {
		spin_unlock_irqrestore(&td_data_lock, flags);
		ret = -EINVAL;
		goto err_out;
	}

	spin_unlock_irqrestore(&td_data_lock, flags);
	

	
	/* Allocate memory for the processing context and set the 'first' bit */
	ctx = kmalloc(sizeof(struct timedoctor_data_ctx), GFP_KERNEL);
	if (ctx == NULL)
		return -ENOMEM;
	
	memset(ctx, 0x00, sizeof(struct timedoctor_data_ctx));
	ctx->first = 1;
	
	/* Create a hash table that is needed to store the individual record types
	 * as they are processed in the loop ... do this first so we can get out
	 * early if it fails.
	 */
	ctx->hash_tbl = timedoctor_alloc_hash_table();
	if (ctx->hash_tbl == NULL) {
		ret = -EINVAL;
		goto err_out;
	}


	/* We first need to read all the data in current log to get the maximum
	 * time value.  This is needed because the GUI doesn't like time values
	 * that wrap, so we avoid this by adjusting the time so it doesn't overflow,
	 * however we lose precision when we do this.
	 */
	ctx->time_scalar = timedoctor_calc_time_scalar(start_idx, end_idx);

	/* Finally open the sequence file, if it fails make sure the state changes */
	if ((ret = seq_open(file, &timedoctor_data_seq_ops)) == 0) {

		/* Store the header hash table in the seq_filep */
		m = (struct seq_file*) file->private_data;
		m->private = ctx;
		
		return ret;
	}
	
err_out:
	/* Free the hash table */
	if (ctx != NULL) {
		if (ctx->hash_tbl != NULL)
			timedoctor_free_hash_table(ctx->hash_tbl);
		kfree(ctx);
	}
	
	/* Change the state */
	mutex_lock(&td_state_lock);
	td_state = stopped;
	mutex_unlock(&td_state_lock);
		
	return ret;
}

/**
 *	timedoctor_release_proc - called when the timedoctor proc entry is closed
 *	@inode: proc file inode
 *	@file: file structure pointer of the proc entry
 *
 *	Frees the hash table created in the timedoctor_open_proc() call and changes
 *	the state to 'stopped'.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure
 */
static int timedoctor_data_release_proc(struct inode *inode, struct file *file)
{
	struct seq_file *m = (struct seq_file *)file->private_data;
	struct timedoctor_data_ctx *ctx = (struct timedoctor_data_ctx*)m->private;
	
	/* Free the has table allocated in timedoctor_open_proc */
	if (ctx != NULL) {
		if (ctx->hash_tbl != NULL)
			timedoctor_free_hash_table(ctx->hash_tbl);
		kfree(ctx);
		m->private = NULL;
	}
	
	/* Change the state to 'stopped' */
	mutex_lock(&td_state_lock);
	td_state = stopped;
	mutex_unlock(&td_state_lock);
		
	/* Close the sequence file */
	return seq_release(inode, file);
}

/* Operations for reading /proc/timedoctor_data */
static struct file_operations timedoctor_data_proc_fops = {
	.owner   = THIS_MODULE,
	.open    = timedoctor_data_open_proc,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = timedoctor_data_release_proc
};




/**
 *	timedoctor_raw_seq_show - display a single record, from pointer prepared by
 *                            timedoctor_data_seq_start/timedoctor_data_seq_next
 *	@s: sequence file to print via
 *	@v: pointer to a single data record
 *
 *	Frees the hash table created in the timedoctor_open_proc() call and changes
 *	the state to 'stopped'.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure
 */
static int timedoctor_raw_seq_show(struct seq_file *s, void *v)
{
	uint32_t* rec = (uint32_t*)v;
	seq_printf(s, "%08x %08x %08x %08x\n", rec[0], rec[1], rec[2], rec[3] );
	return 0;
}

/* Operations for reading /proc/timedoctor one record at a time */
static struct seq_operations timedoctor_raw_seq_ops = {
	.start = timedoctor_data_seq_start,
	.next  = timedoctor_data_seq_next,
	.stop  = timedoctor_data_seq_stop,
	.show  = timedoctor_raw_seq_show
};

/**
 *	timedoctor_raw_seq_open_proc - called when the timedoctor proc entry is opened
 *	@inode: the inode of the proc entry
 *	@file: file structure pointer of the proc entry
 *
 *	Simply opens a 'raw' sequence file that is used to read the raw data from
 *	the timedoctor buffer.  The 'raw' data is printed as raw hexadecimal values.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure.
 */
static int timedoctor_raw_seq_open_proc(struct inode* inode, struct file *file)
{
	return seq_open(file, &timedoctor_raw_seq_ops);
}


/**
 *	timedoctor_write_proc - called when the 'timedoctor' proc entry is written
 *
 *	The callback that processes the string written to the /proc/timedoctor file.
 *	This is where the userspace can modify the behaviour of timedoctor.
 *	
 *
 *	LOCKING:
 *	Takes the timedoctor lock when modifying TD behaviour.
 *
 *	RETURNS:
 *	The number of bytes processed on success, an error code on a failure.
 */
static ssize_t timedoctor_write_proc(struct file *file, const char __user *buffer,
                                     size_t count, loff_t *data)
{
	char command[32];
	unsigned long flags;
	unsigned long newsize;

	/* Sanity check */
	if (!count)
		return 0;
	else if (count > 32) {
		printk(TIME_DOCTOR_ERR "Error invalid /proc/timedoctor string\n");
		return -EFAULT;
	}

	/* Copy the command */
	if (copy_from_user(&command, buffer, count))
		return -EFAULT;


	/* Process the command */
	if (!strncmp(command, "off", 3))
		timedoctor_start_stop(0);
	else if (!strncmp(command, "on", 2))
		timedoctor_start_stop(1);
	else if (!strncmp(command, "reset", 5))
		timedoctor_reset();
	else if (!strncmp(command, "wrapon", 6))
		timedoctor_wrap_ctrl(1);
	else if (!strncmp(command, "wrapoff", 7))
		timedoctor_wrap_ctrl(0);
	else if (!strncmp(command, "status", 6))
		printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[td_state]);

	/* If a deallocation command we need to free the buffer and clean up */
	else if (!strncmp(command, "dealloc", 7)) {

		mutex_lock(&td_state_lock);

		/* Set the state to unallocated */
		if (td_state == running)
			printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[stopped]);
		td_state = unallocated;

		/* Reset the index and entries count */
		spin_lock_irqsave(&td_data_lock, flags);
		td_cur_index = 0;
		td_num_entries = 0;
		td_has_wrapped = 0;
		spin_unlock_irqrestore(&td_data_lock, flags);

		/* Deallocate the memory and release the lock */
		timedoctor_buf_free();
		
		mutex_unlock(&td_state_lock);

		/* Tell the helper if one is installed */
//		if (unlikely(td_usermode_helper[0] != '\0'))
	//		schedule_work(&td_helper_work);
	}

	/* Sets the buffer size (number of samples) */
	else if (!strncmp(command, "bufcount=", 9)) {
		newsize = simple_strtoul(&(command[9]), NULL, 0);
		if (newsize != buffer_count) {

			mutex_lock(&td_state_lock);

			/* Set the state to unallocated */
			if (td_state == running)
				printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[stopped]);
			td_state = unallocated;

			/* Store the new default buffer size */
			buffer_count = newsize;
			
			/* Reset the index and entries count */
			spin_lock_irqsave(&td_data_lock, flags);
			td_cur_index = 0;
			td_num_entries = 0;
			td_has_wrapped = 0;
			spin_unlock_irqrestore(&td_data_lock, flags);

			/* Deallocate the memory */
			timedoctor_buf_free();
			
			/* Reallocate the memory with the new size and change state */
			if (timedoctor_buf_alloc(buffer_count) == 0)
				td_state = stopped;

			mutex_unlock(&td_state_lock);

			/* Tell the helper if one is installed */
	//		if (unlikely(td_usermode_helper[0] != '\0'))
		//		schedule_work(&td_helper_work);
		}
	
	}
		
	return count;
}

/* Operations for reading/writing /proc/timedoctor */
static struct file_operations timedoctor_proc_fops = {
	.owner   = THIS_MODULE,
	.open    = timedoctor_raw_seq_open_proc,
	.read    = seq_read,
	.write   = timedoctor_write_proc,
	.llseek  = seq_lseek,
	.release = seq_release
};


/**
 *	timedoctor_buf_alloc - allocates the buffer to store the samples
 *	@nrecs: the number of records to allocate memory for
 *
 *	Attempts to allocate the memory for the timedoctor entries. If the buffer
 *	was already allocated it will be freed.
 *
 *	LOCKING:
 *	The state lock should held before calling this function.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure
 */
static int timedoctor_buf_alloc(unsigned long nrecs)
{
	td_num_entries = 0;

	/* Free the buffer if already allocated */
	if (td_data != NULL)
		vfree(td_data);

	/* Allocate the buffer again */
	td_data = (td_record*)vmalloc(nrecs * sizeof(td_record));
	if (td_data == NULL) {
		printk(TIME_DOCTOR_ERR "Unable to allocate timedoctor memory\n");
		return ENOMEM;
	}

	/* Store the buffer size */
	td_num_entries = nrecs;

	return 0;
}

/**
 *	timedoctor_buf_free - frees the buffer allocated for the entries
 *
 *	Frees the buffer memory.
 *
 *	LOCKING:
 *	None.  Should be called with the timedoctor lock held.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure
 */
static void timedoctor_buf_free(void)
{
	if (td_data != NULL)
		vfree(td_data);
	
	td_data = NULL;
	td_cur_index = 0;
	td_num_entries = 0;
}




/**
 *	timedoctor_info - inserts a timedoctor entry into the log
 *	@data1: the first 32-bit value to store in the record
 *	@data2: the second 32-bit value to store in the record
 *	@data3: the third 32-bit value to store in the record
 *
 *	Enters a timedoctor entry which has the given data fields.  This is
 *	typically not called directly by clients, rather it's called via the set
 *	of macros in the timedoctor header file.
 *
 *	LOCKING:
 *	None.  Internally takes a spin lock, can be called from IRQ or normal
 *	kernel context.
 *
 *	RETURNS:
 *	Nothing.
 */
void timedoctor_info(uint32_t data1, uint32_t data2, uint32_t data3)
{
	uint32_t time;
	unsigned long flags;
	unsigned int idx;

	/* Get out earlier if we're not logging ... really should be in a critical
	 * section, but it shouldn't matter if we mistakenly add a few entries on
	 * the end.
	 */
	if (td_state != running)
		return;

	/* Disable Interrupt Generation */ 
	spin_lock_irqsave(&td_data_lock, flags);

	/* Read the time counter */
//	time = TD_READ_SAMPLE_CLK();
	time = read_c0_count();


	/* Get the current index and increment */
	idx = td_cur_index++;
	if (td_cur_index >= td_num_entries) {
		if (td_wrap_allowed) {
			td_cur_index = 0;
			td_has_wrapped = 1;
			printk(TIME_DOCTOR_NOTICE "%s:wrapped\n", td_state_strings[td_state]);
		} else {
			td_state = stopped;
			td_cur_index--;
			printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[td_state]);

			/* Tell the helper if one is installed */
//			if (unlikely(td_usermode_helper[0] != '\0'))
	//			schedule_work(&td_helper_work);
		}
	}

	/* Check if we should be stopping after 'so many' ticks */
	if (td_stop_after != TDI_NO_STOP_AFTER) {
		if (td_stop_after-- == 0) {
			td_state = stopped;
			td_cur_index--;
			printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[td_state]);
			td_stop_after = TDI_NO_STOP_AFTER;

			/* Tell the helper if one is installed */
		//	if (unlikely(td_usermode_helper[0] != '\0'))
			//	schedule_work(&td_helper_work);
		}
	}

	
	/* Store the data in the buffer */
	td_data[idx][0] = time;
	td_data[idx][1] = data1;
	td_data[idx][2] = data2;
	td_data[idx][3] = data3;

	spin_unlock_irqrestore(&td_data_lock, flags);
}

/**
 *	timeDoctor_Info - legacy function to add timedoctor sample
 *
 *	This function is here just to maintain backwards compatibility.
 *
 *	LOCKING:
 *	See timedoctor_info.
 *
 *	RETURNS:
 *	Nothing.
 */
void timeDoctor_Info(unsigned int data1, unsigned int data2, unsigned int data3)
{
	timedoctor_info(data1, data2, data3);
}

EXPORT_SYMBOL(timedoctor_info);
EXPORT_SYMBOL(timeDoctor_Info);




/**
 *	timedoctor_start_stop - starts or stops a timedoctor trace
 *	@start: if non-zero timedoctor is started, if zero timedoctor is stopped
 *
 *	Starts or stops a timedoctor trace taking into account the current state
 *	of the device.
 *
 *	LOCKING:
 *	None.  Internally takes the state lock to protect access to the global
 *	device state.
 *
 *	RETURNS:
 *	Zero on success, an error code on a failure.
 */
int timedoctor_start_stop(int start)
{
	unsigned long flags;
	int rc = 0;

	mutex_lock(&td_state_lock);
	
	if (start) {
		/* Transition the state to running */
		switch (td_state) {
			case unallocated:
				if (timedoctor_buf_alloc(buffer_count) != 0) {
					rc = ENOMEM;
					break;
				}
				/* fall through */
			case stopped:
			case running:
				td_state = running;
				printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[td_state]);
				break;
			case reading:
				printk(TIME_DOCTOR_ERR "Error currently reading can't start recording\n");
				rc = EINVAL;
				break;
			default:
				printk(TIME_DOCTOR_ERR "Error unknown previous state\n");
				rc = EINVAL;
				break;
		}
	} else {
		/* Transition the state to stopped */
		switch (td_state) {
			case unallocated:
			case stopped:
			case reading:
				break;
			case running:
				td_state = stopped;
				printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[td_state]);
				break;
			default:
				printk(TIME_DOCTOR_ERR "Error unknown previous state\n");
				rc = EINVAL;
				break;
		}
	}
	
	/* Clear the "stop after" value */
	spin_lock_irqsave(&td_data_lock, flags);
	td_stop_after = TDI_NO_STOP_AFTER;
	spin_unlock_irqrestore(&td_data_lock, flags);
	
	mutex_unlock(&td_state_lock);

//	/* Tell the helper if one is installed */
	//if (unlikely(td_usermode_helper[0] != '\0'))
		//schedule_work(&td_helper_work);

	return rc;
}

/**
 *	timeDoctor_SetLevel - legacy function to start/stop a trace
 *
 *	This function is here just to maintain backwards compatibility.
 *
 *	LOCKING:
 *	See timedoctor_start_stop.
 *
 *	RETURNS:
 *	Nothing.
 */
void timeDoctor_SetLevel(int level)
{
	timedoctor_start_stop(level & 0x1);
}

EXPORT_SYMBOL(timedoctor_start_stop);
EXPORT_SYMBOL(timeDoctor_SetLevel);




/**
 *	timedoctor_stop_after - stops a running trace after a number of samples
 *	                        have been taken
 *	@n: the number of samples to record before stopping the trace.
 *
 *	Starts or stops the timedoctor trace after recording 'n' number of samples.
 *
 *	LOCKING:
 *	Takes both the state lock and the data lock.
 *
 *	RETURNS:
 *	Zero on success, a negative error code on a failure.
 */
int timedoctor_stop_after(unsigned int n)
{
	unsigned long flags;

	mutex_lock(&td_state_lock);

	/* Ensure we are running before setting the 'stop after' value */
	if (td_state != running) {
		mutex_unlock(&td_state_lock);
		return -EINVAL;
	}

	/* Take the data spinlock as this global is used in the _info function */
	spin_lock_irqsave(&td_data_lock, flags);
	td_stop_after = n;
	spin_unlock_irqrestore(&td_data_lock, flags);

	mutex_unlock(&td_state_lock);
	
	return 0;
}

/**
 *	timeDoctor_SetLevelAfter - legacy function to stop the trace after a number
 *	                           of samples.
 *
 *	This function is here just to maintain backwards compatibility.
 *
 *	LOCKING:
 *	See timedoctor_stop_after.
 *
 *	RETURNS:
 *	Nothing.
 */
void timeDoctor_SetLevelAfter(int level, unsigned int samples)
{
	timedoctor_stop_after(samples);
}

EXPORT_SYMBOL(timedoctor_stop_after);
EXPORT_SYMBOL(timeDoctor_SetLevelAfter);



/**
 *	timedoctor_reset - resets either a running or stopped trace
 *
 *	This function simply resets the current write index, the next sample will
 *	be put at the start of the buffer.  This function will fail if currently
 *	in the reading state.
 *
 *	LOCKING:
 *	Zero on success, a negative error code on a failure.
 *
 *	RETURNS:
 *	Zero on success, otherwise a negative error code.
 */
int timedoctor_reset(void)
{
	unsigned long flags;

	mutex_lock(&td_state_lock);
	
	/* If reading then we can't reset the buffer because this will break the
	 * reading code.
	 */
	if (td_state == reading) {
		mutex_unlock(&td_state_lock);
		return -EINVAL;
	}
	
	/* Take the data spinlock as this global is used in the _info function */
	spin_lock_irqsave(&td_data_lock, flags);
	
	td_cur_index = 0;
	td_has_wrapped = 0;
	
	spin_unlock_irqrestore(&td_data_lock, flags);
	
	mutex_unlock(&td_state_lock);

	return 0;
}

/**
 *	timeDoctor_Reset - legacy function to reset a trace
 *
 *	This function is here just to maintain backwards compatibility.
 *
 *	LOCKING:
 *	See timedoctor_reset.
 *
 *	RETURNS:
 *	Nothing.
 */
void timeDoctor_Reset(void)
{
	timedoctor_reset();
}

EXPORT_SYMBOL(timedoctor_reset);
EXPORT_SYMBOL(timeDoctor_Reset);



/**
 *	timedoctor_wrap_ctrl - enables/disables buffer wraps
 *	@enable: If non-zero the buffer is allowed to wrap, if zero the trace will
 *	         stop when the end of the buffer is reached.
 *
 *	Enables or disables wrapping of the sample buffer, if enabled recording
 *	will run continuously until explicitly stopped, if disabled recording will
 *	stop when the buffer is full.
 *
 *	LOCKING:
 *	Takes and releases the data lock.
 *
 *	RETURNS:
 *	Always returns zero.
 */
int timedoctor_wrap_ctrl(int enable)
{
	unsigned long flags;

	/* Take the data spinlock as this global is used in the _info function */
	spin_lock_irqsave(&td_data_lock, flags);
	td_wrap_allowed = !!enable;
	spin_unlock_irqrestore(&td_data_lock, flags);
	
	return 0;
}

EXPORT_SYMBOL(timedoctor_wrap_ctrl);



/**
 *	timedoctor_set_bufsize - sets the size of the buffer
 *	@size: the number of records to allocate
 *
 *
 *
 *
 */
int timedoctor_set_bufsize(size_t size)
{
	unsigned long flags;

	mutex_lock(&td_state_lock);

	/* Set the state to unallocated */
	if (td_state == running)
		printk(TIME_DOCTOR_NOTICE "%s\n", td_state_strings[stopped]);
	td_state = unallocated;

	/* Store the new default buffer size */
	buffer_count = size;

	/* Reset the index and entries count */
	spin_lock_irqsave(&td_data_lock, flags);
	td_cur_index = 0;
	td_num_entries = 0;
	td_has_wrapped = 0;
	spin_unlock_irqrestore(&td_data_lock, flags);

	/* Deallocate the memory */
	timedoctor_buf_free();

	/* Reallocate the memory with the new size and change state */
	if (timedoctor_buf_alloc(buffer_count) == 0)
		td_state = stopped;

	mutex_unlock(&td_state_lock);

	/* Tell the helper if one is installed */
//	if (unlikely(td_usermode_helper[0] != '\0'))
//		schedule_work(&td_helper_work);

	return 0;
}

EXPORT_SYMBOL(timedoctor_set_bufsize);



/**
 *	timedoctor_ioctl - ioctl function for the timedoctor device
 *	@inode: ignored
 *	@filp: ignored
 *	@cmd: the ioctl command
 *	@arg: the argument supplied in the ioctl call
 *
 *	The function performs the ioctl operations specified in the header file.
 *
 *	LOCKING:
 *	May take and release the state and data locks.
 *
 *	RETURNS:
 *	Nothing.
 */
static long timedoctor_ioctl(struct file *filp,
                            unsigned int cmd, unsigned long arg)
{
	int ret;
	unsigned long flags;
	uint32_t data[TIMEDOCTOR_INFO_DATASIZE];
	
	switch (cmd) {
		case TIMEDOCTOR_IOCTL_RESET:
			return timedoctor_reset();
		case TIMEDOCTOR_IOCTL_START:
			return timedoctor_start_stop(1);
		case TIMEDOCTOR_IOCTL_STOP:
			return timedoctor_start_stop(0);

		case TIMEDOCTOR_IOCTL_GET_ENTRIES:
			spin_lock_irqsave(&td_data_lock, flags);
			if (td_has_wrapped)
				ret = td_num_entries;
			else
				ret = td_cur_index;
			spin_unlock_irqrestore(&td_data_lock, flags);
			return ret;

		case TIMEDOCTOR_IOCTL_GET_MAX_ENTRIES:
			return td_num_entries;

		case TIMEDOCTOR_IOCTL_INFO:
			copy_from_user(&data, (uint32_t*)arg,
			               (sizeof(uint32_t) * TIMEDOCTOR_INFO_DATASIZE));
			timedoctor_info(data[0], data[1], data[2]);
			return 0;
	}

	return -ENOSYS;
}


static struct file_operations timedoctor_fops = {
	.unlocked_ioctl =  timedoctor_ioctl,
};

static struct miscdevice timedoctor_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = TIME_DOCTOR_DEVNAME,
	.fops  = &timedoctor_fops
};


/**
 *	timedoctor_init - module startup
 *
 *	Module initialisation function.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Zero on success, error code on failure.
 */
static int __init timedoctor_init(void)
{
	int ret;
	static struct proc_dir_entry *entry;

	/* Initialise the locks */
	spin_lock_init(&td_data_lock);
	mutex_init(&td_state_lock);

	/* Set the default state */
	td_cur_index = 0;
	td_num_entries = 0;
	td_has_wrapped = 0;
	td_stop_after = TDI_NO_STOP_AFTER;
	td_state = unallocated;

	memset(td_usermode_helper, 0x00, sizeof(td_usermode_helper));
	

	/* Allocate the buffer up front if asked to */
	if (allocate) {
		if (timedoctor_buf_alloc(buffer_count) == 0)
			td_state = stopped;
	}

	/* Register a misc device called "timedoctor" */
	ret = misc_register(&timedoctor_misc_dev);
	if (ret < 0) {
		printk(TIME_DOCTOR_ERR "Error can't register misc device (minor %d)\n",
		       timedoctor_misc_dev.minor );
		return ret;
	}


	/* Create proc entry for controlling timedoctor and reading raw entries */
	entry = create_proc_entry(TIME_DOCTOR_DEVNAME, S_IFREG | S_IRUGO | S_IWUGO,
	                          NULL);
	if (!entry) {
		printk(TIME_DOCTOR_ERR "Error create_proc_entry : failed\n");
		timedoctor_exit();
		return -ENOMEM;
	}
	entry->proc_fops = &timedoctor_proc_fops;

	/* Create proc entry for reading formatted data */
	entry = create_proc_entry(TIME_DOCTOR_DEVNAME "-data",
	                          S_IFREG | S_IRUGO, NULL);
	if (!entry) {
		printk(TIME_DOCTOR_ERR "Error create_proc_entry : failed\n");
		timedoctor_exit();
		return -ENOMEM;
	}
	entry->proc_fops = &timedoctor_data_proc_fops;

#if 0
	/* Create proc entry for reading/writing the irq filter */
	entry = create_proc_entry(TIME_DOCTOR_DEVNAME "-irq-filter",
	                          S_IFREG | S_IRUGO | S_IWUGO, NULL);
	if (!entry) {
		printk(TIME_DOCTOR_ERR "Error create_proc_entry : failed\n");
		timedoctor_exit();
		return -ENOMEM;
	}
	entry->read_proc  = timedoctor_irq_filter_read_proc;
	entry->write_proc = timedoctor_irq_filter_write_proc;

	/* Create proc entry for reading/writing the path to the usermode helper */
	entry = create_proc_entry(TIME_DOCTOR_DEVNAME "-helper",
	                          S_IFREG | S_IRUGO | S_IWUGO, NULL);
	if (!entry) {
		printk(TIME_DOCTOR_ERR "Error create_proc_entry : failed\n");
		timedoctor_exit();
		return -ENOMEM;
	}
	entry->read_proc  = timedoctor_helper_read_proc;
	entry->write_proc = timedoctor_helper_write_proc;


	printk("%s (%s-%s) [%ukHz sample clk]\n",
	       TIME_DOCTOR_DESCRIPTION, __DATE__, __TIME__,
	       ((unsigned)TD_SAMPLE_CLK_FREQ) / 1000);
#endif
	return 0;
}



/**
 *	timedoctor_exit - module teardown
 *
 *	De-registers the driver and free's any memory allocated by the driver.
 *
 *	LOCKING:
 *	None.
 *
 *	RETURNS:
 *	Nothing.
 */
static void timedoctor_exit(void)
{
	printk(TIME_DOCTOR_NOTICE "Exiting %s\n", TIME_DOCTOR_DESCRIPTION);

	/* De-register the device */
	misc_deregister(&timedoctor_misc_dev);

	/* Free memory allocated for the buffer */
	timedoctor_buf_free();

	/* Remove proc entries */
	remove_proc_entry(TIME_DOCTOR_DEVNAME, NULL);
	remove_proc_entry(TIME_DOCTOR_DEVNAME "-data", NULL);
//	remove_proc_entry(TIME_DOCTOR_DEVNAME "-irq-filter", NULL);
}

module_init(timedoctor_init);
module_exit(timedoctor_exit);
