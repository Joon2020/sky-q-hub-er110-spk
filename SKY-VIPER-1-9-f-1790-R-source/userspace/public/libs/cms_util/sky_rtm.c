#include "sky_rtm.h"

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_boardcmds.h"
#include "cms_boardioctl.h"
#include "sky_public_utils.h"

#include <string.h>
#include <unistd.h>


#define RTM_REBOOT_NOTIF_SLEEP 5


CmsRet sky_rtm_reboot_notify(void *msg_handle, UINT32 subcode, const char* descr) {
#ifdef SKY_REBOOT_DIAGNOSTICS
	SkyMsgRtmRebootNotif notif;
	CmsRet ret = CMSRET_SUCCESS;

	memset(&notif, 0x00, sizeof(notif));

	notif.head.type = CMS_MSG_SKY_RTM_REBOOT_NOTIF;
	notif.head.dataLength = sizeof(notif) - sizeof(CmsMsgHeader);
	notif.head.dst = EID_RTM;
	notif.head.src = cmsMsg_getHandleEid(msg_handle);

	/* this is an event message */
	notif.head.flags_event = 1;
	notif.head.flags_bounceIfNotRunning = 1;

	notif.body.subcode = subcode;
	notif.body.timestamp = time(NULL);
	strncpy(notif.body.descr, descr, sizeof(notif.body.descr));

	ret = cmsMsg_send(msg_handle, (CmsMsgHeader*)&notif);
	if (CMSRET_SUCCESS != ret) {
		cmsLog_error("Error sending CMS_MSG_SKY_RTM_REBOOT_NOTIF");
	}

    /*
	 * sleep for a while to make sure that rtm has enough time to log the event
	 * before the client code will trigger the real reboot
     */
	sleep(RTM_REBOOT_NOTIF_SLEEP);
    return ret;
#else
    return CMSRET_SUCCESS;
#endif
}
