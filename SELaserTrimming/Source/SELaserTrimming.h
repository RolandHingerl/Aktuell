/*----------------------------------------------------------------------------------------------------------*
 *  modulname       : SELaserTrimming.h                                                                     *
 *----------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                      *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                             *
 *----------------------------------------------------------------------------------------------------------*
 *  description:                                                                                            *
 *  ------------                                                                                            *
 *  Header file for SELaserTrimming.                                                                        *
 *----------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                          *
 * ---------|------------|----------------|---------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                               *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                    *
 *----------------------------------------------------------------------------------------------------------*/
#pragma once

#ifndef _SE_LASER_TRIMMING
	//---------------------------------------------------------------------------------------------------------
	#define _SE_LASER_TRIMMING
	//---------------------------------------------------------------------------------------------------------
	#ifdef _SE_LASER_TRIMMING_INTERNAL
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
	#ifdef _SE_LASER_TRIMMING_INTERNAL
		#define DLL_DECL		DLL_EXPORT
		#define CALLTYPE		__cdecl
	#else
		#define DLL_DECL		DLL_IMPORT
		#define CALLTYPE		__cdecl
	#endif

//-- Global defines -----------------------------------------------------------------------------------------
#ifdef DEBUG_OUTPUT_FILE
	#define printf( ... ) dprintf( TRUE, TRUE, ##__VA_ARGS__ );
#endif

	#ifdef _SIMULATION
		#define SW_VERSION "SIMU"
	#else
		#ifdef _DEBUG
		 #define SW_VERSION "x.xxDBG"
		#else
		 #define SW_VERSION "x.xx"
		#endif
	#endif

	#define SW_VERSION_WORD 0,00,0,00

//-- Global enum --------------------------------------------------------------------------------------------


//-- Global structures --------------------------------------------------------------------------------------


//-- function prototypes ------------------------------------------------------------------------------------
	//chamber 1
	//fast measurement trimming cell chamber 1
	LONG MeasInFastTrimmingCellChamber1( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx );
	void MeasInTrimmingCellChamber1( void );																//normal measurement trimming cell chamber 1
	//fast measurement reference cell chamber 1
	LONG MeasInFastReferenceCellChamber1( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx );
	void MeasInReferenceCellChamber1( void );																//normal measurement reference cell chamber 1
	void HandleChamber1( void );																						//chamber 1 handling

	//chamber 2
	//fast measurement trimming cell chamber 2
	LONG MeasInFastTrimmingCellChamber2(WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx);
	void MeasInTrimmingCellChamber2( void );																//normal measurement trimming cell chamber 2
	//fast measurement reference cell chamber 2
	LONG MeasInFastReferenceCellChamber2(WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx);
	void MeasInReferenceCellChamber2( void );																//normal measurement reference cell chamber 2
	void HandleChamber2( void );																						//chamber 2 handling

	//socket
	void HandleSocketServerPlcChamber1( void );													    //socket server	plc communication
	void HandleSocketClientMicroLasChamber1( void );											  //socket client microlas communication

	//handle actions
	void HandleAction1Chamber1( void );																			//action 1 handling
	void HandleAction2Chamber1( void );																			//action 2 handling
	void HandleAction3Chamber1( void );																			//action 3 handling
	void HandleAction4Chamber1( void );																			//action 4 handling
	void HandleAction5Chamber1( void );																			//action 5 handling
	void HandleAction6Chamber1( void );	 																		//action 6 handling
	void HandleAction7Chamber1( void );	 																		//action 7 handling
	void HandleAction8Chamber1( void );	 																		//action 8 handling
	void HandleAction9Chamber1( void );	 																		//action 9 handling

	void HandleAction1Chamber2( void );																			//action 1 handling
	void HandleAction2Chamber2( void );																			//action 2 handling
	void HandleAction3Chamber2( void );																			//action 3 handling
	void HandleAction4Chamber2( void );																			//action 4 handling
	void HandleAction5Chamber2( void );																			//action 5 handling
	void HandleAction6Chamber2( void );	 																		//action 6 handling
	void HandleAction7Chamber2( void );	 																		//action 7 handling
	void HandleAction8Chamber2( void );	 																		//action 8 handling
	void HandleAction9Chamber2( void );	 																		//action 9 handling

	void HandleActionDummy( void );	 																				//action dummy handling

	//callback functions
	int ParameterOverTakenChamber1( unsigned int SubId );										//parameter overtaken callback
	void CyclicalDataWriteReadChamber1( void );														  //cyclical data callback
	int ResultReadChamber1( unsigned int SubId, unsigned int Tiepoint );		//result read callback
	void ActionFlagChangedChamber1( void );																	//action flag changed callback

	int ParameterOverTakenChamber2( unsigned int SubId );										//parameter overtaken callback
	void CyclicalDataWriteReadChamber2( void );														  //cyclical data callback
	int ResultReadChamber2( unsigned int SubId, unsigned int Tiepoint );		//result read callback
	void ActionFlagChangedChamber2( void );																	//action flag changed callback

	void CheckStatisticDelete( void );                                      //check delete of statistics
	void HandleCheckStatisticDelete( void );                                //handling check and delete statistic

	//main
	int _tmain(int argc, _TCHAR* argv[]);																		//main function


	//---------------------------------------------------------------------------------------------------------
	#ifndef _SE_LASER_TRIMMING_INTERNAL
		#undef DLL_DECL
		#undef CALLTYPE
	#endif
	//---------------------------------------------------------------------------------------------------------
	#undef EXTERN
#endif






