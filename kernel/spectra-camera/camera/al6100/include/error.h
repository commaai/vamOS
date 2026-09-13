/*
 * File: error.h
 * Description: Error code base of modules
 */
/*
 *@defgroup ErrorCode Error Code Definition
 */

/**
 *@file error.h
 *@brief TBirdOS 2.0 module error code definition.
 *Error code definition:
 *Bits 31~20: Module id
 *Bits 19~12: Reserved
 *Bits 11~0: Error code defined in each module
 *@author Gene Hung
 *@version 2005/08/22; Gene; Add Doxygen remark
 *@see ModuleID
 *@ingroup ErrorCode Error Code Definition
 */

/**
 *@defgroup SysCtrlErr System Control Module Error Code
 *@ingroup ErrorCode
 */

/**
 *@defgroup SysMgrErr System Manager Module Error Code
 *@ingroup ErrorCode
 */

#ifndef _ERROR_H_
#define _ERROR_H_

#ifndef _MODULEID_H_
#include "moduleid.h"
#endif
/**
 *@ingroup ErrorCode
 *@{
 */
#define ERR_MODULEID_SHIFTBITS 20
#define ERR_MODULEID(ErrorCode) ((ErrorCode) >> ERR_MODULEID_SHIFTBITS)
#define ERR_BASE(ModuleId) ((ModuleId) << ERR_MODULEID_SHIFTBITS)

/* No error*/
#define ERR_SUCCESS 0

/* The following constants define the module ID and the error code base*/

#define ERR_BASE_PROJECT ERR_BASE(MODULEID_PROJECT)

/**
 *@}
 */
#endif /*_ERROR_H_*/

