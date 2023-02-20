/*----------------------------------------------------------------------------------------------------------*
 *  modulname       : SETrimmingProcess.h                                                                   *
 *----------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                      *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                             *
 *----------------------------------------------------------------------------------------------------------*
 *  description:                                                                                            *
 *  ------------                                                                                            *
 *  Header file for SEMeasurementCell.                                                                      *
 *----------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                          *
 * ---------|------------|----------------|---------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                               *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                    *
 *----------------------------------------------------------------------------------------------------------*/

#pragma once
#include <vector>
#include <list>



#ifndef _SE_TRIMMING_PROCESS
	//---------------------------------------------------------------------------------------------------------
	#define _SE_TRIMMING_PROCESS
	//---------------------------------------------------------------------------------------------------------
	#ifdef _SE_TRIMMING_PROCESS_INTERNAL
		#define EXTERN
	#else
		#define EXTERN extern
	#endif
	//---------------------------------------------------------------------------------------------------------
	#ifndef DLL_IMPORT
		#define DLL_IMPORT	__declspec(dllimport)
	#endif
	//---------------------------------------------------------------------------------------------------------
	#ifndef DLL_EXPORT
		#define DLL_EXPORT	__declspec(dllexport)
	#endif		
	//---------------------------------------------------------------------------------------------------------
	#ifdef _SE_TRIMMING_PROCESS_INTERNAL
		#define DLL_DECL		DLL_EXPORT
		#define CALLTYPE		__cdecl
	#else
		#define DLL_DECL		DLL_IMPORT
		#define CALLTYPE		__cdecl
	#endif
	
//-- Global defines -----------------------------------------------------------------------------------------

//-- Global enum --------------------------------------------------------------------------------------------
	

//-- Global structures --------------------------------------------------------------------------------------

	



class SETrimmingProcess
{

private:
	//-- members
	//input members
	ParamDataTrimmingProcess	ParamTrimmProcess;
	
	
	//output members
	
	//-- memberfunctions
	
	
  
  

public:
	//-- memberfunctions
	SETrimmingProcess();																										//constructor
	~SETrimmingProcess();																										//destructor
	
	int Initializing(void);																									//initialization
	
	int StartSequence(void);
	int StopSequence(void);
	
};






//---------------------------------------------------------------------------------------------------------
	#ifndef _SE_TRIMMING_PROCESS_INTERNAL
		#undef DLL_DECL
		#undef CALLTYPE
	#endif
	//---------------------------------------------------------------------------------------------------------
	#undef EXTERN
#endif


