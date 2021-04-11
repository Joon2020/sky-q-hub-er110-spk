/*!****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: sky_public_utils.c
*
* PROJECT: SkyHub
*
* MODULE: cms_util
*
* Date Created: 26/03/2013
*
* Description:  This file includes  functions created by sky. This is part of cms_util library. This file contains sky specific functions
*
* Notes:
*
*****************************************************************************/
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
#include "sky_public_utils.h"

#include "bcmTag.h" /* in shared/opensource/include/bcm963xx, for FILE_TAG */
#include "board.h" /* in bcmdrivers/opensource/include/bcm963xx, for BCM_IMAGE_CFE */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bcm_hwdefs.h" /* in bcmdrivers/opensource/include/bcm963xx, for public key definitions */
#include "bigd.h"
#include "tomcrypt.h"
#define SIG_SIZE_FIELD_SIZE		4
#define SIG_HEADER_FIELD_SIZE	4
#define MK_SIGNATURE_HEADER		0x47534B4D
#define MK_PUBLIC_KEY_HEADER	0x4B504B4D

static int perform_sig_check(BIGD modulus, BIGD signature, uint8_t *data, unsigned int size);
extern CmsRet sendConfigMsg(const char *imagePtr, UINT32 imageLen, void *msgHandle, CmsMsgType msgType);



UBOOL8 cmsImg_containsCfeAddresses(const char *imageBuff) {
	PFILE_TAG pTag = (PFILE_TAG)imageBuff;
	UBOOL8 ret = FALSE;

	if (NULL == imageBuff) {
		return ret;
	}

	if (strtoul(pTag->cfeAddress, NULL, 10) && 
			strtoul(pTag->cfeLen, NULL, 10)) {
		cmsLog_debug("Image contains CFE address information");
		ret = TRUE;
	}

	return ret;
}

/*!*************************************************************************
* NAME: UBOOL8 cmsImg_isPartialSkyImage(const char *imageBuf, UINT32 imageLen )
*
* Description: This function checks  a SKY_IMAGE tag and verifies that this is SkyHub Partial Image 
*	
*
* INPUT:  
*	const char *imageBuf: pointer to image buffer
*	UINT32 imageLen: length of the image
*
* OUTPUT:
*	None
*
* RETURN:
*	UBOOL8 - Returns TRUE if image is SKY_IHR_PARTIAL_IMAGE, else FALSE.
*
* ADDITIONAL NOTES:
*
**************************************************************************/
UBOOL8 cmsImg_isPartialSkyImage(const char *imageBuf, UINT32 imageLen )
{
	UBOOL8 SkyImageSizeType= FALSE; 
	if (imageBuf != NULL)
    {                   
    	int imageSize = imageLen - (SKY_IMG_TAG_LEN+TOKEN_LEN);
        SKY_IHR_IMAGE_TAG skyImg;
        memset(&skyImg, 0, sizeof(skyImg));
		
        memcpy(&skyImg, imageBuf + imageSize, sizeof(skyImg));
		
		/* Murthy We dont need to differentiate using IHR /SR-102/etc  whole or partial image.
		This was necessary when chipid verification was not present
		With Chip id verification added, we just need to differentiate it as partial image or whole image

		so wwe shall use SKY_IHR_PARTIAL_IMAGE /SKY_IHR_WHOLE_IMAGE  as  the parameter to differentiate
		partial or whole image
		*/
		
		if(skyImg.imageType == SKY_IHR_PARTIAL_IMAGE)
       		SkyImageSizeType= TRUE;  	                
		else
		   cmsLog_error("Image is not SKY_IHR_PARTIAL_IMAGE - %d",skyImg.imageType);
   }
	
   cmsLog_debug("returning SkyImageSizeType=%d", SkyImageSizeType);
   return SkyImageSizeType;
}

/*!*************************************************************************
* NAME: CmsImageFormat sky_verifyImageTagAndSignature(const char *imageBuf, UINT32 imageLen)
*
* Description: This function verifies CRC, SKY_IMAGE_TAG and SIGNATURE from Sky Image. 
*	
*
* INPUT:  
*	const char *imageBuf: pointer to image buffer
*	UINT32 imageLen: length of the image
*
* OUTPUT:
*	None
*
* RETURN:
*	CmsImageFormat :   CMS_IMAGE_FORMAT_INVALID -CRC, SKY_IMAGE_TAG and SIGNATURE validation fails
*					   CMS_IMAGE_FORMAT_FLASH - If validation is successful
*
* ADDITIONAL NOTES:
*
**************************************************************************/

CmsImageFormat sky_verifyImageTagAndSignature(const char *imageBuf, UINT32 imageLen)
{
	unsigned int signed_image_size;
	unsigned int modulus_size;
	unsigned int signature_struct_size;
	unsigned int signature_data_size;
	int signature_header;
    BIGD modulus;
    BIGD signature;
    unsigned char pub_key[PUBLIC_KEY_LENGTH];	
    unsigned int ioctl_ret;
	CmsImageFormat result = CMS_IMAGE_FORMAT_INVALID;

	
	/*  Fix for SR-851:  Verify the chip id  available in WFI tag, this is to make sure that SkyHub 
	      Doesnt flash Sky VDSL image	  and vice versa
	  */
	WFI_TAG wfiTag;
    UINT32 tagChipId = 0;
    UINT32 boardChipId;
	
	//copy wif tag which is the last structure in the image
	memcpy(&wfiTag, imageBuf + (imageLen - TOKEN_LEN), TOKEN_LEN);

	tagChipId = (UINT32)wfiTag.wfiChipId;
	
	 /* get the system's chip id */
    devCtl_getChipId(&boardChipId);
    if (tagChipId == boardChipId)
    {
	    cmsLog_debug("Chip Id Match ...,  Board Chip Id = %04x.", boardChipId);
    }
    else
    {
        cmsLog_error("Chip Id MisMatch..., Image Chip Id = %04x, Board Chip Id = %04x.", tagChipId, boardChipId);
        return CMS_IMAGE_FORMAT_INVALID;
    }
				
	/*  validate NVRAM version, this is to make sure that the image will not
		 be overlapped with meta_data_blk. 
		 But we do allow it to overlap in case if the nvram versions are different
		 In such case  firmware will recreate the meta_data_blk once it starts booting.
	
	*/
	unsigned int sigOffset;
	int imageSize = imageLen - (SKY_IMG_TAG_LEN+TOKEN_LEN);
	SKY_IHR_IMAGE_TAG skyImg;

	memcpy(&skyImg, imageBuf + imageSize, sizeof(skyImg));		
							
	cmsLog_debug("imgNvramVer = %d, curNvramVer = %d \n", skyImg.nvRamVersion, NVRAM_VERSION_NUMBER);
	if (NVRAM_VERSION_NUMBER == skyImg.nvRamVersion)
	{
		cmsLog_debug("NVRAM version is same, Enforce New Image	to fit before NVRAM sector \n");
		if(imageLen > NVRAM_SECTOR*64*1024)
		{
			cmsLog_error("New Image is Overlapping with Existing NVRAM sector.\n"); 					
			cmsLog_error("Resize your Image to less than %d  OR  Change the NVRAM version if the memory mapping is changed\n",NVRAM_SECTOR*64*1024);						
			return CMS_IMAGE_FORMAT_INVALID;
		}
	}
	else
		cmsLog_debug("NVRAM versions are different Dont enforce Image to fit before NVRAM sector \n");

#ifdef SKY_EXTENDER
    /* Shashank - 09/05/2014 - Added new ImageType for Ethan Extender */    
	if(skyImg.routerModel != (unsigned long)ROUTER_MODEL_EE120)
	{
		cmsLog_error("Image Mismatch, Router Model is not ROUTER_MODEL_EE120, model - %d", skyImg.routerModel);
		return CMS_IMAGE_FORMAT_INVALID;
	}
#elif defined (CONFIG_SKY_ETHAN) && !defined(SKY_EXTENDER)
	/* Murthy:  23/10/2013   ER-5: Adding new Imagetype for ETHAN ROUTER image */
	if(skyImg.routerModel != (unsigned long)ROUTER_MODEL_ETHAN)
	{
		cmsLog_error("Image Mismatch, Router Model is not ROUTER_MODEL_ETHAN, model - %d", skyImg.routerModel);
		return CMS_IMAGE_FORMAT_INVALID;
	}
#elif defined (CONFIG_SKY_VDSL) && !defined(CONFIG_SKY_ETHAN)
	if(skyImg.routerModel != (unsigned long)ROUTER_MODEL_SR102)
	{
		cmsLog_error("Image Mismatch, Router Model is not ROUTER_MODEL_SR102");
		return CMS_IMAGE_FORMAT_INVALID;
	}
#else
	if(skyImg.routerModel != (unsigned long)ROUTER_MODEL_SR101)
	{
		cmsLog_error("Image Mismatch, Router Model is not ROUTER_MODEL_SR101");
		return CMS_IMAGE_FORMAT_INVALID;
	}
#endif //SKY_EXTENDER
	
					 
	// CRC check done, now for signature check
	 // initialise BIGDs
	 modulus	= bdNew();
	 signature	= bdNew();
	
	 // read public key from flash	
	 ioctl_ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ,
	 								PUBLIC_KEY,
	 								(char *)&pub_key[0], 
	 								PUBLIC_KEY_LENGTH,
	 								0, 0);
	 if (ioctl_ret != CMSRET_SUCCESS)
	 {
		cmsLog_error("Could not get public key");
		bdFree(&signature);
		bdFree(&modulus);
		return CMS_IMAGE_FORMAT_INVALID;
	 }
					
	 if (MK_PUBLIC_KEY_HEADER != *(unsigned int*)(&pub_key[0]))
	 {
		 cmsLog_error("Invalid public key in flash\n");
		 bdFree(&signature);
		 bdFree(&modulus);
		 return CMS_IMAGE_FORMAT_INVALID;
	 }
					
	 // read modulus size from image
	 modulus_size = *(unsigned int*)((&pub_key[0]) + sizeof(unsigned int));
		
	 // get modulus into a BIGD
	 (void)bdConvFromOctets(modulus,(&pub_key[0]) + sizeof(unsigned int) + sizeof(unsigned int), modulus_size);
					
	 // read signature header first
	 signature_header = *(int*)((imageBuf + (imageLen - (TOKEN_LEN + SKY_IMG_TAG_LEN + SIG_SIZE_FIELD_SIZE))));
	
	 if (MK_SIGNATURE_HEADER != signature_header)
	 {
		cmsLog_error("INVALID SIGNATURE HEADER\n");
		bdFree(&signature);
		bdFree(&modulus);
		return CMS_IMAGE_FORMAT_INVALID;
	}
					
	// read signature structure size from image
	signature_struct_size = *(unsigned int*)((imageBuf + (imageLen - 
				(TOKEN_LEN + SKY_IMG_TAG_LEN+ SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE))));
					
	// read signature data size
	signature_data_size = *(unsigned int*)((imageBuf + (imageLen - 
				(TOKEN_LEN + SKY_IMG_TAG_LEN+ SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE + signature_struct_size))));

	cmsLog_debug("%08x %08x %08x\n", signature_struct_size, signature_data_size, *(unsigned int *)((imageBuf + imageLen) - 
				(TOKEN_LEN + SKY_IMG_TAG_LEN + SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE + (signature_struct_size - SIG_SIZE_FIELD_SIZE))));

	if (signature_data_size > signature_struct_size) {
		cmsLog_error("SIGNATURE IS CORRUPTED [%08x/%08x]\n", 
				signature_struct_size, signature_data_size);
		bdFree(&signature);
		bdFree(&modulus);
		return CMS_IMAGE_FORMAT_INVALID;
	}

// it shouldn't be ever bigger than 1kB (it's ~140 bytes currently)
#define MAX_SIGNATURE_TOTAL_LEN 1024

	if ((signature_data_size > MAX_SIGNATURE_TOTAL_LEN) ||
			(signature_struct_size > MAX_SIGNATURE_TOTAL_LEN)) {
		cmsLog_error("SIGNATURE IS TOO BIG (PROBABLY CORRUPTED) [%08x/%08x]\n", 
				signature_struct_size, signature_data_size);
		bdFree(&signature);
		bdFree(&modulus);
		return CMS_IMAGE_FORMAT_INVALID;
	}
#undef MAX_SIGNATURE_TOTAL_LEN 
					
	// get signature into a BIGD
	(void)bdConvFromOctets(signature,(const unsigned char *)((imageBuf + imageLen) - 
				(TOKEN_LEN + SKY_IMG_TAG_LEN + SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE + (signature_struct_size - SIG_SIZE_FIELD_SIZE))),
					signature_data_size);
					
					
	if(skyImg.imageType == SKY_IHR_WHOLE_IMAGE)
	{
#if defined(SKY_EXTENDER) || defined(CONFIG_SKY_ETHAN)
		// ommit the cfe and the FILE_TAG
		PFILE_TAG pTag = (PFILE_TAG)imageBuf;
		sigOffset = TAG_LEN + strtoul(pTag->cfeLen, NULL, 10);
		signed_image_size = imageLen - (sigOffset + TOKEN_LEN + SKY_IMG_TAG_LEN + SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE + signature_struct_size);
#else
		// legacy format, left for compatibility

		CmsRet ret=CMSRET_SUCCESS;
		unsigned int sectorSize = 0;

		// whole  Image contains CFE and Serialization data sectors, so exclude them for sig check
		/* start writing image after CFE & serialisation sectors */
		ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ,
								SECTOR_SIZE,
								0, 0, 0, &sectorSize);
										   
		if (ret != CMSRET_SUCCESS)
		{
			cmsLog_error("Failed to read flash sector size");
			bdFree(&signature);
			bdFree(&modulus);
			return CMS_IMAGE_FORMAT_INVALID;
		}
	
#if (CONFIG_FLASH_CHIP_SIZE==8)
		sigOffset = sectorSize * 2;
#elif (CONFIG_FLASH_CHIP_SIZE==16)
		sigOffset = sectorSize * 4;
#elif (CONFIG_FLASH_CHIP_SIZE==32)
		sigOffset = sectorSize * 8;
#else
#error Flash chip unsupported (must be either 8, 16, 32)
#endif					

		signed_image_size = imageLen - (sigOffset+TOKEN_LEN + SKY_IMG_TAG_LEN+ SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE + signature_struct_size);
#endif
	}
	else
	{
		// partial Image doesnt contain CFE and Serialization data sectors
#if defined(SKY_EXTENDER) || defined(CONFIG_SKY_ETHAN)
		 sigOffset = TAG_LEN;
		 signed_image_size = imageLen - (TAG_LEN + TOKEN_LEN + SKY_IMG_TAG_LEN+ SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE + signature_struct_size);
#else
		 sigOffset = 0;
		 signed_image_size = imageLen - (TOKEN_LEN + SKY_IMG_TAG_LEN+ SIG_HEADER_FIELD_SIZE + SIG_SIZE_FIELD_SIZE + signature_struct_size);
#endif
	}
					 
	if (0 == perform_sig_check(modulus, signature, (uint8_t *)imageBuf+sigOffset, signed_image_size))
	{
		cmsLog_debug("\nSIGNATURE OK\n");

#if defined(SKY_EXTENDER) || defined(CONFIG_SKY_ETHAN)
		result = CMS_IMAGE_FORMAT_BROADCOM;
#else
		result = CMS_IMAGE_FORMAT_FLASH;
#endif

	}
	else
	{
		cmsLog_error("\nINVALID SIGNATURE\n");
		bdFree(&signature);
		bdFree(&modulus);
		return CMS_IMAGE_FORMAT_INVALID;
	}
					
	// free BIGDs
	bdFree(&signature);
	bdFree(&modulus);	
	return result;

}

/*!*************************************************************************
* NAME: SINT32 sky_getFlashOffset(char *imagePtr, UINT32 imageLen)
*
* Description: This function gets the offset of the rootfs  form the beginning of flash
*	if the flash is 8MB then the offset will be 2*size of flash sector
*     if the flash is 16MB then the offset will be 4*size of flash sector
*
* INPUT:  
*	char *imagePtr: pointer to image buffer
*	UINT32 imageLen: length of the image
*
* OUTPUT:
*	None
*
* RETURN:
*	SINT32: offset from beginning of Flash
*
* ADDITIONAL NOTES:
*
**************************************************************************/

SINT32 sky_getFlashOffset(char *imagePtr, UINT32 imageLen)
{
	unsigned int  sectorSize = 0;
	SINT32 sectorOffset = -1;
	unsigned int ret = 0;
	SKY_IHR_IMAGE_TAG skyImg;
	int imageSize = imageLen - (SKY_IMG_TAG_LEN+TOKEN_LEN);
  //      sky_cms_getRouterModel();			
	cmsLog_notice("Flashing  image...\n");
	memcpy(&skyImg, imagePtr + imageSize, sizeof(skyImg));
	cmsLog_debug("skyImg.imageType = %d,  skyImg.nvramversion = %d\n", skyImg.imageType, skyImg.nvRamVersion);

	if(skyImg.imageType == SKY_IHR_PARTIAL_IMAGE)
	{				
		ret = devCtl_boardIoctl(BOARD_IOCTL_FLASH_READ,
							   SECTOR_SIZE,
							   0, 0, 0, &sectorSize);
		if (ret != CMSRET_SUCCESS)
		{
			cmsLog_error("Failed to read flash sector size");
			return -1;
		}
		else
		{		
#if (CONFIG_FLASH_CHIP_SIZE==8)
			sectorOffset = sectorSize * 2;
#elif (CONFIG_FLASH_CHIP_SIZE==16)
			sectorOffset = sectorSize * 4;
#elif (CONFIG_FLASH_CHIP_SIZE==32)
			sectorOffset = sectorSize * 8;
#else
#error Flash chip unsupported (must be either 8, 16, 32)
#endif
		}
	}
        else if(skyImg.imageType == SKY_IHR_WHOLE_IMAGE) {
             cmsLog_notice("skyImg.imageType = %d\n", skyImg.imageType);
             /*Full Image should start Write from sector 0  */
             sectorOffset = 0;
        }
	return sectorOffset;
}

CmsImageFormat cmsImg_validateConfigFile(const char *imageBuf, UINT32 imageLen, void *msgHandle)
{
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
         cmsLog_debug("sending validate config file message success");
         return CMS_IMAGE_FORMAT_XML_CFG;
      }
      else
      {
         cmsLog_debug("sending validate config file message failed");
         return CMS_IMAGE_FORMAT_INVALID;
      }
   }
   else
   {
      cmsLog_debug("CMS XML config file format not detected");
      return CMS_IMAGE_FORMAT_INVALID;
   }
}

#if 0
CmsRet sendStartFlashWriteMsg(const char *imagePtr, UINT32 imageLen,UINT32 ptrLen, void *msgHandle, CmsMsgType msgType)
{
	char *buf=NULL;
	char *body=NULL;
	CmsMsgHeader *msg;
	CmsRet ret;
	UINT32 msgLen = 2*ptrLen;

	if ((buf = cmsMem_alloc(sizeof(CmsMsgHeader) + msgLen, ALLOC_ZEROIZE)) == NULL)
	{
		cmsLog_error("failed to allocate %d bytes for msg 0x%x", 
				sizeof(CmsMsgHeader) + imageLen, msgType);
		return CMSRET_RESOURCE_EXCEEDED;
	}

	msg = (CmsMsgHeader *) buf;
	body = (char *) (msg + 1);

	msg->type = msgType;
	msg->src = cmsMsg_getHandleEid(msgHandle);
	msg->dst = EID_SKYTEST;
	msg->flags_request = 1;
	msg->dataLength = msgLen;
	printf("\n Sending data to write \n");
	memcpy(body, imagePtr, sizeof(UINT32));
	memcpy(body+sizeof(UINT32), &imageLen, sizeof(UINT32));
	ret = cmsMsg_send(msgHandle, msg);
	cmsMem_free(buf);
	return ret;
}
#endif


static int perform_sig_check(BIGD modulus, BIGD signature, uint8_t *data, unsigned int size)
{
	BIGD 			exponent; 
	BIGD 			message_hash; 
	BIGD 			derived_message_hash; 
	hash_state 		state;
	unsigned char out[32];
	int res;
	
	/* Initialise BIGDs */
	exponent 				= bdNew();
	message_hash 			= bdNew();
	derived_message_hash 	= bdNew();
	
	/* setup exponent to match what was used in generation */
	bdSetShort(exponent, 65537);
	
	/* init hash algorithm */
	sha256_init(&state);
	
	/* process hash of given data */
	sha256_process(&state, data, size);
		
	/* stick results of hash into out buffer */
	sha256_done(&state, out);
	
	/* get hash data into message_hash */
	(void)bdConvFromOctets(message_hash, &out[0], sizeof(out));
		
	/* derive message hash value, ie: m = s^e mod n */
	bdModExp(derived_message_hash, signature, exponent, modulus);
		
	/* now compare the hash value derived from signature with hash calculated from binary */
	res = bdCompare(derived_message_hash, message_hash);
	
	/* free BIGDs */
	bdFree(&derived_message_hash);
	bdFree(&message_hash);
	bdFree(&exponent);
	
	return res;
}
/* This is generic function used for power led control  */
void generic_led_update(unsigned int led_state,int which_led)
{
    int led_action=0;
    BOARD_IOCTL_PARMS IoctlParms = {0};
    static int board_led = 0;
    board_led = open("/dev/brcmboard", 00000002);  // RDWR=02 ,WR 01 RD 00

    if (board_led <= 0)
    {
        printf("Open /dev/brcmboard failed!\n");
        return ;
    }
    cmsLog_debug("\n led_stat=%d and Which led=%d\n",led_state,which_led);	
    led_action|=(led_state & 0xffffffff);
    led_action|=((which_led << 8) & 0xff00);	
    IoctlParms.result = -1;
    IoctlParms.string = (char *)&led_action;
    IoctlParms.strLen = sizeof(led_action);
   ioctl(board_led, BOARD_IOCTL_SET_PWR_LED, &IoctlParms);
    close(board_led);
   return;
}

