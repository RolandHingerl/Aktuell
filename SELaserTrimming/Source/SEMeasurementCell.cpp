/*-------------------------------------------------------------------------------------------------------------------------*
 *  modulname       : SEMeasurementCell.cpp                                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                                     *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                                            *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  description:                                                                                                           *
 *  ------------                                                                                                           *
 *  This module contains control and evaluation routines for SE measurement cell.                                          *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                                         *
 * ---------|------------|----------------|------------------------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                                              *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/

#include <windows.h>
#include <vector>
#include <list>

#include "..\Library\OpConServicesCommon.h"
#define _SE_MEASUREMENT_CELL_INTERNAL
#include "SEMeasurementCell.h"
#undef _SE_MEASUREMENT_CELL_INTERNAL
#include "..\Library\SETestDLL.h"

//thread handle of each measurement cell
HANDLE SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CELL_COUNT] = { NULL, NULL, NULL, NULL };
bool SEMeasurementCell::Dllopened = false;																//init flag dll opened 



//private memberfunctions at SEMeasurementCell-------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SEMeasurementCell::SEMeasurementCell(int CellId, int Type)                                       *
 *                                                                                                                         *
 * input:               : int CellId : cell id (trimming cell chamber 1 = 1, reference cell chamber 1 = 2,                 *
 *                                              trimming cell chamber 2 = 3, reference cell chamber 2 = 4)                 *
 *                        int Type : cell type (trimming cell = 0x0000000F, reference cell = 0x00000001)                   *                                                                                            
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor (overload) function.                                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEMeasurementCell::SEMeasurementCell( int CellId )
{
	this->CellId = CellId;																									//copy cell id
	DeinstallThreadRequest = false;	 
	SequenceStarted = false;
	ReadDataAdvovActive = false;
	ProcessTypeLocal = MeasureTrimmingSelection;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SEMeasurementCell::ExecHeating( int HeatPhase, float StartTime, float EndTime,               *
 *                            float CooldownTime, float HeatVolt, float RampTime, float HeatPow, float MaxHeatVolt,        *
 *                            float * ExecTime )                                                                           *
 *                                                                                                                         *
 * input:               : int HeatPhase :                                                                                  *
 *                        float StartTime :                                                                                * 
 *                        float EndTime :                                                                                  *
 *                        float CooldownTime :                                                                             * 
 *                        float HeatVolt :                                                                                 *
 *                        float RampTime :                                                                                 *
 *                        float HeatPow :                                                                                  * 
 *                        float MaxHeatVolt :                                                                              *
 *                        float * ExecTime :                                                                               *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to execute heating of SE.                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::ExecHeating( int HeatPhase, float StartTime, float EndTime, float CooldownTime, float HeatVolt, 
																		float RampTime, float HeatPow, float MaxHeatVolt, float * ExecTime )
{
	int RetVal = 0;
	float HeaterOnTime = 0.0;
	
	*ExecTime = 0;
	
	*ExecTime = StartTime + CooldownTime;
	if( ( EndTime - StartTime ) >= 1.0 ) 
	{
		if( HeatPow > 0.1 ) 
		{ 
			RetVal = TFlwHeaterPowerSetExt( LSTestHandle, *ExecTime, HeatPow, HeatVolt, MaxHeatVolt );
		} 
		else 
		{
			RetVal = TFlwHeaterVoltSet( LSTestHandle, *ExecTime, HeatVolt, RampTime );
		}
		
		HeaterOnTime = *ExecTime;
		
		RetVal = TFlwMinHeaterVoltIn( LSTestHandle, *ExecTime, 0 );
		RetVal = TFlwMaxHeaterVoltIn( LSTestHandle, *ExecTime, 0 );
		
		*ExecTime = max( *ExecTime, EndTime );
		if( ( *ExecTime - HeaterOnTime ) >= 6.0 )
		{
			if( HeatPow > 0.1 ) 
			{
				RetVal = TFlwHeaterPowIn( LSTestHandle, ( *ExecTime - 0.2f ), HeatPhase); 
			} 
			else 
			{ 
				RetVal = TFlwHeaterVoltIn( LSTestHandle, ( *ExecTime - 0.5f ), HeatPhase );
				RetVal = TFlwMaxHeaterVoltIn( LSTestHandle, ( *ExecTime - 0.5f ), HeatPhase );
				RetVal = TFlwMinHeaterVoltIn( LSTestHandle, ( *ExecTime - 0.5f ), HeatPhase );
			}
		}
		RetVal = TFlwHeaterVoltSet( LSTestHandle, *ExecTime, 0.0, 0.0 );
	}
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SEMeasurementCell::ClearAllValues(void)                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to clear all measured and stored values.                                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::ClearAllValues( void )
{
	int RetVal = 0;

	SequenceStartTime	= 0;

	printf("ClearAllValues Cell Start\n");

	try
	{
		for(int i = 0; i < CELL_TIEPOINT_COUNT; i++)
		{
			RiValues[i].clear();
			IpValues[i].clear();
			RhhValues[i].clear();
			PhValues[i].clear();
		}

		memset(&TemperatureValues, 0, sizeof(TemperatureValues));

		memset(&RiValues1, 0, sizeof(RiValues1));
		memset(&RiValues2, 0, sizeof(RiValues2));

		memset(&IpValues1, 0, sizeof(IpValues1));
		memset(&IpValues2, 0, sizeof(IpValues2));
		memset(&IpValues3, 0, sizeof(IpValues3));

		memset(&UhValues, 0, sizeof(UhValues));
		memset(&UhMinValues, 0, sizeof(UhMinValues));
		memset(&UhMaxValues, 0, sizeof(UhMaxValues));
		memset(&TemperatureValues, 0, sizeof(TemperatureValues));
		memset(&RhkValues, 0, sizeof(RhkValues));
		memset(&EhValues, 0, sizeof(EhValues));
		memset(&RiRegValues, 0, sizeof(RiRegValues));
		memset(&RhDtValues, 0, sizeof(RhDtValues));
		memset(&IhDtValues, 0, sizeof(IhDtValues));
		memset(&HSContValues, 0, sizeof(HSContValues));
		memset(&SSContValues, 0, sizeof(SSContValues));
		memset(&IhValues, 0, sizeof(IhValues));

		memset(&UapeValues, 0, sizeof(UapeValues));
		memset(&UapeValues2, 0, sizeof(UapeValues2));
		memset(&UnValues, 0, sizeof(UnValues));
		memset(&UnValues2, 0, sizeof(UnValues2));
		memset(&UnValues3, 0, sizeof(UnValues3));
		memset(&IpReValues, 0, sizeof(IpReValues));
		memset(&UReValues, 0, sizeof(UReValues));
		memset(&IgRkValues, 0, sizeof(IgRkValues));
		memset(&IlValues, 0, sizeof(IlValues));
		memset(&RiDCnValues, 0, sizeof(RiDCnValues));
		memset(&IprValues, 0, sizeof(IprValues));
	}
	catch(...)
	{
		printf("Exception!\n");	
	}

	printf("ClearAllValues Cell End\n");
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SEMeasurementCell::ClearValues(int Tiepoint, ValueType Type)                                 *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                        ValueType Type : type of value                                                                   *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to clear all measured and stored values at one tiepoint and type.           *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::ClearValues(int Tiepoint, AdvBaseTypes Type)
{
	int RetVal = 0;
	
	switch(Type)
	{
		case AdvRiAC:
			RiValues[Tiepoint - 1].clear();
			break;
			
		case AdvIPu:
			IpValues[Tiepoint - 1].clear();
			break;
		
		case AdvRHh:
			RhhValues[Tiepoint - 1].clear();
			break;
		
		case AdvPH:
			PhValues[Tiepoint - 1].clear();
	break;
	
	default:
		printf( "Cell[%d]:SEMeasurementCell::ClearValues - requested value not present ( type = %d )!\n", CellId, Type );
		RetVal = -1;
		break;
	} 
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SEMeasurementCell::AddValue(int Tiepoint, ValueType Type, SEValueInfo Value)                 *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                        ValueType Type : type of value                                                                   *
 *                        SEValueInfo Value : measured value                                                               *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to store one measured value.                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::AddValue(int Tiepoint, AdvBaseTypes Type, int Index, SEValueInfo Value)
{
	int RetVal = 0;
	std::vector <SEValueInfo>::iterator Iter;
	 
	switch(Type)
	{
		case AdvTemp:
			TemperatureValues[Tiepoint -1] = Value;
			break;
		case AdvRiAC:
			switch( Index )
			{
				case 0:
					//Iter = RiValues[Tiepoint - 1].end(); 
					//RiValues[Tiepoint - 1].insert( Iter, Value );
					RiValues[Tiepoint - 1].push_back( Value ); 
					break;
				case 1:
					RiValues1[Tiepoint - 1] = Value;
					break;
			
				case 2:
					RiValues2[Tiepoint - 1] = Value;
					break;

				default:
					;
					break;
			}
			break;
			
		case AdvIPu:
			switch( Index )
			{
				case 0:
					//Iter = IpValues[Tiepoint - 1].end();
					//IpValues[Tiepoint - 1].insert( Iter, Value );
					IpValues[Tiepoint - 1].push_back( Value );
					#ifdef _DEBUG
					if( ( Value.Value * 1.0e6f ) < 50.0 && abs( Value.Value * 1.0e6f ) > 1.0f )  
					{
						printf( "Cell[%d]:SEMeasurementCell::AddValue:Error: Ip<50: Value=%f\n", CellId, Value.Value * 1.0e6f );
					}
					#endif
					break;
				case 1:
					IpValues1[Tiepoint - 1] = Value;
					break;
			
				case 2:
					IpValues2[Tiepoint - 1] = Value;
					break;

				case 3:
					IpValues3[Tiepoint - 1] = Value;
					break;

				default:
					;
					break;
			}

			break;
		
		case AdvRHh:
			//Iter = RhhValues[Tiepoint - 1].end();
			//RhhValues[Tiepoint - 1].insert( Iter, Value );
			RhhValues[Tiepoint - 1].push_back( Value );
			break;
		
		case AdvPH:
			//Iter = PhValues[Tiepoint - 1].end(); 
			//PhValues[Tiepoint - 1].insert( Iter, Value );
			PhValues[Tiepoint - 1].push_back( Value );
			break;

		case AdvUHS:
			UhValues[Tiepoint - 1] = Value;
			break;

		case AdvUHmin:
			UhMinValues[Tiepoint - 1] = Value;
			break;

		case AdvUHmax:
			UhMaxValues[Tiepoint - 1] = Value;
			break;

		case AdvRHk:
			RhkValues[Tiepoint - 1] = Value;
			break;

		case AdvEH:
			EhValues[Tiepoint - 1] = Value;
			break;

		case AdvdtRiReg:
			RiRegValues[Tiepoint - 1] = Value;
			break;

		case AdvdRHdt:
			RhDtValues[Tiepoint - 1] = Value;
			break;

		case AdvdIHdt:
			IhDtValues[Tiepoint - 1] = Value;
			break;

		case AdvHSCont:
			HSContValues[Tiepoint - 1] = Value;
			break;

		case AdvSSCont:
			SSContValues[Tiepoint - 1] = Value;
			break;

		case AdvIH:
			IhValues[Tiepoint - 1] = Value;
			break;

		case AdvUAPE:
			switch( Index )
			{
				case 0:
				case 1:
					UapeValues[Tiepoint - 1] = Value;
					break;

				case 2:
					UapeValues2[Tiepoint - 1] = Value;
					break;

				default:
					;
					break;
			}

			break;

		case AdvUN:
			switch( Index )
			{
				case 0:
				case 1:
					UnValues[Tiepoint - 1] = Value;
					break;

				case 2:
					UnValues2[Tiepoint - 1] = Value;
					break;

				case 3:
					UnValues3[Tiepoint - 1] = Value;
					break;

				default:
					;
					break;
			}

			break;

		case AdvIpRE:
			IpReValues[Tiepoint - 1] = Value;
			break;

		case AdvURE:
			UReValues[Tiepoint - 1] = Value;
			break;

		case AdvIgRK:
			IgRkValues[Tiepoint - 1] = Value;
			break;

		case AdvIL:
			IlValues[Tiepoint - 1] = Value;
			break;

		case AdvRiDCn:
			RiDCnValues[Tiepoint - 1] = Value;
			break;

		case AdvIPr:
			IprValues[Tiepoint - 1] = Value;
			break;

		default:
			printf( "Cell[%d]:SEMeasurementCell::AddValue - measured value not stored\r ( type = %d, value = %f)!\n", CellId, Type, Value.Value );
			RetVal = -1;
			break;
	} 

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SEMeasurementCell::GetLastValue( int Tiepoint, ValueType Type, int Index,                    *
 *                          SEValueInfo * Value )                                                                          *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                        ValueType Type : type of value                                                                   *
 *                        SEValueInfo * Value : stored value                                                               *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to get the last measured value.                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::GetLastValue( int Tiepoint, AdvBaseTypes Type, int Index, SEValueInfo * Value )
{
	int RetVal = 0;
	
	memset( Value, 0, sizeof( SEValueInfo ) );
	//try
	//{
	switch(Type)
	{
		case AdvTemp:
			*Value = TemperatureValues[Tiepoint - 1];
			break;
		case AdvRiAC:
			switch( Index )
			{
				case 0:
					if( RiValues[Tiepoint - 1].empty() == false )
					{
						*Value = RiValues[Tiepoint - 1].at (RiValues[Tiepoint - 1].size() - 1 );
					}
					break;

					case 1:
						*Value = RiValues1[Tiepoint - 1];
						break;

					case 2:
						*Value = RiValues2[Tiepoint - 1];
						break;
			}
			break;
			
		case AdvIPu:
			switch( Index )
			{
				case 0:
					if( IpValues[Tiepoint - 1].empty() == false )
					{
						*Value = IpValues[Tiepoint - 1].at( IpValues[Tiepoint - 1].size() - 1 );
					}
					#ifdef _DEBUG
					if( ( Value->Value * 1.0e6f ) < 50.0 && abs( Value->Value * 1.0e6f ) > 1.0f )  
					{
						printf( "Cell[%d]:Error: Ip<50: Value=%f\n", CellId, Value->Value * 1.0e6f );
					}
					#endif
					break;

					case 1:
						*Value = IpValues1[Tiepoint - 1];
						break;

					case 2:
						*Value = IpValues2[Tiepoint - 1];
						break;

					case 3:
						*Value = IpValues3[Tiepoint - 1];
						break;
			}
			break;
		
		case AdvRHh:
			if( RhhValues[Tiepoint - 1].empty() == false )
			{
				*Value = RhhValues[Tiepoint - 1].at( RhhValues[Tiepoint - 1].size() - 1 );
			}
			break;
		
		case AdvPH:
			if( PhValues[Tiepoint - 1].empty() == false )
			{
				*Value = PhValues[Tiepoint - 1].at( PhValues[Tiepoint - 1].size() - 1 );
			}
		break;

		case AdvUHS:
			*Value = UhValues[Tiepoint - 1];
			break;

		case AdvUHmin:
			*Value = UhMinValues[Tiepoint - 1];
			break;

		case AdvUHmax:
			*Value = UhMaxValues[Tiepoint - 1];
			break;

		case AdvRHk:
			*Value = RhkValues[Tiepoint - 1];
			break;

		case AdvEH:
			*Value = EhValues[Tiepoint - 1];
			break;

		case AdvdtRiReg:
			*Value = RiRegValues[Tiepoint - 1];
			break;

		case AdvdRHdt:
			*Value = RhDtValues[Tiepoint - 1];
			break;

		case AdvdIHdt:
			*Value = IhDtValues[Tiepoint - 1];
			break;

		case AdvHSCont:
			*Value = HSContValues[Tiepoint - 1];
			break;

		case AdvSSCont:
			*Value = SSContValues[Tiepoint - 1];
			break;

		case AdvIH:
			*Value = IhValues[Tiepoint - 1];
			break;

		case AdvUAPE:
			switch( Index )
			{
				case 0:
				case 1:
					*Value = UapeValues[Tiepoint - 1];
					break;

				case 2:
					*Value = UapeValues2[Tiepoint - 1];
					break;

				default:
					;
					break;
			}

			break;

		case AdvUN:
			switch( Index )
			{
				case 0:
				case 1:
					*Value = UnValues[Tiepoint - 1];
					break;

				case 2:
					*Value = UnValues2[Tiepoint - 1];
					break;

				case 3:
					*Value = UnValues3[Tiepoint - 1];
					break;

				default:
					;
					break;
			}
			break;

		case AdvIpRE:
			*Value = IpReValues[Tiepoint - 1];
			break;

		case AdvURE:
			*Value = UReValues[Tiepoint - 1];
			break;

		case AdvIgRK:
			*Value = IgRkValues[Tiepoint - 1];
			break;

		case AdvIL:
			*Value = IlValues[Tiepoint - 1];
			break;

		case AdvRiDCn:
			*Value = RiDCnValues[Tiepoint - 1];
			break;

		case AdvIPr:
			*Value = IprValues[Tiepoint - 1];
			break;

		default:
			printf( "Cell[%d]:SEMeasurementCell::GetLastValue - requested value not present ( type = %d )!\n", CellId, Type );
			RetVal = -1;
			break;
	} 
	//}
	//catch (...)
	//{
	//	return -1;
	//}
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : __int64 SEMeasurementCell::GetSequenceStartTime( void )                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : __int64 : sequence start time                                                                    *                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to get the sequence start time.                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
__int64 SEMeasurementCell::GetSequenceStartTime( void )
{
	return SequenceStartTime;
}


//public memberfunctions at SEMeasurementCell-------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int StartSequence( void )                                                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to start the sequence at advov card.                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::StartSequence( void )
{
	int RetVal = 0;

	SequenceFinished = false;

	ClearAllValues();

	RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) )	;
	printf( "Cell[%d]:StartSequence:MCxLastError=%d\n", CellId, RetVal );

	do
	{
		RetVal = MCxStartSEQ( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );																//start sequence
		printf( "Cell[%d]:MCxStartSEQ=%d\n", CellId, RetVal );
		Sleep( 100 );
	}while( RetVal != 0 );

	do
	{
		RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) )	;
		printf( "Cell[%d]:MCxLastError=%d\n", CellId, RetVal );
		Sleep( 100 );
	}while( RetVal > 0 );

	SequenceStartTime = GetActualSystemTimeMs();		//store receive time [ms]		

	if( SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] != NULL )
	{
		RetVal = ResumeThread( SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] );
		if( RetVal >= 0 )
		{
			printf( "Cell[%d]:Thread resume=%d\n", CellId, RetVal );
		}
		else
		{
			printf( "Cell[%d]:Thread resume=%d;%d\n", CellId, RetVal, GetLastError() );
		}
	}
	else
	{
		printf( "Cell[%d]:Thread not resumed!\n", CellId );
	}
		
	SequenceStarted = true;

	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int StopSequence( void )                                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to stop the sequence at advov card.                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::StopSequence(void)
{
	int RetVal = 0;
	SEValueInfo Value;
	bool First = false;
	float TemperatureSum = 0.0f;
	int TemperatureCount = 0;

	do
	{
		if( First == false )
		{
			printf("Cell[%d]:Wait for ReadResult finished!\n", CellId);
			First = true;
		}
	}while( IsReadDataAdvovActive() == true );

	SequenceStarted = false;

	AvgCardTemperature = 0.0f;
	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		GetLastValue( i + 1, AdvTemp, 0, &Value );
		if( _isnan( Value.Value )	== 0 )
		{
			if( Value.Value > 1.0f )
			{
				TemperatureSum +=	Value.Value;
				TemperatureCount++;
			}
		}
	}			
	if( TemperatureCount != 0 )
	{
		AvgCardTemperature = TemperatureSum / TemperatureCount; 
		printf( "Cell[%d]:Average Card Temperature=%f\n", CellId, AvgCardTemperature );
	}

	do
	{
		RetVal = MCxStop( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );																//stop sequence
		printf( "Cell[%d]:MCxStop=%d\n", CellId, RetVal );
		Sleep( 100 );
	}while( RetVal != 0 );

	do
	{
		RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) )	;
		printf( "Cell[%d]:MCxLastError=%d\n", CellId , RetVal );
		Sleep( 100 );
	}while( RetVal > 0 );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IsRiStable( int Tiepoint )                                                                  *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : bool : stability                                                                                 *
 *                                false : not stable                                                                       *
 *                                true : stable                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to check ri stability.                                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::IsRiStable( int Tiepoint )
{
	bool RetVal = false;
	int FuncRetVal = 0;
	SEValueInfo Value;

	FuncRetVal = GetLastValue( Tiepoint, AdvRiAC, 0, &Value );

	if( ProcessTypeLocal == MeasureSelection )
	{
		if( ( FuncRetVal == 0 ) &&
				( Value.Value >= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityLowerLimit ) ) &&
				( Value.Value <= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityUpperLimit ) ) )
		{
			RetVal = true;
		}
	}
	else
	{
		if( ( FuncRetVal == 0 ) &&
				( Value.Value >= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiTrimming + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityLowerLimit ) ) &&
				( Value.Value <= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiTrimming + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityUpperLimit ) ) )
		{
			RetVal = true;
		}
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IsContactingOk( int Tiepoint )                                                              *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : bool : contacting                                                                                *
 *                                false : no contact                                                                       *
 *                                true : contact                                                                           *
 *                                                                                                                         *
 * description:         : This is the function to check contacting.                                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::IsContactingOk( int Tiepoint )
{
	bool RetVal = false;
	int FuncRetVal = 0;
	SEValueInfo Value;

	FuncRetVal = GetLastValue(Tiepoint, AdvHSCont, 0, &Value );

	if( ( FuncRetVal == 0 ) &&
			( Value.ReceiveTimePC != 0 ) &&
			( Value.Value == 0.0 ) )
	{
		FuncRetVal = GetLastValue(Tiepoint, AdvSSCont, 0, &Value );

		if( ( FuncRetVal == 0 ) &&
				( Value.ReceiveTimePC != 0 ) &&
				( Value.Value == 0.0 ) )
		{
			RetVal = true;
		}
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool AllCardsReady( void )                                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : all cards status                                                                          *
 *                                false : not all cards ready                                                              *
 *                                true : all cards ready                                                                   *
 *                                                                                                                         *
 * description:         : This is the function to check advov cards ready.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::AllCardsReady( void )
{
	bool RetVal = true;
	int FuncRetVal = 0;
	MCINFO Info;

	for( int i = 0; i < 4; i++)
	{
		FuncRetVal = MCxInfo(LSTestHandle, CellId, i + 1, &Info);
		if( FuncRetVal != 0 )
		{
			RetVal = false;
			break;
		}
		else
		{
			printf( "Cell[%d]:AllCardsReady:Card%d:Type=%s;Serial=%d;MainVer=%.3f;ModVer=%.3f\n", CellId, i + 1, Info.sType, Info.wSerialNo, Info.rMainSoftVer, Info.rModSoftVer );
		}
		
	}
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool DeinstallThreadRequested( void )                                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : deinstall thread requested                                                                *
 *                                  false = deinstall not requested                                                        *
 *                                  true = deinstall requested                                                             *
 *                                                                                                                         *
 * description:         : This is the function to exit chamber handle thread.                                              *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::DeinstallThreadRequested( void )
{
	return DeinstallThreadRequest;
}



/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SEMeasurementCell::HandleTestSequenceValues(WORD wFUIdx,WORD wPartIdx,int eMainType,         *
 *                            int SubIndex, float rMVal,DWORD tsMTime,LONG lResIdx)                                                      *
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
 * description:         : This is the function to handle measured result values inside test sequence (called from ADVOV    *
 *                        callback function).                                                                              *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::HandleTestSequenceValues(WORD wFUIdx,WORD wPartIdx,/*eIMTBasTypes*/int eMainType, int SubIndex,
																								float rMVal,float tsMTime,LONG lResIdx)
{	
	int RetVal = 0;
	SEValueInfo Value;																											//measured values
	
	Value.ReceiveTime	= (__int64)tsMTime;															      //store receive time from ADVOV card [ms]
	Value.ReceiveTimePC = GetActualSystemTimeMs( );	                        //store receive time [ms]		
	Value.Value = rMVal;																										//store measured value
	
	AddValue(wPartIdx + 1, (AdvBaseTypes)eMainType , SubIndex, Value);
	#ifdef _DEBUG
	if( /*wPartIdx == 0 &&*/ eMainType == AdvIPu && ( rMVal * 1.0e6f ) < 50.0f && abs( rMVal * 1.0e6f ) > 1.0f )
	{
		printf("Cell[%d]:Error In: Ip<50: Value=%f\n", CellId, rMVal*1.0e6f);
		//printf("Idx=%d, CellId=%d, Wert=%f, MCTime=%lld, SysTime=%lld\n",wPartIdx,CellId,Value.Value, Value.ReceiveTime , Value.ReceiveTimePC);
	}	 
	#endif
	/*if( wPartIdx == 0 && eMainType == AdvIPu)
	{
		printf("CellId=%d, Wert=%f, MCTime=%lld, SysTime=%lld\n",CellId,Value.Value, Value.ReceiveTime , Value.ReceiveTimePC);
	}*/	

	
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IsSequenceFinished( void )                                                                  *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : sequence finished                                                                         *
 *                                  false = not finished                                                                   *
 *                                  true = finished                                                                        *
 *                                                                                                                         *
 * description:         : This is the function to check sequence finished.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::IsSequenceFinished( void )
{
	return SequenceFinished;	
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IsCardTemperatureOk( void )                                                                 *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : card temperature status                                                                   *
 *                                  false = card temperature ok                                                            *
 *                                  true = card temperature over upper limit                                               *
 *                                                                                                                         *
 * description:         : This is the function to check card temperature.                                                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::IsCardTemperatureOk( void )
{
	bool RetVal = true;

	if( AvgCardTemperature > 60.0f )
	{
		RetVal = false;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IsSequenceStarted( void )                                                                   *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : sequence started                                                                          *
 *                                  false = sequence started                                                               *
 *                                  true = sequence not started                                                            *
 *                                                                                                                         *
 * description:         : This is the function to check sequence start.                                                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::IsSequenceStarted( void )
{
	return SequenceStarted;		
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IsReadDataAdvovActive( void )                                                               *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : read data active                                                                          *
 *                                  false = not active                                                                     *
 *                                  true = active                                                                          *
 *                                                                                                                         *
 * description:         : This is the function to check read data active.                                                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SEMeasurementCell::IsReadDataAdvovActive( void )
{
	return ReadDataAdvovActive;		
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetReadDataAdvov( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void :                                                                                           *
 *                                                                                                                         *
 * description:         : This is the function to set read advov active.                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEMeasurementCell::SetReadDataAdvov( bool Active )
{
	ReadDataAdvovActive = Active;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetSequenceFinished( bool SequenceFinished  )                                               *
 *                                                                                                                         *
 * input:               : bool SequenceFinished : sequence status                                                          *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set the sequence status.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEMeasurementCell::SetSequenceFinished( bool SequenceFinished  )
{
	this->SequenceFinished = SequenceFinished;	
}


//private memberfunctions at SETrimmingCell---------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateTestSequenceTrim( TestSequenceTrimParam Parameter )                                  *
 *                                                                                                                         *
 * input:               : TestSequenceTrimParam Parameter :                                                                *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate test sequence for trimming cell.                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateTestSequence( TestSequenceParam Parameter )
{
	float HeatingEndTime = 0.0;																							//heating end time
	float TestTime = 0.0;																										//actual test time
	float LocalTime = 0.0;																									//local time
	float TimeContChkSS = 0.0;
	float EffStartTime = 0.0;
	float MaxContSSChkTime = 0.0;

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwTemperatureIn( LSTestHandle, TestTime, 800 );												//temperature measurement card

	TestTime = TestTime + 0.2f;

	//contact check heater on
	TFlwContChkHS( LSTestHandle, TestTime, 1 );
	TestTime = TestTime + 2.0f;

	
	//fast continue on
	if( Parameter.FastContinue == 0 )
	{
		TFlwHeaterVoltSet( LSTestHandle, TestTime, 1.0, 0.0 ); 								//set heater voltage to 1.0V, no ramp
		TestTime = TestTime + 2.8f;               														//add delay before RHk measurement
		TFlwHeaterColdResIn( LSTestHandle, TestTime, 0 );											//get heater cold resistance
		TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0, 0.0 );								//turn heater voltage off
		TestTime = TestTime + 0.2f;
	}

	EffStartTime = TestTime + 0.0f;																					//store effective start time

	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst );	//prepare cyclic measurement
	TFlwHeaterPowIn( LSTestHandle, TestTime, 900 );           							//measure heater power
	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst ); //prepare cyclic measurement
	TFlwHeaterHotResIn( LSTestHandle, TestTime, 900 );     									//measure heater hot resistance

	
	HeatingEndTime = 0.0;

	if( Parameter.MeasureFLO < 0.5 )
	{
		 ExecHeating( 1, 
									TestTime, 
									( TestTime + Parameter.Heater[0].HeatTime ), 
									Parameter.Heater[0].CooldownTime, 
									Parameter.Heater[0].HeatVolt, 
									Parameter.Heater[0].RampTime, 
									Parameter.Heater[0].HeatPow,  
									Parameter.Heater[0].MaxHeatVolt,
									&HeatingEndTime );	
	}

	TestTime = max ( TestTime, HeatingEndTime );
	
	

	if( Parameter.RiSetPoint > 0 ) 
	{ 
	 if( Parameter.MeasureFLO > 0.5 ) 
	 {
		TFlwRiPIDSnapRegSet( LSTestHandle, TestTime, Parameter.Heater[0].HeatVolt, Parameter.PHwhenRiRegFailed, Parameter.TimeoutForRiRegFailed, ( Parameter.RiTrigger - 10 ), Parameter.RiTrigger, Parameter.RiSetPoint, 0.5e-3f, 3000.0, 0.1e-3f, Parameter.RiRegMaxUH, Parameter.RiRegKp, Parameter.RiRegKi, Parameter.RiRegKd, 0.1f );
		TFlwHeaterEnergyIn( LSTestHandle, ( TestTime + 5.0f ), 1 );
		TFlwDtEHRiIn( LSTestHandle, ( TestTime + Parameter.MeasureFLODelay ), 1, 2);
		} 
		else 
		{
			 TFlwRiPIDRegSet( LSTestHandle, TestTime, Parameter.RiSetPoint, 0.5e-3f, 3000.0, 0.1e-3f, Parameter.RiRegMaxUH, Parameter.RiRegKp, Parameter.RiRegKi, Parameter.RiRegKd, 0.1f );
		}        
		TFlwDRHdtIn( LSTestHandle, ( TestTime + 0.4f + Parameter.DeltaTIHMeas ), 0e-3, Parameter.DeltaTIHMeas, 30, 1 ); 
		TFlwDIHdtIn( LSTestHandle, ( TestTime + 0.4f + Parameter.DeltaTIHMeas), 0e-3, Parameter.DeltaTIHMeas, 30, 1 );
	
		TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst ); //prepare cyclic measurement
		TFlwRiACIn( LSTestHandle, TestTime, 900 ); //measure RiAC  
	} 
	else 
	{ 
		ExecHeating( 2, 
								TestTime, 
								( TestTime + Parameter.Heater[1].HeatTime ), 
								Parameter.Heater[1].CooldownTime, 
								Parameter.Heater[1].HeatVolt, 
								Parameter.Heater[1].RampTime, 
								Parameter.Heater[1].HeatPow, 
								Parameter.Heater[1].MaxHeatVolt,
								&HeatingEndTime );

		TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst );  //prepare cyclic measurement
		TFlwRiACIn( LSTestHandle, TestTime, 900 );  //measure RiAC  
	}
	HeatingEndTime = 1000.0;

	TimeContChkSS = EffStartTime + 10.0f;

	//contact check sensor on
	MaxContSSChkTime = HeatingEndTime;
	TimeContChkSS = EffStartTime + 20.0f;
	if( TimeContChkSS >= MaxContSSChkTime ) 
	{ 
		TimeContChkSS = MaxContSSChkTime - 1.0f;
	}
	LocalTime = TimeContChkSS;
		
	TFlwPumpVoltAPESet( LSTestHandle, LocalTime, 0.4f );  //set pump voltage APE
	LocalTime = LocalTime + 0.5f;  //add delay
	TFlwPumpCurrAPEIn( LSTestHandle, LocalTime, 910 );  //measure pump current APE
	TFlwPumpVoltAPESet( LSTestHandle, ( LocalTime + 0.01f ), 0.0 );  //turn pump voltage APE off
	
	TFlwRiACIn( LSTestHandle, LocalTime, 910 );  //measure RiACN
	TFlwContChkSS( LSTestHandle, LocalTime, 2400.0f, 0.01e-3f, 1 ); //evaluate contact check
	TestTime = max( TestTime, ( LocalTime + 0.5f ) );


	TestTime = max ( TestTime, 5.0f ); //Erster Modulauftrag mind. 5s nach Start

	//pump current enabled
	if( Parameter.PumpCurrentRE >= 0.1e-6 )
	{
		TFlwPumpCurrRESet( LSTestHandle, TestTime, Parameter.PumpCurrentRE ); //set pump current RE
	}

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, Parameter.PumpVoltADV );  //set pump voltage APE

	if( Parameter.NernstVoltADV >= 0.1e-3 )
	{ 
		LocalTime =  TestTime + Parameter.UNRegDelay;  //set start time of UN regulation
		TFlwUNPIRegSet( LSTestHandle, LocalTime, Parameter.NernstVoltADV, Parameter.PumpVoltADV, -1.0, -0.28f, 30e-3f, 50e-3f );
	}

	TestTime = TestTime + 1.0f;
	TFlwCycMeasPrefix( LSTestHandle, 128, 1.0, -1 );  //prepare cyclic measurement
	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 911 );

	TestTime = max( TestTime, HeatingEndTime );
	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0, 0.0 );  //turn heater voltage off	 

	return 0;
}	

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GeneratePositionNormalContactSequence( void )                                                *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequence for contact check.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GeneratePositionNormalContactSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 0.55f );

	TestTime = 1.0f;

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 1 );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 0.0f);

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GeneratePositionNormalSequence( void )                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequence for the position normal.                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GeneratePositionNormalSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	//contact
	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 0.55f );

	TestTime = 1.0f;

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 3 );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 1.0f );

	TestTime = 2.0f;

	TFlwPumpVoltAPEIn( LSTestHandle, TestTime, 1 );

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 1 );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 1.3f );

	TFlwHeaterVoltSetNoRamp( LSTestHandle, TestTime, 6.0f );

	TestTime = 3.0f;

	TFlwPumpVoltAPEIn( LSTestHandle, TestTime, 2 );

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 2 );

	TFlwTestCurrRESet( LSTestHandle, TestTime, -1.0e-3f );

	TFlwHeaterCurrIn( LSTestHandle, TestTime, 1 );

	TFlwHeaterVoltIn( LSTestHandle, TestTime, 1 );

	TestTime = 4.0f;

	TFlwTestCurrIn( LSTestHandle, TestTime, 1 );

	TFlwPumpVoltREIn( LSTestHandle, TestTime, 1 );


	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 0.0f );

	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0f, 0.0f );

	TFlwTestCurrRESet( LSTestHandle, TestTime, 0.0f );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateHeaterNormalSequence( void )                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequence for the heater normal.                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateHeaterNormalSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwContChkHS( LSTestHandle, TestTime, 1 );

	TestTime = 1.5f;

	TFlwHeaterVoltSet( LSTestHandle, TestTime, ParamNormal->ParamHeaterNormal.HeaterVoltageSetpointHot, 0.0f );

	TestTime = 2.5f;

	TFlwHeaterHotResIn( LSTestHandle, TestTime, 1 );

	TestTime = 4.5f;

	TFlwHeaterCurrIn( LSTestHandle, TestTime, 1 );

	TestTime = 6.0f;

	TFlwHeaterVoltSet( LSTestHandle, TestTime, ParamNormal->ParamHeaterNormal.HeaterVoltageSetpointCold, 0.0f );

	TestTime = 8.0f;

	TFlwHeaterColdResIn( LSTestHandle, TestTime, 1 );

	TestTime = 11.0f;

	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0f, 0.0f );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateUniversalNormalSequence( void )                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequencefor the universal normal.                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateUniversalNormalSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwLeakCurrVoltSet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.UlSetpoint );

	TFlwPumpVoltRESet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.IgrkUpNSetpoint * 1.0e-3f ); 

	TestTime = 2.0f;

	TFlwFinalCurrRefChIn( LSTestHandle, TestTime, 1 );

	TFlwPumpVoltRESet( LSTestHandle, TestTime, 0.0f );

	TestTime = 2.3f;

	TFlwLeakCurrIn( LSTestHandle, TestTime, 1 );

	TFlwHeaterVoltSet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.RidcnHeaterVoltageSetpoint, 0.0f );

	TestTime = 2.8f;

	TFlwPumpVoltRESet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.RidcnUpNSetpoint * 1.0e-3f );

	TestTime = 4.8f;

	TFlwRiDCnIn( LSTestHandle, TestTime, 1 );
	
	TFlwPumpVoltRESet( LSTestHandle, TestTime, 0.0f );

	TFlwHeaterVoltSet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.IpreHeaterVoltageSetpoint, 0.0f );

	TestTime = 5.3f;

	TFlwPumpCurrRESet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.IpreIpNSetpoint * 1.0e-6f );

	TestTime = 7.3f;

	TFlwPumpCurrREIn( LSTestHandle, TestTime, 1 );

	TFlwPumpCurrRESet( LSTestHandle, TestTime, 0.0f );

	TestTime = 12.3f;

	TFlwRiACIn( LSTestHandle, TestTime, 1 );

	TFlwHeaterVoltSet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.RiacHeaterVoltageSetpoint, 0.0f );

	TestTime = 12.8f;

	TFlwRiACIn( LSTestHandle, TestTime, 2 );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateIlmNormalSequence( void )                                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequencefor the universal normal.                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateIlmNormalSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwLeakCurrVoltSet( LSTestHandle, TestTime, ParamNormal->ParamUniversalNormal.UlSetpoint );
	
	TestTime = 1.0f;

	TFlwMaxLeakCurrIn( LSTestHandle, TestTime, 1 );

	TFlwLeakCurrVoltSet( LSTestHandle, TestTime, 0.0f );

	TestTime = 2.0f;

	TFlwTestCurrAPESet( LSTestHandle, TestTime, ParamNormal->ParamIlmNormal.Pt3IpReSetpoint * 1.0e-3f );
																							
	TestTime = 3.0f;

	TFlwTestCurrIn( LSTestHandle, TestTime, 1 );

	TFlwPumpVoltAPEIn( LSTestHandle, TestTime, 1 );

	TFlwTestCurrAPESet( LSTestHandle, TestTime, 0.0f );

	TestTime = 4.0f;

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, ParamNormal->ParamIlmNormal.Pt3Up2Setpoint * 1.0e-3f );

	TestTime = 5.0f;

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 1 );

	TFlwNernstVoltIn( LSTestHandle, TestTime, 1 );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, ParamNormal->ParamIlmNormal.Pt3Up3Setpoint * 1.0e-3f );

	TestTime = 6.0f;

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 2 );

	TFlwNernstVoltIn( LSTestHandle, TestTime, 2 );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, ParamNormal->ParamIlmNormal.Pt3Up4Setpoint * 1.0e-3f );

	TestTime = 7.0f;

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 3 );

	TFlwNernstVoltIn( LSTestHandle, TestTime, 3 );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 0.0f );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateUnNormalSequence( void )                                                             *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequence for the un normal.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateUnNormalSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwHeaterVoltSet( LSTestHandle, TestTime, ParamNormal->ParamUnNormal.HeaterVoltageSetpoint, 0.0f );

	TestTime = 15.0f;

	TFlwNernstVoltIn( LSTestHandle, TestTime, 1 );

	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0f, 0.0f );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateIpNormalSequence( void )                                                             *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequence for the ip normal.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateIpNormalSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwHeaterVoltSet( LSTestHandle, TestTime, ParamNormal->ParamIpNormal.HeaterVoltageSetpoint, 0.0f );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, ParamNormal->ParamIpNormal.IpPumpVoltageSetpoint / 1000.0f ); 

	TestTime = 15.0f;

	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 1 );

	TFlwUAPEIn( LSTestHandle, TestTime, 1 );

	TFlwPumpVoltAPESet( LSTestHandle, TestTime, 0.0f );

	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0f, 0.0f );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateCoolingSequence( void )                                                              *
 *                                                                                                                         *
 * input:               : void                                                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate sequence for cooling.                                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateCoolingSequence( void )
{
	int RetVal = 0;
	float TestTime = 0.0;																										//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );												//channel select

	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.2f, 0.0f );

	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0f, -1, 1.0f );									//prepare cyclic measurement

	TFlwHeaterHotResIn( LSTestHandle, TestTime, 900);     									//measure heater hot resistance

	TestTime+=2000;

	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0, 0.0 );									//turn heater voltage off

	return RetVal;
}

//public memberfunctions at SETrimmingCell----------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Initializing( ParamStationDataTrimmingCell *ParamStationTrimmCell,                           *
 *                          ParamDataTrimmingCell *ParamTrimmCell, ParamDataNormalMeasure *ParamNormal )                   *
 *                                                                                                                         *
 * input:               : ParamStationDataTrimmingCell *ParamStationTrimmCell : station data trimming cell                 *
 *                        ParamDataTrimmingCell *ParamTrimmCell : parameter data trimming cell                             *
 * 												ParamDataNormalMeasure *ParamNormal : parameter data normal trimming cell                        *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the initialization function.                                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::Initializing( ParamStationDataTrimmingCell *ParamStationTrimmCell, ParamDataTrimmingCell *ParamTrimmCell, ParamDataNormalMeasure *ParamNormal )
{	
	int RetVal = 0;																													//return value
	int FuncRetVal = 0;
	bool FwMainLoadNecessary = false;
	bool FwModuleLoadNecessary = false;

	this->ParamStationTrimmCell = ParamStationTrimmCell;

	this->ParamTrimmCell = ParamTrimmCell;

	this->ParamNormal = ParamNormal;
	
	//-- init cell
	/*
	"MCxAssignCell" McListLow = 0x0000000F
	0000 0000 0000 0000 0000 0000 0000 1111
	-> 1st cell (test chamber 1) with 4 ADVOV cards
	"MCxAssignCell" McListLow = 0x00000010
	0000 0000 0000 0000 0000 0000 0001 0000
	-> 2nd cell (test chamber 1) with 1 ADVOV card
	"MCxAssignCell" McListLow = 0x00000F00
	0000 0000 0000 0000 0000 1111 0000 0000
	-> 3rd cell (test chamber 2) with 4 ADVOV cards
	"MCxAssignCell" McListLow = 0x00000F00
	0000 0000 0000 0000 0001 0000 0000 0000
	-> 4th cell (test chamber 2) with 1 ADVOV card
	*/
	FuncRetVal = MCxAssignCell( LSTestHandle, CellId, 0x0000000F << ( ( CellId - 1 ) * 4 ), 0x00000000);
	printf( "Cell[%d]:MCxAssignCell=%x\n", CellId, 0x0000000F << ( ( CellId - 1 ) * 4 ) );

	//init line resistance
	FuncRetVal = MCxInitRL( LSTestHandle, 
													( LONG ) pow( 2.0f, CellId - 1 ), 
													ParamStationTrimmCell->ParamHeaterCabelResistance[0],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[1],
													ParamStationTrimmCell->ParamHeaterCabelResistance[2], 
													ParamStationTrimmCell->ParamHeaterCabelResistance[3],
													ParamStationTrimmCell->ParamHeaterCabelResistance[4],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[5],
													ParamStationTrimmCell->ParamHeaterCabelResistance[6],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[7],
													ParamStationTrimmCell->ParamHeaterCabelResistance[8],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[9],
													ParamStationTrimmCell->ParamHeaterCabelResistance[10],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[11],
													ParamStationTrimmCell->ParamHeaterCabelResistance[12],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[13],
													ParamStationTrimmCell->ParamHeaterCabelResistance[14],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[15],
													ParamStationTrimmCell->ParamHeaterCabelResistance[16],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[17],
													ParamStationTrimmCell->ParamHeaterCabelResistance[18],  
													ParamStationTrimmCell->ParamHeaterCabelResistance[19] );

	//ADVOV test sequence stop
	FuncRetVal = MCxStop( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );

	if( FuncRetVal != 0 )
	{
		printf( "Cell[%d]:MCxStop=%d\n", CellId, RetVal );
	}

	//ADVOV calibration
	FuncRetVal = MCxCalib( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );
			
	//-- init cell
	
	return RetVal;
	
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SETrimmingCell::CheckFirmware( void )                                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                -1 = firmware update main failed                                                         *
 *                                -2 = firmware update module failed                                                       *
 *                                                                                                                         *
 * description:         : This is the function to check and update the firmware.                                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::CheckFirmware( void )
{
	int RetVal = 0;																													//return value
	int FuncRetVal = 0;
	MCINFO Info;
	bool FwMainLoadNecessary = false;
	bool FwModuleLoadNecessary = false;
	char TempString[1024];

	do
	{
		Sleep(1000);
	}while( AllCardsReady() != true );
	for( int i = 0; i < 4; i++)
	{
		FuncRetVal = MCxInfo( LSTestHandle, CellId, i + 1, &Info);
		if( FuncRetVal == 0 )
		{
			printf( "Cell[%d]:Initializing:Card%d:MainVer=%.3f;ModVer=%.3f\n", CellId, i + 1, Info.rMainSoftVer, Info.rModSoftVer );
			if(	Info.rMainSoftVer != (float)ADVOV_FW_MAIN )
			{
				FwMainLoadNecessary = true;
			}
			if(	Info.rModSoftVer != (float)ADVOV_FW_MODULE )
			{
				FwModuleLoadNecessary = true;
			}
		}
	}
	if( FwMainLoadNecessary == true)
	{
		FwMainLoadNecessary = false;
		printf( "Cell[%d]:Initializing:firmware update main processing...\n", CellId );
		do
		{
			setlocale( LC_ALL, "English" );
			sprintf_s( TempString, "%sadvhk_V%.3f_RT.hex", FW_LOCATION, ADVOV_FW_MAIN );
			setlocale( LC_ALL, "German" );

			RetVal = MCxFwUpload(	LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ), 0x08000000, TempString );
		}while( RetVal > 0 );
		do
		{
			RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );
		}while( RetVal > 0 );
		printf( "Cell[%d]:Initializing:firmware update main finished!\n", CellId );
	}

	if( FwModuleLoadNecessary == true)
	{
		FwModuleLoadNecessary = false;
		printf( "Cell[%d]:Initializing:firmware update module processing...\n", CellId );
		do
		{
			setlocale( LC_ALL, "English" );
			sprintf_s( TempString, "%sADVM24_v%.3f.hex", FW_LOCATION, ADVOV_FW_MODULE );
			setlocale( LC_ALL, "German" );
			RetVal = MCxFwUpload(	LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ), 0x04000000, TempString );
		}while( RetVal > 0 );	
		do
		{
			RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );
		}while( RetVal > 0 );
		printf( "Cell[%d]:Initializing:firmware update module finished!\n", CellId );
	}	
	for( int i = 0; i < 4; i++)
	{
		FuncRetVal = MCxInfo( LSTestHandle, CellId, i + 1, &Info);
		if( FuncRetVal == 0 )
		{
			if(	Info.rMainSoftVer != (float)ADVOV_FW_MAIN )
			{
				FwMainLoadNecessary = true;
			}
			if(	Info.rModSoftVer != (float)ADVOV_FW_MODULE )
			{
				FwModuleLoadNecessary = true;
			}
		}
	}

	if( FwMainLoadNecessary == true )
	{
		printf( "Cell[%d]:Initializing:firmware update main failed!\n", CellId );
		RetVal = -1;
	}
	if( FwModuleLoadNecessary == true )
	{
		printf( "Cell[%d]:Initializing:firmware update module failed!\n", CellId );
		RetVal = -2;
	}	 
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int InstallThread( LPTHREAD_START_ROUTINE CallbackAddressCell, 		                               * 
 *                                           LONG ( *pFCallBackCell )( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,*
 *																					 DWORD tsMTime,LONG lResIdx ) )																								 *
 *                                                                                                                         *
 * input:               : LPTHREAD_START_ROUTINE CallbackAddress : thread start routine                                    *
 *                        LONG ( *pFCallBackCell ) (...) : callback function for fast measurement                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                               0 = no error                                                                              *
 *                              -1 = parameter error fast measurement callback                                             *
 *                              -2 = thread running or parameter error                                                     *
 *                              -3 = no thread created                                                                     *
 *                                                                                                                         *
 * description:         : This is the function to create the cell handle handle thread.                                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEMeasurementCell::InstallThread( LPTHREAD_START_ROUTINE CallbackAddressCell, 
																			LONG ( *pFCallBackCell )( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD 
																			tsMTime,LONG lResIdx ) )
{
	int RetVal = 0;
	DWORD dwThreadId, dwThrdParam;

	if( pFCallBackCell != NULL )
	{
		RetVal = MCxCycResCallback( LSTestHandle,
																( LONG ) pow( 2.0f, CellId - 1 ), pFCallBackCell );				        //install receive callback chamber 1 trimming cell
		printf( "Cell[%d]:InstallThread:MCxCycResCallback=%d\n", CellId, RetVal );
	}
	else
	{
		RetVal = -1;
	}
								 
	if( ( SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] == NULL)	&&
			( CallbackAddressCell != NULL ) )
	{
		//create thread
		//no security, default stack size, default creation
		SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] = CreateThread( NULL,		
																											0,                       
																											CallbackAddressCell,             
																											&dwThrdParam,               
																											CREATE_SUSPENDED,                         
																											&dwThreadId); 
		printf( "Cell[%d]:InstallThread:CreateThread=%lx\n", CellId, dwThreadId );
		if( SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] == NULL )
		{
			printf( "Cell[%d]:Thread not created!\n", CellId );
			RetVal = -3;
		}
		else
		{
			printf( "Cell[%d]:Thread created!\n", CellId );
		}
		
	}
	else
	{
		printf( "Cell[%d]:Thread not created Zero!\n", CellId );
		RetVal = -2;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void DeinstallThread( void )                                                                     *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to exit cell handle thread.                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SEMeasurementCell::DeinstallThread( void )
{
	DWORD ExitCode = 0;
	DeinstallThreadRequest = true;
	do
	{
		Sleep(100);
		GetExitCodeThread( SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1], &ExitCode);
	}while( ExitCode == STILL_ACTIVE );

	printf( "Cell[%d]:DeinstallThread:GetExitCodeThread=%d\n", CellId, ExitCode );
			
	DeinstallThreadRequest = false;

	if( SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] != NULL)
	{
		SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] = NULL;
		printf("Cell[%d]:Thread terminated!\n", CellId);
	}

}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateAndTransmittSequence( ProcessType Type )                                             *
 *                                                                                                                         *
 * input:               : ProcessType Type : type of process                                                               *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for generate and transmitt the sequence.                                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingCell::GenerateAndTransmittSequence( ProcessType Type )
{
	int RetVal = 0;																													//return value
	TestSequenceParam Parameter;																						//test sequence parameter

	ProcessTypeLocal = Type;
	
	RetVal = TFlwNew( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );						//initiate new sequence
	printf( "Cell[%d]:TFlwNew=%d\n", CellId, RetVal );

	switch( Type )
	{
		case MeasureTrimming:
		case MeasureSelection:
			//copy input parameter to sequence
			Parameter.PumpCurrentRE = ParamTrimmCell->ParamMainTrimmingCell.IpRef * 1e-6f;
			Parameter.PumpVoltADV = ParamTrimmCell->ParamMainTrimmingCell.PumpVoltage;
			if( ProcessTypeLocal == MeasureSelection )
			{
				Parameter.RiSetPoint = ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset;
			}
			else
			{
				Parameter.RiSetPoint = ParamTrimmCell->ParamMainTrimmingCell.HeatingRiTrimming + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset;
			}
			Parameter.UNRegDelay = ParamTrimmCell->ParamMainTrimmingCell.StartDelay;
			Parameter.NernstVoltADV = ParamTrimmCell->ParamMainTrimmingCell.NernstVoltage;
			Parameter.FastContinue = 0.0;
			Parameter.LowPassConst = 1.0f;
			Parameter.RiTrigger = ParamTrimmCell->ParamMainTrimmingCell.RiTrigger;
			Parameter.DeltaTIHMeas = ParamTrimmCell->ParamMainTrimmingCell.DeltaTihMeas * 1e-3f;
			Parameter.RiRegMaxUH = ParamTrimmCell->ParamRiRegulaton.RiRegMaxUH;
			Parameter.RiRegKp = ParamTrimmCell->ParamRiRegulaton.RiRegKp;
			Parameter.RiRegKi = ParamTrimmCell->ParamRiRegulaton.RiRegKi;
			Parameter.RiRegKd = ParamTrimmCell->ParamRiRegulaton.RiRegKd;
			Parameter.PHwhenRiRegFailed = ParamTrimmCell->ParamMainTrimmingCell.PhDisturbedRi;
			Parameter.TimeoutForRiRegFailed = ParamTrimmCell->ParamMainTrimmingCell.TimeoutDisturbedRi;
			Parameter.Heater[0].CooldownTime = 0.0;
			Parameter.Heater[0].HeatPow = 0.0;
			Parameter.Heater[0].HeatTime = ParamTrimmCell->ParamMainTrimmingCell.HeatingTime;
			Parameter.Heater[0].HeatVolt = ParamTrimmCell->ParamMainTrimmingCell.HeatingVoltage;
			Parameter.Heater[0].MaxHeatVolt = ParamTrimmCell->ParamMainTrimmingCell.HeatingVoltage;
			Parameter.Heater[0].RampTime = ParamTrimmCell->ParamMainTrimmingCell.HeaterRampDuration;
			Parameter.Heater[1].CooldownTime = 0.0;
			Parameter.Heater[1].HeatPow = ParamTrimmCell->ParamMainTrimmingCell.HeatingPower;
			Parameter.Heater[1].HeatTime = 1000.0f;
			Parameter.Heater[1].HeatVolt = 0.0;
			Parameter.Heater[1].MaxHeatVolt = ParamTrimmCell->ParamMainTrimmingCell.HeatingVoltage;
			Parameter.Heater[1].RampTime = 0.0;	 	 
			if( ParamTrimmCell->ParamMainTrimmingCell.MeasFLOEnable == FALSE )
			{
				Parameter.MeasureFLODelay = 10;
				Parameter.MeasureFLO = 0.0f;
			}
			else
			{
				Parameter.MeasureFLODelay = 10;
				Parameter.MeasureFLO = 1.0f;
			}
			if( ParamTrimmCell->ParamIpMeasurement.EnableNernstRegulation == FALSE )
			{
				Parameter.NernstVoltADV = 0.0;
			}

			if( ParamTrimmCell->ParamMainTrimmingCell.HeatingType == TRUE )
			{
				Parameter.RiSetPoint = -1.0f;
			}	
	
			GenerateTestSequence( Parameter );															    //generate sequence function calls for trimming cell
			break;		

		case MeasurePositionNormal:
			GeneratePositionNormalSequence( );															    //generate sequence function calls for position normal
			break;

		case MeasureHeaterNormal:
			GenerateHeaterNormalSequence( );															      //generate sequence function calls for heater normal
			break;
				
		case MeasureUniversalNormal:
			GenerateUniversalNormalSequence( );															    //generate sequence function calls for universal normal
			break;

		case MeasureUnNormal:
			GenerateUnNormalSequence( );															          //generate sequence function calls for un normal
			break;

		case MeasureIpNormal:
			GenerateIpNormalSequence( );															          //generate sequence function calls for ip normal
			break;

		case MeasureIlmNormal:
			GenerateIlmNormalSequence( );															          //generate sequence function calls for ilm normal
			break;

		case Cooling:
			GenerateCoolingSequence( );															            //generate sequence function calls for cooling
			break;

		default:
			;
			break;
	}

	for( int i = 0; i < 100; i++ )
	{
		RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) )	;
		printf( "Cell[%d]:MCxLastError=%d\n", CellId, RetVal );
		if( RetVal == 0 )
		{
			break;
		}
		Sleep(100);
	}

	RetVal = TFlwGenerate( LSTestHandle );																	//generate sequence
	printf("Cell[%d]:TFlwGenerate=%d\n", CellId, RetVal);

	RetVal = MCxTFlowTx( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );														//transmitt sequence to ADVOV cards

	printf("Cell[%d]:MCxTFlowTx=%d\n", CellId, RetVal);
	
	for( int i = 0; i < 100; i++ )
	{
		RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );												//get last error
		printf( "Cell[%d]:MCxLastError=%d\n", CellId, RetVal );
		if( RetVal <= 0 )
		{
			break;
		}
		Sleep(100);
	}

	return RetVal;
}


//private memberfunctions at SEReferenceCell--------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateTestSequenceRefFlow( TestSequenceRefFlowParam Parameter )                            *
 *                                                                                                                         *
 * input:               : TestSequenceRefFlowParam Parameter :                                                             *                                                                                            
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate test sequence for reference cell.                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEReferenceCell::GenerateTestSequence( TestSequenceParam Parameter )
{
	float HeatingEndTime = 0.0;																							//heating end time
	float TestTime = 0.0;																										//actual test time
	float LocalTime = 0.0;																									//local time
	float TimeContChkSS = 0.0;
	float EffStartTime = 0.0;
	
	//-- sequence start --
	TFlwChSelect( LSTestHandle, TestTime, 0x00003 );												//channel select

	TFlwTemperatureIn( LSTestHandle, TestTime, 800 );												//temperature measurement card
		 
	//contact check heater on
	TFlwContChkHS( LSTestHandle, TestTime, 1 );														  //contact check heater
	TestTime = TestTime + 2.0f;
	

	//fast continue on
	if( Parameter.FastContinue == 0)
	{
		TFlwHeaterVoltSet( LSTestHandle, TestTime, 1.0, 0.0 ); 								//set heater voltage to 1.0V, no ramp
		TestTime = TestTime + 2.8f;               														//add delay before rhk measurement
		TFlwHeaterColdResIn( LSTestHandle, TestTime, 0 );											//get heater cold resistance
		TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0, 0.0 );								//turn heater voltage off
		TestTime = TestTime + 0.2f;
	}

	EffStartTime = TestTime + 0.0f;																					//store effective start time

	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst );	//prepare cyclic measurement
	TFlwHeaterPowIn( LSTestHandle, TestTime, 900 );           							//measure heater power
	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst ); //prepare cyclic measurement
	TFlwHeaterHotResIn( LSTestHandle, TestTime, 900);     									//measure heater hot resistance


	//execute heating phase 1
	ExecHeating( 1, 
							TestTime, 
							( TestTime + Parameter.Heater[0].HeatTime ),  
							Parameter.Heater[0].CooldownTime, 
							Parameter.Heater[0].HeatVolt, 
							Parameter.Heater[0].RampTime, 
							Parameter.Heater[0].HeatPow, 
							Parameter.Heater[0].MaxHeatVolt,
							&HeatingEndTime );

	TestTime = max ( TestTime, HeatingEndTime );														//set test time to max time

	//ri set point enabled
	if( Parameter.RiSetPoint > 0 ) 
	{ 
		//set ri regulation
		TFlwRiPIDRegSet( LSTestHandle, TestTime, Parameter.RiSetPoint, 0.5e-3f, 3000.0f, 0.1e-3f, 12.0f, 0.1f, 20.0f, 0.02f, 0.1f );
		TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst ); //prepare cyclic measurement
		TFlwRiACIn( LSTestHandle, TestTime ,900 );															//measure riac
	}
	else 
	{ 
		//execute heating phase 2
		ExecHeating( 2,
								TestTime, 
								( TestTime + Parameter.Heater[1].HeatTime ), 
								Parameter.Heater[1].CooldownTime, 
								Parameter.Heater[1].HeatVolt, 
								Parameter.Heater[1].RampTime, 
								Parameter.Heater[1].HeatPow, 
								Parameter.Heater[1].MaxHeatVolt,
								&HeatingEndTime );
	}	 

	HeatingEndTime = 1000.0;

	TimeContChkSS = EffStartTime + 5.0f;

	//contact check sensor on
	TimeContChkSS = EffStartTime + 7.0f;

	//contact check time point after heating end 
	if( TimeContChkSS >= HeatingEndTime ) 
	{ 
		TimeContChkSS = HeatingEndTime - 1.0f;															//perform contact check before heating end
	}
	LocalTime = TimeContChkSS;																						//set loacal time
		
	TFlwPumpVoltAPESet( LSTestHandle, LocalTime, 0.4f );									//set pump voltage ape
	LocalTime = LocalTime + 0.5f;																					//add delay
	TFlwPumpCurrAPEIn( LSTestHandle, LocalTime, 910 );										//measure pump current ape
	TFlwPumpVoltAPESet( LSTestHandle, ( LocalTime + 0.01f ), 0.0);				//turn pump voltage ape off
	TFlwRiACIn( LSTestHandle, LocalTime, 910 );														//measure riacin
	//evaluate contact check
	TFlwContChkSS( LSTestHandle, LocalTime, 2400.0, 0.01e-3f, 1 );	
	TestTime = max( TestTime, LocalTime);																	//set test time to max time

	//negative pump voltage pulse
	if( Parameter.NegPulseTime > 0.0 ) 
	{
		LocalTime = TestTime + 10.0f;																					//set start time of negative pulse
		TFlwPumpVoltAPESet( LSTestHandle, LocalTime, Parameter.NegPulseVolt );//turn pump voltage ape on
		LocalTime = LocalTime + Parameter.NegPulseTime;												//set end time of negative pulse
		TFlwPumpVoltAPESet( LSTestHandle, LocalTime, 0.0 );										//turn pump voltage ape off
		TestTime = max( TestTime, LocalTime);																	//set test time to max time
	}

	TestTime = max( TestTime, 5.0f);																				//first modul order 5s after start

	//pump current enabled
	if( Parameter.PumpCurrentRE >= 0.1e-6 ) 
	{
		TFlwPumpCurrRESet( LSTestHandle, TestTime, Parameter.PumpCurrentRE );	//set pump current re
	}


	TFlwPumpVoltAPESet( LSTestHandle, TestTime, Parameter.PumpVoltADV );		//set pump voltage ape
	if( Parameter.NernstVoltADV >= 0.1e-3 )
	{ 
		LocalTime = TestTime + Parameter.UNRegDelay;													//set start time of UN regulation
		TFlwUNPIRegSet( LSTestHandle, LocalTime, Parameter.NernstVoltADV, Parameter.PumpVoltADV, -1.0f, -0.28f, 30e-3f, 50e-3f );
	}

	TestTime = TestTime + 1.0f;
	TFlwCycMeasPrefix( LSTestHandle, 128, 1.0, -1 ); 												//prepare cyclic measurement
	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 911 );												//measure pump current ape

	TestTime = max( TestTime, HeatingEndTime );															//set test time to max time
	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0, 0.0 );									//turn heater voltage off

	return 0;
}




//public memberfunctions at SEReferenceCell---------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Initializing( ParamStationDataReferenceCell *ParamStationRefCell,                            *
 *                          ParamDataReferenceCell *ParamRefCell )                                                         *
 *                                                                                                                         *
 * input:               : ParamStationDataReferenceCell *ParamStationRefCell : station data reference cell                 *
 *                        ParamDataReferenceCell *ParamRefCell : parameter data reference cell                             * 
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                -1 = firmware update main failed                                                         *
 *                                -2 = firmware update module failed                                                       *
 *                                                                                                                         *
 * description:         : This is the initialization function.                                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEReferenceCell::Initializing( ParamStationDataReferenceCell *ParamStationRefCell, ParamDataReferenceCell *ParamRefCell )
{	
	int RetVal = 0;																													//return value
	int FuncRetVal = 0;

	this->ParamRefCell = ParamRefCell;

	this->ParamStationRefCell = ParamStationRefCell;
	
	//-- init cell
	/*
	"MCxAssignCell" McListLow = 0x0000000F
	0000 0000 0000 0000 0000 0000 0000 1111
	-> 1st cell (test chamber 1) with 4 ADVOV cards
	"MCxAssignCell" McListLow = 0x00000010
	0000 0000 0000 0000 0000 0000 0001 0000
	-> 2nd cell (test chamber 1) with 1 ADVOV card
	"MCxAssignCell" McListLow = 0x00000F00
	0000 0000 0000 0000 0000 1111 0000 0000
	-> 3rd cell (test chamber 2) with 4 ADVOV cards
	"MCxAssignCell" McListLow = 0x00000F00
	0000 0000 0000 0000 0001 0000 0000 0000
	-> 4th cell (test chamber 2) with 1 ADVOV card
	*/
	FuncRetVal = MCxAssignCell( LSTestHandle, CellId, 0x00000001 << ( ( CellId - 1 ) * 4 ), 0x00000000);
		printf( "Cell[%d]:MCxAssignCell=%x\n", CellId, 0x00000001 << ( ( CellId - 1 ) * 4 ) );
	
	//init line resistance
	FuncRetVal = MCxInitRL( LSTestHandle, 
													( LONG ) pow( 2.0f, CellId - 1 ), 
													ParamStationRefCell->ParamHeaterCabelResistance[0],  
													ParamStationRefCell->ParamHeaterCabelResistance[1],
													0.0, 
													0.0,
													0.0,  
													0.0,
													0.0,  
													0.0,
													0.0,  
													0.0,
													0.0,  
													0.0,
													0.0,  
													0.0,
													0.0,  
													0.0,
													0.0,  
													0.0,
													0.0,  
													0.0 );

	//ADVOV test sequence stop
	FuncRetVal = MCxStop( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );

	if( FuncRetVal != 0 )
	{
		printf( "Cell[%d]:MCxStop=%d\n", CellId, RetVal );
	}

	//ADVOV calibration
	FuncRetVal = MCxCalib( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );

	//-- init cell
	
	return RetVal;
	
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SETrimmingCell::CheckFirmware( void )                                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                -1 = firmware update main failed                                                         *
 *                                -2 = firmware update module failed                                                       *
 *                                                                                                                         *
 * description:         : This is the function to check and update the firmware.                                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEReferenceCell::CheckFirmware( void )
{
	int RetVal = 0;																													//return value
	int FuncRetVal = 0;
	MCINFO Info;
	bool FwMainLoadNecessary = false;
	bool FwModuleLoadNecessary = false;
	char TempString[1024];

	do
	{
		Sleep(1000);
	}while( AllCardsReady() != true );
	for( int i = 0; i < 1; i++)
	{
		FuncRetVal = MCxInfo( LSTestHandle, CellId, i + 1, &Info);
		if( FuncRetVal == 0 )
		{
			printf( "Cell[%d]:Initializing:Card%d:MainVer=%.3f;ModVer=%.3f\n", CellId, i + 1, Info.rMainSoftVer, Info.rModSoftVer );
			if(	Info.rMainSoftVer != (float)ADVOV_FW_MAIN )
			{
				FwMainLoadNecessary = true;
			}
			if(	Info.rModSoftVer != (float)ADVOV_FW_MODULE )
			{
				FwModuleLoadNecessary = true;
			}
		}
	}
	if( FwMainLoadNecessary == true)
	{
		FwMainLoadNecessary = false;
		printf( "Cell[%d]:Initializing:firmware update main processing...\n", CellId );
		do
		{
			setlocale( LC_ALL, "English" );
			sprintf_s( TempString, "%sadvhk_V%.3f_RT.hex", FW_LOCATION, ADVOV_FW_MAIN );
			setlocale( LC_ALL, "German" );

			RetVal = MCxFwUpload( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ), 0x08000000, TempString );
		}while( RetVal > 0 );
		do
		{
			RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );
		}while( RetVal > 0 );
		printf( "Cell[%d]:Initializing:firmware update main finished!\n", CellId );
	}

	if( FwModuleLoadNecessary == true)
	{
		FwModuleLoadNecessary = false;
		printf( "Cell[%d]:Initializing:firmware update module processing...\n", CellId );
		do
		{
			setlocale( LC_ALL, "English" );
			sprintf_s( TempString, "%sADVM24_v%.3f.hex", FW_LOCATION, ADVOV_FW_MODULE );
			setlocale( LC_ALL, "German" );
			RetVal = MCxFwUpload(	LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ), 0x04000000, TempString );
		}while( RetVal > 0 );	
		do
		{
			RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );
		}while( RetVal > 0 );
		printf( "Cell[%d]:Initializing:firmware update module finished!\n", CellId );
	}	
	for( int i = 0; i < 1; i++)
	{
		FuncRetVal = MCxInfo( LSTestHandle, CellId, i + 1, &Info);
		if( FuncRetVal == 0 )
		{
			if(	Info.rMainSoftVer != (float)ADVOV_FW_MAIN )
			{
				FwMainLoadNecessary = true;
			}
			if(	Info.rModSoftVer != (float)ADVOV_FW_MODULE )
			{
				FwModuleLoadNecessary = true;
			}
		}
	}

	if( FwMainLoadNecessary == true )
	{
		printf( "Cell[%d]:Initializing:firmware update main failed!\n", CellId );
		RetVal = -1;
	}
	if( FwModuleLoadNecessary == true )
	{
		printf( "Cell[%d]:Initializing:firmware update module failed!\n", CellId );
		RetVal = -2;
	}	 
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GenerateAndTransmittSequence(void)                                                           *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the destructor function.                                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEReferenceCell::GenerateAndTransmittSequence(void)
{
	int RetVal = 0;																													//return value
	TestSequenceParam Parameter;																						//test sequence parameter
	
	printf( "Generate Start: Handle=%p\n", LSTestHandle );
	RetVal = TFlwNew( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );						//initiate new sequence
	printf( "Cell[%d]:TFlwNew=%d\n", CellId, RetVal );
										
	Parameter.PumpCurrentRE = ParamRefCell->ParamMainReferenceCell.IpRef * 1e-6f;
	Parameter.PumpVoltADV = ParamRefCell->ParamMainReferenceCell.PumpVoltage;
	Parameter.RiSetPoint = ParamRefCell->ParamMainReferenceCell.HeatingRi;
	Parameter.UNRegDelay = ParamRefCell->ParamMainReferenceCell.NernstRegulationStartDelay;
	Parameter.NernstVoltADV = ParamRefCell->ParamMainReferenceCell.NernstVoltage;
	Parameter.FastContinue = 1.0f;
	Parameter.NegPulseVolt = 0.0;
	Parameter.NegPulseTime = 0.0;
	Parameter.LowPassConst = 1.0f;
	Parameter.Heater[0].CooldownTime = 0.0;
	Parameter.Heater[0].HeatPow = 0.0;
	Parameter.Heater[0].HeatTime = ParamRefCell->ParamMainReferenceCell.HeatingTime;
	Parameter.Heater[0].HeatVolt = ParamRefCell->ParamMainReferenceCell.HeatingVoltage;
	Parameter.Heater[0].MaxHeatVolt = 0.0;
	Parameter.Heater[0].RampTime = 0.0;
	Parameter.Heater[1].CooldownTime = 0.0;
	Parameter.Heater[1].HeatPow = ParamRefCell->ParamMainReferenceCell.HeatingPower;
	Parameter.Heater[1].HeatTime = 0.0;
	Parameter.Heater[1].HeatVolt = 0.0;
	Parameter.Heater[1].MaxHeatVolt = 0.0;
	Parameter.Heater[1].RampTime = 0.0;
	if( ParamRefCell->ParamMainReferenceCell.EnableNernstRegulation == FALSE )
	{
		Parameter.NernstVoltADV = 0.0;
	}
	
	if( ParamRefCell->ParamMainReferenceCell.HeatingMode = TRUE )
	{
		Parameter.FastContinue = 1.0f;
	}
	else
	{
		Parameter.FastContinue = 0.0;	
	}

	GenerateTestSequence( Parameter );														//generate sequence function calls for reference cell

	for( int i = 0; i < 100; i++ )
	{
		RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) )	;
		printf( "Cell[%d]:MCxLastError=%d\n", CellId, RetVal );
		if( RetVal == 0 )
		{
			break;
		}
		Sleep(100);
	}


	RetVal = TFlwGenerate( LSTestHandle );																	//generate sequence
	printf("Cell[%d]:TFlwGenerate=%d\n", CellId, RetVal);
	
	RetVal = MCxTFlowTx( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );  //transmitt sequence to ADVOV cards
	printf("Cell[%d]:MCxTFlowTx=%d\n", CellId, RetVal);
	do
	{
		RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) )	;
		printf( "Cell[%d]:MCxLastError=%d\n", CellId, RetVal );
		Sleep( 100 );
	}while( RetVal > 0 );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GetIpReference(int ReferenceSensor, float * IpReference)                                     *
 *                                                                                                                         *
 * input:               : int ReferenceSensor : reference sensor number                                                    *
 *                        float * IpReference : pointer to Ip reference value                                              *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                -1 = parameter error																																		 *
 *                                -2 = no valid value									                                                     *
 *																																																												 *
 * description:         : This is the function to calcualte and get the Ip reference value.                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SEReferenceCell::GetLastIpMean( int Tiepoint, unsigned int MeanCount ,float * IpReference )
{
	int RetVal = 0;
	SEValueInfo Value;
	float ValueSum = 0.0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= 2 ) &&
			( IpReference != NULL )	&&
			( IpValues[Tiepoint - 1].size() >= MeanCount ) )
	{
		for( unsigned int i = 0; i < MeanCount; i++)
		{
			Value = IpValues[Tiepoint - 1].at( IpValues[Tiepoint - 1].size() - i - 1 );
			ValueSum += Value.Value;
			if( _isnan( Value.Value ) != 0 )
			{
				RetVal = -2;
				break;
			}
		}
		*IpReference = ValueSum / MeanCount;
	}
	else
	{
		RetVal = -1;
	}

	return RetVal;
}






