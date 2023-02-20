/*
|-----------------------------------------------------------------------------------------------------------|
| company              : Robert BOSCH GmbH                                                                  |
|-----------------------------------------------------------------------------------------------------------|
| projectfile          : OpConServicesCommon.vcproj                                                         |
| modulname            : OpConServicesCommon.h                                                              |
|-----------------------------------------------------------------------------------------------------------|
| development tool     : Microsoft Visual Studio 2005 Prof. Edition                                         |
| editor               : Microsoft Visual Studio 2005 Prof. Edition (TabStop-value = 2)                     |
|-----------------------------------------------------------------------------------------------------------|
| description:                                                                                              |
| ------------                                                                                              |
| - This hearder file contains defines and function for the OpCon common services module.                   |
|-----------------------------------------------------------------------------------------------------------|
|  version | date       | author         | description of changes                                           |
| ---------|------------|----------------|----------------------------------------------------------------- |
|  V xx.yy | dd.mm.yyyy | BaP/TEF3.4x-XX | -                                                                |
|-----------------------------------------------------------------------------------------------------------|*/
#ifndef _OPCON_SERVICES_COMMON
	//---------------------------------------------------------------------------------------------------------
	#define _OPCON_SERVICES_COMMON
	//---------------------------------------------------------------------------------------------------------
	#ifdef _OPCON_SERVICES_COMMON_INTERNAL
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
	#ifdef _OPCON_SERVICES_COMMON_INTERNAL
		#define DLL_DECL		DLL_EXPORT
		#define CALLTYPE		__stdcall
	#else
		#define DLL_DECL		DLL_IMPORT
		#define CALLTYPE		__stdcall
	#endif

	//-- Global defines ---------------------------------------------------------------------------------------


	//-- Failure defines --------------------------------------------------------------------------------------
	#define FAILURE_NO																					0
	#define FAILURE_DPRINTF_INITIALIZE_CONSOLE_ALLOCATE			 -101
	#define FAILURE_DPRINTF_INITIALIZE_LOGFILE_OPEN					 -102
	#define FAILURE_DPRINTF_INITIALIZE_LOGFILE_NAME					 -103
	#define FAILURE_DPRINTF_INITIALIZE_LOGFILE_NAME_OLD			 -104

	//-- Global structures ------------------------------------------------------------------------------------


	//-- Function prototypes ----------------------------------------------------------------------------------
	// Main function
	EXTERN bool WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, PVOID lpvReserved);
	
	// External library functions
	EXTERN DLL_DECL int CALLTYPE dprintfInitialize(BOOL ConsoleOut, BOOL LogfileOut, char *Logfile, long LogfileSize);
	EXTERN DLL_DECL int CALLTYPE dprintf(BOOL ConsoleOut, BOOL LogfileOut, const char *format, ...);
	EXTERN DLL_DECL int CALLTYPE FileVersionGet(char* Version, unsigned int VersionSize, char* File);

	//---------------------------------------------------------------------------------------------------------
	#ifndef _OPCON_SERVICES_COMMON_INTERNAL
		#undef DLL_DECL
		#undef CALLTYPE
	#endif
	//---------------------------------------------------------------------------------------------------------
	#undef EXTERN
#endif
