/*-------------------------------------------------------------------------------------------------------------------------*
 *  modulname       : SEComMicroLas.cpp                                                                                    *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                                     *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                                            *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  description:                                                                                                           *
 *  ------------                                                                                                           *
 *  This module contains control and evaluation routines for the communication with micro las.                             *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                                         *
 * ---------|------------|----------------|------------------------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                                              *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/

#include <Windows.h>
#include <conio.h>
#include <stdio.h>
#include <Iphlpapi.h>

#include "SEComPlc.h"
#include "..\Library\OpConServicesCommon.h"
#define _SE_COM_MICRO_LAS_INTERNAL
#include "SEComMicroLas.h"
#undef _SE_COM_MICRO_LAS_INTERNAL
#include "SEHelpFunctions.h"

HANDLE SEComMicroLas::hThreadHandleComMicroLas[2] = { NULL, NULL };	
HANDLE SEComMicroLas::ghMutex = NULL;

//public memberfunctions --------------------------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SETrimmingProcess()                                                                              *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor function.                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEComMicroLas::SEComMicroLas()
{
	SocketConnected = false;																								//socket disconnected
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SEComMicroLas(int ChamberId);	                                                                   *
 *                                                                                                                         *
 * input:               : int ChamberId : chamber ientifier                                                                *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor function.                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEComMicroLas::SEComMicroLas(int ChamberId)
{
	this->ChamberId = ChamberId;
	SocketConnected = false;																								//socket disconnected
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ~SETrimmingProcess()                                                                             *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the destructor function.                                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEComMicroLas::~SEComMicroLas()
{
	;																																				//do nothing
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Initializing                                                                                 *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                -1 = socket creation failed                                                              *
 *                                                                                                                         *
 * description:         : This is the initializing function.                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::Initializing( void )
{	
	int RetVal = 0;																													//return value
	int FuncRetVal = 0;																											//function return
	WSADATA wsaData;																												//wsa data

	FuncRetVal = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );									//initialize winsock
	//check no error
	if ( FuncRetVal == NO_ERROR )
	{							
		//create a socket
		SEComMicroLas::ClientSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		//check error
		if ( SEComMicroLas::ClientSocket == INVALID_SOCKET ) 
		{
			RetVal = -1;																												//return error
		}  
	}	
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SetParameter( ParamDataLaser ParamLaser )                                                    *
 *                                                                                                                         *
 * input:               : ParamDataLaser ParamLaser : parameter data for microlas scanner                                  *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to set the microlas scanner parameter.                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::SetParameter( ParamStationDataLaser ParamStationLaser )
{
	int RetVal = 0;																													//return value
	char TempString[1024];																									//send string

	this->ParamStationLaser = ParamStationLaser;													  //set microlas parameter

	//check connection
	if( SocketConnected == true )
	{	
		//create microlas init telegramm
		sprintf_s( TempString, "init<%d;%d;%d;%s;%d;%d;%.8f;%.8f;%.8f;%d;%.5f;%.5f;%.5f;%.5f;%.5f;%d;%d;%.5f;%.5f>",
			ParamStationLaser.RTC5SerialNumber, 
			ChamberId,
			3,
			ParamStationLaser.CorrectionFile,
			ParamStationLaser.CalibrationFactorXY,
			ParamStationLaser.CalibrationFactorZ,
			ParamStationLaser.ZCorrA,
			ParamStationLaser.ZCorrB,
			ParamStationLaser.ZCorrC,
			ParamStationLaser.JumpSpeed,
			ParamStationLaser.LaserOnDelay,
			ParamStationLaser.LaserOffDelay,
			ParamStationLaser.JumpDelay,
			ParamStationLaser.MarkDelay,
			ParamStationLaser.PolygonDelay,
			ParamStationLaser.LaserControlType,
			ParamStationLaser.LaserPowerDACNo,
			ParamStationLaser.LaserPowerScale,
			ParamStationLaser.LaserPowerDelay ); 
		RetVal = SendRecv( TempString );																			//send string
	}

	return RetVal;
}


/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SetParameter( ParamDataLaser ParamLaser )                                                    *
 *                                                                                                                         *
 * input:               : ParamDataLaser ParamLaser : parameter data for microlas scanner                                  *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to set the microlas scanner parameter.                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::SetParameter( ParamDataLaser ParamLaser )
{
	int RetVal = 0;																													//return value

	this->ParamLaser = ParamLaser;																					//set microlas parameter

	return RetVal;
}
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Connect( void )                                                                              *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *		
 *                                -1 = failed to connect                                                                   *
 *                                                                                                                         *
 * description:         : This is the connect function for microlas scanner.                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::Connect( void )
{
	int RetVal = 0;																												//return value
	char TempString[1024];																								//send string
	int FuncRetVal = 0;																										//function return
	sockaddr_in SocketAddress;																						//socket address

	if( SocketConnected == false )
	{
		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_addr.s_addr = inet_addr( LOCALHOST );
		SocketAddress.sin_port = htons( PORT_MICRO_LAS_CHAMBER );
		//connect to a server
		//check error
		if ( connect( SEComMicroLas::ClientSocket, ( SOCKADDR* ) &SocketAddress, sizeof( SocketAddress ) ) == SOCKET_ERROR ) 
		{
			int Temp;
			Temp =  WSAGetLastError();
			if( ghMutex != NULL )
			{
				CloseHandle( ghMutex );
			}
			ghMutex = NULL;
			RetVal = -1;																													//return error																																																								
		}
		else
		{
			printf("SEComMicroLas::Connect:socket connection established!\n");
			SocketConnected = true;																								//set connect flag
			if( ghMutex == NULL )
			{
				ghMutex = CreateMutex( 
					NULL,              // default security attributes
					FALSE,             // initially not owned
					NULL);             // unnamed mutex
				if( ghMutex == NULL ) 
				{
					printf("SEComMicroLas::Connect:create mutex failed!\n");
				}
			}
		}
	}
	

	if( SocketConnected == true	)
	{
		//create microlas init telegramm
		sprintf_s(TempString, "init<%d;%d;%d;%s;%d;%d;%.8f;%.8f;%.8f;%d;%.5f;%.5f;%.5f;%.5f;%.5f;%d;%d;%.5f;%.5f>",
			ParamStationLaser.RTC5SerialNumber, 
			ChamberId,
			3,
			ParamStationLaser.CorrectionFile,
			ParamStationLaser.CalibrationFactorXY,
			ParamStationLaser.CalibrationFactorZ,
			ParamStationLaser.ZCorrA,
			ParamStationLaser.ZCorrB,
			ParamStationLaser.ZCorrC,
			ParamStationLaser.JumpSpeed,
			ParamStationLaser.LaserOnDelay,
			ParamStationLaser.LaserOffDelay,
			ParamStationLaser.JumpDelay,
			ParamStationLaser.MarkDelay,
			ParamStationLaser.PolygonDelay,
			ParamStationLaser.LaserControlType,
			ParamStationLaser.LaserPowerDACNo,
			ParamStationLaser.LaserPowerScale,
			ParamStationLaser.LaserPowerDelay ); 
		RetVal = SendRecv( TempString );																			//send string
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int MoveAbs( float X, float Y, float Z )                                                         *
 *                                                                                                                         *
 * input:               : float X : value for x-axis                                                                       *
 *                        float Y : value for y-axis                                                                       *
 *                        float Z : value for z-axis                                                                       *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                -1 = socket not connected                                                                *
 *                                                                                                                         *
 * description:         : This is the function for moving abolute the microlas scanner.                                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::MoveAbs( float X, float Y, float Z )
{
	int RetVal = 0;																													//return value
	char TempString[1024];																									//send string

	//check connected
	if( SocketConnected == true )
	{						
		//create microlas moveabs telegramm
		sprintf_s( TempString, "moveabs<%d;%.5f;%.5f;%.5f;0>", ChamberId, X, Y, Z ); 
		RetVal = SendRecv( TempString );																			//send string
		//check no error
		if( RetVal >= 0 )
		{
			//repeat every 10 ms (running variable i) 
			for( int i = 0; i < 6000; i++)
			{
				sprintf_s( TempString, "getstate<%d>", ChamberId );		//create microlas getstate telegramm
				RetVal = SendRecv( TempString );																	//send string
				//check error
				if( RetVal <= 0 )
				{
					break;																													//leave loop
				}	
				Sleep( 10 );																											//wait 10 ms
			}
		}
	}
	else
	{
		RetVal = -1;																													//return error
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int WriteText( char Text[255], float X, float Y, float Z, float Angle, float LaserPower )        *
 *                                                                                                                         *
 * input:               : char Text[255] : text to write                                                                   *
 *                        float X : value for x-axis                                                                       *
 *                        float Y : value for y-axis                                                                       *
 *                        float Z : value for z-axis                                                                       *
 *                        float Angle : angle of text                                                                      *
 *                        float LaserPower : laser power                                                                   *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for writing a text with the microlas scanner.                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::WriteText( char Text[255], float X, float Y, float Z, float Angle, float LaserPower )
{
	int RetVal = 0;																													//return value
	char TempString[1024];																									//send string	
	char Scale = 0;
	
	if( SocketConnected == true )
	{
		Scale = 40;
		sprintf_s( TempString, "directloadjob<%d;1;Entity:dLaserPower=%.5f;dLaserRepRate=400000;dScannerSpeed=500;uiRepeatation=1§Text:dX=%.5f;dY=%.5f;dZ=%.5f;dH=0.70;dAngle=%.5f;uiFlags=%u;sText=%s;100;100>",
			ChamberId,
			( LaserPower * 100 * 20 / 50 ),																			//LaserPowerPercent[%] = LaserPower[W] * 100[%] * Teiler[1] / Laserleistung[W]
			X, 
			Y,
			Z,
			Angle,
			( ( ( unsigned int ) Scale << 24 ) | 0x01 ),
			Text ); 
		RetVal = SendRecv(TempString);
		if( RetVal >= 0 )
		{
			sprintf_s( TempString, "runjob<%d;1;0>", ChamberId );
			RetVal = SendRecv(TempString);
			if( RetVal >= 0 )
			{
				//repeat every 10 ms (running variable i) 
				for( int i = 0; i < 6000; i++)
				{
					sprintf_s( TempString, "getstate<%d>", ChamberId );
					RetVal = SendRecv( TempString );
					if( RetVal <= 0 )
					{
						break;
					}	
					Sleep( 10 );
				}
			}
		}
	}
	else
	{
		RetVal = -1;
	}
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int WriteLine( float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ,     *
 *                              float LaserPower )                                                                         *
 *                                                                                                                         *
 * input:               : float StartX : x-axis start value                                                                *
 *                        float StartY : y-axis start value                                                                *
 *                        float StartZ : z-axis start value                                                                *
 *                        float EndX : x-axis end value                                                                    *
 *                        float EndY : y-axis end value                                                                    *
 *                        float EndZ : z-axis end value                                                                    *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for writing a line with the microlas scanner.                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::WriteLine( float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float LaserPower )
{
	int RetVal = 0;
	char TempString[1024];
	float LaserPowerPercent = 0.0f;

	LaserPowerPercent = LaserPower * 100 * 20 / 50;													//LaserPowerPercent[%] = LaserPower[W] * 100[%] * Teiler[1] / Laserleistung[W]

	if( SocketConnected == true )
	{
		sprintf_s( TempString, "directloadjob<%d;1;Entity:dLaserPower=%.5f;dLaserRepRate=400000;dScannerSpeed=500;uiRepeatation=1§Line:dX1=%.5f;dY1=%.5f;dZ1=%.5f;dX2=%.5f;dY2=%.5f;dZ2=%.5f;100;100>",
			ChamberId,
			LaserPowerPercent,
			StartX, 
			StartY, 
			StartZ,
			EndX, 
			EndY,
			EndZ );	

		RetVal = SendRecv(TempString);
		if( RetVal >= 0 )
		{
			sprintf_s( TempString, "runjob<%d;1;0>", ChamberId );
			RetVal = SendRecv(TempString);
			if( RetVal >= 0 )
			{
				//repeat every 10 ms (running variable i) 
				for( int i = 0; i < 6000; i++)
				{
					sprintf_s( TempString, "getstate<%d>", ChamberId );
					RetVal = SendRecv( TempString );
					if( RetVal <= 0 )
					{
						break;
					}	
					Sleep( 10 );
				}
			}
		}
	}
	else
	{
		RetVal = -1;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingCut( float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ,   *
 *                              float LaserPower )                                                                         *
 *                                                                                                                         *
 * input:               : float StartX : x-axis start value                                                                *
 *                        float StartY : y-axis start value                                                                *
 *                        float StartZ : z-axis start value                                                                *
 *                        float EndX : x-axis end value                                                                    *
 *                        float EndY : y-axis end value                                                                    *
 *                        float EndZ : z-axis end value                                                                    *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for processing a cut with the microlas scanner.                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::TrimmingCut( float StartX, float StartY, float StartZ, float EndX, float EndY, float EndZ, float LaserPower )
{
	int RetVal = 0;
	char TempString[1024];
	float LaserPowerPercent = 0.0f;

	LaserPowerPercent = LaserPower * 100 * 20 / 50;													//LaserPowerPercent[%] = LaserPower[W] * 100[%] * Teiler[1] / Laserleistung[W]
  
	if( SocketConnected == true )
	{
		if( ParamLaser.WobbleEnable == 0 )
		{
			//defocus functionality comment out, maybe for later use 
			/*sprintf_s( TempString, "directloadjob<%d;1;Entity:dLaserPower=%.5f;dLaserRepRate=400000;dScannerSpeed=%.5f;uiRepeatation=1§Defocus:dZ=%.5f§Line:dX1=%.5f;dY1=%.5f;dZ1=0.0;dX2=%.5f;dY2=%.5f;dZ2=0.0;100;100>",
				ParamLaser.CardNumber,
				LaserPowerPercent,
				ParamLaser.ScannerSpeed,
				StartZ,
				StartX, 
				StartY,
				EndX, 
				EndY );*/

			sprintf_s( TempString, "directloadjob<%d;1;Entity:dLaserPower=%.5f;dLaserRepRate=400000;dScannerSpeed=%.5f;uiRepeatation=1§Line:dX1=%.5f;dY1=%.5f;dZ1=%.5f;dX2=%.5f;dY2=%.5f;dZ2=%.5f;100;100>",
				ChamberId,
				LaserPowerPercent,
				ParamLaser.ScannerSpeed,
				StartX, 
				StartY, 
				StartZ,
				EndX, 
				EndY,
				EndZ );
				
		}
		else
		{
			sprintf_s( TempString, "directloadjob<%d;1;Entity:dLaserPower=%.5f;dLaserRepRate=400000;dScannerSpeed=%.5f;uiRepeatation=1§WobbelLine:dX1=%.5f;dY1=%.5f;dZ1=%.5f;dX2=%.5f;dY2=%.5f;dZ2=%.5f;dTransversal=%.5f;dLongitudinal=%.5f;dFrequency=%.5f;100;100>",
				ChamberId,
				LaserPowerPercent,
				ParamLaser.ScannerSpeed,
				StartX, 
				StartY, 
				StartZ,
				EndX, 
				EndY,
				EndZ,
				ParamLaser.WobbleTransversal,
				ParamLaser.WobbleLongitudinal,
				ParamLaser.WobbleFrequency );
		}	
		//printf("SendRecv start\n");
		//TimeDiff[TimeDiffIdx] =  1;
		//TimeDiff[TimeDiffIdx++] =  GetActualSystemTimeMs();
		RetVal = SendRecv(TempString);
		//TimeDiff[TimeDiffIdx] =  1;
		//TimeDiff[TimeDiffIdx++] =  GetActualSystemTimeMs();
		//printf("SendRecv end\n");
		if( RetVal >= 0 )
		{
			sprintf_s( TempString, "runjob<%d;1;0>", ChamberId );
			//printf("SendRecv start\n");
			//TimeDiff[TimeDiffIdx] =  2;
			//TimeDiff[TimeDiffIdx++] =  GetActualSystemTimeMs();
			RetVal = SendRecv(TempString);
			//TimeDiff[TimeDiffIdx] =  2;
			//TimeDiff[TimeDiffIdx++] =  GetActualSystemTimeMs();
			//printf("SendRecv end\n");
			if( RetVal >= 0 )
			{
				//repeat every 10 ms (running variable i) 
				for( int i = 0; i < 6000; i++)
				{
					sprintf_s( TempString, "getstate<%d>", ChamberId );
					//printf("SendRecv start\n");
					//TimeDiff[TimeDiffIdx] =  3;
					//TimeDiff[TimeDiffIdx++] =  GetActualSystemTimeMs();
					RetVal = SendRecv( TempString );
					//TimeDiff[TimeDiffIdx] =  3;
					//TimeDiff[TimeDiffIdx++] =  GetActualSystemTimeMs();
					//printf("SendRecv end\n");
					if( RetVal <= 0 )
					{
						break;
					}	
					Sleep( 10 );
				}
			}
		}
	}
	else
	{
		RetVal = -1;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int LaserPoint( float X, float Y, float Z, float LaserPower, float Time )                        *
 *                                                                                                                         *
 * input:               : float StartX : x-axis start value                                                                *
 *                        float StartY : y-axis start value                                                                *
 *                        float StartZ : z-axis start value                                                                *
 *                        float LaserPower : laser power                                                                   *
 *                        float Time : laser on time                                                                       *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for processing a laser point for a specified time with the microlas scanner.*
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::LaserPoint( float X, float Y, float Z, float LaserPower, float Time )
{
	int RetVal = 0;
	char TempString[1024];
	float LaserPowerPercent = 0.0f;

	LaserPowerPercent = LaserPower * 100 * 20 / 50;													//LaserPowerPercent[%] = LaserPower[W] * 100[%] * Teiler[1] / Laserleistung[W]

	if( SocketConnected == true )
	{
		sprintf_s( TempString, "directloadjob<%d;1;Entity:dLaserPower=%.5f;dLaserRepRate=400000;dScannerSpeed=%.5f;uiRepeatation=1§Drill:dX=%.5f;dY=%.5f;dZ=%.5f;dTime=%.5f;100;100>",
			ChamberId,
			LaserPowerPercent,
			ParamLaser.ScannerSpeed,
			X, 
			Y, 
			Z,
			Time );
		RetVal = SendRecv(TempString);
		if( RetVal >= 0 )
		{
			sprintf_s( TempString, "runjob<%d;1;0>", ChamberId );
			RetVal = SendRecv(TempString);
			if( RetVal >= 0 )
			{
				//repeat every 10 ms (running variable i) 
				for( int i = 0; i < 6000; i++)
				{
					sprintf_s( TempString, "getstate<%d>", ChamberId );
					RetVal = SendRecv( TempString );
					if( RetVal <= 0 )
					{
						break;
					}	
					Sleep( 10 );
				}
			}
		}
	}
	else
	{
		RetVal = -1;
	}

	return RetVal;
}


/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SendRecv( char Cmd [1024] )                                                                  *
 *                                                                                                                         *
 * input:               : char Cmd [1024] : commandstring                                                                  *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for processing send and receive commands to the microlas scanner.           *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::SendRecv( char Cmd [1024] )
{
	int RetVal = 0;

	int FuncRetVal = 0;
	
	int BytesSent;
	int BytesRecv = SOCKET_ERROR;
	char Sendbuf[1024] = "";
	char Recvbuf[1024] = "";
	int ErrorCode = 0;
	int ErrorCodeMicroLas = 0;
	BytesRecv = SOCKET_ERROR;
	timeval Time = {1,0};																										//set timeout at communication 1,000 s
	fd_set rfds;

	if( ghMutex != NULL )
	{
		if( WaitForSingleObject( ghMutex, 10000) == WAIT_OBJECT_0 ) 
		{
			FD_ZERO( &rfds );
			FD_SET( SEComMicroLas::ClientSocket, &rfds );

			strcpy_s( Recvbuf, "" );
			strcpy_s( Sendbuf, Cmd );
			//printf("Send start\n");
			BytesSent = send( SEComMicroLas::ClientSocket, Sendbuf, strlen( Sendbuf ), 0 );
			//strcat(Sendbuf, "\n");
			//printf(Sendbuf);
			//printf("Send end\n");
			if( BytesSent != SOCKET_ERROR )
			{
				if( FuncRetVal = select( 0, &rfds, NULL, NULL, &Time ) == 1 )
				{
					//printf("Recv start\n");
					BytesRecv = recv( SEComMicroLas::ClientSocket, Recvbuf, sizeof( Recvbuf ), 0 );
					//printf("Recv end\n");
					if( ( BytesRecv != SOCKET_ERROR ) && 
							( BytesRecv != 0 ) )
					{
						char Temp [10];
						strncpy_s( Temp, Recvbuf, strchr( Recvbuf, '<') - Recvbuf );   
						ErrorCode = atoi(Temp);

						strncpy_s( Temp, strchr( Recvbuf, '<') + 1, strchr( Recvbuf, '>') - strchr( Recvbuf, '<') - 1 );   
						ErrorCodeMicroLas = atoi(Temp);

		
						if( ( ErrorCode != 0 ) || ( ErrorCodeMicroLas < 0 ) )
						{
							
							printf( "SEComMicroLas:SendRecv:Return=%d,ReturnDll=%d\n", ErrorCode, ErrorCodeMicroLas );
							RetVal = ErrorCodeMicroLas;
						}								 
						else
						{
							if( ErrorCodeMicroLas > 0 )
							{
								RetVal = 1;
							}
						}
					}
					else
					{
					 ;
					}
				}
				else
				{
					printf( "SEComMicroLas:SendRecv:FuncRetVal=%d\n", FuncRetVal);
					RetVal = -60;
				}
			}
			else
			{
				Disconnect( );
				RetVal = -61;
			}
			ReleaseMutex( ghMutex );
		}
		else
		{
			printf( "SEComMicroLas:SendRecv:WaitForSingleObject failed!\n" );
			RetVal = -62;
		}
	}
	else
	{
		Disconnect( );
		printf( "SEComMicroLas:SendRecv:No Mutex created!\n" );
		RetVal = -63;
	}

	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int InstallThread( LPTHREAD_START_ROUTINE CallbackAddress )                                      *
 *                                                                                                                         *
 * input:               : LPTHREAD_START_ROUTINE CallbackAddress : thread start routine                                    *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                               0 = no error                                                                              *
 *                              -1 = thread running or parameter error                                                     *
 *                              -2 = no thread created                                                                     *
 *                                                                                                                         *
 * description:         : This is the function to create the microlas handle thread.                                       *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComMicroLas::InstallThread( LPTHREAD_START_ROUTINE CallbackAddress )
{
	int RetVal = 0;
	DWORD dwThreadId, dwThrdParam;

	if( ( SEComMicroLas::hThreadHandleComMicroLas[ChamberId-1] == NULL) &&
		  ( CallbackAddress != NULL )	)
	{
		//create thread
		//no security, default stack size, default creation
		SEComMicroLas::hThreadHandleComMicroLas[ChamberId-1] = CreateThread( NULL,		
																											0,                       
																											CallbackAddress,             
																											&dwThrdParam,               
																											0,                         
																											&dwThreadId );
		if( SEComMicroLas::hThreadHandleComMicroLas[ChamberId-1] == NULL )
		{
			RetVal = -2;
		}
	}	
	else
	{
		RetVal = -1;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IsConnected( void )                                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                               false = not connected                                                                     *
 *                               true = connected                                                                          *
 *                                                                                                                         *
 * description:         : This is the function to check the microlas scanner connection.                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEComMicroLas::IsConnected( void )
{
	return SocketConnected;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void Disconnect( void )                                                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to disconnect the microlas scanner connection.                              *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComMicroLas::Disconnect( void )
{
	closesocket( SEComMicroLas::ClientSocket );
	SocketConnected = false;
	if( ghMutex != NULL )
	{
		CloseHandle( ghMutex );
	}
	ghMutex = NULL;
}




