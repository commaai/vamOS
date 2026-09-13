/*
 * File: moduleid.h
 * Description: Define module id
 */
/**
 *@file moduleid.h
 *@author Gene Hung
 *@version 2005/08/22; Gene; Add Doxygen remark
 *@defgroup ModuleID Module ID definition
 *@brief TBirdOS 2.0 and later module ID definition.
 *code number definition:
 *Bits 31~20: Module id
 *Bits 19~12: Reserved
 *Bits 11~0: Code defined in each module
 */

#ifndef _MODULEID_H_
#define _MODULEID_H_

/**
 *@ingroup ModuleID
 *@{
 */

/**
 *@def MODULEID_SHIFTBITS
 *@brief Module ID MARCO definition
 */
#define MODULEID_SHIFTBITS 20
/**
 *@def MODULEID_ModuleID
 *@brief Get ID number from a CODE
 */
#define MODULEID_ModuleID(code) (code >> MODULEID_SHIFTBITS)
/**
 *@def MODULEID_ModuleBase
 *@brief Get CODE BASE from a module ID
 */
#define MODULEID_ModuleBase(id) (id << MODULEID_SHIFTBITS)

/* Project-dependent module starts from this ID*/
#define MODULEID_PROJECT 0x400


/* Let Project use MODULEID_PROJECT to extend. */
/*Don't define module ID 0x401 0x402... here.*/

/**
 *@}
 */
#endif /*_MODULEID_H_*/
