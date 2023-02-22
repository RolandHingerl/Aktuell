/*-------------------------------------------------------------------------------------------------------------------------*
 *  modulname       : SEComPlc.cpp                                                                                         *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                                     *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                                            *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  description:                                                                                                           *
 *  ------------                                                                                                           *
 *  This module contains control and evaluation routines for the communication with plc.                                   *
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

#include "..\Library\OpConServicesCommon.h"
#define _SE_COM_PLC_INTERNAL
#include "SEComPlc.h"
#undef _SE_COM_PLC_INTERNAL

HANDLE SEComPlc::hThreadHandleComPlc[CHAMBER_COUNT] = { NULL, NULL };	

//public memberfunctions --------------------------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SEComPlc( void )                                                                                 *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor function.                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEComPlc::SEComPlc( void )
{
  memset( &ActionStart, 0, sizeof( ActionStart ) );
  memset( &ActionStarted, 0, sizeof( ActionStarted ) );
  memset( &ActionFinished, 0, sizeof( ActionFinished ) );	 
  memset( &ActionRisingEdge, 0, sizeof( ActionRisingEdge ) );
  memset( &ActionFallingEdge, 1, sizeof( ActionFallingEdge ) );
  memset( &ActionAbort, 0, sizeof( ActionAbort ) );
  memset( &ActualErrorCode, 0, sizeof( ActualErrorCode ) );
  ParameterState = false;
  BootReady = false;
  Ready = false;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SEComPlc( int ChamberId )                                                                        *
 *                                                                                                                         *
 * input:               : int ChamberId : chamber identifier                                                               *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor function.                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEComPlc::SEComPlc( int ChamberId )
{
  this->ChamberId = ChamberId;
  memset( &ActionStart, 0, sizeof( ActionStart ) );
  memset( &ActionStarted, 0, sizeof( ActionStarted ) );
  memset( &ActionFinished, 0, sizeof( ActionFinished ) );	 
  memset( &ActionRisingEdge, 0, sizeof( ActionRisingEdge ) );
  memset( &ActionFallingEdge, 1, sizeof( ActionFallingEdge ) );
  memset( &ActionAbort, 0, sizeof( ActionAbort ) );
  memset( &ActualErrorCode, 0, sizeof( ActualErrorCode ) );
  ParameterState = false;
  BootReady = false;
  Ready = false;
  
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ~SEComPlc( void )                                                                                *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the destructor function.                                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEComPlc::~SEComPlc( void )
{

}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Initializing( void )                                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the initialization function.                                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComPlc::Initializing( void )
{	
  int RetVal = 0;	
                                                        //return value
  int FuncRetVal = 0;
  
  WSADATA wsaData;
  sockaddr_in SocketAddress;
  PMIB_IPADDRTABLE pIPAddrTable;
  DWORD dwSize = 0;

  //initialize winsock
  FuncRetVal = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
  if ( FuncRetVal == NO_ERROR )
  {
    //create a socket
    SEComPlc::ServerSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( SEComPlc::ServerSocket == INVALID_SOCKET ) 
    {
      //WSACleanup();
      RetVal = -1;
    }  
    else
    {	
      pIPAddrTable = (MIB_IPADDRTABLE *) malloc(sizeof (MIB_IPADDRTABLE));
      if( pIPAddrTable != NULL )
      {
        GetIpAddrTable(pIPAddrTable, &dwSize, 0);
        free(pIPAddrTable);
        pIPAddrTable = NULL;
        pIPAddrTable = (MIB_IPADDRTABLE *) malloc(dwSize);
        if( pIPAddrTable != NULL )
        {
          GetIpAddrTable(pIPAddrTable, &dwSize, 0);
        }
      }
      
      //bind the socket
      SocketAddress.sin_family = AF_INET;
      SocketAddress.sin_addr.s_addr = pIPAddrTable->table[0].dwAddr;
      printf( "SEComPlc::Initializing:Network 1 Ip=%s\n", inet_ntoa( SocketAddress.sin_addr ) );
			SocketAddress.sin_addr.s_addr = pIPAddrTable->table[1].dwAddr;
      printf( "SEComPlc::Initializing:Network 2 Ip=%s\n", inet_ntoa( SocketAddress.sin_addr ) );
			SocketAddress.sin_addr.s_addr = inet_addr( IP_SERVER );
			
      switch( ChamberId )
      {
        case 1:
          SocketAddress.sin_port = htons( PORT_PLC_CHAMBER1 );
          break;
      
        case 2:
          SocketAddress.sin_port = htons( PORT_PLC_CHAMBER2 );
          break;
    
        default:
          printf( "SEComPlc::Initializing:chamber identifier not plausible!\n" );
          break;
      }
      if( pIPAddrTable != NULL ) 
      {
        free( pIPAddrTable );
        pIPAddrTable = NULL;
      }

      if ( bind( SEComPlc::ServerSocket, (SOCKADDR*) &SocketAddress, sizeof( SocketAddress ) ) == SOCKET_ERROR )
      {
        printf( "SEComPlc::Initializing:socket bind:RetVal=%d", WSAGetLastError()	);
        closesocket( SEComPlc::ServerSocket );
        RetVal = -1;
      }

      //listen the socket
      FuncRetVal = listen( SEComPlc::ServerSocket, 1 );

    }
  }
  else
  {
    RetVal = -1;
  }
  return RetVal;
  
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataTrimmingChamber GetParamDataTrimming( void )                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataTrimmingChamber : parameter data                                                        *
 *                                                                                                                         *
 * description:         : This is the function to get the trimming chamber parameter data.                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamDataTrimmingChamber SEComPlc::GetParamDataTrimming( void )
{
  return ParamTrimmingChamber;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataTrimmingChamber GetParamStationDataTrimming( void )                                     *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataTrimmingChamber : parameter data                                                        *
 *                                                                                                                         *
 * description:         : This is the function to get the trimming chamber parameter data.                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamStationDataTrimmingChamber SEComPlc::GetParamStationDataTrimming( void )
{
  return ParamStationTrimmingChamber;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataNormalMeasure GetParamDataNormalMeasure( void )                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataNormalMeasure : parameter data                                                          *
 *                                                                                                                         *
 * description:         : This is the function to get the parameter data for normal measure.                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamDataNormalMeasure SEComPlc::GetParamDataNormalMeasure( void )
{
  return ParamNormalMeasure;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultDataTrimming * GetResultDataTrimmingAddress( void )                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ResultDataTrimming *  : pointer to trimming result data                                          *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to trimming result data.                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
ResultDataTrimming * SEComPlc::GetResultDataTrimmingAddress( void )
{
  return &ResultTrimming;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultDataPositionNormal * GetResultDataPositionNormalAddress( void )                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ResultDataPositionNormal *  : pointer to normal result data                                      *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to result data of position normal measurement.           *
 *-------------------------------------------------------------------------------------------------------------------------*/
ResultDataPositionNormal * SEComPlc::GetResultDataPositionNormalAddress( void )
{
  return &ResultPositionNormal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultDataHeaterNormal * GetResultDataHeaterNormalAddress( void )                                *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ResultDataHeaterNormal *  : pointer to normal result data                                        *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to result data of heater normal measurement.             *
 *-------------------------------------------------------------------------------------------------------------------------*/
ResultDataHeaterNormal * SEComPlc::GetResultDataHeaterNormalAddress( void )
{
  return &ResultHeaterNormal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultDataUniversalNormal * GetResultDataUniversalNormalAddress( void )                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ResultDataUniversalNormal *  : pointer to normal result data                                     *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to result data of universal normal measurement.          *
 *-------------------------------------------------------------------------------------------------------------------------*/
ResultDataUniversalNormal * SEComPlc::GetResultDataUniversalNormalAddress( void )
{
  return &ResultUniversalNormal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultDataUnNormal * GetResultDataUnNormalAddress( void )                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ResultDataUnNormal *  : pointer to normal result data                                            *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to result data of un normal measurement.                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
ResultDataUnNormal * SEComPlc::GetResultDataUnNormalAddress( void )
{
  return &ResultUnNormal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultDataIpNormal * GetResultDataIpNormalAddress( void )                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ResultDataIpNormal *  : pointer to normal result data                                            *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to result data of ip normal measurement.                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
ResultDataIpNormal * SEComPlc::GetResultDataIpNormalAddress( void )
{
  return &ResultIpNormal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultDataIlmNormal * GetResultDataIlmNormalAddress( void )                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ResultDataIlmNormal *  : pointer to normal result data                                           *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to result data of ilm normal measurement.                *
 *-------------------------------------------------------------------------------------------------------------------------*/
ResultDataIlmNormal * SEComPlc::GetResultDataIlmNormalAddress( void )
{
  return &ResultIlmNormal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : CyclicDataTrimming * GetCyclicDataTrimmingAddress( void )                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : CyclicDataTrimming * : pointer to cyclical data	                                                 *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to cyclical data.                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
CyclicDataTrimming * SEComPlc::GetCyclicDataTrimmingAddress( void )
{
  return &CyclicTrimming[0];
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : unsigned int * GetChamberStateAddress( void )                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : unsigned int : pointer to chamber state	                                                         *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to chamber state.                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
unsigned int * SEComPlc::GetChamberStateAddress( void )
{
  return &ChamberState;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : unsigned int * GetIpRefValuesAddress( void )                                                     *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : unsigned int : pointer to chamber state	                                                         *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to ip reference sensors.                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
float * SEComPlc::GetIpRefValuesAddress( void )
{
  return ActIpRef;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataScannerMoveAbs GetParamDataMoveAbs( void )                                              *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataScannerMoveAbs : scanner move absolute parameter                                        *
 *                                                                                                                         *
 * description:         : This is the function to get the parameter data of scanner move absolute.                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamDataScannerMoveAbs SEComPlc::GetParamDataMoveAbs( void )
{
  return ParamScannerMoveAbs;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataTrimmingCut GetParamDataTrimmingCut( void )                                             *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataTrimmingCut : scanner trimming cut parameter                                            *
 *                                                                                                                         *
 * description:         : This is the function to get the parameter data of scanner trimming cut.                          *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamDataScannerTrimmingCut SEComPlc::GetParamDataTrimmingCut( )
{
  return ParamScannerTrimmingCut;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataImageProcessing GetParamDataImageProcessing( void )                                     *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataImageProcessing : image processing parameter                                            *
 *                                                                                                                         *
 * description:         : This is the function to get the parameter data of image processing.                              *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamDataImageProcessing SEComPlc::GetParamDataImageProcessing( void )
{
  return ParamImageProcessing;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataScannerLaserPoint GetParamDataLaserPoint( void )                                        * 
 *																																																												 *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataScannerLaserPoint : parameter data laser point                                          *
 *                                                                                                                         *
 * description:         : This is the function to get the parameter data for laser point.                                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamDataScannerLaserPoint SEComPlc::GetParamDataLaserPoint( void )
{
  return ParamScannerLaserPoint;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParamDataScannerWriteText GetParamDataWriteText( void )                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : ParamDataScannerWriteText : parameter data scanner write test                                    *
 *                                                                                                                         *
 * description:         : This is the function to get the parameter data for scanner write text.                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
ParamDataScannerWriteText SEComPlc::GetParamDataWriteText( void )
{
  return ParamScannerWriteText;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : unsigned int GetNormalNumber( void )                                                             *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : unsigned int : normal number                                                                     *
 *                                                                                                                         *
 * description:         : This is the function to get the normal number.                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
unsigned int SEComPlc::GetNormalNumber( void )
{
  return NormalNumber;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : AssemblyDataTrimmingChamber GetAssemblyData( void )                                              *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : AssemblyDataTrimmingChamber : assembly data                                                      *
 *                                                                                                                         *
 * description:         : This is the function to get the assembly data.                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
AssemblyDataTrimmingChamber SEComPlc::GetAssemblyData( void )
{
  return AssemblyData;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Connect( void )                                                                              *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to connect to plc.                                                          *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComPlc::Connect( void )
{
  int FuncRetVal = 0;
  int RetVal = 0;
  fd_set rfds;
  timeval Time = {1,0};	//set timeout at communication 1,000 s

  FD_ZERO( &rfds );
  FD_SET( SEComPlc::ServerSocket, &rfds );
  
  if( FuncRetVal = select( 0, &rfds, &rfds, &rfds, &Time ) == 1 )
  {
    SEComPlc::AcceptSocket = accept( SEComPlc::ServerSocket, NULL, NULL );
    printf("SEComPlc::Connect:socket connection established!\n");
    SocketConnected = true;
  }
  else
  {
    RetVal = -1;
  }

  return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Disconnect( void )                                                                           *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to disconnect from plc.                                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComPlc::Disconnect( void )
{
  closesocket( SEComPlc::AcceptSocket );
  SocketConnected = false;
  return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int RecvSend( void )                                                                             *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to handle the receive and send functionality to plc.                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComPlc::RecvSend( void )
{
  int FuncRetVal = 0;
  int RetVal = 0;
  int BytesSent;
  int BytesRecv = SOCKET_ERROR;
  char Sendbuf[1024] = "";
  char Recvbuf[1024] = "";
  int ErrorCode = 0;
  int ErrorCodeMicroLas = 0;

  PlcRequestStationTrimming StationTrimmingReqPacket;
  PlcRequestParameterTrimming ParameterTrimmingReqPacket;
  PlcRequestParameterNormalMeasure ParameterNormalMeasureReqPacket;
  PlcRequestParameterMoveAbs ParameterMoveAbsReqPacket;
  PlcRequestParameterTrimmingCut ParameterTrimmingCutReqPacket;
  PlcRequestParameterImageProcessing ParameterImageProcessingReqPacket;
  PlcRequestParameterLaserPoint ParameterLaserPointReqPacket;
  PlcRequestParameterWriteText ParameterWriteTextReqPacket;
  PlcRequestParameterTrimmingAssembly ParameterTrimmingAsseblyReqPacket;
  PlcRequestParameterNormalNumber ParameterNormalNumberReqPacket;
  PlcResponseParameter ParameterResPacket;

  PlcRequestResult ResultReqPacket;
  PlcResponseResultTrimming ResultTrimmingResPacket;
  PlcResponseResultPositionNormal ResultPositionNormalResPacket;
  PlcResponseResultHeaterNormal ResultHeaterNormalResPacket;
  PlcResponseResultUniversalNormal ResultUniversalNormalResPacket;
  PlcResponseResultUnNormal ResultUnNormalResPacket;
  PlcResponseResultIpNormal ResultIpNormalResPacket;
  PlcResponseResultIlmNormal ResultIlmNormalResPacket;

  PlcRequestCyclic CyclicReqPacket;
  PlcResponseCyclic CyclicResPacket;

  PlcRequestAction ActionReqPacket;
  PlcResponseAction ActionResPacket;

  PlcRequestState StateReqPacket;
  PlcResponseState StateResPacket;

  PlcResponseError ErrorResPacket;

  BytesRecv = SOCKET_ERROR;

  fd_set rfds;
  timeval Time = { 0, 200000 };																						//set timeout at communication 200 ms

  FD_ZERO( &rfds );
  FD_SET( SEComPlc::AcceptSocket, &rfds );
  
  strcpy_s( Recvbuf, "" );
  strcpy_s( Sendbuf, "" );
  
  if( FuncRetVal = select( 0, &rfds, NULL, &rfds, &Time ) == 1 )
  {
    memset(&Recvbuf, 0,sizeof(Recvbuf));
    BytesRecv = recv( SEComPlc::AcceptSocket, Recvbuf, sizeof( Recvbuf ), 0 );
    if( ( BytesRecv != SOCKET_ERROR ) && 
        ( BytesRecv != 0 ) )
    {
      switch( Recvbuf[0] )
      {
        case SET_PARAMETER:  
          switch( Recvbuf[1] )
          {
            case 0:
              memcpy_s( &StationTrimmingReqPacket, sizeof( StationTrimmingReqPacket ), &Recvbuf, sizeof( StationTrimmingReqPacket ) );
              memcpy_s( &ParamStationTrimmingChamber, sizeof( StationTrimmingReqPacket ), &StationTrimmingReqPacket.ParamStationTrimmingChamber, sizeof( StationTrimmingReqPacket.ParamStationTrimmingChamber ) );
              ParameterState = true;
              break;

            case 1:
              memcpy_s( &ParameterTrimmingReqPacket, sizeof( ParameterTrimmingReqPacket ), &Recvbuf, sizeof( ParameterTrimmingReqPacket ) );
              memcpy_s( &ParamTrimmingChamber, sizeof( ParamDataTrimmingChamber ), &ParameterTrimmingReqPacket.ParamTrimmingChamber, sizeof( ParameterTrimmingReqPacket.ParamTrimmingChamber ) );
              ParameterState = true;
              break;

            case 2:
              memcpy_s( &ParameterNormalMeasureReqPacket, sizeof( ParameterNormalMeasureReqPacket ), &Recvbuf, sizeof( ParameterNormalMeasureReqPacket ) );
              memcpy_s( &ParamNormalMeasure, sizeof( ParamNormalMeasure ), &ParameterNormalMeasureReqPacket.ParamNormalMeasure, sizeof( ParameterNormalMeasureReqPacket.ParamNormalMeasure ) );
              ParameterState = true;
              break;

            case 3:
              memcpy_s( &ParameterMoveAbsReqPacket, sizeof( ParameterMoveAbsReqPacket ), &Recvbuf, sizeof( ParameterMoveAbsReqPacket ) );
              memcpy_s( &ParamScannerMoveAbs, sizeof( ParamScannerMoveAbs ), &ParameterMoveAbsReqPacket.ParamScannerMoveAbs, sizeof( ParameterMoveAbsReqPacket.ParamScannerMoveAbs ) );
              break;

            case 4:
              memcpy_s( &ParameterTrimmingCutReqPacket, sizeof( ParameterTrimmingCutReqPacket ), &Recvbuf, sizeof( ParameterTrimmingCutReqPacket ) );
              memcpy_s( &ParamScannerTrimmingCut, sizeof( ParamScannerTrimmingCut ), &ParameterTrimmingCutReqPacket.ParamScannerTrimmingCut, sizeof( ParameterTrimmingCutReqPacket.ParamScannerTrimmingCut ) );
              break;

            case 5:
              memcpy_s( &ParameterImageProcessingReqPacket, sizeof( ParameterImageProcessingReqPacket ), &Recvbuf, sizeof( ParameterImageProcessingReqPacket ) );
              memcpy_s( &ParamImageProcessing, sizeof( ParamImageProcessing ), &ParameterImageProcessingReqPacket.ParamImageProcessing, sizeof( ParameterImageProcessingReqPacket.ParamImageProcessing ) );
              break;

            case 6:
              ;
              break;

            case 7:
              memcpy_s( &ParameterTrimmingAsseblyReqPacket, sizeof( ParameterTrimmingAsseblyReqPacket ), &Recvbuf, sizeof( ParameterTrimmingAsseblyReqPacket ) );
              memcpy_s( &AssemblyData, sizeof( AssemblyData ), &ParameterTrimmingAsseblyReqPacket.ParamAssembly, sizeof( ParameterTrimmingAsseblyReqPacket.ParamAssembly ) );
              break;

            case 8:
              memcpy_s( &ParameterLaserPointReqPacket, sizeof( ParameterLaserPointReqPacket ), &Recvbuf, sizeof( ParameterLaserPointReqPacket ) );
              memcpy_s( &ParamScannerLaserPoint, sizeof( ParamScannerLaserPoint ), &ParameterLaserPointReqPacket.ParamScannerLaserPoint, sizeof( ParameterLaserPointReqPacket.ParamScannerLaserPoint ) );
              break; 

            case 9:
              memcpy_s( &ParameterWriteTextReqPacket, sizeof( ParameterWriteTextReqPacket ), &Recvbuf, sizeof( ParameterWriteTextReqPacket ) );
              memcpy_s( &ParamScannerWriteText, sizeof( ParamScannerWriteText ), &ParameterWriteTextReqPacket.ParamScannerWriteText, sizeof( ParameterWriteTextReqPacket.ParamScannerWriteText ) );
              ParamScannerWriteText.Code[6] = '\0';
              break;

            case 10:
              memcpy_s( &ParameterNormalNumberReqPacket, sizeof( ParameterNormalNumberReqPacket ), &Recvbuf, sizeof( ParameterNormalNumberReqPacket ) );
              memcpy_s( &NormalNumber, sizeof( NormalNumber ), &ParameterNormalNumberReqPacket.NormalNumber, sizeof( ParameterNormalNumberReqPacket.NormalNumber ) );
              break;

            default:
              ;
              break;
          }
          ParameterOverTaken( Recvbuf[1] );
          ParameterResPacket.Id = Recvbuf[0];
          ParameterResPacket.RetVal	= ReadErrorCode( SET_PARAMETER );
          memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ParameterResPacket, sizeof( ParameterResPacket ) );
          break;
        case GET_RESULT:
          if( BootReady == true )
          {
						memcpy_s( &ResultReqPacket, sizeof( PlcRequestResult ), &Recvbuf, sizeof( PlcRequestResult ) );
            FuncRetVal = ResultRead( ResultReqPacket.SubId, ResultReqPacket.Tiepoint ); 
            if( FuncRetVal == 0 )
            {
              
              switch( Recvbuf[1] )
              {
                case 1:
                  memcpy_s( &ResultTrimmingResPacket.ResultTrimming, sizeof( ResultTrimming ), &ResultTrimming, sizeof( ResultTrimming ) );
                  ResultTrimmingResPacket.Id = Recvbuf[0];
                  ResultTrimmingResPacket.SubId = Recvbuf[1] ;
                  ResultTrimmingResPacket.RetVal	= ReadErrorCode( GET_RESULT );
                  memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ResultTrimmingResPacket, sizeof( ResultTrimmingResPacket ) );
                  break;

                case 2:
                  memcpy_s( &ResultPositionNormalResPacket.ResultPositionNormal, sizeof( ResultPositionNormal ), &ResultPositionNormal, sizeof( ResultPositionNormal ) );
                  ResultPositionNormalResPacket.Id = Recvbuf[0];
                  ResultPositionNormalResPacket.SubId = Recvbuf[1] ;
                  ResultPositionNormalResPacket.RetVal	= ReadErrorCode( GET_RESULT );
                  memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ResultPositionNormalResPacket, sizeof( ResultPositionNormalResPacket ) );
                  break;

                case 3:
                  memcpy_s( &ResultHeaterNormalResPacket.ResultHeaterNormal, sizeof( ResultHeaterNormal ), &ResultHeaterNormal, sizeof( ResultHeaterNormal ) );
                  ResultHeaterNormalResPacket.Id = Recvbuf[0];
                  ResultHeaterNormalResPacket.SubId = Recvbuf[1] ;
                  ResultHeaterNormalResPacket.RetVal	= ReadErrorCode( GET_RESULT );
                  memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ResultHeaterNormalResPacket, sizeof( ResultHeaterNormalResPacket ) );
                  break;

                case 4:
                  memcpy_s( &ResultUniversalNormalResPacket.ResultUniversalNormal, sizeof( ResultUniversalNormal ), &ResultUniversalNormal, sizeof( ResultUniversalNormal ) );
                  ResultUniversalNormalResPacket.Id = Recvbuf[0];
                  ResultUniversalNormalResPacket.SubId = Recvbuf[1] ;
                  ResultUniversalNormalResPacket.RetVal	= ReadErrorCode( GET_RESULT );
                  memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ResultUniversalNormalResPacket, sizeof( ResultUniversalNormalResPacket ) );
                  break;

                case 5:
                  memcpy_s( &ResultUnNormalResPacket.ResultUnNormal, sizeof( ResultUnNormal ), &ResultUnNormal, sizeof( ResultUnNormal ) );
                  ResultUnNormalResPacket.Id = Recvbuf[0];
                  ResultUnNormalResPacket.SubId = Recvbuf[1] ;
                  ResultUnNormalResPacket.RetVal	= ReadErrorCode( GET_RESULT );
                  memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ResultUnNormalResPacket, sizeof( ResultUnNormalResPacket ) );
                  break;

                case 6:
                  memcpy_s( &ResultIpNormalResPacket.ResultIpNormal, sizeof( ResultIpNormal ), &ResultIpNormal, sizeof( ResultIpNormal ) );
                  ResultIpNormalResPacket.Id = Recvbuf[0];
                  ResultIpNormalResPacket.SubId = Recvbuf[1] ;
                  ResultIpNormalResPacket.RetVal	= ReadErrorCode( GET_RESULT );
                  memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ResultIpNormalResPacket, sizeof( ResultIpNormalResPacket ) );
                  break;

                case 7:
                  memcpy_s( &ResultIlmNormalResPacket.ResultIlmNormal, sizeof( ResultIlmNormal ), &ResultIlmNormal, sizeof( ResultIlmNormal ) );
                  ResultIlmNormalResPacket.Id = Recvbuf[0];
                  ResultIlmNormalResPacket.SubId = Recvbuf[1] ;
                  ResultIlmNormalResPacket.RetVal	= ReadErrorCode( GET_RESULT );
                  memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ResultIlmNormalResPacket, sizeof( ResultIlmNormalResPacket ) );
                  break;

                  break;
                default:
                  ;
                  break;
              }
            }
            else
            {
              SetErrorCode( GET_RESULT, FLT_RESULT_DATA_REQUEST_OUTSIDE_LIMIT );
            }
          }
          else
          {
            SetErrorCode( GLOBAL_ERROR, FLT_BOOT_NOT_FINISHED );
          }
          break;

        case SET_GET_CYCLICAL:
          CyclicResPacket.ApplicationState = 0;
          if( Ready == true )
          {
            CyclicResPacket.ApplicationState = 1;
            if( BootReady == true )
            {
              memcpy( &CyclicReqPacket , &Recvbuf, sizeof( CyclicReqPacket ) );  
              memcpy_s( &ChamberPressureActual, sizeof( ChamberPressureActual ), &CyclicReqPacket.ChamberPressureActual, sizeof( ChamberPressureActual ) );
              memcpy_s( &ChamberFlowActual, sizeof( ChamberFlowActual ), &CyclicReqPacket.ChamberFlowActual, sizeof( ChamberFlowActual ) );
							memcpy_s( &ChamberLaserPowerActual, sizeof( ChamberLaserPowerActual ), &CyclicReqPacket.ChamberLaserPowerActual, sizeof( ChamberLaserPowerActual ) );
              memcpy_s( &DigitalIn, sizeof( DigitalIn ), &CyclicReqPacket.DigitalIn, sizeof( DigitalIn ) );
              CyclicalReceived();
              CyclicResPacket.ApplicationState = 2;
              if( ParameterState == true )
              {
                CyclicResPacket.ApplicationState = 3;
              }
            }
          }
          CyclicResPacket.ChamberState = ChamberState;

          memcpy_s( &CyclicResPacket.DigitalOut, sizeof( DigitalOut ), &DigitalOut, sizeof( DigitalOut ) );
          memcpy_s( &CyclicResPacket.CyclicTrimming, sizeof( CyclicTrimming ), &CyclicTrimming, sizeof( CyclicTrimming ) );
					memcpy_s( &CyclicResPacket.ActIpRef, sizeof( ActIpRef ), &ActIpRef, sizeof( ActIpRef ) );
          CyclicResPacket.Id = Recvbuf[0];
          CyclicResPacket.RetVal	= ReadErrorCode( SET_GET_CYCLICAL );
          memcpy_s( &Sendbuf, sizeof( Sendbuf ), &CyclicResPacket, sizeof( CyclicResPacket ) );			
          break;

        case SET_ACTION:
          if( BootReady == true )
          {
            memcpy_s( &ActionReqPacket, sizeof( ActionReqPacket ), &Recvbuf, sizeof( ActionReqPacket ) );  
            memcpy_s( &ActionStart, sizeof( ActionStart ), &ActionReqPacket.ActionStart, sizeof( ActionReqPacket.ActionStart ) );
            ActionFlagChanged();
          }
          else
          {
            SetErrorCode( GLOBAL_ERROR, FLT_BOOT_NOT_FINISHED );
          }
          ActionResPacket.Id = Recvbuf[0];
          ActionResPacket.RetVal	= ReadErrorCode( SET_ACTION );
          memcpy_s( &Sendbuf, sizeof( Sendbuf ), &ActionResPacket, sizeof( ActionResPacket ) );
          break;

        case GET_STATE:   
          if( BootReady == true )
          {
            memcpy_s( &StateReqPacket, sizeof( StateReqPacket ), &Recvbuf, sizeof( StateReqPacket ) );
            memcpy_s( &StateResPacket.ActionStarted, sizeof( StateResPacket.ActionStarted ), &ActionStarted, sizeof( ActionStarted ) );
            memcpy_s( &StateResPacket.ActionFinished, sizeof( StateResPacket.ActionFinished ), &ActionFinished, sizeof( ActionFinished ) );
          }
          else
          {
            SetErrorCode( GLOBAL_ERROR, FLT_BOOT_NOT_FINISHED );
          }
          StateResPacket.Id = Recvbuf[0];
          StateResPacket.RetVal	= ReadErrorCode( GET_STATE );
          memcpy_s( &Sendbuf, sizeof( Sendbuf ), &StateResPacket, sizeof( StateResPacket ) );
        break;

        default:
          ErrorResPacket.Id = 99;
          memcpy_s( &Sendbuf, sizeof(Sendbuf), &ErrorResPacket, sizeof( ErrorResPacket ) );
          break;
      }
    
      BytesSent = send( SEComPlc::AcceptSocket, Sendbuf, sizeof( Sendbuf ), 0 );
    }
    else
    {
      Disconnect();
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
 * function name        : int InstallThread( LPTHREAD_START_ROUTINE CallbackAddress )                                      *
 *                                                                                                                         *
 * input:               : LPTHREAD_START_ROUTINE CallbackAddress : thread start routine                                    *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                               0 = no error                                                                              *
 *                              -1 = thread running or parameter error                                                     *
 *                              -2 = no thread created                                                                     *
 *                                                                                                                         *
 * description:         : This is the function to create the plc handle thread.                                            *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComPlc::InstallThread( LPTHREAD_START_ROUTINE CallbackAddress )
{
  int RetVal = 0;
  DWORD dwThreadId, dwThrdParam;

  if( ( SEComPlc::hThreadHandleComPlc[ChamberId-1] == NULL) &&
      ( CallbackAddress != NULL ) )
  {
    //create thread
    //no security, default stack size, default creation
    SEComPlc::hThreadHandleComPlc[ChamberId-1] = CreateThread( NULL,		
                                                      0,                       
                                                      CallbackAddress,             
                                                      &dwThrdParam,               
                                                      0,                         
                                                      &dwThreadId); 
    if( SEComPlc::hThreadHandleComPlc[ChamberId-1] == NULL )
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
 * function name        : void MasterReset( void )                                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the reset function.                                                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::MasterReset( void )
{
  memset( &ActionStart, 0, sizeof( ActionStart ) );
  ActionFlagChanged();
  memset( &ActionStarted, 0, sizeof( ActionStarted ) );
  memset( &ActionFinished, 0, sizeof( ActionFinished ) );	
  memset( &ActionRisingEdge, 0, sizeof( ActionRisingEdge ) );
  memset( &ActionFallingEdge, 1, sizeof( ActionFallingEdge ) );
  ParameterState = false;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool GetActionFlag( char Flag )                                                                  *
 *                                                                                                                         *
 * input:               : char Flag : action flag number                                                                   *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = action flag not set                                                             *
 *                                 true = action flag set                                                                  *
 *                                                                                                                         *
 * description:         : This is the function to get the action flag state.                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEComPlc::GetActionFlag( char Flag )
{
  return ActionStart[Flag - 1];
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool GetRisingEdge( char Flag )                                                                  *
 *                                                                                                                         *
 * input:               : char Flag : action flag number                                                                   *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = no edge detected                                                                *
 *                                 true = rising edge detected                                                             *
 *                                                                                                                         *
 * description:         : This is the function to check the rising edge of specified start flag.                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEComPlc::GetRisingEdge( char Flag )
{
  if( ( ActionStart[Flag - 1] == true ) &&
      ( ActionRisingEdge[Flag - 1] == false ) )
  {
    ActionFallingEdge[Flag - 1] = false;
    ActionRisingEdge[Flag - 1] = true;
    return true;
  }
  else
  {
    return false;
  }
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool GetFallingEdge( char Flag )                                                                 *
 *                                                                                                                         *
 * input:               : char Flag : action flag number                                                                   *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = no edge detected                                                                *
 *                                 true = falling edge detected                                                            *
 *                                                                                                                         *
 * description:         : This is the function to check the falling edge of specified start flag.                          *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEComPlc::GetFallingEdge( char Flag )
{
  if( ( ActionStart[Flag - 1] == false ) &&
      ( ActionFallingEdge[Flag - 1] == false ) )
  {
    ActionRisingEdge[Flag - 1] = false;
    ActionFallingEdge[Flag - 1] = true;
    return true;
  }
  else
  {
    return false;
  }
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool GetAbortFlag( char Flag )                                                                   *
 *                                                                                                                         *
 * input:               : char Flag : action flag number                                                                   *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = no abort detected                                                               *
 *                                 true = abort flag detected                                                              *
 *                                                                                                                         *
 * description:         : This is the function to get the abort flag.                                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEComPlc::GetAbortFlag( char Flag )
{
  return ActionAbort[Flag - 1];
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void WriteActionStartedFlag( char Flag, bool State )                                             *
 *                                                                                                                         *
 * input:               : char Flag : action flag number                                                                   *
 *                        bool State : state of started flag                                                     					 *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to write a specified started flag.                                          *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::WriteActionStartedFlag( char Flag, bool State )
{
  ActionStarted[Flag - 1] = State;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void WriteActionFinishedFlag( char Flag, bool State )                                            *
 *                                                                                                                         *
 * input:               : char Flag : action flag number                                                                   *
 *                        bool State : state of finished flag                                                   					 *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to write a specified finished flag.                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::WriteActionFinishedFlag( char Flag, bool State )
{
  ActionFinished[Flag - 1] = State;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void WriteActionAbortFlag( char Flag, bool State )                                               *
 *                                                                                                                         *
 * input:               : char Flag : action flag number                                                                   *
 *                        bool State : state of abort flag                                                      					 *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to write a specified abort flag.                                            *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::WriteActionAbortFlag( char Flag, bool State )
{
  ActionAbort[Flag - 1] = State;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetErrorCode( unsigned int Type, int ErrorCode )                                            *
 *                                                                                                                         *
 * input:               : unsigned int Type : type of error                                                                *
 *                        int ErrorCode : error code                                                            					 *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set the error code.                                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::SetErrorCode( unsigned int Type, int ErrorCode )
{
  ActualErrorCode[Type] = ErrorCode;
  return;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadErrorCode( unsigned int Type )                                                           *
 *                                                                                                                         *
 * input:               : unsigned int Type : type of error                                                                *
 *                                                                                                                         *
 * output:              : int : error code                                                                                 *
 *                                                                                                                         *
 * description:         : This is the function to read the error code.                                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEComPlc::ReadErrorCode( unsigned int Type )
{
  int RetVal = 0;

  if( ActualErrorCode[GLOBAL_ERROR] != 0 )
  {
    RetVal = ActualErrorCode[GLOBAL_ERROR];
    ActualErrorCode[GLOBAL_ERROR] = 0;
  }
  else
  {
    RetVal = ActualErrorCode[Type];
    ActualErrorCode[Type] = 0;
  }

  return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetReady( void )                                                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set ready flag.                                                          *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::SetReady( void )
{
  Ready = true;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetBootReady( void )                                                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set booting ready flag.                                                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::SetBootReady( void )
{
  BootReady = true;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : float GetActualPressure( void )                                                                  *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : float : actual pressure                                                                          *
 *                                                                                                                         *
 * description:         : This is the function to get the actual chamber pressure.                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
float SEComPlc::GetActualPressure( void )
{
  return ChamberPressureActual;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : float GetActualFlow( void )                                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : float : actual flow                                                                              *
 *                                                                                                                         *
 * description:         : This is the function to get the actual chamber flow.                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
float SEComPlc::GetActualFlow( void )
{
  return ChamberFlowActual;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : float GetActualLaserPower( void )                                                                *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : float : actual laser power                                                                       *
 *                                                                                                                         *
 * description:         : This is the function to get the actual chamber laser power.                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
float SEComPlc::GetActualLaserPower( void )
{
  return ChamberLaserPowerActual;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : unsigned int GetDigitalIn( void )                                                                *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : unsigned int : digital in flag                                                                   *
 *                                                                                                                         *
 * description:         : This is the function to get the digital trigger flags (output from plc communication).           *
 *-------------------------------------------------------------------------------------------------------------------------*/
unsigned int SEComPlc::GetDigitalIn( void )
{
  return DigitalIn;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetDigitalOut( unsigned int DigitalOut )                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : unsigned int : digital out flag                                                                  *
 *                                                                                                                         *
 * description:         : This is the function to set the digital trigger flags (input from plc communication).            *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEComPlc::SetDigitalOut( unsigned int DigitalOut )
{
  this->DigitalOut = DigitalOut;
}
