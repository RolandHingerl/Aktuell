// SELaserTrimming.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <tchar.h>
#include <Windows.h>
#include <conio.h>
#include <winsock.h>
#include <locale.h>


#include "..\Library\OpConServicesCommon.h"

#include "SELaserTrimming.h"
#include "SETrimmingChamber.h"
#include "SEComPlc.h"
#if 0
#include "..\Library\SETestDLL.h"
#else
#include "..\Library\LSTestDLL.h"
#include "..\Library\MCDataEx.h"
#endif
HANDLE hThreadHandleAction[2][32] = {
	{ NULL, NULL, NULL,	NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL },
	{ NULL, NULL, NULL,	NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL, NULL, NULL, NULL, NULL,
	  NULL, NULL } }; 

LPTHREAD_START_ROUTINE ActionStartRoutine[2][32] = {
	{ ( LPTHREAD_START_ROUTINE ) HandleAction1Chamber1, 
	  ( LPTHREAD_START_ROUTINE ) HandleAction2Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleAction3Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleAction4Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleAction5Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleAction6Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleAction7Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleAction8Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleAction9Chamber1,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy },
    { ( LPTHREAD_START_ROUTINE ) HandleAction1Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction2Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction3Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction4Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction5Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction6Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction7Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction8Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleAction9Chamber2,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy,
	  ( LPTHREAD_START_ROUTINE ) HandleActionDummy } };

SETrimmingChamber *SETrimmChamber[CHAMBER_COUNT];
SEComPlc *ComPlc[CHAMBER_COUNT];
SEComMicroLas *ComMicroLas[CHAMBER_COUNT];

HANDLE hThreadDeleteStatistics = NULL;
bool DeinstallThreadCheckStatisticDelete = false;

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : LONG MeasInFastTrimmingCellChamber1( ... )                                                       *
 *                                                                                                                         *
 * input:               : WORD wFUIdx :                                                                                    *
 *                        WORD wPartIdx :                                                                                  *
 *                        WORD eMainType : type of measurement value                                                       *
 *                        WORD rMVal : measured value                                                                      *
 *                        WORD tsMTime : receive time                                                                      *
 *                        WORD lResIdx : result index                                                                      *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the callback function (ADVOV card) to handle measured result values (fast measurement)   *
 *                        inside test sequence at trimming cell chamber 1.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
LONG MeasInFastTrimmingCellChamber1( WORD wFUIdx,WORD wPartIdx,/*eIMTBasTypes*/int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx )
{
	//handle measured values
	SETrimmChamber[CHAMBER1]->GetTrimmingCell()->HandleTestSequenceValues( wFUIdx, wPartIdx, eMainType, 0, rMVal, (float)tsMTime * 10, lResIdx );
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void MeasInTrimmingCellChamber1( void )                                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the thread function to handle measured result values (normally measurement)              *
 *                        inside test sequence at trimming cell chamber 1.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
void MeasInTrimmingCellChamber1( void )
{
	int RetVal = 0;			//return value
	TFLWRESULT tTFlwResult; //flow result
	LONG lMainIndex, lSubIndex, lLocIdx; //split index

	//endless loop (thread)
	do
	{
		if( SETrimmChamber[CHAMBER1]->GetTrimmingCell()->IsSequenceStarted() == true )
		{
			//get result
			SETrimmChamber[CHAMBER1]->GetTrimmingCell()->SetReadDataAdvov( true );
			RetVal = MCxTFlowResult( LSTestHandle, 1, &tTFlwResult );

			//check ok
			if( ( RetVal == 0 ) && ( tTFlwResult.ResultIdx != 0 ) )
			{
				//split index
				RetVal = MeasSplittResIdx( LSTestHandle, &lMainIndex, &lSubIndex, &lLocIdx, tTFlwResult.ResultIdx );
#if 0
#if defined (_INDUTRON_PRINT_MORE)
				printf("###MeasInTrimmingCellChamber1### lMainIndex=%d, lSubIndex=%d, lLocIdx=%d, Value=%f\n", lMainIndex, lSubIndex, lLocIdx, tTFlwResult.MeasRes[0]);
#endif
#endif
				//check ok
				if( RetVal == 0 )
				{
				
					switch ( SETrimmChamber[CHAMBER1]->getProcessType() )
					{
					case ProcessType::Ip4MeasureUNernstControl:   //10: IP4 measurement under Nernst voltage control  
					case ProcessType::Ip4Measure2PointUp:		  //11:	IP4 measurement via 2-point UP measurement
						//the lSubIndex of the cyclic measurements are <900 (900 .. 999)
						if ( lSubIndex < 900 )
						{							//repeat over all tiepoints (running variable i)
							for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
							{
								//handle measured values
								SETrimmChamber[CHAMBER1]->GetTrimmingCell()->HandleTestSequenceValues(0, i, lMainIndex, lSubIndex, tTFlwResult.MeasRes[i], tTFlwResult.TestTime * 1000, 0);
							}
						}
						break;
					default:
						//?RH: Warum diese Unterscheidung 
						if ((lMainIndex != IMT_RiAC && lMainIndex != IMT_IPu && lMainIndex != IMT_RHh && lMainIndex != IMT_PH) || lSubIndex == 1 || lSubIndex == 2 || lSubIndex == 3)
						{
							//repeat over all tiepoints (running variable i)
							for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
							{
								if ((lSubIndex != 1) &&
									(lSubIndex != 2) &&
									(lSubIndex != 3))
								{
									lSubIndex = 0;
								}
								//handle measured values
								SETrimmChamber[CHAMBER1]->GetTrimmingCell()->HandleTestSequenceValues(0, i, lMainIndex, lSubIndex, tTFlwResult.MeasRes[i], tTFlwResult.TestTime * 1000, 0);
							}
						}
						break;
					}
				}
			
				if( tTFlwResult.TFlowEnd != 0 ) 
				{
					SETrimmChamber[CHAMBER1]->GetTrimmingCell()->SetSequenceFinished( true );
					SETrimmChamber[CHAMBER1]->GetTrimmingCell()->SequenceStarted = false;		
				}
				else
				{
					SETrimmChamber[CHAMBER1]->GetTrimmingCell()->SetSequenceFinished( false );
				}
			}
			SETrimmChamber[CHAMBER1]->GetTrimmingCell()->SetReadDataAdvov( false );
			Sleep( 100 );
		}
	}while( SETrimmChamber[CHAMBER1]->GetTrimmingCell()->DeinstallThreadRequested() == false );	

	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : LONG MeasInFastReferenceCellChamber1( ... )                                                      *
 *                                                                                                                         *
 * input:               : WORD wFUIdx :                                                                                    *
 *                        WORD wPartIdx :                                                                                  *
 *                        WORD eMainType : type of measurement value                                                       *
 *                        WORD rMVal : measured value                                                                      *
 *                        WORD tsMTime : receive time                                                                      *
 *                        WORD lResIdx : result index                                                                      *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the callback function (ADVOV card) to handle measured result values (fast measurement)   *
 *                        inside test sequence at reference cell chamber 1.                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
LONG MeasInFastReferenceCellChamber1( WORD wFUIdx,WORD wPartIdx,/*eIMTBasTypes*/int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx )
{
	//handle measured values
	SETrimmChamber[CHAMBER1]->GetReferenceCell()->HandleTestSequenceValues( wFUIdx, wPartIdx, eMainType, 0, rMVal, (float)tsMTime * 10, lResIdx );
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void MeasInreferenceCellChamber1( void )                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the thread function to handle measured result values (normally measurement)              *
 *                        inside test sequence at reference cell chamber 1.                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
void MeasInReferenceCellChamber1( void )
{
	int RetVal = 0;		//return value
	TFLWRESULT tTFlwResult; //flow result
	LONG lMainIndex, lSubIndex, lLocIdx;//split index
	
	//endless loop (thread)
	do
	{
		if( SETrimmChamber[CHAMBER1]->GetReferenceCell()->IsSequenceStarted() == true )
		{
			//get result
			SETrimmChamber[CHAMBER1]->GetReferenceCell()->SetReadDataAdvov( true );
			RetVal = MCxTFlowResult( LSTestHandle, 2, &tTFlwResult );
			//check ok
			if ((RetVal == 0) && (tTFlwResult.ResultIdx != 0))
			{
				//check ok
				if ((RetVal == 0) && (tTFlwResult.ResultIdx != 0))
				{
					//split index
					RetVal = MeasSplittResIdx(LSTestHandle, &lMainIndex, &lSubIndex, &lLocIdx, tTFlwResult.ResultIdx);

					//check ok
					if (RetVal == 0)
					{
						//repeat over all tiepoints (running variable i)
						for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
						{
							//handle measured values
							if (lSubIndex > 900)
							{
								//RH: no cyclic measurements at recference cell
								//RH: cyclic measurement
								lSubIndex = 0;
							}
							SETrimmChamber[CHAMBER1]->GetReferenceCell()->HandleTestSequenceValues(0, i, lMainIndex, lLocIdx, tTFlwResult.MeasRes[i], tTFlwResult.TestTime * 1000, 0);
						}
					}
				}
				if (tTFlwResult.TFlowEnd != 0)
				{
					SETrimmChamber[CHAMBER1]->GetReferenceCell()->SetSequenceFinished(true);
					SETrimmChamber[CHAMBER1]->GetReferenceCell()->SequenceStarted = false;
				}
				else
				{
					SETrimmChamber[CHAMBER1]->GetReferenceCell()->SetSequenceFinished(false);
				}
			}
			SETrimmChamber[CHAMBER1]->GetReferenceCell()->SetReadDataAdvov( false );
			Sleep( 100 );
		}
	}while( SETrimmChamber[CHAMBER1]->GetReferenceCell()->DeinstallThreadRequested() == false );	

	ExitThread(0);	
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void HandleChamber1( void )                                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function to handle the process chamber 1.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleChamber1( void )
{
	do
	{
		SETrimmChamber[CHAMBER1]->HandleProcess(); 
		//Dll bringt manchmal MVAL Ueberlauf
		Sleep(100+200);
	}while( SETrimmChamber[CHAMBER1]->DeinstallThreadRequested() == false );	

	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : LONG MeasInFastTrimmingCellChamber2( ... )                                                       *
 *                                                                                                                         *
 * input:               : WORD wFUIdx :                                                                                    *
 *                        WORD wPartIdx :                                                                                  *
 *                        WORD eMainType : type of measurement value                                                       *
 *                        WORD rMVal : measured value                                                                      *
 *                        WORD tsMTime : receive time                                                                      *
 *                        WORD lResIdx : result index                                                                      *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the callback function (ADVOV card) to handle measured result values (fast measurement)   *
 *                        inside test sequence at trimming cell chamber 2.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
LONG MeasInFastTrimmingCellChamber2(WORD wFUIdx,WORD wPartIdx,/*eIMTBasTypes*/int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx)
{
	//handle measured values
	SETrimmChamber[CHAMBER2]->GetTrimmingCell()->HandleTestSequenceValues( wFUIdx, wPartIdx, eMainType, 0, rMVal, (float)tsMTime * 10, lResIdx );
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void MeasInTrimmingCellChamber2( void )                                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the thread function to handle measured result values (normally measurement)              *
 *                        inside test sequence at trimming cell chamber 2.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
void MeasInTrimmingCellChamber2( void )
{
	int RetVal = 0;																													//return value
	TFLWRESULT tTFlwResult; 																								//flow result
	LONG lMainIndex, lSubIndex, lLocIdx;																		//split index

	//endless loop (thread)
	do
	{
		if( SETrimmChamber[CHAMBER2]->GetTrimmingCell()->IsSequenceStarted() == true )
		{
			//get result
			SETrimmChamber[CHAMBER2]->GetTrimmingCell()->SetReadDataAdvov( true );
			RetVal = MCxTFlowResult( LSTestHandle, 4, &tTFlwResult );

			//check ok
			if( ( RetVal == 0 ) && ( tTFlwResult.ResultIdx != 0 ) )
			{
				//split index
				RetVal = MeasSplittResIdx( LSTestHandle, &lMainIndex, &lSubIndex, &lLocIdx, tTFlwResult.ResultIdx );

				//check ok
				if( RetVal == 0 )
				{
					if( ( lMainIndex != IMT_RiAC && lMainIndex != IMT_IPu && lMainIndex != IMT_RHh && lMainIndex != IMT_PH) || lSubIndex == 1 || lSubIndex == 2 || lSubIndex == 3 )
					{
						//repeat over all tiepoints (running variable i)
						for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
						{
							if( ( lSubIndex != 1 ) &&
									( lSubIndex != 2 ) &&
									( lSubIndex != 3 ) )
							{
								lSubIndex = 0;
							}
							//handle measured values
							SETrimmChamber[CHAMBER2]->GetTrimmingCell()->HandleTestSequenceValues( 0, i, lMainIndex, lSubIndex, tTFlwResult.MeasRes[i], tTFlwResult.TestTime * 1000 ,0 );
						}																								
					}
				}

				if( tTFlwResult.TFlowEnd != 0 ) 
				{
					SETrimmChamber[CHAMBER2]->GetTrimmingCell()->SetSequenceFinished( true );
				}
				else
				{
					SETrimmChamber[CHAMBER2]->GetTrimmingCell()->SetSequenceFinished( false );
				}

			}
			SETrimmChamber[CHAMBER2]->GetTrimmingCell()->SetReadDataAdvov( false );
			Sleep( 100 );
		}
	}while( SETrimmChamber[CHAMBER2]->GetTrimmingCell()->DeinstallThreadRequested() == false );	

	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : LONG MeasInFastReferenceCellChamber2( ... )                                                      *
 *                                                                                                                         *
 * input:               : WORD wFUIdx :                                                                                    *
 *                        WORD wPartIdx :                                                                                  *
 *                        WORD eMainType : type of measurement value                                                       *
 *                        WORD rMVal : measured value                                                                      *
 *                        WORD tsMTime : receive time                                                                      *
 *                        WORD lResIdx : result index                                                                      *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the callback function (ADVOV card) to handle measured result values (fast measurement)   *
 *                        inside test sequence at reference cell chamber 2.                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
LONG MeasInFastReferenceCellChamber2(WORD wFUIdx,WORD wPartIdx,/*eIMTBasTypes*/int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx)
{
	//handle measured values
	SETrimmChamber[CHAMBER2]->GetReferenceCell()->HandleTestSequenceValues( wFUIdx, wPartIdx, eMainType, 0, rMVal, (float)tsMTime * 10, lResIdx );
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void MeasInReferenceCellChamber2( void )                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the thread function to handle measured result values (normally measurement)              *
 *                        inside test sequence at reference cell chamber 2.                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
void MeasInReferenceCellChamber2( void )
{
	int RetVal = 0;																													//return value
	TFLWRESULT tTFlwResult; 																								//flow result
	LONG lMainIndex, lSubIndex, lLocIdx;																		//split index
	
	//endless loop (thread)
	do
	{
		if( SETrimmChamber[CHAMBER2]->GetReferenceCell()->IsSequenceStarted() == true )
		{
			//get result
			SETrimmChamber[CHAMBER2]->GetReferenceCell()->SetReadDataAdvov( true );
			RetVal = MCxTFlowResult( LSTestHandle, 8, &tTFlwResult );

			//check ok
			if( ( RetVal == 0 ) && ( tTFlwResult.ResultIdx != 0 ) )
			{
				//split index
				RetVal = MeasSplittResIdx( LSTestHandle, &lMainIndex, &lSubIndex, &lLocIdx, tTFlwResult.ResultIdx );

				//check ok
				if( RetVal == 0 )
				{
					//repeat over all tiepoints (running variable i)
					for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
					{
						//handle measured values
						SETrimmChamber[CHAMBER2]->GetReferenceCell()->HandleTestSequenceValues( 0, i, lMainIndex, 0, tTFlwResult.MeasRes[i], tTFlwResult.TestTime * 1000 ,0 );
					}
				}
			}
			SETrimmChamber[CHAMBER2]->GetReferenceCell()->SetReadDataAdvov( false );
			Sleep( 100 );
		}
	}while( SETrimmChamber[CHAMBER2]->GetReferenceCell()->DeinstallThreadRequested() == false );	

	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void HandleChamber2( void )                                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function to handle the process chamber 2.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleChamber2( void )
{
	do
	{
		SETrimmChamber[CHAMBER2]->HandleProcess(); 
		Sleep(100);
	}while( SETrimmChamber[CHAMBER2]->DeinstallThreadRequested() == false );	

	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleSocketServerPlcChamber1( void )                                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the socket server receiving and sending.                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleSocketServerPlcChamber1( void )
{
	do
	{
		if( ComPlc[CHAMBER1]->RecvSend() == -2 )
		{
			ComPlc[CHAMBER1]->SetErrorCode( GLOBAL_ERROR, FLT_PLC_CONNECT );
			printf( "Chamber[1]:HandleSocketServerPlc:Plc:socket communication lost!\n" );
			SETrimmChamber[CHAMBER1]->MasterReset();
			while( ComPlc[CHAMBER1]->Connect() != 0 )
			{
				Sleep( 500 );
			}	
		}
		Sleep( 10 );
	}while( 1 );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleSocketClientMicroLasChamber1( void )                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the socket client connection.                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleSocketClientMicroLasChamber1( void )
{
	do
	{
		if( ComMicroLas[CHAMBER1]->IsConnected() == false )
		{
			ComPlc[CHAMBER1]->SetErrorCode( GLOBAL_ERROR, FLT_MICROLAS_CONNECT );
			printf( "Chamber[1]:HandleSocketClientMicroLas:socket communication lost!\n" );
			ComPlc[CHAMBER1]->MasterReset();
			SETrimmChamber[CHAMBER1]->MasterReset();
			while( ( ComMicroLas[CHAMBER1]->Initializing() != 0 ) || ( ComMicroLas[CHAMBER1]->Connect() != 0 ) )
			{
				Sleep( 500 );
			}
		}
		Sleep( 10 );
	}while( 1 ); 
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleSocketServerPlcChamber2( void )                                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the socket server receiving and sending.                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleSocketServerPlcChamber2( void )
{
	do
	{
		if( ComPlc[CHAMBER2]->RecvSend() == -2 )
		{
			ComPlc[CHAMBER2]->SetErrorCode( GLOBAL_ERROR, FLT_PLC_CONNECT );
			printf( "Chamber[2]:HandleSocketServerPlc:Plc:socket communication lost!\n" );
			SETrimmChamber[CHAMBER2]->MasterReset();
			while( ComPlc[CHAMBER2]->Connect() != 0 )
			{
				Sleep( 500 );
			}	
		}
		Sleep( 10 );
	}while( 1 );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleSocketClientMicroLasChamber2( void )                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the socket client connection.                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleSocketClientMicroLasChamber2( void )
{
	do
	{
		if( ComMicroLas[CHAMBER2]->IsConnected() == false )
		{
			ComPlc[CHAMBER2]->SetErrorCode( GLOBAL_ERROR, FLT_MICROLAS_CONNECT );
			printf( "Chamber[2]:HandleSocketClientMicroLas:socket communication lost!\n" );
			ComPlc[CHAMBER2]->MasterReset();
			SETrimmChamber[CHAMBER2]->MasterReset();
			while( ( ComMicroLas[CHAMBER2]->Initializing() != 0 ) || ( ComMicroLas[CHAMBER2]->Connect() != 0 ) )
			{
				Sleep( 500 );
			}
		}
		Sleep( 10 );
	}while( 1 ); 
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction1Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 1 (manual: master reset).                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction1Chamber1( void )
{
	ComPlc[CHAMBER1]->WriteActionStartedFlag( 1, true );
	SETrimmChamber[CHAMBER1]->MasterReset();
	ComPlc[CHAMBER1]->WriteActionFinishedFlag( 1, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction2Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 2 (automatic: start trimming).                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction2Chamber1( void )
{
	if( ( SETrimmChamber[CHAMBER1]->ChamberAllCellsReady() == true ) && 
			( SETrimmChamber[CHAMBER1]->ParameterReadyForTrimming() == true ) )
	{
		if( ( SETrimmChamber[CHAMBER1]->GetTrimmingCell()->IsCardTemperatureOk() == true ) &&
				( SETrimmChamber[CHAMBER1]->GetReferenceCell()->IsCardTemperatureOk() == true ) )
		{
			SETrimmChamber[CHAMBER1]->InstallThread( (LPTHREAD_START_ROUTINE) MeasInTrimmingCellChamber1, 
																		(LPTHREAD_START_ROUTINE) MeasInReferenceCellChamber1, 
																		(LPTHREAD_START_ROUTINE) HandleChamber1, 
																		MeasInFastTrimmingCellChamber1, MeasInFastReferenceCellChamber1 );
#if defined _INDUTRON_PRINT_MORE
			//RH:
			//SETrimmChamber[CHAMBER1]->StartProcess( MeasureTrimmingSelection );
			printf("## ProcessType: %d\n", (ProcessType)ComPlc[CHAMBER1]->GetAssemblyData().AssemblyDataCommon.ProcessType);
#endif
			SETrimmChamber[CHAMBER1]->StartProcess( (ProcessType)ComPlc[CHAMBER1]->GetAssemblyData().AssemblyDataCommon.ProcessType );

			ComPlc[CHAMBER1]->WriteActionStartedFlag( 2, true );
			do
			{
				Sleep( 100 );
			}
			while( ( SETrimmChamber[CHAMBER1]->ProcessFinished() == false ) && ( ComPlc[CHAMBER1]->GetAbortFlag( 2 ) == false ) );  
			SETrimmChamber[CHAMBER1]->StopProcess( );
			ComPlc[CHAMBER1]->WriteActionFinishedFlag( 2, true );			
		}
		else
		{
			ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_ADVOV_OVERTEMP );
			ComPlc[CHAMBER1]->WriteActionStartedFlag( 2, true );
			ComPlc[CHAMBER1]->WriteActionFinishedFlag( 2, true );
		}
	}
	else
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_ACTION_START_CONDITIONS );
		ComPlc[CHAMBER1]->WriteActionStartedFlag( 2, true );
		ComPlc[CHAMBER1]->WriteActionFinishedFlag( 2, true );
	}
	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction3Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 3 (manual: scanner move abs).                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction3Chamber1( void )
{
	ComPlc[CHAMBER1]->WriteActionStartedFlag( 3, true );
	if( ComMicroLas[CHAMBER1]->MoveAbs( ComPlc[CHAMBER1]->GetParamDataMoveAbs().XPos,
																			 ComPlc[CHAMBER1]->GetParamDataMoveAbs().YPos,
																			 ComPlc[CHAMBER1]->GetParamDataMoveAbs().ZPos ) != 0 )
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_MICROLAS_SCANNER );
	}						
	ComPlc[CHAMBER1]->WriteActionFinishedFlag( 3, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction4Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 4 (manual: scanner do trimming cut).           *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction4Chamber1( void )
{
	ComPlc[CHAMBER1]->WriteActionStartedFlag( 4, true );
	
	if( ComMicroLas[CHAMBER1]->TrimmingCut( ComPlc[CHAMBER1]->GetParamDataTrimmingCut().StartXPos,
																					 ComPlc[CHAMBER1]->GetParamDataTrimmingCut().StartYPos,
																					 ComPlc[CHAMBER1]->GetParamDataTrimmingCut().StartZPos,
																					 ComPlc[CHAMBER1]->GetParamDataTrimmingCut().EndXPos,
																					 ComPlc[CHAMBER1]->GetParamDataTrimmingCut().EndYPos,
																					 ComPlc[CHAMBER1]->GetParamDataTrimmingCut().EndZPos,
																					 ComPlc[CHAMBER1]->GetParamDataTrimmingCut().LaserPower ) != 0 )
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_MICROLAS_SCANNER );
	}
	ComPlc[CHAMBER1]->WriteActionFinishedFlag( 4, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction5Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 5 (manual: scanner do laser point).            *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction5Chamber1( void )
{
	ComPlc[CHAMBER1]->WriteActionStartedFlag( 5, true );
	
	if( ComMicroLas[CHAMBER1]->LaserPoint( ComPlc[CHAMBER1]->GetParamDataLaserPoint().XPos,
																					ComPlc[CHAMBER1]->GetParamDataLaserPoint().YPos,
																					ComPlc[CHAMBER1]->GetParamDataLaserPoint().ZPos,
																					ComPlc[CHAMBER1]->GetParamDataLaserPoint().LaserPower,
																					ComPlc[CHAMBER1]->GetParamDataLaserPoint().LaserTime ) != 0 )
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_MICROLAS_SCANNER );
	}
	ComPlc[CHAMBER1]->WriteActionFinishedFlag( 5, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction6Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 6 (manual: scanner write text).                *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction6Chamber1( void )
{
	int FuncRetVal = 0;

	ComPlc[CHAMBER1]->WriteActionStartedFlag( 6, true );
	
	if( FuncRetVal = ComMicroLas[CHAMBER1]->WriteText( ComPlc[CHAMBER1]->GetParamDataWriteText().Code,
																				ComPlc[CHAMBER1]->GetParamDataWriteText().XPos,
																				ComPlc[CHAMBER1]->GetParamDataWriteText().YPos,
																				ComPlc[CHAMBER1]->GetParamDataWriteText().ZPos,
																				ComPlc[CHAMBER1]->GetParamDataWriteText().Angle,
																				ComPlc[CHAMBER1]->GetParamDataWriteText().LaserPower ) != 0 )
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_MICROLAS_MICROLAS_FIRST + FuncRetVal );
	}
	ComPlc[CHAMBER1]->WriteActionFinishedFlag( 6, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction7Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 7 (manual: start heat and measure).            *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction7Chamber1( void )
{
	int ActTimeout = 0;
	if( ( SETrimmChamber[CHAMBER1]->ChamberAllCellsReady() == true ) && 
			( SETrimmChamber[CHAMBER1]->ParameterReadyToStart() ) )
	{
		ComPlc[CHAMBER1]->WriteActionStartedFlag( 7, true );
		SETrimmChamber[CHAMBER1]->InstallThread( (LPTHREAD_START_ROUTINE) MeasInTrimmingCellChamber1, 
																			(LPTHREAD_START_ROUTINE) MeasInReferenceCellChamber1, 
																			NULL, 
																			MeasInFastTrimmingCellChamber1, 
																			MeasInFastReferenceCellChamber1 );
#if defined _INDUTRON_PRINT_MORE
		//RH:
		//SETrimmChamber[CHAMBER1]->StartProcess( MeasureTrimmingSelection );
		printf("## ProcessType: %d\n", (ProcessType)ComPlc[CHAMBER1]->GetAssemblyData().AssemblyDataCommon.ProcessType);
#endif
		SETrimmChamber[CHAMBER1]->StartProcess( (ProcessType)ComPlc[CHAMBER1]->GetAssemblyData().AssemblyDataCommon.ProcessType );
		do
		{
			ActTimeout++;																												//increment act timeout
			Sleep( 100 );																											  //wait 100ms
		}
		while( ( ActTimeout <= 600 ) && ( ComPlc[CHAMBER1]->GetAbortFlag( 7 ) == false ) );
		SETrimmChamber[CHAMBER1]->StopProcess( );
		ComPlc[CHAMBER1]->WriteActionFinishedFlag( 7, true );
	}
	else
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_ACTION_START_CONDITIONS );
		ComPlc[CHAMBER1]->WriteActionStartedFlag( 7, true );
		ComPlc[CHAMBER1]->WriteActionFinishedFlag( 7, true );
	}
	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction8Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 8 (manual: start normal measure).              *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction8Chamber1( void )
{
	if( SETrimmChamber[CHAMBER1]->ChamberAllCellsReady() )
	{
		ComPlc[CHAMBER1]->WriteActionStartedFlag( 8, true );
		SETrimmChamber[CHAMBER1]->InstallThread( (LPTHREAD_START_ROUTINE) MeasInTrimmingCellChamber1, 
																			(LPTHREAD_START_ROUTINE) NULL, 
																			(LPTHREAD_START_ROUTINE) HandleChamber1, 
																			NULL, 
																			NULL );
		switch( ComPlc[CHAMBER1]->GetNormalNumber( ) )
		{
		case NormalNumber::PositionNormal:
				SETrimmChamber[CHAMBER1]->StartProcess( ProcessType::MeasurePositionNormal );
				break;

		case NormalNumber::HeaterNormal:
				SETrimmChamber[CHAMBER1]->StartProcess( ProcessType::MeasureHeaterNormal );
				break;

		case NormalNumber::UniversalNormal:
				SETrimmChamber[CHAMBER1]->StartProcess( ProcessType::MeasureUniversalNormal );
				break;

		case NormalNumber::UnNormal:
				SETrimmChamber[CHAMBER1]->StartProcess( ProcessType::MeasureUnNormal );
				break;

		case NormalNumber::IpNormal:
				SETrimmChamber[CHAMBER1]->StartProcess( ProcessType::MeasureIpNormal );
				break;

		case NormalNumber::IlmNormal:
				SETrimmChamber[CHAMBER1]->StartProcess( ProcessType::MeasureIlmNormal );
				break;

			default:

				break;

		}

		do
		{
			Sleep( 100 );																																										
		}
		while( ( SETrimmChamber[CHAMBER1]->ProcessFinished() == false ) && ( ComPlc[CHAMBER1]->GetAbortFlag( 8 ) == false ) );  
		SETrimmChamber[CHAMBER1]->StopProcess( );
		ComPlc[CHAMBER1]->WriteActionFinishedFlag( 8, true );
	}
	else
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_ACTION_START_CONDITIONS );
		ComPlc[CHAMBER1]->WriteActionStartedFlag( 8, true );
		ComPlc[CHAMBER1]->WriteActionFinishedFlag( 8, true );
	}
	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction9Chamber1( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 9 (manual: check and update firmware).         *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction9Chamber1( void )
{
	int RetVal = 0;

	ComPlc[CHAMBER1]->WriteActionStartedFlag( 9, true );
	
	RetVal = SETrimmChamber[CHAMBER1]->CheckFirmware();
	if( RetVal == -1 )																											//check retval
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_ADVOV_UPDATE_MAIN ); //set error code
	}
	if( RetVal == -2 )																											//check retval
	{
		ComPlc[CHAMBER1]->SetErrorCode( GET_STATE, FLT_ADVOV_UPDATE_MODULE ); //set error code
	}
	
	ComPlc[CHAMBER1]->WriteActionFinishedFlag( 9, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction10Chamber1( void )                                                                   *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 10 ( ).                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction10Chamber1( void )
{
	ComPlc[CHAMBER1]->WriteActionStartedFlag( 10, true );

	ComPlc[CHAMBER1]->WriteActionFinishedFlag( 10, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction1Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 1 (manual: master reset).                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction1Chamber2( void )
{
	ComPlc[CHAMBER2]->WriteActionStartedFlag( 1, true );
	SETrimmChamber[CHAMBER2]->MasterReset();
	ComPlc[CHAMBER2]->WriteActionFinishedFlag( 1, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction2Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 2 (automatic: start trimming).                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction2Chamber2( void )
{
	if( ( SETrimmChamber[CHAMBER2]->ChamberAllCellsReady() == true ) && 
			( SETrimmChamber[CHAMBER2]->ParameterReadyForTrimming() == true ) )
	{
		if( ( SETrimmChamber[CHAMBER2]->GetTrimmingCell()->IsCardTemperatureOk() == true ) &&
				( SETrimmChamber[CHAMBER2]->GetReferenceCell()->IsCardTemperatureOk() == true ) )
		{
			SETrimmChamber[CHAMBER2]->InstallThread( (LPTHREAD_START_ROUTINE) MeasInTrimmingCellChamber2, 
																		(LPTHREAD_START_ROUTINE) MeasInReferenceCellChamber2, 
																		(LPTHREAD_START_ROUTINE) HandleChamber2, 
																		MeasInFastTrimmingCellChamber2, 
																		MeasInFastReferenceCellChamber2 );
			SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasureTrimmingSelection );
			ComPlc[CHAMBER2]->WriteActionStartedFlag( 2, true );
			do
			{
				Sleep( 100 );
			}
			while( ( SETrimmChamber[CHAMBER2]->ProcessFinished() == false ) && ( ComPlc[CHAMBER2]->GetAbortFlag( 2 ) == false ) );  
			SETrimmChamber[CHAMBER2]->StopProcess( );
			ComPlc[CHAMBER2]->WriteActionFinishedFlag( 2, true );
		}
		else
		{
			ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_ADVOV_OVERTEMP );
			ComPlc[CHAMBER2]->WriteActionStartedFlag( 2, true );
			ComPlc[CHAMBER2]->WriteActionFinishedFlag( 2, true );
		}
	}
	else
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_ACTION_START_CONDITIONS );
		ComPlc[CHAMBER2]->WriteActionStartedFlag( 2, true );
		ComPlc[CHAMBER2]->WriteActionFinishedFlag( 2, true );
	}
	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction3Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 3 (manual: scanner move abs).                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction3Chamber2( void )
{
	ComPlc[CHAMBER2]->WriteActionStartedFlag( 3, true );
	if( ComMicroLas[CHAMBER2]->MoveAbs( ComPlc[CHAMBER2]->GetParamDataMoveAbs().XPos,
																			 ComPlc[CHAMBER2]->GetParamDataMoveAbs().YPos,
																			 ComPlc[CHAMBER2]->GetParamDataMoveAbs().ZPos ) != 0 )
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_MICROLAS_SCANNER );
	}	 
	ComPlc[CHAMBER2]->WriteActionFinishedFlag( 3, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction4Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 4 (manual: scanner do trimming cut).           *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction4Chamber2( void )
{
	ComPlc[CHAMBER2]->WriteActionStartedFlag( 4, true );
	
	if( ComMicroLas[CHAMBER2]->TrimmingCut( ComPlc[CHAMBER2]->GetParamDataTrimmingCut().StartXPos,
																					 ComPlc[CHAMBER2]->GetParamDataTrimmingCut().StartYPos,
																					 ComPlc[CHAMBER2]->GetParamDataTrimmingCut().StartZPos,
																					 ComPlc[CHAMBER2]->GetParamDataTrimmingCut().EndXPos,
																					 ComPlc[CHAMBER2]->GetParamDataTrimmingCut().EndYPos,
																					 ComPlc[CHAMBER2]->GetParamDataTrimmingCut().EndZPos,
																					 ComPlc[CHAMBER2]->GetParamDataTrimmingCut().LaserPower ) != 0 )
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_MICROLAS_SCANNER );
	}
	ComPlc[CHAMBER2]->WriteActionFinishedFlag( 4, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction5Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 5 (manual: scanner do laser point).            *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction5Chamber2( void )
{
	ComPlc[CHAMBER2]->WriteActionStartedFlag( 5, true );
	
	if( ComMicroLas[CHAMBER2]->LaserPoint( ComPlc[CHAMBER2]->GetParamDataLaserPoint().XPos,
																					ComPlc[CHAMBER2]->GetParamDataLaserPoint().YPos,
																					ComPlc[CHAMBER2]->GetParamDataLaserPoint().ZPos,
																					ComPlc[CHAMBER2]->GetParamDataLaserPoint().LaserPower,
																					ComPlc[CHAMBER2]->GetParamDataLaserPoint().LaserTime ) != 0 )
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_MICROLAS_SCANNER );
	}
	ComPlc[CHAMBER2]->WriteActionFinishedFlag( 5, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction6Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 6 (manual: scanner write text).                *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction6Chamber2( void )
{
	ComPlc[CHAMBER2]->WriteActionStartedFlag( 6, true );
	
	if( ComMicroLas[CHAMBER2]->WriteText( ComPlc[CHAMBER2]->GetParamDataWriteText().Code,
																				ComPlc[CHAMBER2]->GetParamDataWriteText().XPos,
																				ComPlc[CHAMBER2]->GetParamDataWriteText().YPos,
																				ComPlc[CHAMBER2]->GetParamDataWriteText().ZPos,
																				ComPlc[CHAMBER2]->GetParamDataWriteText().Angle,
																				ComPlc[CHAMBER2]->GetParamDataWriteText().LaserPower ) != 0 )
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_MICROLAS_SCANNER );
	}
	ComPlc[CHAMBER2]->WriteActionFinishedFlag( 6, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction7Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 7 (manual: start heat and measure).            *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction7Chamber2( void )
{
	int ActTimeout = 0;
	if( ( SETrimmChamber[CHAMBER2]->ChamberAllCellsReady() == true ) && 
			( SETrimmChamber[CHAMBER2]->ParameterReadyToStart() ) )
	{
		ComPlc[CHAMBER2]->WriteActionStartedFlag( 7, true );
		SETrimmChamber[CHAMBER2]->InstallThread( (LPTHREAD_START_ROUTINE) MeasInTrimmingCellChamber2, 
																			(LPTHREAD_START_ROUTINE) MeasInReferenceCellChamber2, 
																			NULL, 
																			MeasInFastTrimmingCellChamber2, 
																			MeasInFastReferenceCellChamber2 );

		SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasureTrimmingSelection );
		do
		{
			ActTimeout++;																												//increment act timeout
			Sleep( 100 );																											  //wait 100ms
		}
		while( ( ActTimeout <= 600 ) && ( ComPlc[CHAMBER2]->GetAbortFlag( 7 ) == false ) );
		SETrimmChamber[CHAMBER2]->StopProcess( );
		ComPlc[CHAMBER2]->WriteActionFinishedFlag( 7, true );
	}
	else
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_ACTION_START_CONDITIONS );
		ComPlc[CHAMBER2]->WriteActionStartedFlag( 7, true );
		ComPlc[CHAMBER2]->WriteActionFinishedFlag( 7, true );
	}
	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction8Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 8 (manual: start normal measure).             *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction8Chamber2( void )
{
	if( SETrimmChamber[CHAMBER2]->ChamberAllCellsReady() )
	{
		ComPlc[CHAMBER2]->WriteActionStartedFlag( 8, true );
		SETrimmChamber[CHAMBER2]->InstallThread( (LPTHREAD_START_ROUTINE) MeasInTrimmingCellChamber2, 
																			(LPTHREAD_START_ROUTINE) NULL, 
																			(LPTHREAD_START_ROUTINE) HandleChamber2, 
																			NULL, 
																			NULL );
		switch (ComPlc[CHAMBER2]->GetNormalNumber())
		{
		case NormalNumber::PositionNormal:
			SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasurePositionNormal);
			break;

		case NormalNumber::HeaterNormal:
			SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasureHeaterNormal);
			break;

		case NormalNumber::UniversalNormal:
			SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasureUniversalNormal);
			break;

		case NormalNumber::UnNormal:
			SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasureUnNormal);
			break;

		case NormalNumber::IpNormal:
			SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasureIpNormal);
			break;

		case NormalNumber::IlmNormal:
			SETrimmChamber[CHAMBER2]->StartProcess(ProcessType::MeasureIlmNormal);
			break;

		default:

			break;

		}

		do
		{
			Sleep( 100 );																																										
		}
		while( ( SETrimmChamber[CHAMBER2]->ProcessFinished() == false ) && ( ComPlc[CHAMBER2]->GetAbortFlag( 8 ) == false ) );  
		SETrimmChamber[CHAMBER2]->StopProcess( );
		ComPlc[CHAMBER2]->WriteActionFinishedFlag( 8, true );
	}
	else
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_ACTION_START_CONDITIONS );
		ComPlc[CHAMBER2]->WriteActionStartedFlag( 8, true );
		ComPlc[CHAMBER2]->WriteActionFinishedFlag( 8, true );
	}
	ExitThread(0);
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction9Chamber2( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 9 (manual: check and update firmware).         *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction9Chamber2( void )
{
	int RetVal = 0;

	ComPlc[CHAMBER2]->WriteActionStartedFlag( 9, true );
	
	RetVal = SETrimmChamber[CHAMBER2]->CheckFirmware();
	if( RetVal == -1 )																											//check retval
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_ADVOV_UPDATE_MAIN ); //set error code
	}
	if( RetVal == -2 )																											//check retval
	{
		ComPlc[CHAMBER2]->SetErrorCode( GET_STATE, FLT_ADVOV_UPDATE_MODULE ); //set error code
	}

	ComPlc[CHAMBER2]->WriteActionFinishedFlag( 9, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleAction10Chamber2( void )                                                                   *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action 10 ( ).                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleAction10Chamber2( void )
{
	ComPlc[CHAMBER2]->WriteActionStartedFlag( 10, true );

	ComPlc[CHAMBER2]->WriteActionFinishedFlag( 10, true );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleActionDummy( void )                                                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the action dummy.                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleActionDummy( void )
{
	;																																				//do nothing
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParameterOverTakenChamber1( unsigned int SubId )                                                 *
 *                                                                                                                         *
 * input:               : unsigned int SubId : parameter sub identifier                                                    *  
 *																																																												 *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the callback function for received parameter over taken. This function is called every   *
 *                        parameter receive.                                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int ParameterOverTakenChamber1( unsigned int SubId )
{
	int RetVal = 0;
	switch( SubId )
	{
		case 0:
			//trimming station parameter
			RetVal = SETrimmChamber[CHAMBER1]->SetParameter( ComPlc[CHAMBER1]->GetParamStationDataTrimming() );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_STATION_PARAMETER_DATA_TRIMMING_OUTSIDE_LIMIT );
			}
			RetVal = ComMicroLas[CHAMBER1]->SetParameter( ComPlc[CHAMBER1]->GetParamStationDataTrimming().ParamStationTrimmProcess.ParamStationLaser );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_MICROLAS_STATION_DATA_NOT_PLAUSIBLE );
			}
			break;

		case 1:
			//trimming parameter
			RetVal = SETrimmChamber[CHAMBER1]->SetParameter( ComPlc[CHAMBER1]->GetParamDataTrimming() );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_PARAMETER_DATA_TRIMMING_OUTSIDE_LIMIT );
			}
			RetVal = ComMicroLas[CHAMBER1]->SetParameter( ComPlc[CHAMBER1]->GetParamDataTrimming().ParamTrimmProcess.ParamLaser );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_MICROLAS_DATA_NOT_PLAUSIBLE );
			}
			break;
		case 2:
			//normal measure parameter
			RetVal = SETrimmChamber[CHAMBER1]->SetParameter( ComPlc[CHAMBER1]->GetParamDataNormalMeasure() );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_PARAMETER_DATA_NORMAL_OUTSIDE_LIMIT );
			}
			break;
		case 3:
			//move scanner abs
			;//no parameter copy
			break;
		case 4:
			//make trimming cut
			;//no parameter copy
			break;
		case 5:
			//parameter image processing
			RetVal = SETrimmChamber[CHAMBER1]->SetPositionImageProcessing( ComPlc[CHAMBER1]->GetParamDataImageProcessing().Tiepoint, 
																															ComPlc[CHAMBER1]->GetParamDataImageProcessing().XPos, 
																															ComPlc[CHAMBER1]->GetParamDataImageProcessing().YPos,
																															ComPlc[CHAMBER1]->GetParamDataImageProcessing().ZPos,
																															ComPlc[CHAMBER1]->GetParamDataImageProcessing().Diameter );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_IMAGEPROCESSING_DATA_OUTSIDE_LIMIT );
			}
			break;
		case 6:
			break;
		case 7:
			RetVal = SETrimmChamber[CHAMBER1]->SetAssemblyData( ComPlc[CHAMBER1]->GetAssemblyData() ); 
			switch( RetVal )
			{
				case -1:
					ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_ASSEMBLY_NOT_PLAUSIBLE );
					break;

				case -2:
					ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_EVACUATED_PRESSURE_NOT_PLAUSIBLE );
					break;

				case -3:
					ComPlc[CHAMBER1]->SetErrorCode( SET_PARAMETER, FLT_LASER_POWER_ADJUST_NOT_PLAUSIBLE );
					break;

				default:
						;
					break;
			}
			
			break;
		case 8:
			//make laser point
			;//no parameter copy
			break;

		case 9:
			//write text
			;//no parameter copy
			break;

		default:
			;
			break;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : CyclicalDataWriteReadChamber1                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the callback function for received cyclical data. This function is called every          *
 *                        cyclical data receive.                                                                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
void CyclicalDataWriteReadChamber1( void )
{
	SETrimmChamber[CHAMBER1]->ReadCyclic( ComPlc[CHAMBER1]->GetCyclicDataTrimmingAddress(), ComPlc[CHAMBER1]->GetChamberStateAddress(), ComPlc[CHAMBER1]->GetIpRefValuesAddress() );
	SETrimmChamber[CHAMBER1]->SetActualPressure( ComPlc[CHAMBER1]->GetActualPressure( ) );
	SETrimmChamber[CHAMBER1]->SetActualFlow( ComPlc[CHAMBER1]->GetActualFlow( ) );

	SETrimmChamber[CHAMBER1]->SetDigitalIn( ComPlc[CHAMBER1]->GetDigitalIn() );

	ComPlc[CHAMBER1]->SetDigitalOut( SETrimmChamber[CHAMBER1]->GetDigitalOut() );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultReadChamber1                                                                               *
 *                                                                                                                         *
 * input:               : unsigned int SubId : parameter sub identifier                                                    *  
 *																																																												 *
 * output:              : int                                                                                              *
 *                                                                                                                         *
 * description:         : This is the callback function for received parameter over taken. This function is called every   *
 *                        parameter receive.                                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int ResultReadChamber1( unsigned int SubId, unsigned int Tiepoint )
{
	int RetVal = 0;
	switch( SubId )
	{
		case 1:
			//trimming result
			RetVal = SETrimmChamber[CHAMBER1]->ReadResult( ComPlc[CHAMBER1]->GetResultDataTrimmingAddress(), Tiepoint );
			break;

		case 2:
			//position normal result
			RetVal = SETrimmChamber[CHAMBER1]->ReadResult( ComPlc[CHAMBER1]->GetResultDataPositionNormalAddress(), Tiepoint );
			break;

		case 3:
			//heater normal result
			RetVal = SETrimmChamber[CHAMBER1]->ReadResult( ComPlc[CHAMBER1]->GetResultDataHeaterNormalAddress(), Tiepoint );
			break;

		case 4:
			//universal normal result
			RetVal = SETrimmChamber[CHAMBER1]->ReadResult( ComPlc[CHAMBER1]->GetResultDataUniversalNormalAddress(), Tiepoint );
			break;

		case 5:
			//un normal result
			RetVal = SETrimmChamber[CHAMBER1]->ReadResult( ComPlc[CHAMBER1]->GetResultDataUnNormalAddress(), Tiepoint );
			break;

		case 6:
			//ip normal result
			RetVal = SETrimmChamber[CHAMBER1]->ReadResult( ComPlc[CHAMBER1]->GetResultDataIpNormalAddress(), Tiepoint );
			break;

		case 7:
			//ilm normal result
			RetVal = SETrimmChamber[CHAMBER1]->ReadResult( ComPlc[CHAMBER1]->GetResultDataIlmNormalAddress(), Tiepoint );
			break;
		case 99:
			 MCxCalib(LSTestHandle, (LONG)pow(2.0f, 1 - 1));
			 break;
		default:
			;
			break;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ActionFlagChangedChamber1                                                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the callback function for received action flag. This function is called every            *
 *                        action flag receive.                                                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
void ActionFlagChangedChamber1( void )
{
	bool ActionStarted = false;

	for( int i = 0; i < ACTION_COUNT; i++ )
	{
		//-- action1: master reset
		//-- action2: start trimming
		//-- action3: scanner move abs
		//-- action4: scanner trimming cut
		//-- action5: scanner laser point 
		//-- action6: scanner write text
		//-- action7: heating and measure
		//-- action8: measure normal
		//-- action9: check and update firmware
		//detect rising edge
		if( ComPlc[CHAMBER1]->GetRisingEdge( i + 1 ) == true )
		{
			for( int j = 0; j < ACTION_COUNT; j++ )
			{
				if(hThreadHandleAction[CHAMBER1][j] != NULL)
				{
					ActionStarted = true;
					break;
				}
			}
				
			if( ( hThreadHandleAction[CHAMBER1][i] == NULL) /*&& 
					( ActionStarted == false )*/ )	 //todo bz: check action started deactivated because of heating test
			{
				DWORD dwThreadId, dwThrdParam;
				//create thread
				//no security, default stack size, default creation
				hThreadHandleAction[CHAMBER1][i] = CreateThread( NULL,		
																								0,                       
																								ActionStartRoutine[CHAMBER1][i],             
																								&dwThrdParam,               
																								0,                         
																								&dwThreadId); 

				SetThreadPriority(hThreadHandleAction[CHAMBER1][i], THREAD_PRIORITY_ABOVE_NORMAL);
				printf( "Chamber[1]:ActionFlagChanged:Thread action %d created!\n", i + 1 );
			}	
			else
			{
				printf( "Chamber[1]:ActionFlagChanged:Action start not allowed!\n" );
				ComPlc[CHAMBER1]->SetErrorCode( SET_ACTION, FLT_ACTION_START_NOT_ALLOWED );
			}
		}
		//detect falling edge
		if( ComPlc[CHAMBER1]->GetFallingEdge( i + 1  ) == true )
		{
			DWORD ExitCode = 0;
			ComPlc[CHAMBER1]->WriteActionAbortFlag( i + 1, true );
			printf("Chamber[1]:ActionFlagChanged:Action abort requested!\n", i + 1);
			do
			{
				Sleep(10);
				GetExitCodeThread( hThreadHandleAction[CHAMBER1][i], &ExitCode);
			}while( ExitCode == STILL_ACTIVE );
			
			ComPlc[CHAMBER1]->WriteActionAbortFlag( i + 1, false );

			if( hThreadHandleAction[CHAMBER1][i] != NULL)
			{
				hThreadHandleAction[CHAMBER1][i] = NULL;
				printf("Chamber[1]:ActionFlagChanged:Thread action %d terminated!\n", i + 1);
			}
			ComPlc[CHAMBER1]->WriteActionStartedFlag( i + 1, false );
			ComPlc[CHAMBER1]->WriteActionFinishedFlag( i + 1, false );
		}
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ParameterOverTakenChamber2                                                                       *
 *                                                                                                                         *
 * input:               : unsigned int SubId : parameter sub identifier                                                    *  
 *																																																												 *
 * output:              : int                                                                                             *
 *                                                                                                                         *
 * description:         : This is the callback function for received parameter over taken. This function is called every   *
 *                        parameter receive.                                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int ParameterOverTakenChamber2( unsigned int SubId )
{
	int RetVal = 0;
	switch( SubId )
	{
		case 0:
			//trimming station parameter
			RetVal = SETrimmChamber[CHAMBER2]->SetParameter( ComPlc[CHAMBER2]->GetParamStationDataTrimming() );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_PARAMETER_DATA_TRIMMING_OUTSIDE_LIMIT );
			}
			RetVal = ComMicroLas[CHAMBER2]->SetParameter( ComPlc[CHAMBER2]->GetParamStationDataTrimming().ParamStationTrimmProcess.ParamStationLaser );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_MICROLAS_DATA_NOT_PLAUSIBLE );
			}
			break;

		case 1:
			//trimming parameter
			RetVal = SETrimmChamber[CHAMBER2]->SetParameter( ComPlc[CHAMBER2]->GetParamDataTrimming() );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_PARAMETER_DATA_TRIMMING_OUTSIDE_LIMIT );
			}
			RetVal = ComMicroLas[CHAMBER2]->SetParameter( ComPlc[CHAMBER2]->GetParamDataTrimming().ParamTrimmProcess.ParamLaser );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_MICROLAS_DATA_NOT_PLAUSIBLE );
			}
			break;
		case 2:
			//normal measure parameter
			RetVal = SETrimmChamber[CHAMBER2]->SetParameter( ComPlc[CHAMBER2]->GetParamDataNormalMeasure() );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_PARAMETER_DATA_NORMAL_OUTSIDE_LIMIT );
			}
			break;
		case 3:
			//move scanner abs
			;//no parameter copy
			break;
		case 4:
			//make trimming cut
			;//no parameter copy
			break;
		case 5:
			//parameter image processing
			RetVal = SETrimmChamber[CHAMBER2]->SetPositionImageProcessing( ComPlc[CHAMBER2]->GetParamDataImageProcessing().Tiepoint, 
																															ComPlc[CHAMBER2]->GetParamDataImageProcessing().XPos, 
																															ComPlc[CHAMBER2]->GetParamDataImageProcessing().YPos,
																															ComPlc[CHAMBER2]->GetParamDataImageProcessing().ZPos,
																															ComPlc[CHAMBER2]->GetParamDataImageProcessing().Diameter );
			if( RetVal != 0 )
			{
				ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_IMAGEPROCESSING_DATA_OUTSIDE_LIMIT );
			}
			break;
		case 6:
			break;
		case 7:
			RetVal = SETrimmChamber[CHAMBER2]->SetAssemblyData( ComPlc[CHAMBER2]->GetAssemblyData() ); 
			switch( RetVal )
			{
				case -1:
					ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_ASSEMBLY_NOT_PLAUSIBLE );
					break;

				case -2:
					ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_EVACUATED_PRESSURE_NOT_PLAUSIBLE );
					break;

				case -3:
					ComPlc[CHAMBER2]->SetErrorCode( SET_PARAMETER, FLT_LASER_POWER_ADJUST_NOT_PLAUSIBLE );
					break;

				default:
						;
					break;
			}
			
			break;
		case 8:
			//make laser point
			;//no parameter copy
			break;

		case 9:
			//write text
			;//no parameter copy
			break;

		default:
			;
			break;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : CyclicalDataWriteReadChamber2( void )                                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the callback function for received cyclical data. This function is called every          *
 *                        cyclical data receive.                                                                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
void CyclicalDataWriteReadChamber2( void )
{
	SETrimmChamber[CHAMBER2]->ReadCyclic( ComPlc[CHAMBER2]->GetCyclicDataTrimmingAddress(), ComPlc[CHAMBER2]->GetChamberStateAddress(), ComPlc[CHAMBER2]->GetIpRefValuesAddress() );
	SETrimmChamber[CHAMBER2]->SetActualPressure( ComPlc[CHAMBER2]->GetActualPressure( ) );
	SETrimmChamber[CHAMBER2]->SetActualFlow( ComPlc[CHAMBER2]->GetActualFlow( ) );

	SETrimmChamber[CHAMBER2]->SetDigitalIn( ComPlc[CHAMBER2]->GetDigitalIn() );

	ComPlc[CHAMBER2]->SetDigitalOut( SETrimmChamber[CHAMBER2]->GetDigitalOut() );
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ResultReadChamber1                                                                               *
 *                                                                                                                         *
 * input:               : unsigned int SubId : parameter sub identifier                                                    *  
 *                        insigned int Tiepoint : parameter tiepoint                                                       *
 *																																																												 *
 * output:              : int                                                                                              *
 *                                                                                                                         *
 * description:         : This is the callback function for received parameter over taken. This function is called every   *
 *                        parameter receive.                                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int ResultReadChamber2( unsigned int SubId, unsigned int Tiepoint )
{
	int RetVal = 0;
	switch( SubId )
	{
		case 1:
			//trimming result
			RetVal = SETrimmChamber[CHAMBER2]->ReadResult( ComPlc[CHAMBER2]->GetResultDataTrimmingAddress(), Tiepoint );
			break;

		case 2:
			//position normal result
			RetVal = SETrimmChamber[CHAMBER2]->ReadResult( ComPlc[CHAMBER2]->GetResultDataPositionNormalAddress(), Tiepoint );
			break;

		case 3:
			//heater normal result
			RetVal = SETrimmChamber[CHAMBER2]->ReadResult( ComPlc[CHAMBER2]->GetResultDataHeaterNormalAddress(), Tiepoint );
			break;

		case 4:
			//universal normal result
			RetVal = SETrimmChamber[CHAMBER2]->ReadResult( ComPlc[CHAMBER2]->GetResultDataUniversalNormalAddress(), Tiepoint );
			break;

		case 5:
			//un normal result
			RetVal = SETrimmChamber[CHAMBER2]->ReadResult( ComPlc[CHAMBER2]->GetResultDataUnNormalAddress(), Tiepoint );
			break;

		case 6:
			//ip normal result
			RetVal = SETrimmChamber[CHAMBER2]->ReadResult( ComPlc[CHAMBER2]->GetResultDataIpNormalAddress(), Tiepoint );
			break;

		case 7:
			//ilm normal result
			RetVal = SETrimmChamber[CHAMBER2]->ReadResult( ComPlc[CHAMBER2]->GetResultDataIlmNormalAddress(), Tiepoint );
			break;

		default:
			;
			break;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ActionFlagChangedChamber2( void )                                                                *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the callback function for received action flag. This function is called every            *
 *                        action flag receive.                                                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
void ActionFlagChangedChamber2( void )
{
	bool ActionStarted = false;

	for( int i = 0; i < ACTION_COUNT; i++ )
	{
		//-- action1: master reset
		//-- action2: start trimming
		//-- action3: scanner move abs
		//-- action4: scanner trimming cut
		//-- action5: scanner laser point 
		//-- action6: scanner write text
		//-- action7: heating and measure
		//-- action8: measure normal
		//detect rising edge
		if( ComPlc[CHAMBER2]->GetRisingEdge( i + 1 ) == true )
		{
			for( int j = 0; j < ACTION_COUNT; j++ )
			{
				if(hThreadHandleAction[CHAMBER2][j] != NULL)
				{
					ActionStarted = true;
					break;
				}
			}
				
			if( ( hThreadHandleAction[CHAMBER2][i] == NULL) /*&& 
					( ActionStarted == false )*/ )	 //todo bz: check action started deactivated because of heating test
			{
				DWORD dwThreadId, dwThrdParam;
				//create thread
				//no security, default stack size, default creation
				hThreadHandleAction[CHAMBER2][i] = CreateThread( NULL,		
																								0,                       
																								ActionStartRoutine[CHAMBER2][i],             
																								&dwThrdParam,               
																								0,                         
																								&dwThreadId); 

				SetThreadPriority(hThreadHandleAction[CHAMBER2][i], THREAD_PRIORITY_ABOVE_NORMAL);
				printf( "Chamber[2]:ActionFlagChanged:Thread action %d created!\n", i + 1 );
			}	
			else
			{
				printf( "Chamber[2]:ActionFlagChangedChamber2:Action start not allowed!\n" );
				ComPlc[CHAMBER2]->SetErrorCode( SET_ACTION, FLT_ACTION_START_NOT_ALLOWED );
			}
		}
		//detect falling edge
		if( ComPlc[CHAMBER2]->GetFallingEdge( i + 1  ) == true )
		{
			DWORD ExitCode = 0;
			ComPlc[CHAMBER2]->WriteActionAbortFlag( i + 1, true );
			printf("Chamber[2]:ActionFlagChanged:Action abort requested!\n", i + 1);
			do
			{
				Sleep(10);
				GetExitCodeThread( hThreadHandleAction[CHAMBER2][i], &ExitCode);
			}while( ExitCode == STILL_ACTIVE );
			
			ComPlc[CHAMBER2]->WriteActionAbortFlag( i + 1, false );

			if( hThreadHandleAction[CHAMBER2][i] != NULL)
			{
				hThreadHandleAction[CHAMBER2][i] = NULL;
				printf("Chamber[2]:ActionFlagChanged:Thread action %d terminated!\n", i + 1);
			}
			ComPlc[CHAMBER2]->WriteActionStartedFlag( i + 1, false );
			ComPlc[CHAMBER2]->WriteActionFinishedFlag( i + 1, false );
		}
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : CheckStatisticDelete( void )                                                                     *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to check statistic delete                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
void CheckStatisticDelete( void )
{
	HANDLE FileHandle;
	WIN32_FIND_DATA FindData;
	SYSTEMTIME SystemTime;
	FILETIME FileTime;								
	char FileName[255];
	int DaysDeleteBefore = 180;
	
	GetSystemTime( &SystemTime );
	SystemTimeToFileTime( &SystemTime, &FileTime );

	FileTime.dwHighDateTime -= (DWORD)( DaysDeleteBefore*864000000000 / UINT_MAX );
	FileTime.dwLowDateTime -= (DWORD)( DaysDeleteBefore*864000000000 % UINT_MAX );

	sprintf_s( FileName,"%s*",DATA_SAVE_LOCATION );
	FileHandle = FindFirstFile( FileName, &FindData );

	do
	{
		if ((FileHandle != INVALID_HANDLE_VALUE) && (!((FindData.cFileName[0]=='.') && ((FindData.cFileName[1]=='.' && FindData.cFileName[2]==0) || FindData.cFileName[1]==0))))
		{
			// Check if folder or file
			if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				;// Do nothing
			}
			else
			{
				if( CompareFileTime( &FindData.ftLastWriteTime, &FileTime ) == -1  )
				{
					sprintf_s( FileName,"%s%s",DATA_SAVE_LOCATION, FindData.cFileName );
					if( remove( FileName ) != 0 )
					{
						printf( "CheckStatisticDelete: File: %s sucessfully delete!\n", FindData.cFileName );
					}
					else
					{
						printf( "CheckStatisticDelete: File: %s delete failed !\n", FindData.cFileName );
					}
				}
			}
		}
	}
	while( FindNextFile( FileHandle, &FindData ) );
	FindClose( FileHandle );
}	

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : HandleCheckStatisticDelete( void )                                                               *
 *                                                                                                                         *
 * input:               : void                                                                                             *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the thread function for handle the check and delete statistic files after time.          *
 *-------------------------------------------------------------------------------------------------------------------------*/
void HandleCheckStatisticDelete( void )
{
	__int64 StartSystemTime;

	StartSystemTime = 0;
	do
	{
		//check all 24h
		if( ( GetActualSystemTimeMs() - StartSystemTime ) >= 86400000 )
		{
			CheckStatisticDelete();
			printf( "HandleCheckStatisticDelete: Running!\n" );
			StartSystemTime = GetActualSystemTimeMs();
		}
		Sleep( 5000 );
	}while( DeinstallThreadCheckStatisticDelete == false );	

	ExitThread(0);
}	

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : _tmain                                                                                           *
 *                                                                                                                         *
 * input:               : int argc                                                                                         *
 *                        _TCHAR* argv[]                                                                                   *  
 *																																																												 *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the main function (entry point).                                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
int _tmain(int argc, _TCHAR* argv[])
{
	int RetVal = 0;
	int RetryCount = 0;
	char ConsoleInput = 0;
	char TempString [255];
	DWORD ExitCode;

	#ifdef DEBUG_OUTPUT_FILE
		dprintfInitialize(TRUE, TRUE, "OpConServicesLogfile.txt", 10000000);	//init dprintf
	#endif

	setlocale( LC_ALL, "German" );	//set locale to germany

	FileVersionGet( TempString, sizeof( TempString ), "SELaserTrimming.exe");

	printf( "---------------------------------\n" );		//output version string
	printf( "SELasertrimming Version: V%s\n", TempString );	//output version string
	printf( "---------------------------------\n" );		//output version string

	if( hThreadDeleteStatistics == NULL) 
	{
			DWORD dwThreadId, dwThrdParam;
			//create thread
			//no security, default stack size, default creation
			hThreadDeleteStatistics = CreateThread( NULL, 0,                       
				                                 	(LPTHREAD_START_ROUTINE)HandleCheckStatisticDelete,             
													&dwThrdParam,               
													0,                         
													&dwThreadId );
	}	
	
	ComPlc[CHAMBER1] = new SEComPlc(1);																			//create plc communication for chamber 1
	ComMicroLas[CHAMBER1] = new SEComMicroLas(1);														//create microlas communication for chamber 1
	SETrimmChamber[CHAMBER1] = new SETrimmingChamber(1, ComMicroLas[CHAMBER1], ComPlc[CHAMBER1]);		//create chamber 1
	ComPlc[CHAMBER1]->InstallOverTakenCallback( &ParameterOverTakenChamber1 ); //install parameter overtaken function
	ComPlc[CHAMBER1]->InstallResultReadCallback( &ResultReadChamber1 );			//install result read callback function
	ComPlc[CHAMBER1]->InstallActionFlagChangedCallback( &ActionFlagChangedChamber1 ); //install action flag change callback function 
	ComPlc[CHAMBER1]->InstallCyclicalReceivedCallback( &CyclicalDataWriteReadChamber1 ); //install cyclical callback function

	ComPlc[CHAMBER2] = new SEComPlc(2);																			//create plc communication for chamber 2
	ComMicroLas[CHAMBER2] = new SEComMicroLas(2);														//create microlas communication for chamber 2
	SETrimmChamber[CHAMBER2] = new SETrimmingChamber(2, ComMicroLas[CHAMBER2], ComPlc[CHAMBER2]);		//create chamber 2
	ComPlc[CHAMBER2]->InstallOverTakenCallback( &ParameterOverTakenChamber2 ); //install parameter overtaken function
	ComPlc[CHAMBER2]->InstallResultReadCallback( &ResultReadChamber2 );				//install result read callback function
	ComPlc[CHAMBER2]->InstallActionFlagChangedCallback( &ActionFlagChangedChamber2 ); //install action flag change callback function 
	ComPlc[CHAMBER2]->InstallCyclicalReceivedCallback( &CyclicalDataWriteReadChamber2 ); //install cyclical callback function																																
	
	//#ifndef CHAMBER1_DISABLE
	printf( "Chamber[1]:Main:Waiting for plc connection...\n" );
	ComPlc[CHAMBER1]->Initializing( );																			//plc communication init
	#ifndef PLC_SIMULATION
	while( ComPlc[CHAMBER1]->Connect() != 0 )																//plc communication connect
	{
		Sleep( 1000 );																												//wait 1s
	}	
	ComPlc[CHAMBER1]->InstallThread( ( LPTHREAD_START_ROUTINE ) HandleSocketServerPlcChamber1 );	//install thread to handle plc communication
	#endif
	ComPlc[CHAMBER1]->SetReady();																			      //set ready
	printf( "Chamber[1]:Main:Plc connected!\n" );
	//#endif

	#ifndef CHAMBER2_DISABLE
	printf( "Chamber[2]:Main:Waiting for plc connection...\n" );
	ComPlc[CHAMBER2]->Initializing( );																			//plc communication init
	#ifndef PLC_

	while( ComPlc[CHAMBER2]->Connect() != 0 )																//plc communication connect
	{
		Sleep( 1000 );																												//wait 1s
	}	 
	ComPlc[CHAMBER2]->InstallThread( ( LPTHREAD_START_ROUTINE ) HandleSocketServerPlcChamber2 );	//install thread to handle plc communication
	#endif
	ComPlc[CHAMBER2]->SetReady();																			      //set ready
	printf( "Chamber[2]:Main:Plc connected!\n" );
	#endif

	#ifndef CHAMBER1_DISABLE
	printf( "Chamber[1]:Main:Waiting for plc parameter data...\n" );
	RetryCount = 0;
	do
	{
		if( SETrimmChamber[CHAMBER1]->ParameterReadyToStart() == false )			  //check all cells ready
			{
				if( RetryCount == 60 )	//check timeout 60s reached
				{
					ComPlc[CHAMBER1]->SetErrorCode( GLOBAL_ERROR, FLT_NO_PARAMETER_DATA ); //set error code
					RetryCount = 0;	 //init retry count
					printf( "Chamber[1]:Main:Error no parameter data after 60s!\n" );
				}
				RetVal = -1;	//set retval to error
				RetryCount++;	//increment retry count
				Sleep( 1000 );	//wait 1s
			}										
			else
			{
				RetVal = 0;	//set retval to ok
			}		//wait 1s
	}
	while( RetVal != 0 );
	printf("Chamber[1]:Main:Plc data present!\n");
	#endif

	#ifndef CHAMBER2_DISABLE
	printf( "Chamber[2]:Main:Waiting for plc parameter data...\n" );
	RetryCount = 0;
	do
	{
		if( SETrimmChamber[CHAMBER2]->ParameterReadyToStart() == false )			  //check all cells ready
			{
				if( RetryCount == 60 )																							//check timeout 60s reached
				{
					ComPlc[CHAMBER2]->SetErrorCode( GLOBAL_ERROR, FLT_NO_PARAMETER_DATA ); //set error code
					RetryCount = 0;																										//init retry count
					printf( "Chamber[2]:Main:Error no parameter data after 60s!\n" );
				}
				RetVal = -1;																												//set retval to error
				RetryCount++;																												//increment retry count
				Sleep( 1000 );																											//wait 1s
			}										
			else
			{
				RetVal = 0;																													//set retval to ok
			}																										//wait 1s
	}
	while( RetVal != 0 );
	printf("Chamber[2]:Main:Plc data present!\n");
	#endif

	#ifndef CHAMBER1_DISABLE
	printf( "Chamber[1]:Main:Waiting for initializing...\n" );	
	RetVal = SETrimmChamber[CHAMBER1]->Initializing();	//init chamber 1
	printf( "Chamber[1]:Main:Initialization ready!\n" );	
	#endif

	#ifndef CHAMBER2_DISABLE
	printf( "Chamber[2]:Main:Waiting for initializing...\n" );
	RetVal = SETrimmChamber[CHAMBER2]->Initializing();										 //init chamber 1	
	printf( "Chamber[2]:Main:Initialization ready!\n" );
	#endif
	
	#ifndef CHAMBER1_DISABLE
	printf( "Chamber[1]:Main:Waiting for advov cards ready ...\n" );
	RetryCount = 0;																													//init retry count
	do
	{
		if( SETrimmChamber[CHAMBER1]->ChamberAllCellsReady() == false )			  //check all cells ready
		{
			if( RetryCount == 60 )																							//check timeout 60s reached
			{
				ComPlc[CHAMBER1]->SetErrorCode( GLOBAL_ERROR, FLT_ADVOV_NOT_READY ); //set error code
				RetryCount = 0;																										//init retry count
				printf( "Chamber[1]:Main:Error advov not ready after 60s!\n" );
			}
			RetVal = -1;																												//set retval to error
			RetryCount++;																												//increment retry count
			Sleep( 1000 );																											//wait 1s
		}										
		else
		{
			RetVal = 0;																													//set retval to ok
		}
	}
	while( RetVal != 0 );																										//repeat until error state
	printf( "Chamber[1]:Main:Advov cards ready!\n" );
	
	ComPlc[CHAMBER1]->SetBootReady();																			  //set boot process ready
	#endif
	
	#ifndef CHAMBER2_DISABLE
	printf( "Chamber[2]:Main:Waiting for advov cards ready...\n" );
	RetryCount = 0;																													//init retry count
	do
	{
		if( SETrimmChamber[CHAMBER2]->ChamberAllCellsReady() == false )			  //check all cells ready
		{

			if( RetryCount == 60 )																							//check timeout 60s reached
			{
				ComPlc[CHAMBER2]->SetErrorCode( GLOBAL_ERROR, FLT_ADVOV_NOT_READY ); //set error code
				RetryCount = 0;																										//init retry count
				printf( "Chamber[2]:Main:Error advov not ready after 60s!\n" );
			}
			RetVal = -1;																												//set retval to error
			RetryCount++;																												//increment retry count
			Sleep( 1000 );																											//wait 1s
		}										
		else
		{
			RetVal = 0;																													//set retval to ok
		}
	}
	while( RetVal != 0 );																										//repeat until error state
	printf( "Chamber[2]:Main:Advov cards ready!\n" );	
	
	ComPlc[CHAMBER2]->SetBootReady();																			  //set boot process ready
	#endif

	#ifndef CHAMBER1_DISABLE
	#ifndef MICRO_LAS_SIMULATION
	printf( "Chamber[1]:Main:Waiting for microlas connection...\n" );
	RetryCount = 0;																													//init retry count
	ComMicroLas[CHAMBER1]->Initializing();															    //microlas communication init
	do
	{
		RetVal = ComMicroLas[CHAMBER1]->Connect();												    //microlas communication connect
		if( RetVal != 0 )																											//check retval
		{
			if( RetryCount == 60 )																							//check timeout 60s reached
			{
				ComPlc[CHAMBER1]->SetErrorCode( GLOBAL_ERROR, FLT_MICROLAS_CONNECT ); //set error code
				RetryCount = 0;
				printf( "Chamber[1]:Main:Error microlas not connected after 60s!\n" );
			}	
			RetryCount++;																												//increment retry count
			Sleep( 1000 );																											//sleep 1s
		}
	}
	while( RetVal != 0 );																										//repeat until error state

	ComMicroLas[CHAMBER1]->InstallThread( ( LPTHREAD_START_ROUTINE ) HandleSocketClientMicroLasChamber1 );	//install thread to handle microlas communication
	printf( "Chamber[1]:Main:Microlas connected!\n" );
	#endif
	#endif

	#ifndef CHAMBER2_DISABLE
	#ifndef MICRO_LAS_SIMULATION
	printf( "Chamber[2]:Main:Waiting for microlas connection...\n" );
	RetryCount = 0;																													//init retry count
	ComMicroLas[CHAMBER2]->Initializing();                                  //microlas communication init
	do
	{
		RetVal = ComMicroLas[CHAMBER2]->Connect();												    //microlas communication connect
		if( RetVal != 0 )																											//check retval
		{
			if( RetryCount == 60 )																							//check timeout 60s reached
			{
				ComPlc[CHAMBER2]->SetErrorCode( GLOBAL_ERROR, FLT_MICROLAS_CONNECT ); //set error code
				RetryCount = 0;
				printf( "Chamber[2]:Main:Error microlas not connected after 60s!\n" );
			}	
			RetryCount++;																												//increment retry count
			Sleep( 1000 );																											//sleep 1s
		}
	}
	while( RetVal != 0 );																										//repeat until error state

	ComMicroLas[CHAMBER2]->InstallThread( ( LPTHREAD_START_ROUTINE ) HandleSocketClientMicroLasChamber2 );	//install thread to handle microlas communication
	
	printf( "Chamber[2]:Main:Microlas connected!\n" );	
	#endif
	#endif

	//loop for action handling
	do
	{
		Sleep( 1000 );																												//wait 1s
		if( _kbhit() != 0 )
		{
			ConsoleInput = _getch();
		}
	} 
	while( ConsoleInput != 0x1B );		

	DeinstallThreadCheckStatisticDelete = true;
	do
	{
		Sleep(100);
		GetExitCodeThread( hThreadDeleteStatistics, &ExitCode);
	}while( ExitCode == STILL_ACTIVE );

	delete SETrimmChamber[CHAMBER1];																				//destroy chamber 1

	delete SETrimmChamber[CHAMBER2];																				//destroy chamber 2

	delete ComPlc[CHAMBER1];																								//destroy plc communication

	delete ComPlc[CHAMBER2];																								//destroy plc communication

	delete ComMicroLas[CHAMBER1];																					  //destroy microlas communication
																																				 
	delete ComMicroLas[CHAMBER2];																					  //destroy microlas communication

	return 0;
}

