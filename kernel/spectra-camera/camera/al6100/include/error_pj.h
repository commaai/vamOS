/*
 * File: error_pj.h
 * Description: Error code base of modules
 */
#ifndef _ERROR_PJ_H_
#define _ERROR_PJ_H_

#include "error.h"
#include "moduleid_pj.h"

/**
 *@ingroup ErrorCode
 *@{
 */

/* The following constants define the module ID and the error code base*/
#define ERR_BASE_PJ_MINIISP_STATE	   ERR_BASE(MODULEID_PJ_MINIISP_STATE)
#define ERR_BASE_PJ_ISPCTRLIF_SLAVE	 ERR_BASE(MODULEID_PJ_ISPCTRLIF_SLAVE)
#define ERR_BASE_PJ_ISPCTRLIF_MASTER	ERR_BASE(MODULEID_PJ_ISPCTRLIF_MASTER)
#define ERR_BASE_PJ_MINIISP			 ERR_BASE(MODULEID_PJ_MINIISP)



#endif
