/*-------------------------------------------------------------------------------------------------------------------------*
 *  modulname       : SETrimmingProcess.cpp                                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                                     *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                                            *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  description:                                                                                                           *
 *  ------------                                                                                                           *
 *  This module contains control and evaluation routines for the SE trimming process.                                      *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                                         *
 * ---------|------------|----------------|------------------------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                                              *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/

#include <windows.h>
#include <vector>
#include <list>

#define _SE_TRIMMING_PROCESS_INTERNAL
#include "SETrimmingProcess.h"
#undef _SE_TRIMMING_PROCESS_INTERNAL
#include "..\Library\SETestDLL.h"

//public memberfunctions --------------------------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SETrimmingProcess::SETrimmingProcess()                                                           *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor function.                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
SETrimmingProcess::SETrimmingProcess()
{

}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SETrimmingProcess::~SETrimmingProcess()                                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the destructor function.                                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
SETrimmingProcess::~SETrimmingProcess()
{

}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SETrimmingProcess::Initializing                                                              *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the destructor function.                                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingProcess::Initializing(void)
{	
	int RetVal = 0;																													//return value
	
	return RetVal;
	
}

int SETrimmingProcess::StartSequence(void)
{
	int RetVal = 0;
	
	return RetVal;
}
int SETrimmingProcess::StopSequence(void)
{
	int RetVal = 0;
	
	return RetVal;
}



