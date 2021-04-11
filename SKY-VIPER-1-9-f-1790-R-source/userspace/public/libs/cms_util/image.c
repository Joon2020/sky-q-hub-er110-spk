/***********************************************************************
 * <:copyright-BRCM:2007:DUAL/GPL:standard
 * 
 *    Copyright (c) 2007 Broadcom Corporation
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
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <net/if.h>

#include "cms.h"
#include "cms_util.h"
#include "cms_msg.h"
#include "cms_boardcmds.h"
#include "cms_boardioctl.h"

#include "bcmTag.h" /* in shared/opensource/include/bcm963xx, for FILE_TAG */
#include "board.h" /* in bcmdrivers/opensource/include/bcm963xx, for BCM_IMAGE_CFE */

#include "image_modsw_linux.h" /* for the various Linux modular sw functions */

#ifdef SKY
#include "sky_public_utils.h"
#endif // SKY

static UBOOL8 matchChipId(const char *strTagChipId, const char *signature2);
CmsRet verifyBroadcomFileTag(FILE_TAG *pTag, int imageLen);
#ifndef SKY
static CmsRet flashImage(char *imagePtr, UINT32 imageLen, CmsImageFormat format, UINT32 opts);
static CmsRet sendConfigMsg(const char *imagePtr, UINT32 imageLen, void *msgHandle, CmsMsgType msgType);
#else
CmsRet flashImage(char *imagePtr, UINT32 imageLen, CmsImageFormat format, UINT32 opts);
CmsRet sendConfigMsg(const char *imagePtr, UINT32 imageLen, void *msgHandle, CmsMsgType msgType);
#endif


/**
 * @return TRUE if the modem's chip id matches that of the image.
 */
UBOOL8 matchChipId(const char *strTagChipId, const char *signature2 __attribute__((unused)))
{
    UINT32 tagChipId = 0;
    UINT32 chipId; 
    UBOOL8 match;

    /* this is the image's chip id */
    tagChipId = (UINT32) strtol(strTagChipId, NULL, 16);
    
    /* get the system's chip id */
    devCtl_getChipId(&chipId);
    if (tagChipId == chipId)
    {
        match = TRUE;
    }
    else if (tagChipId == BRCM_CHIP_HEX)
    {
        match = TRUE;
    }
    else
    {
        cmsLog_error("Chip Id error.  Image Chip Id = %04x, Board Chip Id = %04x.", tagChipId, chipId);
        match = FALSE;
    }

    return match;
}

// verify the tag of the image
CmsRet verifyBroadcomFileTag(FILE_TAG *pTag, int imageLen)
{
    UINT32 crc;
    int totalImageSize;
    int tagVer, curVer;
    UINT32 tokenCrc, imageCrc;

        
    cmsLog_debug("start of pTag=%p tagversion %02x %02x %02x %02x", pTag,
                  pTag->tagVersion[0], 
                  pTag->tagVersion[1], 
                  pTag->tagVersion[2], 
                  pTag->tagVersion[3]);

    tokenCrc = *((UINT32 *)pTag->tagValidationToken);
    imageCrc = *((UINT32 *)pTag->imageValidationToken);                  
#ifdef DESKTOP_LINUX
    /* assume desktop is running on little endien intel, but the CRC has been
     * written for big endien mips, so swap before compare.
     */
    tokenCrc = htonl(tokenCrc);
    imageCrc = htonl(imageCrc);
#endif
                  
    // check tag validate token first
    crc = CRC_INITIAL_VALUE;
    crc = cmsCrc_getCrc32((UINT8 *) pTag, (UINT32)TAG_LEN-TOKEN_LEN, crc);
    if (crc != tokenCrc)
    {
        /* this function is called even when we are not sure the image is
         * a broadcom image.  So if crc fails, it is not a big deal.  It just
         * means this is not a broadcom image.
         */
        cmsLog_debug("token crc failed, this is not a valid broadcom image");
        cmsLog_debug("calculated crc=0x%x tokenCrc=0x%x", crc, tokenCrc);
        return CMSRET_INVALID_IMAGE;
    }
    cmsLog_debug("header CRC is OK.");
    
    // check imageCrc
    totalImageSize = atoi((char *) pTag->totalImageLen);
    cmsLog_debug("totalImageLen=%d, imageLen=%d, TAG_LEN=%d\n", totalImageSize, imageLen, TAG_LEN);

    if (totalImageSize > (imageLen -TAG_LEN)) {
	 cmsLog_error("invalid size\n");
        return CMSRET_INVALID_IMAGE;
	}

    crc = CRC_INITIAL_VALUE;
    crc = cmsCrc_getCrc32(((UINT8 *)pTag + TAG_LEN), (UINT32) totalImageSize, crc);      
    if (crc != imageCrc)
    {
        /*
         * This should not happen.  We already passed the crc check on the header,
         * so we should pass the crc check on the image.  If this fails, something
         * is wrong.
         */
        cmsLog_error("image crc failed after broadcom header crc succeeded");
        cmsLog_error("calculated crc=0x%x imageCrc=0x%x totalImageSize=0x%x", crc, imageCrc, totalImageSize); 
        return CMSRET_INVALID_IMAGE;
    }
    cmsLog_debug("image crc is OK");

    tagVer = atoi((char *) pTag->tagVersion);
    curVer = atoi(BCM_TAG_VER);

    if (tagVer != curVer)

    {
       cmsLog_error("Firmware tag version [%d] is not compatible with the current Tag version [%d]", tagVer, curVer);
       return CMSRET_INVALID_IMAGE;
    }
    cmsLog_debug("tarVer=%d, curVar=%d", tagVer, curVer);

    if (!matchChipId((char *) pTag->chipId, (char *) pTag->signiture_2))
    {
       cmsLog_error("chipid check failed");
       return CMSRET_INVALID_IMAGE;
    }

    cmsLog_debug("Good broadcom image");

    return CMSRET_SUCCESS;
}


#ifdef DMP_DEVICE2_HOMEPLUG_1
CmsRet cmsImg_setPLCconfigState(const char *state)
{
  if (cmsFil_writeToProc("/data/plc/plc_pconfig_state", state) != CMSRET_SUCCESS)
  {
    cmsLog_error("[PLC] Can't write the paramconfig state\n");
    return CMSRET_INTERNAL_ERROR;
  }

  return CMSRET_SUCCESS;
}
#endif


// depending on the image type, do the brcm image or whole flash image
CmsRet flashImage(char *imagePtr, UINT32 imageLen, CmsImageFormat format, UINT32 opts)
{
    FILE_TAG *pTag = (FILE_TAG *) imagePtr;
    int cfeSize, rootfsSize, kernelSize, noReboot;
    unsigned long cfeAddr, rootfsAddr, kernelAddr;
    CmsRet ret;

    if( opts & CMS_IMAGE_WR_OPT_PART1 )
    {
        noReboot = (opts & CMS_IMAGE_WR_OPT_NO_REBOOT)
            ? FLASH_PART1_NO_REBOOT : FLASH_PART1_REBOOT;
    }
    else if( opts & CMS_IMAGE_WR_OPT_PART2 )
    {
        noReboot = (opts & CMS_IMAGE_WR_OPT_NO_REBOOT)
            ? FLASH_PART2_NO_REBOOT : FLASH_PART2_REBOOT;
    }
    else
    {
        noReboot = (opts & CMS_IMAGE_WR_OPT_NO_REBOOT)
            ? FLASH_PARTDEFAULT_NO_REBOOT : FLASH_PARTDEFAULT_REBOOT;
    }

    cmsLog_debug("format=%d noReboot=0x%x", format, noReboot);

    if (format != CMS_IMAGE_FORMAT_FLASH && format != CMS_IMAGE_FORMAT_BROADCOM)
    {
       cmsLog_error("invalid image format %d", format);
       return CMSRET_INVALID_IMAGE;
    }

#ifdef DMP_DEVICE2_HOMEPLUG_1
    cmsImg_setPLCconfigState("2");
#endif

    /* disable printk with interrupt enabled feature to make sure that progressing print didn't get preempt out */
    cmsFil_writeToProc("/proc/sys/kernel/printk_with_interrupt_enabled", "0");

    if (format == CMS_IMAGE_FORMAT_FLASH)  
    {
#ifndef SKY
        cmsLog_notice("Flash whole image...");
        // Pass zero for the base address of whole image flash. It will be filled by kernel code
        // was sysFlashImageSet
        ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_WRITE,
                                BCM_IMAGE_WHOLE,
                                imagePtr,
                                imageLen-TOKEN_LEN,
                                noReboot, 0);
#else // SKY
		/* Murthy: 26/03/2013: not sure why we need flashOffset, this was added by RED embedded.
		  at the moment to keep the changes minimal , retain this as it is
		*/
		
		SINT32 flashOffSet = sky_getFlashOffset(imagePtr, imageLen);
		if(flashOffSet != -1) {
			cmsLog_debug("Writing WHOLE flash image");
			ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_WRITE,
									BCM_IMAGE_WHOLE,
									imagePtr,
									imageLen-(TOKEN_LEN+SKY_IMG_TAG_LEN),
									flashOffSet, "SKY");
		}
		else
			ret = CMSRET_INTERNAL_ERROR;
#endif // SKY

        if (ret != CMSRET_SUCCESS)
        {
           cmsLog_error("Failed to flash whole image");
           return CMSRET_IMAGE_FLASH_FAILED;
        }
        else
        {
           return CMSRET_SUCCESS;
        }
    }

    /* this must be a broadcom format image */
    // check imageCrc
    cfeSize = rootfsSize = kernelSize = 0;

    // check cfe's existence
    cfeAddr = (unsigned long) strtoul((char *) pTag->cfeAddress, NULL, 10);
    cfeSize = atoi((char *) pTag->cfeLen);
    // check kernel existence
    kernelAddr = (unsigned long) strtoul((char *) pTag->kernelAddress, NULL, 10);
    kernelSize = atoi((char *) pTag->kernelLen);
    // check root filesystem existence
    rootfsAddr = (unsigned long) strtoul((char *) pTag->rootfsAddress, NULL, 10);
    rootfsSize = atoi((char *) pTag->rootfsLen);
    cmsLog_debug("cfeSize=%d kernelSize=%d rootfsSize=%d", cfeSize, kernelSize, rootfsSize);

    if (cfeAddr) 
    {
        printf("Flashing CFE...\n");

        ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_WRITE,
                                BCM_IMAGE_CFE,
                                imagePtr+TAG_LEN,
                                cfeSize,
                                (int) cfeAddr, 0);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to flash CFE");
            return CMSRET_IMAGE_FLASH_FAILED;
        }
    }

    if (rootfsAddr && kernelAddr) 
    {
        char *tagFs = imagePtr;

        // tag is alway at the sector start of fs
        if (cfeAddr)
        {
            tagFs = imagePtr + cfeSize;       // will trash cfe memory, but cfe is already flashed
            memmove(tagFs, imagePtr, TAG_LEN);
			imageLen -= cfeSize;
        }

        printf("Flashing root file system and kernel...\n");
        /* only the buf pointer and length is needed, the offset parameter
         * was present in the legacy code, but is not used. */
        ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_WRITE,
                                BCM_IMAGE_FS,
                                tagFs,
#ifndef SKY
                                TAG_LEN+rootfsSize+kernelSize,
#else
								imageLen-SKY_IMG_TAG_LEN,
#endif
                                noReboot, 0);
        if (ret != CMSRET_SUCCESS)
        {
            cmsLog_error("Failed to flash root file system and kernel");
            return CMSRET_IMAGE_FLASH_FAILED;
        }
    }

    cmsLog_notice("Image flash done.");
    
    return CMSRET_SUCCESS;
}


UINT32 cmsImg_getImageFlashSize(void)
{
   UINT32 flashSize=0;
   CmsRet ret;
   
   ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ,
                           FLASH_SIZE,
                           0, 0, 0, &flashSize);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("Could not get flash size, return 0");
      flashSize = 0;
   }
   
   return flashSize;
}


UINT32 cmsImg_getBroadcomImageTagSize(void)
{
   return TOKEN_LEN;
}


UINT32 cmsImg_getConfigFlashSize(void)
{
   UINT32 realSize;

   realSize = cmsImg_getRealConfigFlashSize();

#ifdef COMPRESSED_CONFIG_FILE   
   /*
    * A multiplier of 2 is now too small for some of the big voice and WLAN configs,   
    * so allow for the possibility of 4x compression ratio.  In a recent test on the
    * 6368 with wireless enabled, I got a compression ratio of 3.5.
    * The real test comes in management.c: after we do the
    * compression, writeValidatedConfigBuf will verify that the compressed buffer can
    * fit into the flash.
    * A 4x multiplier should be OK for small memory systems such as the 6338.
    * The kernel does not allocate physical pages until they are touched.
    * However, allocating an overly large buffer could be rejected immediately by the
    * kernel because it does not know we don't actually plan to use the entire buffer.
    * So if this is a problem on the 6338, we could improve this algorithm to
    * use a smaller factor on low memory systems.
    */
   realSize = realSize * 4;
#endif

   return realSize;
}


UINT32 cmsImg_getRealConfigFlashSize(void)
{
   CmsRet ret;
   UINT32 size=0;

   ret = devCtl_boardIoctl(BOARD_IOCTL_GET_PSI_SIZE, 0, NULL, 0, 0, (void *)&size);
   if (ret != CMSRET_SUCCESS)
   {
      cmsLog_error("boardIoctl to get config flash size failed.");
      size = 0;
   }

   return size;
}


UBOOL8 cmsImg_willFitInFlash(UINT32 imageSize)
{
   UINT32 flashSize;
   
   flashSize = cmsImg_getImageFlashSize();

   cmsLog_debug("flash size is %u bytes, imageSize=%u bytes", flashSize, imageSize);
                     
   return (flashSize > (imageSize + CMS_IMAGE_OVERHEAD));
}


UBOOL8 cmsImg_isBackupConfigFlashAvailable(void)
{
   static UBOOL8 firstTime=TRUE;
   static UBOOL8 avail=FALSE;
   UINT32 size=0;
   CmsRet ret;

   if (firstTime)
   {
      ret = devCtl_boardIoctl(BOARD_IOCTL_GET_BACKUP_PSI_SIZE, 0, NULL, 0, 0, (void *)&size);
      if (ret == CMSRET_SUCCESS && size > 0)
      {
         avail = TRUE;
      }

      firstTime = FALSE;
   }

   return avail;
}

UBOOL8 cmsImg_isConfigFileLikely(const char *buf)
{
   const char *header = "<?xml version";
   const char *dslCpeConfig = "<DslCpeConfig";
   UINT32 len, i;
   UBOOL8 likely=FALSE;
   char *meshObjPos = NULL;

   if (strncmp(buf, "<?xml version", strlen(header)) == 0)
   {
      len = strlen(dslCpeConfig);
      for (i=20; i<50 && !likely; i++)
      {
         if (strncmp(&(buf[i]), dslCpeConfig, len) == 0)
         {
            likely = TRUE;
         }
      }
   }

#ifdef SKY
   if ( likely == TRUE)
   {
      cmsLog_debug("Checking whether Config file Valid for the product...");

      /*VIPER config file will have mesh objects <X_SKY_MESH>. But SR101/SR102  will not have this object.*/
      /*This is only best possible way to identify product in our circutstance as FactoryModel is not allowed to  expose in the config file.*/
      meshObjPos = strstr(buf,"<X_SKY_MESH>");

#ifdef SKY_MESH_SUPPORT
      if(meshObjPos == NULL)
      {
         cmsLog_error("Config file is invalid for the product.");
         likely = FALSE;
      }
#else /* SR101 & SR102.*/
      if(meshObjPos != NULL)
      {
         cmsLog_error("Config file is invalid for the product.");
         likely =  FALSE;
      }
#endif /*SKY_MESH_SUPPORT*/
   }
#endif  /*SKY*/
   cmsLog_debug("returning likely=%d", likely);

   return likely;
}


/** General entry point for writing the image.
 *  The image can be a flash image or a config file.
 *  This function will first determine the image type, which has the side
 *  effect of validating it.
 */
CmsRet cmsImg_writeImage(char *imagePtr, UINT32 imageLen, void *msgHandle)
{
   CmsImageFormat format;
   CmsRet ret;
   
   if ((format = cmsImg_validateImage(imagePtr, imageLen, msgHandle)) == CMS_IMAGE_FORMAT_INVALID)
   {
      ret = CMSRET_INVALID_IMAGE;
   }
   else
   {
      ret = cmsImg_writeValidatedImage(imagePtr, imageLen, format, msgHandle);
   }

   return ret;
}


CmsRet cmsImg_writeValidatedImage(char *imagePtr, UINT32 imageLen,
                                  CmsImageFormat format, void *msgHandle)
{
   if (0xb0 & (UINT32)format)
   {
      /*
       * Prior to 4.14L.01 release, we had some overloaded bits in the
       * format enum.  They have been moved out to CMS_IMAGE_WR_OPT_XXX,
       * but leave some code here to detect old code and usage.
       * If you see this message and you are sure you are using the code
       * correctly, you can ignore this message or delete it.
       */
      cmsLog_error("suspicious old bits set in format enum, format=%d (0x%x)",
                   format, format);
   }

   return (cmsImg_writeValidatedImageEx(imagePtr, imageLen,
                                        format, msgHandle, 0));
}


CmsRet cmsImg_writeValidatedImageEx(char *imagePtr, UINT32 imageLen,
                                    CmsImageFormat format, void *msgHandle,
                                    UINT32 opts)
{
   CmsRet ret=CMSRET_SUCCESS;

   switch(format)
   {
   case CMS_IMAGE_FORMAT_BROADCOM:
   case CMS_IMAGE_FORMAT_FLASH:
	  cmsLog_debug("Will write image type [%d]", format);
      ret = flashImage(imagePtr, imageLen, format, opts);
      break;
      
   case CMS_IMAGE_FORMAT_XML_CFG:
      ret = sendConfigMsg(imagePtr, imageLen, msgHandle, CMS_MSG_WRITE_CONFIG_FILE);
      if (ret == CMSRET_SUCCESS)
      {
         /*
          * Emulate the behavior of the driver when a flash image is written.
          * When we write a config image, also request immediate reboot
          * because we don't want to let any other app save the config file
          * to flash, thus wiping out what we just written.
          */
         cmsLog_debug("config file download written, request reboot");
         cmsUtil_sendRequestRebootMsg(msgHandle);
      }
      break;
      
#ifdef DMP_X_BROADCOM_COM_MODSW_LINUXPFP_1
   case CMS_IMAGE_FORMAT_MODSW_LINUXPFP:
   {
      ret = cmsImg_writeValidatedLinuxPfp(imagePtr, imageLen, msgHandle);
      break;
   }
#endif

   default:
       cmsLog_error("Unrecognized image format=%d", format);
       ret = CMSRET_INVALID_IMAGE;
       break;
    }
   
   return ret;
}

 
CmsImageFormat cmsImg_validateImage(const char *imageBuf, UINT32 imageLen, void *msgHandle)
{
   CmsImageFormat result = CMS_IMAGE_FORMAT_INVALID;
   CmsRet ret;
   
   if (imageBuf == NULL)
   {
      return CMS_IMAGE_FORMAT_INVALID;
   }
   
   if (imageLen > CMS_CONFIG_FILE_DETECTION_LENGTH &&
       cmsImg_isConfigFileLikely(imageBuf))
   {
      cmsLog_debug("possible CMS XML config file format detected");
      ret = sendConfigMsg(imageBuf, imageLen, msgHandle, CMS_MSG_VALIDATE_CONFIG_FILE);
      if (ret == CMSRET_SUCCESS)
      { 
         cmsLog_debug("CMS XML config format verified.");
         return CMS_IMAGE_FORMAT_XML_CFG;
      }
   } 
   
   cmsLog_debug("not a config file");
   
#ifdef SUPPORT_MODSW_LINUXPFP
   if (cmsImg_isModSwLinuxPfp((unsigned char *) imageBuf, imageLen))
   {
      cmsLog_debug("detected Modular Software Linux Primary Firmware Patch format!");
      return CMS_IMAGE_FORMAT_MODSW_LINUXPFP;
   }

   cmsLog_debug("not a ModSW Linux PFP");
#endif

#if defined(SKY_EXTENDER) || defined(CONFIG_SKY_ETHAN)

   if ((imageLen > sizeof(FILE_TAG)) 
        && (verifyBroadcomFileTag((FILE_TAG *) imageBuf, imageLen) == CMSRET_SUCCESS))
   {
      UINT32 maxLen;
      
      /* Found a valid Broadcom defined TAG record at the beginning of the image */
      cmsLog_debug("Broadcom format verified.");
      maxLen = cmsImg_getImageFlashSize() + cmsImg_getBroadcomImageTagSize();
      if (imageLen > maxLen)
      {
         cmsLog_error("broadcom image is too large for flash, got %u, max %u", imageLen, maxLen);
      }
      else
      {
#ifndef SKY
         result = CMS_IMAGE_FORMAT_BROADCOM;
#else /* SKY */		
		 result = sky_verifyImageTagAndSignature(imageBuf, imageLen);	
#endif /* SKY */				 			 
      }
   }
   else
#endif // SKY_EXTENDER or ETHAN 
   {

	  /*
	   * [TW]
	   * This is an old legacy way of software update which is used only on SR101 and SR102
	   * VIPER uses the standard broadcom image format and partial update procedures
	   */

	  /* if It is not a Broadcom flash format file.  Now check if it is a
	   * flash image format file.  A flash image format file must have a
	   * CRC at the end of the image.
	   */

	  UINT32 crc = CRC_INITIAL_VALUE;
      UINT32 imageCrc;
      UINT8 *crcPtr;
      
      if (imageLen > TOKEN_LEN)
      {
         crcPtr = (UINT8 *) (imageBuf + (imageLen - TOKEN_LEN));
         /*
          * CRC may not be word aligned, so extract the bytes out one-by-one.
          * Whole image CRC is calculated, then htonl, then written out using
          * fwrite (see addvtoken.c in hostTools).  Because of the way we are
          * extracting the CRC here, we don't have to swap for endieness when
          * doing compares on desktop Linux and modem (?).
          */
         imageCrc = (crcPtr[0] << 24) | (crcPtr[1] << 16) | (crcPtr[2] << 8) | crcPtr[3];
      
         crc = cmsCrc_getCrc32((unsigned char *) imageBuf, imageLen - TOKEN_LEN, crc);      
         if (crc == imageCrc)
         {
            UINT32 maxLen;
          
            cmsLog_debug("Whole flash image format [%ld bytes] verified.", imageLen);
            maxLen = cmsImg_getImageFlashSize();
            if (imageLen > maxLen)
            {
               cmsLog_error("whole image is too large for flash, got %u, max %u", imageLen, maxLen);
            }
            else
            {
#ifndef SKY
               result = CMS_IMAGE_FORMAT_FLASH;
#else /* SKY */		
				result = sky_verifyImageTagAndSignature(imageBuf, imageLen);	
#endif /* SKY */				 			 
            }
         }
         else
         {
#if defined(EPON_SDK_BUILD)
            cmsLog_debug("Could not determine image format [%d bytes]", imageLen);
#else
            cmsLog_error("Could not determine image format [%d bytes]", imageLen);
#endif
            cmsLog_debug("calculated crc=0x%x image crc=0x%x", crc, imageCrc);
         }
      }
   }

   cmsLog_debug("returning image format %d", result);

   return result;
}

CmsRet sendConfigMsg(const char *imagePtr, UINT32 imageLen, void *msgHandle, CmsMsgType msgType)
{
   char *buf=NULL;
   char *body=NULL;
   CmsMsgHeader *msg;
   CmsRet ret;

   if ((buf = cmsMem_alloc(sizeof(CmsMsgHeader) + imageLen, ALLOC_ZEROIZE)) == NULL)
   {
      cmsLog_error("failed to allocate %d bytes for msg 0x%x", 
                   sizeof(CmsMsgHeader) + imageLen, msgType);
      return CMSRET_RESOURCE_EXCEEDED;
   }
   
   msg = (CmsMsgHeader *) buf;
   body = (char *) (msg + 1);
    
   msg->type = msgType;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_SMD;
   msg->flags_request = 1;
   msg->dataLength = imageLen;
   
   memcpy(body, imagePtr, imageLen);

   ret = cmsMsg_sendAndGetReply(msgHandle, msg);
   
   cmsMem_free(buf);
   
   return ret;
}


void cmsImg_sendLoadStartingMsg(void *msgHandle, const char *connIfName)
{
   CmsMsgHeader *msg;
   char *data;
   void *msgBuf;
   UINT32 msgDataLen=0;

   /* for the msg and the connIfName */
   if (connIfName)
   {
      msgDataLen = strlen(connIfName) + 1;
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader) + msgDataLen, ALLOC_ZEROIZE);
   } 
   else
   {
      cmsLog_error("msg without connIfName");
      msgBuf = cmsMem_alloc(sizeof(CmsMsgHeader), ALLOC_ZEROIZE);
   }
   
   msg = (CmsMsgHeader *)msgBuf;
   msg->src = cmsMsg_getHandleEid(msgHandle);
   msg->dst = EID_SMD;
   msg->flags_request = 1;
   msg->type = CMS_MSG_LOAD_IMAGE_STARTING;

   if (connIfName)
   {
      data = (char *) (msg + 1);
      msg->dataLength = msgDataLen;
      memcpy(data, (char *)connIfName, msgDataLen);      
   }
   
   cmsLog_debug("connIfName=%s", connIfName);

   cmsMsg_sendAndGetReply(msgHandle, msg);

   CMSMEM_FREE_BUF_AND_NULL_PTR(msgBuf);

}


void cmsImg_sendLoadDoneMsg(void *msgHandle)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   msg.type = CMS_MSG_LOAD_IMAGE_DONE;
   msg.src = cmsMsg_getHandleEid(msgHandle);
   msg.dst = EID_SMD;
   msg.flags_request = 1;
   
   cmsMsg_sendAndGetReply(msgHandle, &msg);
}


/* using a cmsUtil_ prefix because this can be used in non-upload scenarios */
void cmsUtil_sendRequestRebootMsg(void *msgHandle)
{
   CmsMsgHeader msg = EMPTY_MSG_HEADER;

   msg.type = CMS_MSG_REBOOT_SYSTEM;
   msg.src = cmsMsg_getHandleEid(msgHandle);
   msg.dst = EID_SMD;
   msg.flags_request = 1;

   cmsMsg_sendAndGetReply(msgHandle, &msg);
}


CmsRet cmsImg_saveIfNameFromSocket(SINT32 socketfd, char *connIfName)
{
  
   SINT32 i = 0;
   SINT32 fd = 0;
   SINT32 numifs = 0;
   UINT32 bufsize = 0;
   struct ifreq *all_ifr = NULL;
   struct ifconf ifc;
   struct sockaddr local_addr;
   socklen_t local_len = sizeof(struct sockaddr_in);

   if (socketfd < 0 || connIfName == NULL)
   {
      cmsLog_error("cmsImg_saveIfNameFromSocket: Invalid parameters: socket=%d, connIfName=%s", socketfd, connIfName);
      return CMSRET_INTERNAL_ERROR;
   }
   memset(&ifc, 0, sizeof(struct ifconf));
   memset(&local_addr, 0, sizeof(struct sockaddr));
   
   if (getsockname(socketfd, &local_addr,&local_len) < 0) 
   {
      cmsLog_error("cmsImg_saveIfNameFromSocket: Error in getsockname.");
      return CMSRET_INTERNAL_ERROR;
   }

   /* cmsLog_error("cmsImg_saveIfNameFromSocket: Session comes from: %s", inet_ntoa(((struct sockaddr_in *)&local_addr)->sin_addr)); */
   
   if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
   {
      cmsLog_error("cmsImg_saveIfNameFromSocket: Error openning socket when getting socket intface info");
      return CMSRET_INTERNAL_ERROR;
   }
   
   numifs = 16;

   bufsize = numifs*sizeof(struct ifreq);
   all_ifr = (struct ifreq *)cmsMem_alloc(bufsize, ALLOC_ZEROIZE);
   if (all_ifr == NULL) 
   {
      cmsLog_error("cmsImg_saveIfNameFromSocket: out of memory");
      close(fd);
      return CMSRET_INTERNAL_ERROR;
   }

   ifc.ifc_len = bufsize;
   ifc.ifc_buf = (char *)all_ifr;
   if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) 
   {
      cmsLog_error("cmsImg_saveIfNameFromSocket: Error getting interfaces\n");
      close(fd);
      cmsMem_free(all_ifr);
      return CMSRET_INTERNAL_ERROR;
   }

   numifs = ifc.ifc_len/sizeof(struct ifreq);
   /* cmsLog_error("cmsImg_saveIfNameFromSocket: numifs=%d\n",numifs); */
   for (i = 0; i < numifs; i ++) 
   {
	  /* cmsLog_error("cmsImg_saveIfNameFromSocket: intface name=%s\n", all_ifr[i].ifr_name); */
	  struct in_addr addr1,addr2;
	  addr1 = ((struct sockaddr_in *)&(local_addr))->sin_addr;
	  addr2 = ((struct sockaddr_in *)&(all_ifr[i].ifr_addr))->sin_addr;
	  if (addr1.s_addr == addr2.s_addr) 
	  {
	      strcpy(connIfName, all_ifr[i].ifr_name);
	  	break;
	  }
   }

   close(fd);
   cmsMem_free(all_ifr);

   cmsLog_debug("connIfName=%s", connIfName);

   return CMSRET_SUCCESS;
   
}


UBOOL8 cmsImg_isBcmTaggedImage(const char *imageBuf, UINT32 *imageSize)
{
   FILE_TAG *pTag = (FILE_TAG *)imageBuf;
   UINT32 crc;
   UINT32 tokenCrc;
   UBOOL8 isBcmImg = FALSE;
         
   tokenCrc = *((UINT32 *)pTag->tagValidationToken);
                 
   /* check tag validate token to see if it is a valid bcmTag */
   crc = CRC_INITIAL_VALUE;
   crc = cmsCrc_getCrc32((UINT8 *) pTag, (UINT32)TAG_LEN-TOKEN_LEN, crc);
   if (crc == tokenCrc)
   {
      isBcmImg = TRUE;
      *imageSize = (UINT32) atoi((char *)pTag->totalImageLen) + TAG_LEN;
      cmsLog_debug("It's a broadcom tagged image with length %d", *imageSize );
   }
   else
   {
      cmsLog_debug("token crc failed, this is not a valid broadcom image");
   }

   
   return isBcmImg;
   
}

