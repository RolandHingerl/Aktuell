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
#if 0
#include "..\Library\SETestDLL.h"
#else
#include "..\Library\LSTestDLL.h"
#include "..\Library\MCDataEx.h"
#endif

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
	ProcessTypeLocal = ProcessType::MeasureTrimmingSelection;


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

	// SequenceStartTime	= 0;

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

		memset(&RhhPoint1, 0, sizeof(RhhPoint1));
		memset(&RhhPoint2, 0, sizeof(RhhPoint2));


		memset(&PhPoint1, 0, sizeof(PhPoint1));
		memset(&PhPoint2, 0, sizeof(PhPoint2));

		memset(&RiValues1, 0, sizeof(RiValues1));
		memset(&RiValues2, 0, sizeof(RiValues2));

		memset(&RiPoint1, 0, sizeof(RiPoint1));
		memset(&RiPoint2, 0, sizeof(RiPoint2));

		memset(&RiMinValues, 0, sizeof(RiMinValues));
		memset(&RiMaxValues, 0, sizeof(RiMaxValues));

		memset(&IpValues1, 0, sizeof(IpValues1));
		memset(&IpValues2, 0, sizeof(IpValues2));
		memset(&IpValues3, 0, sizeof(IpValues3));
		memset(&Ip450Average, 0, sizeof(Ip450Average)); //measured Ip values (ident=259, SubIndex=100)

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
		memset(&UapeAverage, 0, sizeof(UapeAverage));
		memset(&UapePoint1, 0, sizeof(UapePoint1));
		memset(&UapePoint2, 0, sizeof(UapePoint2));

		memset(&UnValues, 0, sizeof(UnValues));
		memset(&UnValues2, 0, sizeof(UnValues2));
		memset(&UnValues3, 0, sizeof(UnValues3));
		memset(&IpReValues, 0, sizeof(IpReValues));
		memset(&IpReValue1, 0, sizeof(IpReValue1));
		memset(&IpReValue2, 0, sizeof(IpReValue2));

		memset(&IPuPoint1, 0, sizeof(IPuPoint1));
		memset(&IPuPoint2, 0, sizeof(IPuPoint2));

		memset(&UReValues, 0, sizeof(UReValues));
		memset(&UReValues2, 0, sizeof(UReValues2));

		memset(&URePoint1, 0, sizeof(URePoint1));
		memset(&URePoint2, 0, sizeof(URePoint2));

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
int SEMeasurementCell::ClearValues(int Tiepoint, eIMTBasTypes eImtType)
{
	int RetVal = 0;
	
	switch(eImtType)
	{
		case IMT_RiAC:
			RiValues[Tiepoint - 1].clear();
			break;
			
		case IMT_IPu:
			IpValues[Tiepoint - 1].clear();
			break;
		
		case IMT_RHh:
			RhhValues[Tiepoint - 1].clear();
			break;
		
		case IMT_PH:
			PhValues[Tiepoint - 1].clear();
	break;
	
	default:
		printf( "Cell[%d]:SEMeasurementCell::ClearValues - requested value not present ( type = %d )!\n", CellId, eImtType );
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
int SEMeasurementCell::AddValue(int Tiepoint, eIMTBasTypes eImtType, int Index, SEValueInfo Value)
{
	int RetVal = 0;
	std::vector <SEValueInfo>::iterator Iter;
	 
	switch( eImtType )
	{
		case IMT_Temp:
			TemperatureValues[Tiepoint -1] = Value;
			break;

		case IMT_RiAC:
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
				case 101:
					RiPoint1[Tiepoint - 1] = Value;
					break;
				case 102:
					RiPoint2[Tiepoint - 1] = Value;
					break;
				default:
					;
					break;
			}
			break;

		case IMT_IPu:
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
				case 100:
					Ip450Average[Tiepoint - 1] = Value;
#if defined _INDUTRON_PRINT_MORE
					if (Tiepoint == 1)
					{
						printf("## IPu average value: %f\n", Value.Value);
					}
#endif
					break;
				case 101:
					IPuPoint1[Tiepoint - 1] = Value;
					break;
				case 102:
					IPuPoint2[Tiepoint - 1] = Value;
					break;
				default:
					;
					break;
			}

			break;
		
		case IMT_RHh:
			switch (Index)
			{
			case 101:
				RhhPoint1[Tiepoint - 1] = Value;
				break;
			case 102:
				RhhPoint2[Tiepoint - 1] = Value;
				break;
			default:

				//Iter = RhhValues[Tiepoint - 1].end();
				//RhhValues[Tiepoint - 1].insert( Iter, Value );
				RhhValues[Tiepoint - 1].push_back(Value);
				break;
			}
			break;
		
		case IMT_PH:
			switch (Index)
			{
			case 101:
				PhPoint1[Tiepoint - 1] = Value;
				break;
			case 102:
				PhPoint2[Tiepoint - 1] = Value;
				break;
			default:
				//Iter = PhValues[Tiepoint - 1].end(); 
				//PhValues[Tiepoint - 1].insert( Iter, Value );
				PhValues[Tiepoint - 1].push_back(Value);
				break;
			}
			break;

		case IMT_UHS:
			UhValues[Tiepoint - 1] = Value;
			break;

		case IMT_UHmin:
			UhMinValues[Tiepoint - 1] = Value;
			break;

		case IMT_UHmax:
			UhMaxValues[Tiepoint - 1] = Value;
			break;

		case IMT_RHk:
			RhkValues[Tiepoint - 1] = Value;
			break;

		case IMT_EH:
			EhValues[Tiepoint - 1] = Value;
			break;

		case IMT_dtRiReg:
			RiRegValues[Tiepoint - 1] = Value;
			break;

		case IMT_dRHdt:
			RhDtValues[Tiepoint - 1] = Value;
			break;

		case IMT_dIHdt:
			IhDtValues[Tiepoint - 1] = Value;
			break;

		case IMT_HSCONT:
			HSContValues[Tiepoint - 1] = Value;
			break;

		case IMT_SSCONT:
			SSContValues[Tiepoint - 1] = Value;
			break;

		case IMT_IH:
			IhValues[Tiepoint - 1] = Value;
			break;

		case IMT_UAPE:
			switch( Index )
			{
				case 0:
				case 1:
					UapeValues[Tiepoint - 1] = Value;
					break;
				case 2:
					UapeValues2[Tiepoint - 1] = Value;
					break;
				case 100:
					UapeAverage[Tiepoint - 1] = Value;
					break;
				case 101:
					UapePoint1[Tiepoint - 1] = Value;
					break;
				case 102:
					UapePoint2[Tiepoint - 1] = Value;
				default:
					;
					break;
			}

			break;

		case IMT_UN:
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

		case IMT_IpRE:
		
			switch (Index)
			{
			case 101:
				IpReValue1[Tiepoint - 1] = Value;
				break;
			case 102:
				IpReValue2[Tiepoint - 1] = Value;
				break;
			default:
				IpReValues[Tiepoint - 1] = Value;
				break;
			}
			break;

		case IMT_URE:
			switch (Index)
			{
			case 3:
				UReValues2[Tiepoint - 1] = Value;
				break;
			case 101:
				URePoint1[Tiepoint -1] = Value;
				break;
			case 102:
				URePoint2[Tiepoint - 1] = Value;
				break;
			default:
				UReValues[Tiepoint - 1] = Value;
				break;
			}
			break;

		case IMT_IgRK:
			IgRkValues[Tiepoint - 1] = Value;
			break;

		case IMT_IL:
			IlValues[Tiepoint - 1] = Value;
			break;

		case IMT_RiDCn:
			RiDCnValues[Tiepoint - 1] = Value;
			break;

		case IMT_IPr:
			IprValues[Tiepoint - 1] = Value;
			break;

		case IMT_RiMin:
			RiMinValues[Tiepoint - 1] = Value;
			break;

		case IMT_RiMax:
			RiMaxValues[Tiepoint - 1] = Value;
			break;
		default:
			printf( "Cell[%d]:SEMeasurementCell::AddValue - measured value not stored\r ( type = %d, value = %f)!\n", CellId, eImtType, Value.Value );
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
int SEMeasurementCell::GetLastValue( int Tiepoint, eIMTBasTypes eImtType, int Index, SEValueInfo * Value )
{
	int RetVal = 0;
	
	memset( Value, 0, sizeof( SEValueInfo ) );
	//try
	//{
	switch(eImtType)
	{
		case IMT_Temp:
			*Value = TemperatureValues[Tiepoint - 1];
			break;
		case IMT_RiAC:
			switch (Index)
			{
			case 0:
				if (RiValues[Tiepoint - 1].empty() == false)
				{
					*Value = RiValues[Tiepoint - 1].at(RiValues[Tiepoint - 1].size() - 1);
				}
				break;

			case 1:
				*Value = RiValues1[Tiepoint - 1];
				break;

			case 2:
				*Value = RiValues2[Tiepoint - 1];
				break;

			case 101:
				*Value = RiPoint1[Tiepoint - 1];
				break;

			case 102:
				*Value = RiPoint2[Tiepoint - 1];
				break;

			}
			break;

		case IMT_IPu:
			switch (Index)
			{
			case 0:
				if (IpValues[Tiepoint - 1].empty() == false)
				{
					*Value = IpValues[Tiepoint - 1].at(IpValues[Tiepoint - 1].size() - 1);
				}
#ifdef _DEBUG
				if ((Value->Value * 1.0e6f) < 50.0 && abs(Value->Value * 1.0e6f) > 1.0f)
				{
					printf("Cell[%d]:Error: Ip<50: Value=%f\n", CellId, Value->Value * 1.0e6f);
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
			case 100:
				*Value = Ip450Average[Tiepoint - 1];
				break;
			case 101:
				*Value = IPuPoint1[Tiepoint - 1];
				break;
			case 102:
				*Value = IPuPoint2[Tiepoint - 1];
				break;
			}
			break;
		
		case IMT_RHh:
			switch (Index)
			{
			case 101:
				*Value = RhhPoint1[Tiepoint - 1];
				break;
			case 102:
				*Value = RhhPoint2[Tiepoint - 1];
				break;
			default:
				if (RhhValues[Tiepoint - 1].empty() == false)
				{
					*Value = RhhValues[Tiepoint - 1].at(RhhValues[Tiepoint - 1].size() - 1);
				}
				break;
			}
			break;
		
		case IMT_PH:
			switch (Index)
			{
			case 101:
				*Value = PhPoint1[Tiepoint - 1];
				break;
			case 102:
				*Value = PhPoint2[Tiepoint - 1];
				break;
			default:
				if (PhValues[Tiepoint - 1].empty() == false)
				{
					*Value = PhValues[Tiepoint - 1].at(PhValues[Tiepoint - 1].size() - 1);
				}
				break;
			}
		break;

		case IMT_UHS:
			*Value = UhValues[Tiepoint - 1];
			break;

		case IMT_UHmin:
			*Value = UhMinValues[Tiepoint - 1];
			break;
		case IMT_UHmax:
			*Value = UhMaxValues[Tiepoint - 1];
			break;

		case IMT_RHk:
			*Value = RhkValues[Tiepoint - 1];
			break;

		case IMT_EH:
			*Value = EhValues[Tiepoint - 1];
			break;

		case IMT_dtRiReg:
			*Value = RiRegValues[Tiepoint - 1];
			break;

		case IMT_dRHdt:
			*Value = RhDtValues[Tiepoint - 1];
			break;

		case IMT_dIHdt:
			*Value = IhDtValues[Tiepoint - 1];
			break;

		case IMT_HSCONT:
			*Value = HSContValues[Tiepoint - 1];
			break;

		case IMT_SSCONT:
			*Value = SSContValues[Tiepoint - 1];
			break;

		case IMT_IH:
			*Value = IhValues[Tiepoint - 1];
			break;

		case IMT_UAPE:
			switch( Index )
			{
				case 0:
				case 1:
					*Value = UapeValues[Tiepoint - 1];
					break;

				case 2:
					*Value = UapeValues2[Tiepoint - 1];
					break;
				case 100:
					*Value = UapeAverage[Tiepoint - 1];
					break; 
				case 101:
					*Value = UapePoint1[Tiepoint - 1];
					break;
				case 102:
					*Value = UapePoint2[Tiepoint - 1];
					break;
				default:
					;
					break;
			}

			break;

		case IMT_UN:
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

		case IMT_IpRE:
			switch (Index)
			{
			case 101:
				*Value = IpReValue1[Tiepoint - 1];
				break;
			case 102:
				*Value = IpReValue2[Tiepoint - 1];
				break;
			default:
				*Value = IpReValues[Tiepoint - 1];
				break;
			}
			break;

		case IMT_URE:
			switch (Index)
			{
			case 2:
				*Value = UReValues2[Tiepoint - 1];
				break;
			case 101:
				*Value = URePoint1[Tiepoint - 1];
				break;
			case 102:
				*Value = URePoint2[Tiepoint - 1];
				break;
			default:
				*Value = UReValues[Tiepoint - 1];
				break;
			}
			break;

		case IMT_IgRK:
			*Value = IgRkValues[Tiepoint - 1];
			break;

		case IMT_IL:
			*Value = IlValues[Tiepoint - 1];
			break;

		case IMT_RiDCn:
			*Value = RiDCnValues[Tiepoint - 1];
			break;

		case IMT_IPr:
			*Value = IprValues[Tiepoint - 1];
			break;
		case IMT_RiMin:
			*Value = RiMinValues[Tiepoint - 1];
			break;
		case IMT_RiMax:
			*Value = RiMaxValues[Tiepoint - 1];
			break;
		default:
			printf( "Cell[%d]:SEMeasurementCell::GetLastValue - requested value not present ( type = %d )!\n", CellId, eImtType );
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

	bMeasureIPu4Event = false;
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
	//gImtStartTime = GetActualSystemTimeMs();
	printf("##GetActualSystemTimeMs() = %lld, SequenceStartTime = %lld\n", GetActualSystemTimeMs(), SequenceStartTime);

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
		GetLastValue( i + 1, IMT_Temp, 0, &Value );
		if( _isnan( Value.Value )	== 0 )
		{
			if( Value.Value > 1.0f )
			{
				TemperatureSum += Value.Value;
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

	FuncRetVal = GetLastValue( Tiepoint, IMT_RiAC, 0, &Value );

	//[16.02.2023] RH: ProcessType was permanently set by the PLC to "ProcessType::MeasureTrimming"
	if( ProcessTypeLocal == ProcessType::MeasureSelection )
	{
		if( ( FuncRetVal == 0 ) &&
				( Value.Value >= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityLowerLimit ) ) &&
				( Value.Value <= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityUpperLimit ) ) )
		{
			
			RetVal = true;
		}
	}
	else
	// if( ProcessTypeLocal == 	ProcessType::Ip4MeasureUNernstControl ) // 10: IP4 measurement under Nernst voltage control  
	// if( ProcessTypeLocal == 	ProcessType::Ip4Measure2PointUp )		// 11: IP4 measurement via 2-point UP measurement
	{
		if( ( FuncRetVal == 0 ) &&
				( Value.Value >= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection/*HeatingRiTrimming*/ + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityLowerLimit)) &&
				( Value.Value <= ( ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection/*HeatingRiTrimming */ +ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset + ParamTrimmCell->ParamRiStability.RiStabilityUpperLimit)))
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

	FuncRetVal = GetLastValue(Tiepoint, IMT_HSCONT, 0, &Value );

	if( ( FuncRetVal == 0 ) &&
		( Value.ReceiveTimePC != 0 ) &&
		( Value.Value == 0.0 ) )
	{
		FuncRetVal = GetLastValue(Tiepoint, IMT_SSCONT, 0, &Value );

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
 *                               false : not all cards ready                                                              *
 *                               true : all cards ready                                                                   *
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
	SEValueInfo Value;	//measured values
	
	Value.ReceiveTime	= (__int64)tsMTime;			 //store receive time from ADVOV card [ms]
	Value.ReceiveTimePC = GetActualSystemTimeMs( );	 //store receive time [ms]		
	Value.Value = rMVal;	//store measured value
	
	AddValue(wPartIdx + 1, (eIMTBasTypes)eMainType , SubIndex, Value);

#if defined _INDUTRON_PRINT_MORE 
	if (wPartIdx == 0)
	{
		printf("##Cell[%d], PartIdx[%d]: IMT: %d, Value=%f, SubIdx: %d, ResultIdx %d\n", CellId, wPartIdx, eMainType, rMVal, SubIndex, lResIdx);
	}
#endif

	return 0;
}

bool SEMeasurementCell::GetEventIPu4Measurement(void)
{
	return bMeasureIPu4Event;
}

void SEMeasurementCell::SetEventIPu4Measurement(bool bEvent)
{
	bMeasureIPu4Event = bEvent;
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
	float HeatingEndTime = 0.0;		//heating end time
	float TestTime = 0.0;	//actual test time
	float LocalTime = 0.0;	//local time
	float TimeContChkSS = 0.0;
	float EffStartTime = 0.0;
	float MaxContSSChkTime = 0.0;

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );	//channel select
	TFlwTemperatureIn( LSTestHandle, TestTime, 800 );	//temperature measurement card

	TestTime = TestTime + 0.2f;

	//contact check heater on
	TFlwContChkHS( LSTestHandle, TestTime, 1 );
	TestTime = TestTime + 2.0f;

	
	//fast continue on
	if( Parameter.FastContinue == 0 )
	{
		TFlwHeaterVoltSet( LSTestHandle, TestTime, 1.0, 0.0 ); 	//set heater voltage to 1.0V, no ramp
		TestTime = TestTime + 2.8f;               				//add delay before RHk measurement
		TFlwHeaterColdResIn( LSTestHandle, TestTime, 0 );		//get heater cold resistance
		TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0, 0.0 );	//turn heater voltage off
		TestTime = TestTime + 0.2f;
	}

	EffStartTime = TestTime + 0.0f;																					//store effective start time

	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst );	//prepare cyclic measurement
	TFlwHeaterPowIn( LSTestHandle, TestTime, 900 );  //measure heater power
	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst ); //prepare cyclic measurement
	TFlwHeaterHotResIn( LSTestHandle, TestTime, 900 ); //measure heater hot resistance
		
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


int SETrimmingCell::GenerateTestSequence_IP450UNernstControl(TestSequenceParam Parameter)
{
	int iRet = 0;
	float fMeasTime = 0.0f;

	// channel select
	TFlwChSelect(LSTestHandle, fMeasTime, 0xFFFFF);		
	
	// temperature measurement card
	TFlwTemperatureIn(LSTestHandle, fMeasTime, 800);	
	fMeasTime += 1.0f;
	
	// Contaction test heater side
#if 0
	//TFlwContChkHS(LSTestHandle, fMeasTime, 0);
	fMeasTime += 2.0f;
#else
	int uFlags = 0;
	const float rQUHloLim = 0.5f;
	const float rQUHhiLim = 2.0f;
	const float rQHSloLim = 0.8f;
	const float rQHShiLim = 1.2f;

	TFlwHeaterColdResCompIn(LSTestHandle, fMeasTime, uFlags,
		Parameter.fRHkMin,//1.9f, //tTest.GetVal("TD%s.HeatColdRes.RHkMin"), 
		0.5f, //tTest.GetVal("TD%s.HeatColdRes.HeatEnergyMax"),
		Parameter.fHeatVoltMax,//1.0f, //tTest.GetVal("TD%s.HeatColdRes.HeatVoltMax"),
		20.0f,
		3.85f,//tTest.GetVal("TD%s.HeatColdRes.TempComp.Alpha"),
		rQUHloLim, rQUHhiLim, rQHSloLim, rQHShiLim,
		301, 302, 1, 930, 930, 930, 930); //heater cold measurement, 
	fMeasTime += 2.5f;
#endif
	// Target value 307 Ohm.
	float fRiTarget = Parameter.RiSetPoint; 

	// Trigger value for start Ri regulator  
	float fRiSnapHigh = Parameter.RiTrigger;

	// Leistung, mit der geheizt wird
	// wenn der Ri-Regler nach #fSnapInTime nicht funktioniert 
	float fMaxSnapInTime = Parameter.TimeoutForRiRegFailed; 

	// maximale Zeit bis Regler gestartet sein muss 
	float fPowerWhenFail = Parameter.PHwhenRiRegFailed;

	// Ri Regler mit 12V UH Regelung am Anfang bis RiAC unter #fRiSnapHigh
	// 12.0V
	float fUHmax = Parameter.RiRegMaxUH;
	// 7.0f
	float fKi = Parameter.RiRegKi;		 
	// 0.06f
	float fKp = Parameter.RiRegKp;		 
	// 0.0f
	float fKd = Parameter.RiRegKd;		
	// 0.1f
	float fTa = Parameter.Ta;
	// 0.5e-3f
	float fIac = Parameter.Amplitude; 
	// 3000.0f
	float fFreq = Parameter.Freq;
	// 50e-6f
	float fMeasDly = Parameter.Delay; 
	// 11.7f
	float fUhStart = Parameter.Heater[0].HeatVolt;

	float fStartTime = fMeasTime;
	TFlwRiPIDSnapRegSet3(LSTestHandle, fStartTime, RIREGFL_OLDRHK | RIREGFL_RegUH | RIREGFL_NOUHMAXLIM | RIREGFL_NOUHSTARTLIM | RIREGFL_RISNAPLASTUH, // Flags
		                 fUhStart, fPowerWhenFail, fMaxSnapInTime, 0.0f, fRiSnapHigh, fRiTarget, 0, 0, fIac, fFreq, fMeasDly, fUHmax, fKp, fKi, fKd, fTa);
	
	printf("#### fUHmax: %f\n", fUHmax);
	TFlwMeasSupvSet(LSTestHandle, fMeasTime, /*UH=1*/ 1, 0.0f, fUHmax);
	TFlwMaxHeaterVoltIn(LSTestHandle, fMeasTime, 0);

	// cyclic riac measurement
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 1.0f, -1);
	TFlwRiACInExt(LSTestHandle, fMeasTime, fIac/*0.5e-3f*/, fFreq/* 3000.0f*/, fMeasDly /*50e-6f*/, 950);

	//A: RiLowerLimit is negative in the type data
	float fRiSupvTime = fStartTime + Parameter.DelayRiMonitoring;
	TFlwMeasSupvSet(LSTestHandle, fRiSupvTime, MMWIDX_Ri, fRiTarget + Parameter.RiLowerLimit, fRiTarget + Parameter.RiUpperLimit);

	fMeasTime += 6.5f;
	// contacting test on sensor side after 6.5 sec.
	{
		// RiAC measurement for contacting test sensor side
		TFlwRiACInExt(LSTestHandle, fMeasTime, 0.5e-3f, 3000.0f, 50e-6f, 0);
		// pump voltage for contacting test sensor side
		// Pumpspannung auf erwarteten Endwert der Regelung einstellen
		TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.5f);
		fMeasTime += 0.5f;
		// pump current measurement for contacting test sensor side
		TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 0);
		// result of contacting  test sensor side
		// #KONTSEFL_DONTTURNOFF aktivieren, wenn wegen thermischem Gleichgewicht unter den SEs weitergeheizt werden soll
		TFlwContChkSSExt(LSTestHandle, fMeasTime, 0 | KONTSEFL_DONTTURNOFF /*weiterheizen, wenn Kontakt nio*/, 10.0f, 1200.0f, 10e-6f, 1);

	//	TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.9f);
		fMeasTime += 0.1f;
	}

	// IP,ref (20 ± 5)µA
	float fIPref = Parameter.PumpCurrentRE; 
	TFlwPumpCurrRESet(LSTestHandle, fMeasTime, fIPref);
	
	// UN,Target (450 ± 25)mV
	float fUNtarget = Parameter.NernstVoltADV; 

	// UP,max = (1795 ± 5)mV
	float fUPmax = Parameter.PumpVoltADV; 
 	
	//Mindest-Messverzögerung nach Stellgrößenausgabe
	float fMeasDelay = 30e-3f;

	float fUnRegTime = fStartTime + Parameter.UNRegDelay;
	
	// PI Regler für UN, stellt Spannung an APE solange nach, bis die Spannung an RE der Zielspannung entspricht
	TFlwUNPIRegSet(LSTestHandle, fUnRegTime, fUNtarget, fUPmax, -0.6f , -0.14f , fMeasDelay, 100e-3f);//

	// cyclic measuremants  
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 0.5f, -1);
	TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 910);

	// "LSU5.2S_selection_MAE_sensor_operation_onepager.pdf" gibt es eine tStart in den Typdaten

	DurIp4Meas =  (long long)(Parameter.AvgTime * 1000); //ms (1 Sekunde)

	// Event erst starten, wenn UN-Regelung 
	fMeasTime = max(fRiSupvTime, fMeasTime);
	fMeasTime = max(fUnRegTime, fMeasTime);
	
	float ftTimeOut = fMeasTime + 120.0f;
	float ftEnd = ftTimeOut + Parameter.AvgTime;

	// Event: Event trigger, when partial pressure reached
	TFlwExtSyncIn(LSTestHandle, fMeasTime, 0x0001, ftTimeOut, 0);
			
	// IAPE und UAPE ab ftStart bis ftEnd mit 10 Hz messen
	TFlwMeasAverage2(LSTestHandle, ftTimeOut, 0, IMT_IPu, IMT_UAPE, Parameter.AvgTime, 10.0f, 100, 100);

	// measure Ipref
	TFlwPumpCurrREIn(LSTestHandle, ftEnd, 101);

	// measure heater voltage for plausibility check
	TFlwHeaterVoltIn(LSTestHandle, ftEnd, 0);
	
	// measure Nernst voltage for plausibility check
	TFlwPumpVoltREIn(LSTestHandle, ftEnd, 100);

	// heater power measurement enabled via type data
	if ( Parameter.bEnablePh )
	{
		//measure heater power
		TFlwHeaterPowIn(LSTestHandle, ftEnd, 101);    
	}

	// heater hot resistance enabled via type data
	if ( Parameter.bEnableRhh )
	{ 
		//measure heater hot resistance
		TFlwHeaterHotResIn(LSTestHandle, ftEnd, 101);    
	}

	// read the results of the riac monitoring
	TFlwMeasSupvIn(LSTestHandle, ftEnd, MMWIDX_Ri, 4, 5, 6);

	// measure heater voltage at the end of the test sequence
	TFlwMaxHeaterVoltIn(LSTestHandle, ftEnd, 100);

	// turn off heater voltage
	TFlwHeaterVoltSet(LSTestHandle, ftEnd, 0.0f, 0.0f);
	
	// Übernehmen für Timeoutüberwachung 
	fMeasTime = ftEnd;

	return (iRet);
}


int SETrimmingCell::GenerateTestSequence_IP450TwoPointMeasUp( TestSequenceParam Parameter )
{
	int iRet = 0;
	float fMeasTime = 0.0f;
	//channel select
	TFlwChSelect(LSTestHandle, fMeasTime, 0xFFFFF);		

	//temperature measurement card
	TFlwTemperatureIn(LSTestHandle, fMeasTime, 800);	
	fMeasTime += 1.0f;

#if 0
	// Kontaktierüberprüfung heizerseitig
	TFlwContChkHS(LSTestHandle, fMeasTime, 0);
	fMeasTime += 2.0f;
#else

	int uFlags = 0;
	const float rQUHloLim = 0.5f;
	const float rQUHhiLim = 2.0f;
	const float rQHSloLim = 0.8f;
	const float rQHShiLim = 1.2f;

	TFlwHeaterColdResCompIn(LSTestHandle, fMeasTime, uFlags,
		Parameter.fRHkMin,//1.9f, //tTest.GetVal("TD%s.HeatColdRes.RHkMin"), 
		0.5f,			  //tTest.GetVal("TD%s.HeatColdRes.HeatEnergyMax"),
		Parameter.fHeatVoltMax,//1.0f, //tTest.GetVal("TD%s.HeatColdRes.HeatVoltMax"),
		20.0f,
		3.85f, //tTest.GetVal("TD%s.HeatColdRes.TempComp.Alpha"),
		rQUHloLim, rQUHhiLim, rQHSloLim, rQHShiLim,
		301, 302, 1, 930, 930, 930, 930); //heater cold measurement, 
	fMeasTime += 2.5f;

#endif

	// "6.1 Bedingungen bei der Funktionsprüfung" :
	// Ri,Target (307 ± 4) Ohm
//	float fRiACTolerance = 4.0f;
	float fRiTarget = Parameter.RiSetPoint;  //307.0f; //Kp 0,06 Ki: 7 Kd 0 Ta 0,1s bei Zielwert 307 Ohm.
	float fRiSnapHigh = Parameter.RiTrigger; //325.0f;
	/* Leistung, mit der geheizt wird, wenn der Ri-Regler nach #fSnapInTime nicht funktioniert */
	float fMaxSnapInTime = Parameter.TimeoutForRiRegFailed;// 10.0f;
	// maximum time until ri controller must be started
	float fPowerWhenFail = Parameter.PHwhenRiRegFailed;// 11.0f;
	// Ri Regler mit 12V UH Regelung am Anfang bis RiAC unter #fRiSnapHigh
	// Messung Ri mit 158 µA statt bisher mit 500µA
	float fUHmax = Parameter.RiRegMaxUH; //12.0V
	float fKi    = Parameter.RiRegKi;	 // 7.0f
	float fKp    = Parameter.RiRegKp;	 // 0.06f
	float fKd    = Parameter.RiRegKd;	 //  0.0f
	float fTa    = Parameter.Ta;	//0.1f;
	float fIac   = Parameter.Amplitude; // 0.5e-3f;
	float fFreq  = Parameter.Freq;  //3000.0f;
	float fMeasDly = Parameter.Delay;  // 50e-6f;
	float fUhStart = Parameter.Heater[0].HeatVolt; //11.7f; 

	float fStartTime = fMeasTime;
	TFlwRiPIDSnapRegSet3(LSTestHandle, fMeasTime, RIREGFL_OLDRHK | RIREGFL_RegUH | RIREGFL_NOUHMAXLIM | RIREGFL_NOUHSTARTLIM | RIREGFL_RISNAPLASTUH/* Flags */,
		fUhStart,
		fPowerWhenFail,
		fMaxSnapInTime,
		0.0f, fRiSnapHigh, fRiTarget, 0, 0, fIac, fFreq, fMeasDly, fUHmax,
		fKp, fKi, fKd, fTa);
	
	printf("#### fUHmax: %f\n", fUHmax);
	//Max. heater voltage monitoring 
	TFlwMeasSupvSet(LSTestHandle, fMeasTime, /*UH=1*/ 1, 0.0f, 12.0f);
	//Reset UHmax value
	TFlwMaxHeaterVoltIn(LSTestHandle, fMeasTime, 0);

	// RiAC zyklisch für Anzeige
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 1.0f, -1);
	TFlwRiACInExt(LSTestHandle, fMeasTime, 0.5e-3f, 3000.0f, 50e-6f, 950);

	//A: RiLowerLimit is negative in the type data
	float fRiSupvTime = fStartTime + Parameter.DelayRiMonitoring;
	TFlwMeasSupvSet(LSTestHandle, fRiSupvTime, MMWIDX_Ri, fRiTarget + Parameter.RiLowerLimit, fRiTarget + Parameter.RiUpperLimit);

	//Contacting test on sensore side:
	{
		fMeasTime += 6.5f;
		// RiAC measurement for contacting test on sensore side
		TFlwRiACInExt(LSTestHandle, fMeasTime, 0.5e-3f, 3000.0f, 50e-6f, 0);
		// Pumpspannung für Kontaktierüberprüfung sensorseitig
		TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.5f);
		fMeasTime += 0.5f;
		// Pumpstrommessung für Kontaktierüberprüfung sensorseitig
		TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 0);
		// Kontaktierüberprüfung sensorseitig abfragen.
		// #KONTSEFL_DONTTURNOFF aktivieren, wenn wegen thermischem Gleichgewicht weitergeheizt werden soll
		TFlwContChkSSExt(LSTestHandle, fMeasTime, 0 | KONTSEFL_DONTTURNOFF /*weiterheizen, wenn Kontakt nio*/, 10.0f, 1200.0f, 10e-6f, 1);
		// Pumpspannung ausschalten
		//TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.0f);
		fMeasTime += 0.1f;
	}

	// adjust pump current
	// IP,ref (20 ± 5)µA
	float fIPref = Parameter.PumpCurrentRE;
	TFlwPumpCurrRESet(LSTestHandle, fMeasTime, fIPref);

	// set UAPE target for measurement 1
	TFlwPumpVoltAPESet(LSTestHandle, fStartTime + Parameter.fTimeStartUp1, Parameter.fUp1Target);

	// measure average of UPu and UNernst (Point 1)
	// cyclic measuremants  
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 0.5f, -1);
	TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 910);

	float ftTimeOut = 120.0f;
	
	//Event: Trigger event, when partial pressure reached
	fMeasTime = max(fRiSupvTime, fMeasTime);
	TFlwExtSyncIn(LSTestHandle, fMeasTime, 0x0001, ftTimeOut, 0);
		
	// Bei 25s Messung machen, eigentlich müsste man laut FVPV warten, bis alle SEs stabil sind, laut 
	// "LSU5.2S_selection_MAE_sensor_operation_onepager.pdf" gibt es t1Start und t2Start in den Typdaten
	float ft1Start = ftTimeOut + fMeasTime;
	
	float fIntegartionTime = Parameter.fIntTimeMeasPoint1;
	
	TFlwMeasAverage2(LSTestHandle, ft1Start, 0, IMT_IPu, IMT_URE, fIntegartionTime, 10.0f, 101, 101);
	
	//measure IPRE
	TFlwPumpCurrREIn(LSTestHandle, ft1Start + fIntegartionTime, 101);

	// measure UAPE (Point 1)
	TFlwPumpVoltAPEIn(LSTestHandle, ft1Start + fIntegartionTime, 101);

	// measure heater hot resistance  (Point 1)
	TFlwHeaterHotResIn(LSTestHandle, ft1Start + fIntegartionTime, 101);

	// measure heater power  (Point 1)
	TFlwHeaterPowIn(LSTestHandle, ft1Start + fIntegartionTime, 101);

	// measure riac  (Point 1)
	TFlwRiACInExt(LSTestHandle, ft1Start + fIntegartionTime, 0.5e-3f, 3000.0f, 50e-6f, 101);

	// set UAPE target for measurement 2
	TFlwPumpVoltAPESet(LSTestHandle, ft1Start + fIntegartionTime, Parameter.fUp2Target);

	// Ip ab t2Start bis ft2End messen
	float fWaitBeforeMeas = 2;
	float ft2Start = ft1Start + fIntegartionTime + fWaitBeforeMeas;
	
	// measure average of UPu and UNernst (Point 2)
	TFlwMeasAverage2(LSTestHandle, ft2Start + 1.0f, 0, IMT_IPu, IMT_URE, 1.0f, 10.0f, 102, 102);
	
	// APE Spannung messen für Plausibiltätsprüfung
	float ftEnd = ft2Start + 2.0f;

	// measure IPRE
	TFlwPumpCurrREIn(LSTestHandle, ftEnd, 102);

	// measure UAPE (Point 2)
	TFlwPumpVoltAPEIn(LSTestHandle, ftEnd, 102);

	// measure heater hot resistance  (Point 2)
	TFlwHeaterHotResIn(LSTestHandle, ftEnd, 102);

	// measure heater power  (Point 2)
	TFlwHeaterPowIn(LSTestHandle, ftEnd, 102);

	// measure riac  (Point 2)
	TFlwRiACInExt(LSTestHandle, ftEnd, 0.5e-3f, 3000.0f, 50e-6f, 102);

	// get RiACmin, RiACmax, SnapInTime
	TFlwMeasSupvIn(LSTestHandle, ftEnd, MMWIDX_Ri, 6, 7, 8);

	//measure heater voltage max
	TFlwMaxHeaterVoltIn(LSTestHandle, ftEnd, 100);

	//measure heater voltag
	TFlwHeaterVoltIn(LSTestHandle, ftEnd, 9);
	
	DurIp4Meas = (__int64)(ftEnd - ft1Start) * 1000; //ms (1 Sekunde)
	// Übernehmen für Timeoutüberwachung 
	fMeasTime = ftEnd;
	return (iRet);
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
	float TestTime = 0.0;	//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );	//channel select

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
	float TestTime = 0.0;		//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );	//channel select

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
	float TestTime = 0.0;			//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );	//channel select

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
	float TestTime = 0.0;	//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );	//channel select

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
	float TestTime = 0.0;			//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );	//channel select

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
	float TestTime = 0.0;	//actual test time

	TFlwChSelect( LSTestHandle, TestTime, 0xFFFFF );//channel select

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
	FuncRetVal = MCxInitRL(	 LSTestHandle, 
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


int SETrimmingCell::PerformCalib( void )
{
	int iRetVal = 0;	//return value
	int iFuncRetVal = 0;
	int iCount = 0;

	do
	{
		iFuncRetVal = MCxCalib( LSTestHandle, CellId );
		iCount++;
		iCount++;
	} while ( iFuncRetVal || (iCount >10 ) );

	return iRetVal;
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
		SEMeasurementCell::hThreadMeasInMeasurementCellChamber[CellId - 1] = CreateThread( NULL,	0,      	CallbackAddressCell,             
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

TestSequenceParam SEMeasurementCell::PrepParameter(void)
{
	TestSequenceParam Parameter;	//test sequence parameter

			//copy input parameter to sequence
	Parameter.PumpCurrentRE = ParamTrimmCell->ParamMainTrimmingCell.IpRef * 1e-6f;

	Parameter.MinPumpCurrentRE = ParamTrimmCell->ParamMainTrimmingCell.fIpRefRngMin * 1e-6f;

	Parameter.MaxPumpCurrentRE = ParamTrimmCell->ParamMainTrimmingCell.fIpRefRngMax * 1e-6f;

	Parameter.PumpVoltADV = ParamTrimmCell->ParamMainTrimmingCell.PumpVoltage;

	Parameter.RiSetPoint = ParamTrimmCell->ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmCell->ParamMainTrimmingCell.HeatingRiOffset;
	
	Parameter.RiLowerLimit = ParamTrimmCell->ParamRiStability.RiStabilityLowerLimit;

	Parameter.RiUpperLimit = ParamTrimmCell->ParamRiStability.RiStabilityUpperLimit;

	Parameter.DelayRiMonitoring = ParamTrimmCell->ParamMainTrimmingCell.HeatingTime;

	Parameter.UNRegDelay = ParamTrimmCell->ParamMainTrimmingCell.StartDelay;

	Parameter.NernstVoltADV = ParamTrimmCell->ParamMainTrimmingCell.NernstVoltage;

	Parameter.MinUNernst = ParamTrimmCell->ParamMainTrimmingCell.fMinNernstVoltage;

	Parameter.MaxUNernst = ParamTrimmCell->ParamMainTrimmingCell.fMaxNernstVoltage;

	Parameter.Freq = ParamTrimmCell->ParamRiRegulaton.Freq;

	Parameter.Amplitude = ParamTrimmCell->ParamRiRegulaton.Amplitude * 1e-6f;

	Parameter.Ta = ParamTrimmCell->ParamRiRegulaton.Ta * 1e-3f;

	Parameter.Delay = ParamTrimmCell->ParamRiRegulaton.Delay * 1e-6f;;

	Parameter.AvgTime = ParamTrimmCell->ParamMainTrimmingCell.IntegrationTime;

	//			Parameter.FastContinue = 0.0;
	//			Parameter.LowPassConst = 1.0f;
	//			Parameter.Heater[0].CooldownTime = 0.0;
	//			Parameter.Heater[0].HeatPow = 0.0;

	Parameter.RiTrigger = ParamTrimmCell->ParamMainTrimmingCell.RiTrigger;

	Parameter.DeltaTIHMeas = ParamTrimmCell->ParamMainTrimmingCell.DeltaTihMeas * 1e-3f;

	Parameter.RiRegMaxUH = ParamTrimmCell->ParamRiRegulaton.RiRegMaxUH;
					
	Parameter.RiRegKp = ParamTrimmCell->ParamRiRegulaton.RiRegKp;

	Parameter.RiRegKi = ParamTrimmCell->ParamRiRegulaton.RiRegKi;

	Parameter.RiRegKd = ParamTrimmCell->ParamRiRegulaton.RiRegKd;

	Parameter.PHwhenRiRegFailed = ParamTrimmCell->ParamMainTrimmingCell.PhDisturbedRi;

	Parameter.TimeoutForRiRegFailed = ParamTrimmCell->ParamMainTrimmingCell.TimeoutDisturbedRi;

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

	Parameter.bEnableRhh = ( ParamTrimmCell->ParamIpMeasurement.EvaluateRhh == 1 );
	Parameter.bEnablePh =  ( ParamTrimmCell->ParamIpMeasurement.EvaluatePh == 1 );

	if (ParamTrimmCell->ParamMainTrimmingCell.MeasFLOEnable == FALSE && 0)
	{
		Parameter.MeasureFLODelay = 10;
		Parameter.MeasureFLO = 0.0f;
	}
	else
	{
		Parameter.MeasureFLODelay = 10;
		Parameter.MeasureFLO = 1.0f;
	}


	Parameter.fRHkMin = ParamTrimmCell->ParamMainTrimmingCell.fSeRHkMin;
	Parameter.fHeatVoltMax = ParamTrimmCell->ParamMainTrimmingCell.fRHkVMax;

	Parameter.fTimeStartUp1 = ParamTrimmCell->ParamMainTrimmingCell.fTimeStartUp1;				//APE pump voltage start time point of measurement 1
	Parameter.fUp1Target = ParamTrimmCell->ParamMainTrimmingCell.fUp1Target;
	Parameter.fIntTimeMeasPoint1 = ParamTrimmCell->ParamMainTrimmingCell.fIntTimeMeasPoint1;
	Parameter.fUp2Target = ParamTrimmCell->ParamMainTrimmingCell.fUp2Target;
	Parameter.fWaitingTimeIp2Un2Meas = ParamTrimmCell->ParamMainTrimmingCell.fWaitingTimeIp2Un2Meas;
	Parameter.fIntTimeMeasPoint2 = ParamTrimmCell->ParamMainTrimmingCell.fIntTimeMeasPoint2;

	return (Parameter);
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
	int RetVal = 0;	//return value
	TestSequenceParam Parameter;	//test sequence parameter

	ProcessTypeLocal = Type;
	
	RetVal = TFlwNew( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );	//initiate new sequence
	printf( "Cell[%d]:TFlwNew=%d\n", CellId, RetVal );
	
	switch( Type )
	{
		case ProcessType::MeasureTrimming:
		case ProcessType::MeasureSelection:
			//copy input parameter to sequence
			Parameter.PumpCurrentRE = ParamTrimmCell->ParamMainTrimmingCell.IpRef * 1e-6f;
			Parameter.PumpVoltADV = ParamTrimmCell->ParamMainTrimmingCell.PumpVoltage;
			if( ProcessTypeLocal == ProcessType::MeasureSelection )
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
	
			GenerateTestSequence( Parameter );	//generate sequence function calls for trimming cell
			break;		

		case ProcessType::MeasurePositionNormal:
			GeneratePositionNormalSequence( );	//generate sequence function calls for position normal
			break;

		case ProcessType::MeasureHeaterNormal:
			GenerateHeaterNormalSequence( );	//generate sequence function calls for heater normal
			break;
				
		case ProcessType::MeasureUniversalNormal:
			GenerateUniversalNormalSequence( );	 //generate sequence function calls for universal normal
			break;

		case ProcessType::MeasureUnNormal:
			GenerateUnNormalSequence( );	//generate sequence function calls for un normal
			break;

		case ProcessType::MeasureIpNormal:
			GenerateIpNormalSequence( );	//generate sequence function calls for ip normal
			break;

		case ProcessType::MeasureIlmNormal:
			GenerateIlmNormalSequence( );	//generate sequence function calls for ilm normal
			break;

		case ProcessType::Cooling:
			GenerateCoolingSequence( );	   //generate sequence function calls for cooling
			break;

		// IP4 measurement under Nernst voltage control  
		case ProcessType::IP450MeasUNernstControl:
			GenerateTestSequence_IP450UNernstControl( PrepParameter() );	//generate sequence function calls for trimming cell (Nernst voltage control)
			break;

		// IP4 measurement via 2-point UP measuremant
		case ProcessType::IP450Meas2PointUp:		 
			GenerateTestSequence_IP450TwoPointMeasUp( PrepParameter() );	//generate sequence function calls for trimming cell (2-point measurement)
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

	//generate sequence
	RetVal = TFlwGenerate( LSTestHandle );	
	printf("Cell[%d]:TFlwGenerate=%d\n", CellId, RetVal);

	//transmitt sequence to ADVOV cards
	RetVal = MCxTFlowTx( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );	
	printf("Cell[%d]:MCxTFlowTx=%d\n", CellId, RetVal);
	
	for( int i = 0; i < 100; i++ )
	{
		//get last error
		RetVal = MCxLastError( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );	
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
	TFlwHeaterPowIn( LSTestHandle, TestTime, 900 );     //measure heater power

	TFlwCycMeasPrefix2( LSTestHandle, 0, 1.0, -1, Parameter.LowPassConst ); //prepare cyclic measurement
	TFlwHeaterHotResIn( LSTestHandle, TestTime, 900);     //measure heater hot resistance

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
	TestTime = max( TestTime, LocalTime );																	//set test time to max time

	//negative pump voltage pulse
	if( Parameter.NegPulseTime > 0.0 ) 
	{
		LocalTime = TestTime + 10.0f;	//set start time of negative pulse
		TFlwPumpVoltAPESet( LSTestHandle, LocalTime, Parameter.NegPulseVolt ); //turn pump voltage ape on
		LocalTime = LocalTime + Parameter.NegPulseTime;	//set end time of negative pulse
		TFlwPumpVoltAPESet( LSTestHandle, LocalTime, 0.0 );	//turn pump voltage ape off
		TestTime = max( TestTime, LocalTime); //set test time to max time
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
	TFlwCycMeasPrefix( LSTestHandle, 128, 1.0, -1 ); 	//prepare cyclic measurement
	TFlwPumpCurrAPEIn( LSTestHandle, TestTime, 911 );	//measure pump current ape

	TestTime = max( TestTime, HeatingEndTime );															//set test time to max time
	TFlwHeaterVoltSet( LSTestHandle, TestTime, 0.0, 0.0 );									//turn heater voltage off

	return 0;
}


//Testsequence for ADV reference probe:
// E-Mail Cizek Petr (PS-SU/ESU4-Bj) IP4 selection 2.03.2023
// Nernst voltage target: 450 mV; RiAC target: 300R 
int SEReferenceCell::GenerateTestSequence_IP450UNernstControl(TestSequenceParam Parameter)
{
	int iRet = 0;
	float fMeasTime = 0.0f;
	//-- sequence start --
	TFlwChSelect(LSTestHandle, fMeasTime, 0x00003);	//channel select
	TFlwTemperatureIn(LSTestHandle, fMeasTime, 800);	//temperature measurement card
	fMeasTime += 1.0f;
	
	// Kontaktierüberprüfung heizerseitig
	TFlwContChkHS(LSTestHandle, fMeasTime, 0);
	fMeasTime += 2.0f;

	// "6.1 Bedingungen bei der Funktionsprüfung" :
	// Ri,Target (307 ± 4) Ohm
	float fRiACTolerance = 4.0f;
	float fRiTarget = Parameter.RiSetPoint; //Kp 0,06 Ki: 7 Kd 0 Ta 0,1s bei Zielwert 300 Ohm.
	float fRiSnapHigh = 325.0f;
	/* Leistung, mit der geheizt wird, wenn der Ri-Regler nach #fSnapInTime nicht funktioniert */
	float fMaxSnapInTime = Parameter.Heater[0].HeatTime;
	/* maximale Zeit bis Regler gestartet sein muss */
	float fPowerWhenFail = Parameter.Heater[1].HeatPow;
	// Ri Regler mit 12V UH Regelung am Anfang bis RiAC unter #fRiSnapHigh
	// Messung Ri mit 158 µA statt bisher mit 500µA
	
	float fStartTime = fMeasTime;
	TFlwRiPIDSnapRegSet3(LSTestHandle, fMeasTime, RIREGFL_OLDRHK | RIREGFL_RegUH | RIREGFL_NOUHMAXLIM | RIREGFL_NOUHSTARTLIM | RIREGFL_RISNAPLASTUH/* Flags */,
		Parameter.Heater[0].HeatVolt,
		fPowerWhenFail,
		fMaxSnapInTime,
		0.0f, fRiSnapHigh, fRiTarget, 0, 0, 0.5e-3f, 3000.0f, 50e-6f, 12.0f,
		0.06f, 7.0f, 0.0f, 0.1f);

	// RiAC Überwachung auf +- #fRiACTolerance Ohm ab Start + #fStartRiSupv 
		//A: RiLowerLimit is negative in the type data
	float fRiSupvTime = fStartTime + Parameter.DelayRiMonitoring;
	TFlwMeasSupvSet(LSTestHandle, fRiSupvTime, MMWIDX_Ri, fRiTarget - fRiACTolerance, fRiTarget + fRiACTolerance);

	// Nach 6,5 s sensorseitige Kontaktierüberprüfung
	fMeasTime += 6.5f;
	// RiAC für Kontaktierüberprüfung sensorseitig
	TFlwRiACInExt(LSTestHandle, fMeasTime, 0.5e-3f, 3000.0f, 50e-6f, 0);
	// Pumpspannung für Kontaktierüberprüfung sensorseitig
	// Pumpspannung auf erwarteten Endwert der Regelung einstellen
	TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.9f);
	fMeasTime += 0.5f;
	// Pumpstrommessung für Kontaktierüberprüfung sensorseitig
	TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 0);

	// Kontaktierüberprüfung sensorseitig abfragen.
	// #KONTSEFL_DONTTURNOFF aktivieren, wenn wegen thermischem Gleichgewicht unter den SEs weitergeheizt werden soll
	TFlwContChkSSExt(LSTestHandle, fMeasTime, 0 | KONTSEFL_DONTTURNOFF /*weiterheizen, wenn Kontakt nio*/, 10.0f, 1200.0f, 10e-6f, 1);


	//	TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.9f);
	fMeasTime += 0.1f;

	// Pumpstrom einstellen
	// "6.1 Bedingungen bei der Funktionsprüfung" :
	// IP,ref (20 ± 5)µA
	float fIpRE = Parameter.PumpCurrentRE;
	TFlwPumpCurrRESet(LSTestHandle, fMeasTime, fIpRE);

	// UN,Target (450 ± 25)mV
	float fUNtarget = Parameter.NernstVoltADV;//0.450f;

	// UP,max = (1795 ± 5)mV
	float fUPmax = Parameter.PumpVoltADV;//1795.0e-3f;
	float fMeasDelay = 30e-3f; //Mindest-Messverzögerung nach Stellgrößenausgabe

	// PI Regler für UN, stellt Spannung an APE solange nach, bis die Spannung an RE der Zielspannung entspricht
	TFlwUNPIRegSet(LSTestHandle, fStartTime + Parameter.UNRegDelay, fUNtarget, fUPmax, -0.6f /*-1.0f*/, -0.14f /*-0.28f*/, fMeasDelay, 100e-3f /*50e-3f*/);//

	// Durch zyklische Messung Ip Werte für Stabilitätsüberprüfung bereitstellen, 
	// Punkt 12 in "7.2 Prozessablauf im Automatikbetrieb"
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 0.5f, -1);
	TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 910);


	// Bei 25s Messung machen, eigentlich müsste man laut FVPV warten, bis alle SEs stabil sind, laut 
	// "LSU5.2S_selection_MAE_sensor_operation_onepager.pdf" gibt es eine tStart in den Typdaten
	float ftTimeOut = 120.0f;
	float ftEnd = ftTimeOut;

	float ftExtSync = max(fMeasTime, fStartTime + Parameter.UNRegDelay);


	//Event: Event trigger, when partial pressure reached
	TFlwExtSyncIn(LSTestHandle, ftExtSync, 0x0001, ftTimeOut, 0);

#if 0
	// IAPE und UAPE ab ftStart bis ftEnd mit 10 Hz messen
	TFlwMeasAverage2(LSTestHandle, ftTimeOut, 0, IMT_IPu, IMT_UAPE, ftEnd - ftTimeOut, 10.0f, 100, 100);
#endif

	// Ergebnis RiAC Überwachung auslesen für Plausibilitätsprüfung
	TFlwMeasSupvIn(LSTestHandle, ftEnd, MMWIDX_Ri, 4, 5, 6);
	// Heizerspannung messen für Plausibilitätsprüfung
	TFlwHeaterVoltIn(LSTestHandle, ftEnd, 7);
	// RE Spannung messen für Plausibiltätsprüfung
	TFlwPumpVoltREIn(LSTestHandle, ftEnd, 8);
	// Übernehmen für Timeoutüberwachung 
	fMeasTime = ftEnd;
	return (iRet);
}

//E-Mail Cizek Petr (PS-SU/ESU4-Bj) IP4 selection 2.03.2023
// always run the eference probe by Ri- and Nernst voltage conrol 
#if 0
int SEReferenceCell::GenerateTestSequence_Ip4TwoPointMeasUp(TestSequenceParam Parameter)
{
	int iRet = 0;
	float fMeasTime = 0.0f;

	//-- sequence start --
	TFlwChSelect(LSTestHandle, fMeasTime, 0x00003);	//channel select
	TFlwTemperatureIn(LSTestHandle, fMeasTime, 800);	//temperature measurement card	
	fMeasTime += 1.0f;
	
	// Kontaktierüberprüfung heizerseitig
	TFlwContChkHS(LSTestHandle, fMeasTime, 0);
	fMeasTime += 2.0f;

	float fStartRiSupv = 14.0f;
	// "6.1 Bedingungen bei der Funktionsprüfung" :
	// Ri,Target (307 ± 4) Ohm
	float fRiACTolerance = 4.0f;
	float fRiTarget = 307.0f; //Kp 0,06 Ki: 7 Kd 0 Ta 0,1s bei Zielwert 307 Ohm.
	float fRiSnapHigh = 325.0f;
	/* Leistung, mit der geheizt wird, wenn der Ri-Regler nach #fSnapInTime nicht funktioniert */
	float fMaxSnapInTime = 10.0f;
	/* maximale Zeit bis Regler gestartet sein muss */
	float fPowerWhenFail = 11.0f;
	// Ri Regler mit 12V UH Regelung am Anfang bis RiAC unter #fRiSnapHigh
	// Messung Ri mit 158 µA statt bisher mit 500µA
	TFlwRiPIDSnapRegSet3(LSTestHandle, fMeasTime, RIREGFL_OLDRHK | RIREGFL_RegUH | RIREGFL_NOUHMAXLIM | RIREGFL_NOUHSTARTLIM | RIREGFL_RISNAPLASTUH/* Flags */,
		12.0f,
		fPowerWhenFail,
		fMaxSnapInTime,
		0.0f, fRiSnapHigh, fRiTarget, 0, 0, /*0.158e-3f*/0.5e-3f, 3000.0f, 50e-6f, 12.0f,
		0.06f, 7.0f, 0.0f, 0.1f);

	// RiAC zyklisch für Anzeige
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 1.0f, -1);
	TFlwRiACInExt(LSTestHandle, fMeasTime, /*0.158e-3f*/0.5e-3f, 3000.0f, 50e-6f, 950);

	// RiAC Überwachung auf +- #fRiACTolerance Ohm ab Start + #fStartRiSupv 
	TFlwMeasSupvSet(LSTestHandle, fMeasTime + fStartRiSupv, MMWIDX_Ri, fRiTarget - fRiACTolerance, fRiTarget + fRiACTolerance);

	fMeasTime += 6.5f;
	// RiAC für Kontaktierüberprüfung sensorseitig
	TFlwRiACInExt(LSTestHandle, fMeasTime, /*0.158e-3f*/0.5e-3f, 3000.0f, 50e-6f, 0);
	// Pumpspannung für Kontaktierüberprüfung sensorseitig
	TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 1.2f);
	fMeasTime += 0.5f;
	// Pumpstrommessung für Kontaktierüberprüfung sensorseitig
	TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 0);
	// Kontaktierüberprüfung sensorseitig abfragen.
	// #KONTSEFL_DONTTURNOFF aktivieren, wenn wegen thermischem Gleichgewicht weitergeheizt werden soll
	TFlwContChkSSExt(LSTestHandle, fMeasTime, 0 | KONTSEFL_DONTTURNOFF /*weiterheizen, wenn Kontakt nio*/, 10.0f, 1200.0f, 10e-6f, 1);
	// Pumpspannung ausschalten
	TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.0f);

	fMeasTime += 0.1f;
	// Pumpstrom einstellen
	// "6.1 Bedingungen bei der Funktionsprüfung" :
	// IP,ref (20 ± 5)µA
	TFlwPumpCurrRESet(LSTestHandle, fMeasTime, 20.0e-6f);

	// Up1
	TFlwPumpVoltAPESet(LSTestHandle, fMeasTime, 0.760f);

	// Warten vor Messungsbeginn
	fMeasTime += 0.5f;

	// Ip Werte für Stabilitätsüberprüfung, Punkt 12 in "7.2 Prozessablauf im Automatikbetrieb"
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 0.5f, -1);
	TFlwPumpCurrAPEIn(LSTestHandle, fMeasTime, 920);

	// Zyklische Messwerte für Anzeige 
	TFlwCycMeasPrefix(LSTestHandle, 0x0, 1.0f, -1);
	TFlwPumpVoltAPEIn(LSTestHandle, fMeasTime, 930);

	TFlwCycMeasPrefix(LSTestHandle, 0x0, 1.0f, -1);
	TFlwPumpVoltREIn(LSTestHandle, fMeasTime, 940);

	// Bei 25s Messung machen, eigentlich müsste man laut FVPV warten, bis alle SEs stabil sind, laut 
	// "LSU5.2S_selection_MAE_sensor_operation_onepager.pdf" gibt es t1Start und t2Start in den Typdaten
	float ft1Start = 25.0f;
	float ft1End = ft1Start + 1.0f;

	// Mittelwert von Ip und UN
	TFlwMeasAverage2(LSTestHandle, ft1Start, 0, IMT_IPu, IMT_URE, ft1End - ft1Start, 10.0f, 2, 3);

	// Rückmessung UAPE
	TFlwPumpVoltAPEIn(LSTestHandle, ft1End, 1);

	// Up2 einstellen
	TFlwPumpVoltAPESet(LSTestHandle, ft1End, 0.940f);

	// Ip ab t2Start bis ft2End messen
	float ft2Start = ft1End + 2.0f;
	float ft2End = ft2Start + 1.0f;

	// Mittelwert von Ip und UN
	TFlwMeasAverage2(LSTestHandle, ft2Start, 0, IMT_IPu, IMT_URE, ft2End - ft2Start, 10.0f, 4, 5);

	float ftEnd = ft2End;

	// Ergebnis RiAC Überwachung auslesen für Plausibilitätsprüfung
	TFlwMeasSupvIn(LSTestHandle, ftEnd, MMWIDX_Ri, 6, 7, 8);

	// Heizerspannung messen für Plausibilitätsprüfung
	TFlwHeaterVoltIn(LSTestHandle, ftEnd, 9);

	// APE Spannung messen für Plausibiltätsprüfung
	TFlwPumpVoltAPEIn(LSTestHandle, ftEnd, 10);

	// Übernehmen für Timeoutüberwachung 
	fMeasTime = ftEnd;

	return (iRet);
}
#endif

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
int SEReferenceCell::GenerateAndTransmittSequence(ProcessType Type)
{
	int RetVal = 0;	//return value
	TestSequenceParam Parameter;	//test sequence parameter
	
	printf( "Generate Start: Handle=%p\n", LSTestHandle );
	RetVal = TFlwNew( LSTestHandle, ( LONG ) pow( 2.0f, CellId - 1 ) );	//initiate new sequence
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




	switch (Type)
	{
	case ProcessType::MeasureTrimmingSelection:	// = 0,	//measure, trimming and selection
	case ProcessType::MeasureTrimming:			// = 1,	//measure and trimming
		if (ParamRefCell->ParamMainReferenceCell.EnableNernstRegulation == FALSE)
		{
			Parameter.NernstVoltADV = 0.0;
		}

		if (ParamRefCell->ParamMainReferenceCell.HeatingMode == TRUE)
		{
			Parameter.FastContinue = 1.0f;
		}
		else
		{
			Parameter.FastContinue = 0.0;
		}
		GenerateTestSequence(Parameter);	//generate sequence function calls for reference cell
		break;

	case ProcessType::IP450MeasUNernstControl:	// = 10,  // IP4 measurement under Nernst voltage control  
	case ProcessType::IP450Meas2PointUp:		// = 11,  // IP4 measurement via 2-point UP measurement
	
		// generate test sequenc with Nernst voltage control for reference cell 
		// ADV reference probe
		GenerateTestSequence_IP450UNernstControl(Parameter);  
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


	RetVal = TFlwGenerate( LSTestHandle );	//generate sequence
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
 *                                -1 = parameter error																	   *
 *                                -2 = no valid value									                                   *
 *																														   *
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






