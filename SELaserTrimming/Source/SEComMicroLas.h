/*----------------------------------------------------------------------------------------------------------*
 *  modulname       : SEComMicroLas.h                                                                       *
 *----------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                      *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                             *
 *----------------------------------------------------------------------------------------------------------*
 *  description:                                                                                            *
 *  ------------                                                                                            *
 *  Header file for SEComMicroLas.                                                                          *
 *----------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                          *
 * ---------|------------|----------------|---------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                               *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                    *
 *----------------------------------------------------------------------------------------------------------*/

#pragma once

#ifndef _SE_COM_MICRO_LAS
  //---------------------------------------------------------------------------------------------------------
  #define _SE_COM_MICRO_LAS
  //---------------------------------------------------------------------------------------------------------
  #ifdef _SE_COM_MICRO_LAS_INTERNAL
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
  #ifdef _SE_COM_MICRO_LAS_INTERNAL
    #define DLL_DECL		DLL_EXPORT
    #define CALLTYPE		__cdecl
  #else
    #define DLL_DECL		DLL_IMPORT
    #define CALLTYPE		__cdecl
  #endif
  
//-- Global defines -----------------------------------------------------------------------------------------
#define PORT_MICRO_LAS_CHAMBER 19553																			//microlas port
#define LOCALHOST "127.0.0.1"																							//microlas ip

#define printf( ... ) dprintf( TRUE, TRUE, ##__VA_ARGS__ );

static __int64 TimeDiff[1000000];
static __int64 TimeDiffType[1000000];
static int TimeDiffIdx = 0;
static bool TimeDiffOnce = false;

//-- Global enum --------------------------------------------------------------------------------------------
  

//-- Global structures --------------------------------------------------------------------------------------



/*-------------------------------------------------------------------------------------------------------------------------*
 * class name           : SEComMicroLas                                                                                    *
 *                                                                                                                         *
 * description:         : This is the base class of microlas scanner system.                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
class SEComMicroLas
{

private:
  //-- members
  int ChamberId;																													//chamber id
  bool SocketConnected;																										//socket connected flag
  SOCKET ClientSocket;																										//client socket
  ParamDataLaser ParamLaser;																							//parameter for microlas scanner
  ParamStationDataLaser ParamStationLaser;																//parameter for microlas scanner

  //-- memberfunctions
  int SendRecv( char Cmd [1024] );																				//send and receive function					

public:
  static HANDLE hThreadHandleComMicroLas[2];									            //thread handle static
  static HANDLE ghMutex; 

  //-- memberfunctions
  SEComMicroLas();																										    //constructor
  SEComMicroLas( int ChamberId );	                                        //constructor (overloaded)
  ~SEComMicroLas();																										    //destructor
  
  int Initializing( void );													                      //initialization
  int SetParameter( ParamStationDataLaser ParamStationLaser );            //set parameter
  int SetParameter( ParamDataLaser ParamLaser );                          //set parameter
  int Connect( void );																										//connect
  int MoveAbs( float X, float Y, float Z );																//move absolute
  //write text
  int WriteText( char Text[255], float X, float Y, float Z, float Angle, float LaserPower );
  //write line
  int WriteLine( float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float LaserPower );
  //process trimming cut
  int TrimmingCut( float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float LaserPower );
  //process laser point
  int LaserPoint( float X, float Y, float Z, float LaserPower, float Time );
  int InstallThread( LPTHREAD_START_ROUTINE CallbackAddress );						//install thread
  bool IsConnected( void );																								//check connection
  void Disconnect( void );																								//disconnect socket
};

//---------------------------------------------------------------------------------------------------------
  #ifndef _SE_COM_MICRO_LAS_INTERNAL
    #undef DLL_DECL
    #undef CALLTYPE
  #endif
  //---------------------------------------------------------------------------------------------------------
  #undef EXTERN
#endif


