/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
 *  All Rights Reserved
 *
 * <:label-BRCM:2011:DUAL/GPL:standard
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
 *
 ************************************************************************/

#ifndef __CMS_EID_H__
#define __CMS_EID_H__

/*!\file cms_eid.h
 * \brief Header file containing entity id assignments in
 * the CPE Management System (CMS).
 */


/*!\enum CmsEntityId
 * \brief Entity Id's (eid) are like process id's (pid), except they identify an
 * application in general, not the specific instance of that application.
 *
 * For example, the ppp application has an entity id of 22.  That entity id
 * refers to the ppp application in general, not a specific instance of ppp,
 * which is uniquely identified by its process id (pid).
 * 
 * Entity Id's are used by the various CMS API's to identify its caller.
 * They are also used by the messaging sub-system to identify the source
 * and destination of messages.  In most cases, there is only one instance
 * of a particular application, so specifying the CmsEntityId is enough to
 * identify the application.  However, for applications that may have multiple
 * instances, additional steps may be required.  (For details on how
 * to identify an application for the purpose of sending or receiving messages,
 * see cms_msg.h).
 *
 * The first 15 eid's (1-15) are special because they are apps whose access
 * to the MDM can be controlled using the TR69 protocol.  See eid_bcm_mgmt.txt
 *
 * Adding an EID to this file has the benefit of preventing multiple uses
 * of the same EID, but adding an EID here is optional (CMS still works
 * if an EID is not defined here).  Broadcom developers should continue to
 * add EID's here, but third party developers do not need to.  Instead,
 * please follow the allocation ranges below to avoid conflicts.
 *
 * Reserved for Broadcom Use                            1 - 3999
 * -- Mangement Apps                                    1 -   15
 * -- various legacy and singleton EIDs                16 -  499
 * -- Modular Software                                500 -  539
 * -- Diagnostic related apps and commands            540 -  559
 *
 * Reserved for kernel threads                       4000 - 4999
 *
 * Reserved for Customer use                         5000 - 9999
 *
 * All other EIDs are reserved for future allocation
 *
 */

typedef enum {
   EID_INVALID=0,
   EID_TR69C=1,        /* Begin TR69 controlled Management App EID range */
   EID_TR64C=2,
   EID_HTTPD=3,
   EID_SNMPD=4,
   EID_CONSOLED=5,
   EID_TELNETD=6,
   EID_SSHD=7,
   EID_UPNP=8,
#ifdef SKY  
   EID_SKYANYTIMEUPNP=9,
   EID_SKYDHCPC=10,
   EID_FUS=11,
   EID_HEALTHMONITOR=12,
   EID_FACTORY=13,
   EID_SKYSELFHEAL=14,
   EID_SKYPLC=15,  /* End TR69 controlled Management App EID range */
#endif // SKY
   EID_WLNVRAM=17,
   EID_WLWAPID=18,
   EID_WLEVENT=19,
   EID_SMD=20,
   EID_SSK=21,
   EID_PPP=22,
   EID_DHCPC=23,
   EID_DHCPD=24,
   EID_FTPD=25,
   EID_TFTPD=26,
   EID_TFTP=27,
   EID_DNSPROBE=28,
   EID_SYSLOGD=30,
   EID_KLOGD=31,
   EID_DDNSD=33,
   EID_ZEBRA=34,
   EID_RIPD=35,
   EID_SNTP=36,
   EID_URLFILTERD=37,
   EID_TRACERT=39,
   EID_PING=40,
   EID_DHCP6C=41,
   EID_DHCP6S=42,
   EID_RADVD=43,
   EID_DNSPROXY=44,
   EID_IPPD=45,
   EID_FTP=46,
   EID_DSLDIAGD=48,
   EID_SOAPSERVER=49,
   EID_PWRCTL=50,
   EID_HOTPLUG=51,
   EID_L2TPD=52,
   EID_SAMBA=53,
   EID_PPTPD=54,
   EID_DECT=56,
   EID_OMCID=60,
   EID_OMCIPMD=61,   
   EID_RASTATUS6=62,
   EID_EPON_APP=70,
   EID_EPON_OAM_PORT_LOOP_DETECT=71,
   EID_VECTORINGD=80,
   EID_UNITTEST=90,
   EID_MISC=91,
   EID_WLMNGR=92,
   EID_WLWPS=93,
   EID_CMFD=94,
   EID_MCPD=95,
   EID_MOCAD=96,
   EID_RNGD=97,
   EID_DMSD=98,
   EID_SWMDK=100,
   EID_OLT_TEST=101,
   EID_BMUD=103,
   EID_BMUCTL=104,
   EID_PLC_NVM=105,
   EID_PLC_BOOT=106,
   EID_MCPCTL=107,
   EID_HOMEPLUGD=108,
   EID_HOMEPLUGCTL=109,
   EID_VODSL=150,
   EID_DECTDBGD=199,
   EID_SEND_CMS_MSG=280,
   EID_OSGID=500,              /* Begin BCM Modular Software EID range */
   EID_LINMOSD=510,
   EID_MODUPDTV=511,
   EID_MODSW_RESERVED_END=539, /* End BCM Modular Software EID range */
   EID_DOWNLOAD_DIAG=540,      /* Begin Diag reserved range          */
   EID_UPLOAD_DIAG=541,
   EID_UDPECHO=542,
   EID_TMSCTL=543,
   EID_DIAG_RESERVED_END=559,  /* End Diag reserved range            */
   EID_BCM_USERSPACE_MAX=3999, /* End BCM userspace threads EID range */
   EID_BCM_KTHREAD_MIN=4000,   /* Begin BCM kernel threads EID range */
   EID_BCM_KTHREAD_MAX=4999,   /* End   BCM kernel threads EID range */
   EID_CUSTOMER_MIN=5000,      /* Customers can use EID starting from 5000 */
#ifdef SKY
   EID_DEVICEMONITOR=5001,
   EID_WIFIMONITOR=5002,
   EID_SKYTEST=5003,
   EID_LANSUBSYSEVT=5004,
   EID_SKY_GW_PINGER=5005,     /* Sky Booster 2 process to verify gateway connection */
#ifdef SKY_REBOOT_DIAGNOSTICS
   EID_RTM=5006,     /* Sky Booster 2 process to verify gateway connection */
#endif
#ifdef SKY_WLAN_DIAG
   EID_SKY_WLAN_DIAG=5007,
#endif
   EID_TINYSVCMDNS=5008,
#endif // SKY
   EID_CUSTOMER_MAX=9999,      /* Customers can use 5000-9999 */
   EID_RESERVED=10000,   /* keep EID's >= 10000 reserved for future use */
EID_LAST=65535
} CmsEntityId;


/** Param accessible by TR69C;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_TR69C              0x0001

/** Param accessible by TR64C;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_TR64C              0x0002

/** Param accessible by httpd;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_HTTPD              0x0004

/** Param accessible by SNMP;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_SNMPD              0x0008

/** Param accessible by consoled/cli;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_CONSOLED           0x0010

/** Param accessible by telnetd/cli;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_TELNETD            0x0020

/** Param accessible by sshd/cli;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_SSHD               0x0040

/** Param accessible by UPnp;  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_UPNP               0x0080
#ifdef SKY
/** Param accessible by (avail1);Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_SKYDHCPC         0x0100

/** Param accessible by (avail10);  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_FACTORY        0x0200

/** Param accessible by (avail11);  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_SKYSELFHEAL        0x0400

/** Param accessible by (avail12);  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_FUS        0x0800

/** Param accessible by (avail2);Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_SKYANYTIMEUPNP         0x1000

/** Param accessible by (avail14);  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_HEALTHMONITOR        0x2000

/** Param accessible by (avail15);  Used in MdmNodeAttributes.accessBitMask. */
#define NDA_ACCESS_SKYPLC        0x4000

#define NDA_ACCESS_RTM        0x4000

#endif //SKY
/* the access bits must not exceed 0x4000.  There are only 15 bits available! */



/** Subscriber LAN side configuration apps.
 *
 * TR-069 A.3.2.4 describes how the ACS is able to use the
 * setParameterAttributes RPC to control MDM write access by
 * "subscriber LAN side configuration apps" and "other management apps".
 * The current design of the MDM uses 15 bits in
 * MdmNodeAttributes.accessBitMask to control MDM write access.
 * The first bit is used by tr69c itself.  All other bits are currently
 * allocated to the subscriber LAN side configuration apps.  No bits are
 * currently allocated to control "other management apps".  In the current
 * design, each subscriber LAN side configuration app has its own
 * NDA_ACCESS_xxx bit, which are combined into NDA_ACCESS_SUBSCRIBER.
 * A future or alternate design could make all subscriber LAN side
 * configuration apps share a single NDA_ACCESS_xxx bit, and define
 * NDA_ACCESS_xxx bits for our "other management apps" such as OMCID, EPOND.
 *
 * See also DEFAULT_ACCESS_BIT_MASK in cms_mdm.h
 *
 */
#define NDA_ACCESS_SUBSCRIBER  (NDA_ACCESS_TR64C        |                         \
                                NDA_ACCESS_HTTPD        | NDA_ACCESS_SNMPD      | \
                                NDA_ACCESS_CONSOLED     | NDA_ACCESS_TELNETD    | \
                                NDA_ACCESS_SSHD         | NDA_ACCESS_UPNP       | \
		                        NDA_ACCESS_SKYDHCPC   | NDA_ACCESS_FACTORY | \
         			            NDA_ACCESS_SKYSELFHEAL  | NDA_ACCESS_FUS | \
                    			NDA_ACCESS_SKYANYTIMEUPNP   | NDA_ACCESS_HEALTHMONITOR  | \
								NDA_ACCESS_SKYPLC )					   


/** Entity is a server, which opens a socket and listens for requests.
 *
 * This flag is used in CmsEntityInfo.flags below.
 */
#define EIF_SERVER                    0x01


/** If entity is a server, server socket uses TCP (otherwise, uses UDP)
 *
 * This flag is used in CmsEntityInfo.flags below.
 */
#define EIF_SERVER_TCP                0x02


/** Entity should be launched during stage 1 of CMS startup..
 *
 * This flag is used in CmsEntityInfo.flags below.
 * So far, the only app we launch during stage 1 is ssk.
 */
#define EIF_LAUNCH_IN_STAGE_1         0x04


/** Entity should be launched on system boot (during stage 2).
 *
 * This flag is used in CmsEntityInfo.flags below.
 */
#define EIF_LAUNCH_ON_BOOT            0x08


/** Entity will need access to the MDM.
 *
 * This flag is used in CmsEntityInfo.flags below.
 * This flag will cause smd to pass the MDM shared memory id to the
 * app in the -m option.  
 * If the entity is going to access the MDM, it must have a NDA_ACCESS_xxx
 * bit defined for it.
 */
#define EIF_MDM                       0x10


/** Entity can run on the Desktop Linux.
 *
 * Some entities, such as httpd, can run on the Desktop Linux as
 * well as modems.  Others, such as dhcpd, can only run on the modem.
 */
#define EIF_DESKTOP_LINUX_CAPABLE     0x20


/** Entity will create a cms_msg communications link to smd. */
#define EIF_MESSAGING_CAPABLE         0x40


/** Entity can have multiple instances, currently only ppp and dhcpc */
#define EIF_MULTIPLE_INSTANCES        0x80


/** Entity supports IPv6. */
#define EIF_IPV6                      0x100

/** Tell smd to relaunch this App if it dies abnormally (sigsegv, etc) */
#define EIF_AUTO_RELAUNCH             0x200

/** Smd will set this app's real-time policy and priority.
 */
#define EIF_SET_SCHED                 0x400

/** Smd will set this app's CPU mask (for CPU pinning).
 */
#define EIF_SET_CPUMASK               0x800

/** Smd will set this app's cgroups membership.
 */
#define EIF_SET_CGROUPS               0x1000

/** Request termination via the CMS_MSG_TERMINATE instead of signal.
 */
#define EIF_MSG_TERMINATE             0x8000


#include "cms_dlist.h"

/** A structure that contains info about an CmsEntityId.
 *
 * For each new CmsEntityId defined, add a corresponding entry to the
 * EntityInfoArray in cms_util/eid.c
 *
 */
typedef struct
{
   DlistNode dlist;    /**< internal use only, must be first field */
   CmsEntityId eid;    /**< The entity id for this entry. */
   UINT16 accessBit;   /**< This entity's bit position in MdmNodeAttributes.accessBitMask (only 15 bits available) */
   char *name;         /**< Short name for this entity. */
   char *path;         /**< Full path to the executable. */
   char *runArgs;      /**< Args to pass to this executable on startup */
   UINT32 flags;       /**< Additional info about this entity, see EIF_xxx */
   UINT8  backLog;     /**< Backlog arg for the server socket */
   UINT16 port;        /**< If server, port number for the server socket. */
   UINT32 maxMemUsage;    /**< Maximum memory usage, in KB, 0 if unknown. */
   UINT32 normalMemUsage; /**< Typical memory usage, in KB, 0 if unknown. */
   UINT32 minMemUsage;    /**< Minimum memory usage, in KB, 0 if unknown. */
   UINT32 schedPolicy;   /**< (use vals from sched.h) 0=NORMAL, 1=FIFO, 2=RR */
   UINT32 schedPriority; /**< 99 is highest, 1 is lowest, but see bcm_realtime.h */
   UINT32 cpuMask;    /**< bitmask of CPU's this thread is allowed to run on.  1=TP0, 2=TP1, 3=both (0=both) */
   char *cpuGroupName;  /**< name of cpugroup this belongs to (leave blank for default) */
} CmsEntityInfo;


/** Entity Info name used when we need to create a CmsEntityInfo but
 *  could not find any predefined info.
 */
#define CMS_UNKNOWN_ENTITY_NAME "unknown"


/** Get the CmsEntityInfo for the specified CmsEntityId.
 *
 * @param eid (IN) CmsEntityId to get info on.
 * @return pointer to EntityInfo structure corresponding to the specified eid.
 *         Caller must not modify the returned CmsEntityInfo structure.
 *         Caller must not free the returned CmsEntityInfo structure.
 *         Returns NULL if CmsEntityId is invalid.
 */
const CmsEntityInfo *cmsEid_getEntityInfo(CmsEntityId eid);


/** Same as cmsEid_getEntityInfo but will create a blank structure if
 *  given eid is not found.
 *
 * @param eid (IN) CmsEntityId to get info on.
 * @return pointer to EntityInfo structure corresponding to the specified eid.
 *         Caller must not modify the returned CmsEntityInfo structure.
 *         Caller must not free the returned CmsEntityInfo structure.
 *         Returns NULL if allocation failed.
 */
const CmsEntityInfo *cmsEid_getEntityInfoAutoCreate(CmsEntityId eid);


/** Get the CmsEntityInfo for the specified bit in the bitmask.
 *
 * @param bit (IN) A bit in the MdmNodeAttributes.accessBitMask.  Only a 
 *                 single bit is allowed.
 * @return pointer to EntityInfo structure corresponding to the specified bit.
 *         Caller must not modify the returned CmsEntityInfo structure.
 *         Returns NULL if bit is not recognized.
 */
const CmsEntityInfo *cmsEid_getEntityInfoByAccessBit(UINT16 bit);


/** Get the CmsEntityInfo for the specified entity's short string name.
 *
 * @param stringName (IN) The short string name of the entity.
 * @return pointer to EntityInfo structure corresponding to the specified name.
 *         Caller must not modify the returned CmsEntityInfo structure.
 *         Caller must not free the returned CmsEntityInfo structure.
 *         Returns NULL if name is not recognized.
 */
const CmsEntityInfo *cmsEid_getEntityInfoByStringName(const char *stringName);


/** Convert the given bitmask into a buffer of comma separated string names.
 *
 * If the bitmask contains bits which are not recognized, this function
 * will ignore them and fill the buffer with the string names for bits that it
 * does recognize and return CMSRET_SUCCESS_UNRECOGNIZED_DATA_IGNORED.
 * If the bitmask is empty or does not contain any bits which this function
 * recognized, a buffer containing empty string will be returned.
 *
 * @param bitMask  (IN) bitmask.
 * @param buf     (OUT) A buffer containing the comma separated string names of the
 *                      entities in the bit mask.  The caller is responsible for
 *                      freeing the string buffer.
 * @return CmsRet enum.
 */
CmsRet cmsEid_getStringNamesFromBitMask(UINT16 bitMask, char **buf);


/** Convert the given comma separated strings names into a bit mask.
 *
 * If the buffer contains string names which are not recognized, this function
 * will ignore them and fill the bitMask with bits for the string names that it
 * does recognize and return CMSRET_SUCCESS_UNRECOGNIZED_DATA_IGNORED.
 *
 * @param buf      (IN) A buffer containing the comma separated string names of the
 *                      entities in the bit mask.  The caller is responsible for
 *                      freeing the string buffer.
 * @param bitMask (OUT) Resulting bitmask representation.
 *
 * @return CmsRet enum.
 */
CmsRet cmsEid_getBitMaskFromStringNames(const char *buf, UINT16 *bitMask);


/** Get the process/thread id of the caller.
 *
 * Unlike the entity id, a pid is unique for all processes.  So two
 * instances of the same entity id will have different pid's.  This is
 * useful when you need to distinguish among multiple instances of
 * the same eid.
 *
 * @return SINT32 representing the pid.
 */
SINT32 cmsEid_getPid(void);


/** Get the number of enties in the entityInfoArray.
 *
 * This function is for use by smd/oal_event.c.  Other callers should not use it.
 * 
 * @return number of entries in the entityInfoArray.
 */
UINT32 cmsEid_getNumberOfEntityInfoEntries(void);


/** Get the first entry in the entityInfoArray.
 *
 * This function is for use by smd/oal_event.c.  Other callers should not use it.
 * 
 * @return pointer to the first entry in the entityInfoArray.
 */
const CmsEntityInfo *cmsEid_getFirstEntityInfo(void);


/** Get the next entry after the given eInfo
 *
 * This function is for use by smd/oal_event.c.  Other callers should not use it.
 *
 * @param Return the entry after this one.
 *
 * @return pointer to the next entry in the entityInfoArray.
 */
const CmsEntityInfo *cmsEid_getNextEntityInfo(const CmsEntityInfo *eInfo);


/** Print out the fields of the given eInfo.
 *
 * @param (IN)  pointer to CmsEntityInfo structure
 */
void cmsEid_dumpEntityInfo(const CmsEntityInfo *eInfo);


/** Mask to get the generic eid out of a UINT32 eid field. */
#define EID_EID_MASK  0x0000ffff

/** Mask to get the pid out of a UINT32 eid field */
#define EID_PID_MASK  0xffff0000

/** Get the generic eid out of a UINT32 eid field */
#define GENERIC_EID(e) (((UINT32) e) & EID_EID_MASK)

/** Get the pid out of a UINT32 eid field */
#define PID_IN_EID(e) ((((UINT32) e) & EID_PID_MASK) >> 16)

/** Make a specific UINT32 eid field out of a pid and a generic eid */
#define MAKE_SPECIFIC_EID(p, e) ((p << 16) | (((UINT32) e) & EID_EID_MASK))

#endif  /* __EID_H__ */
