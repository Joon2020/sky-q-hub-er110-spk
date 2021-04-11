/*!****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: sky_routermodel_util.c
*
* PROJECT: SkyHub
*
* MODULE: cms_util
*
* Date Created: 30/05/2013
*
* Description:  This file includes utility functions for sky router model versions. 
*
*
* Notes:
*
*****************************************************************************/
#include <stdio.h>
#include <sky_routermodel_util.h>
/************************************************************************** 
* NAME: SkyRouterModel getRouterModel() 
* 
* Description: This function will return the current router as
	UK/ROI version. 
* 
* INPUT: 
* Input variables: None
* OUTPUT: 
* Output variables:None 
* 
* RETURN: 
* Return Variables:eSkyRouterModel 
* 
* ADDITIONAL NOTES: 
* 
**************************************************************************/ 

SkyRouterModel sky_getRouterModel()
{
	FILE *fp = NULL;
	SkyRouterModel eSkyRouterModel = eSkyUnKnown;	
	int skyModel = -1;

	if( (fp = fopen(SKY_ROUTER_MODEL_FILENAME,"r")) != NULL)	
	{
		if( fscanf(fp,"%d",&skyModel) <= 0)
		{
			perror("\nRouter model cannot read from file");
		}
		else // SUCCESS
		{
			eSkyRouterModel = (SkyRouterModel) skyModel;
		}

		fclose(fp);
	}
	else
	{
		perror("\nRouter model file cannot be opened");
	}

	return eSkyRouterModel;
}
