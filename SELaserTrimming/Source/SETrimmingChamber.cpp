/*-------------------------------------------------------------------------------------------------------------------------*
 *  modulname       : SEMeasurement.cpp                                                                                    *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                                     *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                                            *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  description:                                                                                                           *
 *  ------------                                                                                                           *
 *  This module contains control and evaluation routines for SE measurement.                                               *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                                         *
 * ---------|------------|----------------|------------------------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                                              *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
#include "Stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <share.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "..\Library\OpConServicesCommon.h"
#define _SE_TRIMMING_CHAMBER_INTERNAL
#include "SETrimmingChamber.h"
#undef _SE_TRIMMING_CHAMBER_INTERNAL
#if 0
#include "..\Library\SETestDLL.h"
#else
#include "..\Library\LSTestDLL.h"
#include "..\Library\MCDataEx.h"

#endif

SETrimmingRetain SETrimmingChamber::SETrimmingRetainData[CHAMBER_COUNT] = { 0, 0 };
//thread handle of each process chamber
HANDLE SETrimmingChamber::hThreadHandleProcessChamber[CHAMBER_COUNT] = { NULL, NULL };	
HANDLE SETrimmingChamber::ghMutex[CHAMBER_COUNT] = { NULL, NULL };


	/*-------------------------------------------------------------------------------------------------------------------------*
	 * function name        : void HandleProcess( void )                                                                       *
	 *                                                                                                                         *
	 * input:               : void                                                                                             *
	 *                                                                                                                         *
	 * output:              : void                                                                                             *
	 *                                                                                                                         *
	 * description:         : This is function to handle all chamber processes (trimming and normal).                          *
	 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::HandleProcess(void)
{

	bool ContactOk = true;
	bool RiStableOk = true;
	bool ContactRefOk = true;
	bool StabilityOk = true;
	SEValueInfo Value;
	SEValueInfo ValueU1;
	SEValueInfo ValueU2;
	SEValueInfo ValueI1;
	SEValueInfo ValueI2;

	float IpRefAct = 0.0f;
	
	PartStatus ePartStatus = Rework;
	if ((eProcessType == ProcessType::IP450Meas2PointUp) ||
		(eProcessType == ProcessType::IP450MeasUNernstControl))
	{
	// Götz Stefan, 03.05.2023: es soll keinen Teilestatus "Nacharbeit" geben. 
	// Nur:
	//  2: IoDiesel
	//	3: SelectGasoline 
		ePartStatus = SelectGasoline;
	}

	__int64	MinLastTime = 0;

#if defined _INDUTRON_PRINT_MORE 
	if (CurrProcessStep != ProcessStep)
	{
		printf( "##ProcessStep Old: %d -> New: %d\n", CurrProcessStep, ProcessStep);
		CurrProcessStep = ProcessStep;
	}
#endif

	switch (eProcessType)
	{

	case ProcessType::MeasureTrimming: // measure and trimming
	case ProcessType::MeasureSelection:// measure and selection
	case ProcessType::IP450MeasUNernstControl:
	case ProcessType::IP450Meas2PointUp:
	{
		switch (ProcessStep)
		{
		case EProcessStep::Initialization:
			//initialize trimming
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				TrimmingInitRaw(i + 1);

				if ((SETrimmingValues.SETrimmingValuesTiepoint[i].AssemblyDataTiepoint.SEPartStatus != NotProcessed) &&
					(SETrimmingValues.SETrimmingValuesTiepoint[i].AssemblyDataTiepoint.SEPartStatus != ReworkOnceAgain))
				{
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
				}

				if ((SETrimmingValues.AssemblyDataCommon.EvacuatedPressure > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.EvacuatedPressureAllowedMax))
				{
					SetPartState(i + 1, ePartStatus, EvacuatedPressureOutside);

					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
					ProcessStep = EProcessStep::TrimmingFinished;
				}
				if ((SETrimmingValues.AssemblyDataCommon.LeakageRate > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.LeackageRateAllowedMax))
				{
					SetPartState(i + 1, ePartStatus, LeackageRateOutside);

					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
					ProcessStep = EProcessStep::TrimmingFinished;
				}
			}
			ProcessStep = EProcessStep::Heating;
			break;
			//contact check trimming cell
		case EProcessStep::Heating:

			ChamberState = INF_WAIT_RI_STABILITY;

			ContactOk = true;
			RiStableOk = true;

			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ContactOk = TrimmCell->IsContactingOk(i + 1);

				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.RiStability = TrimmCell->IsRiStable(i + 1);

				ContactOk &= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ContactOk;

				RiStableOk &= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.RiStability;
			}

			if (((ContactOk == true) &&
				(RiStableOk == true)) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + (ParamTrimmChamber.ParamTrimmCell.ParamRiStability.RiStabilityDuration * 1000)))
			{
				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					TrimmCell->GetLastValue(i + 1, IMT_RiAC, 0, &Value);
#if 1
					printf("Chamber[%d]:HandleProcess:SE%d:ContactOk=%d\n", ChamberId, i + 1, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ContactOk);
					printf("Chamber[%d]:HandleProcess:SE%d:RiStable=%d;%f\n", ChamberId, i + 1, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.RiStability, Value.Value);
#endif
					if ((SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ContactOk == false) ||
						(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.RiStability == false))
					{
						SetPartState(i + 1, ePartStatus, ContactRiStabilityNok);
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
					}
				}
				ProcessStep = EProcessStep::MeasureIpRefCell1;
			}
			break;

		case EProcessStep::MeasureIpRefCell1:
		{	//check contact at reference cell and measure Ip reference zero correction
			ContactRefOk = true;

#ifndef REF_SIMULATION
			ContactRefOk = ReferenceCell->IsContactingOk(1) && ReferenceCell->IsContactingOk(2);
			printf("Chamber[%d]:HandleProcess:REF1:ContactOk=%d;REF2:ContactOk=%d\n", ChamberId, ReferenceCell->IsContactingOk(1), ReferenceCell->IsContactingOk(2));
#endif

			if ((ContactRefOk == true) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + 45000))
			{
				ReferenceCell->GetLastIpMean(1, 4, &IpRefCorrectionZero[0]);
				ReferenceCell->GetLastIpMean(2, 4, &IpRefCorrectionZero[1]);

				printf("Chamber[%d]:HandleProcess:REF1:Ip=%f\n", ChamberId, IpRefCorrectionZero[0] * 1.0e6f);
				printf("Chamber[%d]:HandleProcess:REF2:Ip=%f\n", ChamberId, IpRefCorrectionZero[1] * 1.0e6f);

				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ContactRefOk = ContactRefOk;
					if (SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ContactRefOk = false)
					{
						SetPartState(i + 1, ePartStatus, ContactRefNok);
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
					}
				}
				DigitalOut = HEATING_FINISHED_FLAG;
				printf("Chamber[%d]:HandleProcess:Set flag heating finished\n", ChamberId);
				ProcessStep = EProcessStep::WaitingpPrtialPressureReached;
			}
			break;
			}
		case EProcessStep::WaitingpPrtialPressureReached:
		{	
			//wait for flag
			if ((DigitalIn & PRESSURE_CONTROL_DONE) == PRESSURE_CONTROL_DONE)
			{
				//output Ip corrected
				SEValueInfo Value;
				TrimmCell->GetLastValue(1, IMT_IPu, 0, &Value);
				MinLastTime = _I64_MAX;
				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					TrimmCell->GetLastValue(i + 1, IMT_IPu, 0, &Value);
					printf("Chamber[%d]:HandleProcess:SE%d:Ipkorr=%f\n", ChamberId, i + 1, IpCorrection(Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) * 1.0e6f);
					if ((Value.ReceiveTime != 0) &&
						(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone == false))
					{
						MinLastTime = min(MinLastTime, Value.ReceiveTime);
					}
				}
				StabilityStartTime = MinLastTime;//GetActualSystemTimeMs( );


				DigitalOut = NONE_FLAG;
				printf("Chamber[%d]:HandleProcess:Reset flag heating finished\n", ChamberId);

				ProcessStep = EProcessStep::IPuStabilityCheck;

			}
			break;
		}
		case EProcessStep::IPuStabilityCheck:
		{
			//Ip stability check	
			//IgnoreNewPressure = true;
			ChamberState = INF_WAIT_IP_STABILITY;

			SEValueInfo Value;
			MinLastTime = _I64_MAX;
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				TrimmCell->GetLastValue(i + 1, IMT_IPu, 0, &Value);
				if ((Value.ReceiveTime != 0) &&
					(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone == false))
				{
					MinLastTime = min(MinLastTime, Value.ReceiveTime);
				}
			}

			switch (StabilityStep)
			{
			case 0:
				if (( /*GetActualSystemTimeMs( )*/MinLastTime + 50.0f >=
					StabilityStartTime + ((ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.PauseTime +
						ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.StartDelay +
						ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingTime) * 1000.0f)) ||
					(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + (ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.ProcessTimeout * 1000)))
				{
					printf("Chamber[%d]:HandleProcess:Stabilitycheck 1\n", ChamberId);
					for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
					{
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability = IpStabilityCheck(i + 1, 1);
					}
					StabilityStartTime = /*GetActualSystemTimeMs( )*/MinLastTime;
					StabilityStep++;

				}
				break;

			case 1:
				if (( /*GetActualSystemTimeMs( )*/MinLastTime + 50.0f >=
					StabilityStartTime + ((ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpStableTime1) * 1000.0f)))
				{
					printf("Chamber[%d]:HandleProcess:Stabilitycheck 2\n", ChamberId);
					StabilityOk = true;
					for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
					{
						if (SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability == false)
						{
							SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability = IpStabilityCheck(i + 1, 2);
							StabilityOk &= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability;
							printf("Chamber[%d]:HandleProcess:Stabilitycheck 2:SE%d:OK=%d\n", ChamberId, i + 1, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability);
						}
					}
					if (StabilityOk == true)
					{
						StabilityStep = 0;
						StabilityStartTime = 0;
						ProcessStep = EProcessStep::GasControl;
					}
					else
					{
						StabilityStartTime = /*GetActualSystemTimeMs( )*/MinLastTime;
						StabilityStep++;
					}
				}
				break;

			case 2:
				if (( /*GetActualSystemTimeMs( )*/MinLastTime + 50.0f >=
					StabilityStartTime + ((ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpStableTime2) * 1000.0f)))
				{
					printf("Chamber[%d]:HandleProcess:Stabilitycheck 3\n", ChamberId);
					StabilityOk = true;
					for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
					{
						if (SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability == false)
						{
							SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability = IpStabilityCheck(i + 1, 3);
							StabilityOk &= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability;
							printf("Chamber[%d]:HandleProcess:Stabilitycheck 3:SE%d:OK=%d\n", ChamberId, i + 1, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability);
						}
					}
					if (StabilityOk == true)
					{
						StabilityStep = 0;
						StabilityStartTime = 0;
						ProcessStep = EProcessStep::GasControl;
					}
					else
					{
						StabilityStartTime = /*GetActualSystemTimeMs( )*/MinLastTime;
						StabilityStep++;
					}
				}
				break;

			case 3:
				if (( /*GetActualSystemTimeMs( )*/MinLastTime + 50.0f >=
					StabilityStartTime + ( (ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpStableTime3) * 1000.0f) ) )
				{
					printf("Chamber[%d]:HandleProcess:Stabilitycheck 4\n", ChamberId);
					StabilityOk = true;
					for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
					{
						if (SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability == false)
						{
							SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability = IpStabilityCheck(i + 1, 4);
							StabilityOk &= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability;
							//	printf("Chamber[%d]:HandleProcess:Stabilitycheck 4:SE%d:OK=%d\n", ChamberId, i + 1, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability);
							if (SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpStability == false)
							{
								SetPartState(i + 1, ePartStatus, NoStability);
								SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
							}
						}
					}
					StabilityStep = 0;
					StabilityStartTime = 0;
					ProcessStep = EProcessStep::GasControl;
				}
				break;
			}
			break;
		}
			//gascontrol
		case EProcessStep::GasControl:
		{
			if (ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.TestGasOverRef == 1)
			{
				GasControl();
			}

			//save ip from reference 1
			ReferenceCell->GetLastIpMean(1, 3, &IpRefAct);
			IpRefBeforeProcess[0] = IpRefAct * 1.0e6f;
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpRef[0] = IpRefBeforeProcess[0];
			}

			//save ip from reference 2
			ReferenceCell->GetLastIpMean(2, 3, &IpRefAct);
			IpRefBeforeProcess[1] = IpRefAct * 1.0e6f;
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpRef[1] = IpRefBeforeProcess[1];
			}

			if ((eProcessType == ProcessType::IP450Meas2PointUp) ||
				(eProcessType == ProcessType::IP450MeasUNernstControl))
			{
				ProcessStep = EProcessStep::StartIPu4Meas;
			}
			else
			{
				ProcessStep = EProcessStep::CheckDieselQualification;
			}
			break;
		}
		case EProcessStep::StartIPu4Meas:
		{
	
			TimerStart = GetActualSystemTimeMs();
			TrimmCell->SetEventIPu4Measurement(true);
			ReferenceCell->SetEventIPu4Measurement(true);
			ProcessStep = EProcessStep::WaitIPu4MeasDone;
			break;
		}
		case EProcessStep::WaitIPu4MeasDone:
		{
			printf( "##TrimmCell->IsSequenceFinished(): %d\n", TrimmCell->IsSequenceFinished() );

			if ( TrimmCell->IsSequenceFinished() ||
			   ( GetActualSystemTimeMs() >= (TimerStart + TrimmCell->DurIp4Meas + 40000 /*+1 Sek reserve*/) )	)
			{
				ProcessStep = EProcessStep::CheckBmwQualification;
			}
			break;
		}
		case EProcessStep::CheckBmwQualification:

			BmwQualification( eProcessType );
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
			}
			ProcessStep = EProcessStep::MeasureIPRefAfterTrimming;
			ChamberState = INF_EMPTY;
			break;

		case EProcessStep::CheckDieselQualification:
			//diesel qualification
			switch (SETrimmingValues.AssemblyDataCommon.ProcessType)
			{
			case 1:
				//seperation diesel and gasoline 
				DieselQualification();
				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
				}
				ProcessStep = EProcessStep::TrimmingFinished;
				break;

			case 2:
				//only gasoline
				ProcessStep = EProcessStep::Trimming;
				break;

			case 3:
				//seperation diesel and trimming gasoline
				DieselQualification();
				ProcessStep = EProcessStep::Trimming;
				break;

			default:
				;
				break;

			}
			break;

		case EProcessStep::Trimming:
			ChamberState = INF_TRIMMING_ACTIVE;
			TrimmingEntryCheck();
			ProcessStep = EProcessStep::WaitTrimmingFinished;
			break;

		case EProcessStep::WaitTrimmingFinished:
			if (Trimming() == 0)
			{
				ProcessStep = EProcessStep::WaitTrimmingFinished;
			}
			if (GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + (ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.ProcessTimeout * 1000))
			{

			}
			break;

		case EProcessStep::TrimmingFinished:
			ChamberState = INF_TRIMMING_FINISHED;
			TrimmingEndCheck();
			ProcessStep = EProcessStep::MeasureIPRefAfterTrimming;
			break;
		case EProcessStep::MeasureIPRefAfterTrimming:

			//save ip from reference 1
			ReferenceCell->GetLastIpMean(1, 3, &IpRefAct);
			IpRefAfterProcess[0] = IpRefAct * 1.0e6f;

			//save ip from reference 2
			ReferenceCell->GetLastIpMean(2, 3, &IpRefAct);
			IpRefAfterProcess[1] = IpRefAct * 1.0e6f;

			if ((eProcessType == ProcessType::IP450Meas2PointUp) ||
				(eProcessType == ProcessType::IP450MeasUNernstControl))
			{
				ProcessStep = EProcessStep::SavePartsJournal;
			}
			else
			{
				ProcessStep = EProcessStep::Labeling;
			}
			
			break;

		case EProcessStep::Labeling:
			ChamberState = INF_LABELING;
			if (ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.Labeling != 0)
			{
				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					if ((SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus == Nio) ||
						(SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus == IoDiesel) ||
						(SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus == IoGasoline))
					{
						GetNewSerialNumberAndCode(SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus,
							&SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SerialNumber,
							SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SerialCode);

						float StartX, StartY, Phi;
						char ChamberIdString[5];

						//write text (serial code)
						RotatePosition(i + 1, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.XPosition, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.YPosition, &StartX, &StartY, NULL);
						StartX -= 0.79f;
						StartY -= 1.43f;
						RotateBackPosition(i + 1, StartX, StartY, &StartX, &StartY, &Phi);
						if (ComMicroLas->WriteText(SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SerialCode,
							StartX,
							StartY,
							SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ZPosition,
							Phi + 90.0f,
							(100.0f / SETrimmingValues.AssemblyDataCommon.AdjustPollutePercent) * ParamTrimmChamber.ParamTrimmProcess.ParamLaser.LaserPower[i] * 2.0f) != 0)
						{
							ComPlc->SetErrorCode(GLOBAL_ERROR, FLT_MICROLAS_SCANNER);
						}

						//write text (global chamber ident)
						RotatePosition(i + 1, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.XPosition, SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.YPosition, &StartX, &StartY, NULL);
						StartX -= 2.10f;
						StartY += 0.50f;
						RotateBackPosition(i + 1, StartX, StartY, &StartX, &StartY, &Phi);
						sprintf_s(ChamberIdString, "%01d", 2 * SETrimmingValues.AssemblyDataCommon.GlobalMachineNumber + SETrimmingValues.AssemblyDataCommon.ChamberNumber - 2);
						if (ComMicroLas->WriteText(ChamberIdString,
							StartX,
							StartY,
							SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ZPosition,
							Phi + 180.0f,
							(100.0f / SETrimmingValues.AssemblyDataCommon.AdjustPollutePercent) * ParamTrimmChamber.ParamTrimmProcess.ParamLaser.LaserPower[i] * 2.0f))
						{
							ComPlc->SetErrorCode(GLOBAL_ERROR, FLT_MICROLAS_SCANNER);
						}

					}
				}
			}
			ProcessStep = EProcessStep::SavePartsJournal;
			ChamberState = INF_EMPTY;
			break;
		case EProcessStep::SavePartsJournal:
			
			if ( ( eProcessType == ProcessType::IP450Meas2PointUp ) ||
				 ( eProcessType == ProcessType::IP450MeasUNernstControl ) )
			{
				SaveIpSelectionPartsJournal( eProcessType );
			}
			else
			{
				SavePartsJournal( );
			}

			ProcessStep = EProcessStep::CoolingSelection;

			break;
		case  EProcessStep::CoolingSelection:
			switch (ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingType)
			{
			case 0:
				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
				}
				ProcessStep = EProcessStep::ProcessFinished;
				break;

			case 1:
				CoolingStartTime = GetActualSystemTimeMs();
				ProcessStep = EProcessStep::StartCoolingActive;
				break;

			case 2:
				CoolingStartTime = GetActualSystemTimeMs();
				ProcessStep = EProcessStep::StartCoolingPassive;
				break;

			default:
				ProcessStep = EProcessStep::ProcessFinished;
				break;
			}
			break;

		case EProcessStep::StartCoolingPassive:
			ChamberState = INF_COOLING_PASSIVE;
			TrimmCell->StopSequence();
			Sleep(100);
			TrimmCell->GenerateAndTransmittSequence(ProcessType::Cooling);
			Sleep(100);
			ProcessStep = EProcessStep::StartCoolingActive;
			break;

		case EProcessStep::StartCoolingActive:

			switch (ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingType)
			{
			case 0:
				;
				break;

			case 1:
				if (GetActualSystemTimeMs() - CoolingStartTime >= ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingTimePassive * 1000)
				{
					ChamberState = INF_COOLING_ACTIVE;
					DigitalOut = COOLING_ACTIVE_FLAG;
					if ((GetActualSystemTimeMs() - CoolingStartTime >=
						(ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingTimeActive + ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingTimePassive) * 1000) &&
						((DigitalIn & COOLING_ACTIVE_FLAG) == COOLING_ACTIVE_FLAG))
					{
						for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
						{
							SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
						}
						DigitalOut = NONE_FLAG;
					}
				}
				break;

			case 2:
				float MaxRhh;
				MaxRhh = 0.0f;
				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					TrimmCell->GetLastValue(i + 1, IMT_RHh, 0, &Value);

					if (_isnan(Value.Value) == 0)
					{
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActRhh = Value.Value;
						MaxRhh = max(MaxRhh, Value.Value);
					}
				}

				printf("Chamber[%d]:Cooling: Rhh(Max)=%f\n", ChamberId, MaxRhh);

				if (MaxRhh != 0.0f)
				{
					if ((MaxRhh <= ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingRhhPasssive) ||
						(GetActualSystemTimeMs() - CoolingStartTime >= ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingTimeoutRhhPasssive * 1000.0f))
					{
						ChamberState = INF_COOLING_ACTIVE;
						DigitalOut = COOLING_ACTIVE_FLAG;
						if (((MaxRhh <= ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingRhhActive) &&
							((DigitalIn & COOLING_ACTIVE_FLAG) == COOLING_ACTIVE_FLAG)) ||
							(GetActualSystemTimeMs() - CoolingStartTime >= (ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingTimeoutRhhPasssive + ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CoolingTimeoutRhhActive) * 1000))
						{
							for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
							{
								SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
							}
							DigitalOut = NONE_FLAG;
						}
					}
				}
				break;

			default:
				break;
			}
			Sleep(1000);
			break;

		default:

			break;

		}

		for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
		{
			if ( ProcessStep < EProcessStep::StartCoolingPassive )
			{
				TrimmCell->GetLastValue(i + 1, IMT_RiAC, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if ((Value.Value > ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset) &&
						(Value.Value < 1000.0f))
					{
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActRi = Value.Value - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset;
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_IPu, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0f)
					{
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActIp = IpCorrectionOffsetAndDynamic(IpCorrection(Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue));
					}
				}
			
				ReferenceCell->GetLastValue(1, IMT_IPu, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0f)
					{
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActIpRef[0] = Value.Value;
					}
				}

				ReferenceCell->GetLastValue(2, IMT_IPu, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0f)
					{
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActIpRef[1] = Value.Value;
					}
				}
			}

			if (ProcessStep == EProcessStep::StartCoolingActive)
			{
				TrimmCell->GetLastValue(i + 1, IMT_RHh, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActRhh = Value.Value;
				}
			}

			if (ProcessStep >= EProcessStep::GasControl)
			{
				if (abs(ChamberPressureActual - ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint) > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PMaxAllowedDiff)
				{	
					printf("abs(ChamberPressureActual - ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint): %f, PMaxAllowedDiff: %f\n", 
						abs(ChamberPressureActual - ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint), ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PMaxAllowedDiff);
					SetPartState(i + 1, SelectGasoline, PressureOutside);
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
				}

				if (abs(ChamberFlowActual - ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.FlowSetpoint) > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.FlowMaxAllowedDiff)
				{
					SetPartState(i + 1, SelectGasoline, FlowOutside);
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
				}
			}
		}
		break;
	}
	case ProcessType::MeasurePositionNormal:  //normal 1 (position normal)
	{
		bool NormalDone = true;
		switch (NormalStep)
		{
		case 0:
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SENormalValues[i].ResultDataPosition.Status = 0x0000001F;

				TrimmCell->GetLastValue(i + 1, IMT_IPu, 3, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataPosition.IpCont = Round(Value.Value * 1.0e6f, 2);
						if ((SENormalValues[i].ResultDataPosition.IpCont > 0.2))
						{
							SENormalValues[i].ResultDataPosition.Status ^= 0x00000004;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_UAPE, 1, &ValueU1);
				TrimmCell->GetLastValue(i + 1, IMT_IPu, 1, &ValueI1);

				TrimmCell->GetLastValue(i + 1, IMT_UAPE, 2, &ValueU2);
				TrimmCell->GetLastValue(i + 1, IMT_IPu, 2, &ValueI2);

				if ((_isnan(ValueU1.Value) == 0) &&
					(_isnan(ValueI1.Value) == 0) &&
					(_isnan(ValueU2.Value) == 0) &&
					(_isnan(ValueI2.Value) == 0))
				{
					if ((ValueU1.Value != 0.0) &&
						(ValueI1.Value != 0.0) &&
						(ValueU2.Value != 0.0) &&
						(ValueI2.Value != 0.0))
					{
						SENormalValues[i].ResultDataPosition.RiDcp = Round((ValueU2.Value - ValueU1.Value) / (ValueI2.Value - ValueI1.Value) * 0.9f, 2);
						if ((SENormalValues[i].ResultDataPosition.RiDcp >= ParamNormalMeasure.ParamPositionNormal.ChkSensor[i].LowerLimit) &&
							(SENormalValues[i].ResultDataPosition.RiDcp <= ParamNormalMeasure.ParamPositionNormal.ChkSensor[i].UpperLimit))
						{
							SENormalValues[i].ResultDataPosition.Status ^= 0x00000001;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_UHS, 1, &ValueU1);
				TrimmCell->GetLastValue(i + 1, IMT_IH, 1, &ValueI1);

				TrimmCell->GetLastValue(i + 1, IMT_URE, 1, &ValueU2);
				TrimmCell->GetLastValue(i + 1, IMT_IPr, 1, &ValueI2);

				if ((_isnan(ValueU1.Value) == 0) &&
					(_isnan(ValueI1.Value) == 0))
				{
					if ((ValueU1.Value != 0.0) &&
						(ValueI1.Value != 0.0))
					{
						SENormalValues[i].ResultDataPosition.Rhhot = Round((ValueU1.Value - 0.4f) / ValueI1.Value, 2);
						if ((SENormalValues[i].ResultDataPosition.Rhhot >= ParamNormalMeasure.ParamPositionNormal.ChkHeater[i].LowerLimit) &&
							(SENormalValues[i].ResultDataPosition.Rhhot <= ParamNormalMeasure.ParamPositionNormal.ChkHeater[i].UpperLimit))
						{
							SENormalValues[i].ResultDataPosition.Status ^= 0x00000010;
						}

						SENormalValues[i].ResultDataPosition.Ih = Round(ValueI1.Value, 2);
						if (SENormalValues[i].ResultDataPosition.Ih > 0.001)
						{
							SENormalValues[i].ResultDataPosition.Status ^= 0x00000008;
						}
					}
				}

				if ((_isnan(ValueU2.Value) == 0) &&
					(_isnan(ValueI2.Value) == 0))
				{
					if ((ValueU2.Value != 0.0) &&
						(ValueI2.Value != 0.0))
					{
						SENormalValues[i].ResultDataPosition.RiDcn = Round((ValueU2.Value + 0.72f) / ValueI2.Value, 2);
						if ((SENormalValues[i].ResultDataPosition.RiDcn >= 180.0f) &&
							(SENormalValues[i].ResultDataPosition.RiDcn <= 280.0f))
						{
							SENormalValues[i].ResultDataPosition.Status ^= 0x00000002;
						}

					}
				}

				if ((SENormalValues[i].ResultDataPosition.RiDcp != 0.0f) &&
					(SENormalValues[i].ResultDataPosition.Rhhot != 0.0f) &&
					(SENormalValues[i].ResultDataPosition.RiDcn != 0.0f))
				{
					NormalDone &= true;
				}
				else
				{
					NormalDone &= false;
				}

			}

			if ((NormalDone == true) ||
				(TrimmCell->IsSequenceFinished() == true) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + 30.0f * 1000.0f))
			{
				NormalStep = 1;
			}
			break;
		case 1:
			SaveNormalJournal();
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
			}
			NormalStep = 2;
			break;
		default:
			break;
		}
		break;
	}
	case ProcessType::MeasureHeaterNormal:    //normal 2 (heater normal)
	{
		bool NormalDone = true;
		switch (NormalStep)
		{
		case 0:
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SENormalValues[i].ResultDataHeater.Status = 0x0000000F;

				TrimmCell->GetLastValue(i + 1, IMT_HSCONT, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					SENormalValues[i].ResultDataHeater.ContactOk = Value.Value;
					if (SENormalValues[i].ResultDataHeater.ContactOk == 0.0f)
					{
						SENormalValues[i].ResultDataHeater.Status ^= 0x00000001;
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_RHh, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataHeater.Rhhot = Round(Value.Value, 2);
						if ((SENormalValues[i].ResultDataHeater.Rhhot >= ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceHot.LowerLimit) &&
							(SENormalValues[i].ResultDataHeater.Rhhot <= ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceHot.UpperLimit))
						{
							SENormalValues[i].ResultDataHeater.Status ^= 0x00000008;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_IH, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataHeater.Ih = Round(Value.Value, 2);
						if ((SENormalValues[i].ResultDataHeater.Ih >= ParamNormalMeasure.ParamHeaterNormal.ChkHeaterCurrent.LowerLimit) &&
							(SENormalValues[i].ResultDataHeater.Ih <= ParamNormalMeasure.ParamHeaterNormal.ChkHeaterCurrent.UpperLimit))
						{
							SENormalValues[i].ResultDataHeater.Status ^= 0x00000004;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_RHk, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataHeater.RhCold = Round(Value.Value, 2);
						if ((SENormalValues[i].ResultDataHeater.RhCold >= ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceCold.LowerLimit) &&
							(SENormalValues[i].ResultDataHeater.RhCold <= ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceCold.UpperLimit))
						{
							SENormalValues[i].ResultDataHeater.Status ^= 0x00000002;
						}
					}
				}

				if ((SENormalValues[i].ResultDataHeater.Rhhot != 0.0f) &&
					(SENormalValues[i].ResultDataHeater.Ih != 0.0f) &&
					(SENormalValues[i].ResultDataHeater.RhCold != 0.0f))
				{
					NormalDone &= true;
				}
				else
				{
					NormalDone &= false;
				}

			}

			if ((NormalDone == true) ||
				(TrimmCell->IsSequenceFinished() == true) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + 30.0f * 1000.0f))
			{
				NormalStep = 1;
			}

			break;
		case 1:
			SaveNormalJournal();
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
			}
			NormalStep = 2;
			break;
		default:
			break;
		}
		break;
	}
	case ProcessType::MeasureUniversalNormal: // normal 3 (universal normal)
	{
		bool NormalDone = true;
		switch (NormalStep)
		{
		case 0:
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SENormalValues[i].ResultDataUniversal.Status = 0x0000003F;

				TrimmCell->GetLastValue(i + 1, IMT_IgRK, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataUniversal.Igrk = Round(Value.Value * 1e6f, 2);
						if ((SENormalValues[i].ResultDataUniversal.Igrk >= ParamNormalMeasure.ParamUniversalNormal.ChkIgrk.LowerLimit) &&
							(SENormalValues[i].ResultDataUniversal.Igrk <= ParamNormalMeasure.ParamUniversalNormal.ChkIgrk.UpperLimit))
						{
							SENormalValues[i].ResultDataUniversal.Status ^= 0x00000001;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_IL, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataUniversal.Il = Round(Value.Value * 1e6f, 2);
						if ((SENormalValues[i].ResultDataUniversal.Il >= ParamNormalMeasure.ParamUniversalNormal.ChkIl.LowerLimit) &&
							(SENormalValues[i].ResultDataUniversal.Il <= ParamNormalMeasure.ParamUniversalNormal.ChkIl.UpperLimit))
						{
							SENormalValues[i].ResultDataUniversal.Status ^= 0x00000002;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_RiDCn, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataUniversal.RiDcn = Round(Value.Value - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset, 2);
						if ((SENormalValues[i].ResultDataUniversal.RiDcn >= ParamNormalMeasure.ParamUniversalNormal.ChkRidc.LowerLimit) &&
							(SENormalValues[i].ResultDataUniversal.RiDcn <= ParamNormalMeasure.ParamUniversalNormal.ChkRidc.UpperLimit))
						{
							SENormalValues[i].ResultDataUniversal.Status ^= 0x00000004;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_IpRE, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataUniversal.IpRe = Round(Value.Value * 1.0e6f, 2);
						if ((SENormalValues[i].ResultDataUniversal.IpRe >= ParamNormalMeasure.ParamUniversalNormal.ChkIpre.LowerLimit) &&
							(SENormalValues[i].ResultDataUniversal.IpRe <= ParamNormalMeasure.ParamUniversalNormal.ChkIpre.UpperLimit))
						{
							SENormalValues[i].ResultDataUniversal.Status ^= 0x00000008;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_RiAC, 2, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataUniversal.RiAc = Round(Value.Value - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset, 2);
						if ((SENormalValues[i].ResultDataUniversal.RiAc >= ParamNormalMeasure.ParamUniversalNormal.ChkRiac.LowerLimit) &&
							(SENormalValues[i].ResultDataUniversal.RiAc <= ParamNormalMeasure.ParamUniversalNormal.ChkRiac.UpperLimit))
						{
							SENormalValues[i].ResultDataUniversal.Status ^= 0x00000010;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_RiAC, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataUniversal.RiAcstat = Round(Value.Value - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset, 2);
						if ((SENormalValues[i].ResultDataUniversal.RiAcstat >= ParamNormalMeasure.ParamUniversalNormal.ChkRiacStat.LowerLimit) &&
							(SENormalValues[i].ResultDataUniversal.RiAcstat <= ParamNormalMeasure.ParamUniversalNormal.ChkRiacStat.UpperLimit))
						{
							SENormalValues[i].ResultDataUniversal.Status ^= 0x00000020;
						}
					}
				}

				if ((SENormalValues[i].ResultDataUniversal.Igrk != 0.0f) &&
					(SENormalValues[i].ResultDataUniversal.Il != 0.0f) &&
					(SENormalValues[i].ResultDataUniversal.RiDcn != 0.0f) &&
					(SENormalValues[i].ResultDataUniversal.IpRe != 0.0f) &&
					(SENormalValues[i].ResultDataUniversal.RiAc != 0.0f) &&
					(SENormalValues[i].ResultDataUniversal.RiAcstat != 0.0f))
				{
					NormalDone &= true;
				}
				else
				{
					NormalDone &= false;
				}

			}

			if ((NormalDone == true) ||
				(TrimmCell->IsSequenceFinished() == true) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + 30.0f * 1000.0f))
			{
				NormalStep = 1;
			}

			break;
		case 1:
			SaveNormalJournal();
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
			}
			NormalStep = 2;
			break;
		default:
			break;
		}
		break;
	}
	case ProcessType::MeasureUnNormal:	      // normal 4 (un normal)
	{
		bool NormalDone = true;
		switch (NormalStep)
		{
		case 0:
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SENormalValues[i].ResultDataUn.Status = 0x00000001;

				TrimmCell->GetLastValue(i + 1, IMT_UN, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataUn.Un = Round(Value.Value, 2);
						if ((SENormalValues[i].ResultDataUn.Un >= ParamNormalMeasure.ParamUnNormal.ChkNernstVoltage.LowerLimit) &&
							(SENormalValues[i].ResultDataUn.Un <= ParamNormalMeasure.ParamUnNormal.ChkNernstVoltage.UpperLimit))
						{
							SENormalValues[i].ResultDataUn.Status ^= 0x00000001;
						}
					}
				}

				if ((SENormalValues[i].ResultDataUn.Un != 0.0f)) {
					NormalDone &= true;
				}
				else
				{
					NormalDone &= false;
				}

			}

			if ((NormalDone == true) ||
				(TrimmCell->IsSequenceFinished() == true) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + 30.0f * 1000.0f))
			{
				NormalStep = 1;
			}

			break;
		case 1:
			SaveNormalJournal();
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
			}
			NormalStep = 2;
			break;
		default:
			break;
		}
		break;
	}
	case ProcessType::MeasureIpNormal:        // normal 5 (ip normal)
	{
		bool NormalDone = true;
		switch (NormalStep)
		{
		case 0:
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SENormalValues[i].ResultDataIp.Status = 0x00000003;

				TrimmCell->GetLastValue(i + 1, IMT_IPu, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataIp.Ip = Round(Value.Value * 1e6f, 2);
						if ((SENormalValues[i].ResultDataIp.Ip >= ParamNormalMeasure.ParamIpNormal.ChkIp.LowerLimit) &&
							(SENormalValues[i].ResultDataIp.Ip <= ParamNormalMeasure.ParamIpNormal.ChkIp.UpperLimit))
						{
							SENormalValues[i].ResultDataIp.Status ^= 0x00000002;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_UAPE, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataIp.UApe = Round(Value.Value, 2);
						if ((SENormalValues[i].ResultDataIp.UApe >= ParamNormalMeasure.ParamIpNormal.ChkUp.LowerLimit) &&
							(SENormalValues[i].ResultDataIp.UApe <= ParamNormalMeasure.ParamIpNormal.ChkUp.UpperLimit))
						{
							SENormalValues[i].ResultDataIp.Status ^= 0x00000001;
						}
					}
				}

				if ((SENormalValues[i].ResultDataIp.Ip != 0.0f) &&
					(SENormalValues[i].ResultDataIp.UApe != 0.0f))
				{
					NormalDone &= true;
				}
				else
				{
					NormalDone &= false;
				}

			}

			if ((NormalDone == true) ||
				(TrimmCell->IsSequenceFinished() == true) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + 30.0f * 1000.0f))
			{
				for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
				{
					printf("Ip[%d]=%f\n", i + 1, SENormalValues[i].ResultDataIp.Ip);
				}
				NormalStep = 1;
			}
			break;

		case 1:
			SaveNormalJournal();
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
			}
			NormalStep = 2;
			break;
		default:
			break;
		}
		break;
	}
	case ProcessType::MeasureIlmNormal:       // normal 6 (ilm normal)
	{
		bool NormalDone = true;

		switch (NormalStep)
		{
		case 0:
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SENormalValues[i].ResultDataIlm.Status = 0x000001FF;

				TrimmCell->GetLastValue(i + 1, IMT_IL, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataIlm.Ilm = Round(Value.Value * 1e6f, 2);
						if ((SENormalValues[i].ResultDataIlm.Ilm >= ParamNormalMeasure.ParamIlmNormal.ChkIlMax.LowerLimit) &&
							(SENormalValues[i].ResultDataIlm.Ilm <= ParamNormalMeasure.ParamIlmNormal.ChkIlMax.UpperLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000001;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_IPr, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataIlm.IaIpn = Round(Value.Value * 1.0e3f, 2);
						if ((SENormalValues[i].ResultDataIlm.IaIpn >= ParamNormalMeasure.ParamIlmNormal.ChkIaIpn.LowerLimit) &&
							(SENormalValues[i].ResultDataIlm.IaIpn <= ParamNormalMeasure.ParamIlmNormal.ChkIaIpn.UpperLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000002;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_UAPE, 1, &Value);
				if (_isnan(Value.Value) == 0)
				{
					if (Value.Value != 0.0)
					{
						SENormalValues[i].ResultDataIlm.UIpn = Round(Value.Value - SENormalValues[i].ResultDataIlm.IaIpn / 1.0e3f * ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset / 2.0f, 2);
						if ((SENormalValues[i].ResultDataIlm.UIpn >= ParamNormalMeasure.ParamIlmNormal.ChkUIpn.LowerLimit) &&
							(SENormalValues[i].ResultDataIlm.UIpn <= ParamNormalMeasure.ParamIlmNormal.ChkUIpn.UpperLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000004;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_IPu, 1, &ValueI1);
				TrimmCell->GetLastValue(i + 1, IMT_UN, 1, &ValueU1);

				if ((_isnan(ValueI1.Value) == 0) && (_isnan(ValueU1.Value) == 0))
				{
					if ((ValueI1.Value != 0.0) && (ValueU1.Value != 0.0))
					{
						SENormalValues[i].ResultDataIlm.Un2 = Round(ValueU1.Value * 1.0e3f + ValueI1.Value / 1.0e3f * ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset / 2.0f, 2);
						if ((SENormalValues[i].ResultDataIlm.Un2 >= ParamNormalMeasure.ParamIlmNormal.ChkPt3Un2.LowerLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000010;
						}

						SENormalValues[i].ResultDataIlm.Ip2 = Round(SENormalValues[i].ResultDataIlm.Un2 / ((ValueU1.Value / ValueI1.Value) - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset / 2.0f), 2);
						if ((SENormalValues[i].ResultDataIlm.Ip2 >= ParamNormalMeasure.ParamIlmNormal.ChkPt3Ip2.LowerLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000008;
						}
					}
				}

				TrimmCell->GetLastValue(i + 1, IMT_IPu, 2, &ValueI1);
				TrimmCell->GetLastValue(i + 1, IMT_UN, 2, &ValueU1);

				if ((_isnan(ValueI1.Value) == 0) && (_isnan(ValueU1.Value) == 0))
				{
					if ((ValueI1.Value != 0.0) && (ValueU1.Value != 0.0))
					{
						SENormalValues[i].ResultDataIlm.Un3 = Round(ValueU1.Value * 1.0e3f + ValueI1.Value / 1.0e3f * ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset / 2.0f, 2);
						if ((SENormalValues[i].ResultDataIlm.Un3 >= ParamNormalMeasure.ParamIlmNormal.ChkPt3Un3.LowerLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000040;
						}

						SENormalValues[i].ResultDataIlm.Ip3 = Round(SENormalValues[i].ResultDataIlm.Un3 / ((ValueU1.Value / ValueI1.Value) - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset / 2.0f), 2);
						SENormalValues[i].ResultDataIlm.Status ^= 0x00000020;
					}

				}

				TrimmCell->GetLastValue(i + 1, IMT_IPu, 3, &ValueI1);
				TrimmCell->GetLastValue(i + 1, IMT_UN, 3, &ValueU1);

				if ((_isnan(ValueI1.Value) == 0) && (_isnan(ValueU1.Value) == 0))
				{
					if ((ValueI1.Value != 0.0) && (ValueU1.Value != 0.0))
					{
						SENormalValues[i].ResultDataIlm.Un4 = Round(ValueU1.Value * 1.0e3f + ValueI1.Value / 1.0e3f * ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset / 2.0f, 2);
						if ((SENormalValues[i].ResultDataIlm.Un4 >= ParamNormalMeasure.ParamIlmNormal.ChkPt3Un4.LowerLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000100;
						}

						SENormalValues[i].ResultDataIlm.Ip4 = Round(SENormalValues[i].ResultDataIlm.Un4 / ((ValueU1.Value / ValueI1.Value) - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset / 2.0f), 2);
						if ((SENormalValues[i].ResultDataIlm.Ip4 >= ParamNormalMeasure.ParamIlmNormal.ChkPt3Ip4.LowerLimit))
						{
							SENormalValues[i].ResultDataIlm.Status ^= 0x00000080;
						}
					}
				}

				if ((SENormalValues[i].ResultDataIlm.Ilm != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.IaIpn != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.UIpn != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.Ip2 != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.Un2 != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.Ip3 != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.Un3 != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.Ip4 != 0.0f) &&
					(SENormalValues[i].ResultDataIlm.Un4 != 0.0f)) {
					NormalDone &= true;
				}
				else
				{
					NormalDone &= false;
				}

			}

			if ((NormalDone == true) ||
				(TrimmCell->IsSequenceFinished() == true) ||
				(GetActualSystemTimeMs() >= TrimmCell->GetSequenceStartTime() + 30.0f * 1000.0f))
			{
				NormalStep = 1;
			}


			break;
		case 1:
			SaveNormalJournal();
			for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
			{
				SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone = true;
			}
			NormalStep = 2;
			break;

		default:
			break;

		}
	}
	default:
		break;
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool ChamberAllCellsReady( void )                                                                *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = not all cells ready                                                             *
 *                                 true = all cells ready                                                                  *
 *                                                                                                                         *
 * description:         : This is the function to check the state of cells.                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SETrimmingChamber::ChamberAllCellsReady( void )
{
	bool RetVal = false;

	if( ( TrimmCell->AllCardsReady() ) &&
			( ReferenceCell->AllCardsReady() ) )
	{
		RetVal = true; 
	}
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool IpStabilityCheck( int Tiepoint )                                                            *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = no stability                                                                    *
 *                                 true = stability ok                                                                     *
 *                                                                                                                         *
 * description:         : This is the function to check the stability of se.                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SETrimmingChamber::IpStabilityCheck( int Tiepoint, int Count  )
{
	bool RetVal = false;

	SEValueInfo Value;

	//get ip ref sens 1
	ReferenceCell->GetLastValue(1, IMT_IPu, 0, &Value);
	IpRef[0] = IpCorrection( Value.Value , ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);

	//get ip ref sens 2
	ReferenceCell->GetLastValue(2, IMT_IPu, 0, &Value);
	IpRef[1] = IpCorrection( Value.Value , ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);


	float IpDiff = 0.0f, IpAct = 0.0f, IpDiffUnscaled = 0.0f;

	//get ip 
	TrimmCell->GetLastValue( Tiepoint, IMT_IPu, 0, &Value );

	if( _isnan( Value.Value )	== 0 )
	{
		if( Value.Value != 0.0f )
		{
			IpAct = IpCorrectionOffsetAndDynamic( IpCorrection( Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );
			if( ( _isnan( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpLastStabilityUnscaled ) == 0 )	&& ( _isnan( Value.Value ) == 0 ) )
			{
				IpDiffUnscaled = abs( ( Value.Value / SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpLastStabilityUnscaled - 1.0f ) * 100.0f );
			}
			if( ( _isnan( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpLastStability ) == 0 )	&& ( _isnan( IpAct ) == 0 ) )
			{
				IpDiff = abs( ( IpAct / SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpLastStability - 1.0f ) * 100.0f );
				if( ( IpDiff < ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpStable )	)
				{
					RetVal = true;//SETrimmingValues[ Tiepoint - 1 ].ProcessData.IpStability = true;
				}
				printf( "Chamber[%d]:Stability SE%d,Ipact=%f,Ipunscaled=%f,Ipdiff=%f,Ipdiffunscaled=%f,ActPres=%f,Time=%lld,TimePC=%lld\n", ChamberId, Tiepoint, IpAct * 1.0e6f, Value.Value * 1.0e6f,IpDiff,IpDiffUnscaled,ChamberPressureActual,Value.ReceiveTime,Value.ReceiveTimePC );
			}
			SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.IpLastStabilityUnscaled = Value.Value;
		}
	}
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.IpLastStability = IpAct;
	
 return RetVal;
}

float SETrimmingChamber::CalcIPu4OrUPu4TwoPointMeas(float fIPu1_UPu1, float fIPu2_UPu2, float fUNernst1, float fUNernst2, float UnTarget)
{
	float fIPu4_UPu4; // IPu4 or UPu4 value

	// calc. delta IPu
	float dIPu_UPu = fIPu2_UPu2 - fIPu1_UPu1;

	//calc. delta UNernst
	float dUNernst = fUNernst2 - fUNernst1;

	if ( ( _isnan(fIPu1_UPu1) == 0) && 
		 ( _isnan(fIPu2_UPu2) == 0) &&
		 ( _isnan(fUNernst1)  == 0) &&
		 ( _isnan(dIPu_UPu)   == 0) &&
		 ( _isnan(dUNernst)   == 0) &&
		 (dUNernst != 0) )
	{
		fIPu4_UPu4 = (fIPu1_UPu1 + (dIPu_UPu / dUNernst) * (UnTarget - fUNernst1));
	}
	else
	{
		fIPu4_UPu4 = 0;
	}

	return (fIPu4_UPu4);
}
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GasControl( void )                                                                           *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to check the gas athmosphrere at chamber.                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::GasControl( void )
{
	int RetVal = 0;
	float IpAct = 0.0, IpDiff = 0.0;

	bool GasControlOk = true;

#if defined (REF_SIMULATION)
	printf("FEF_SIMULATION: No gas control");
#else

	ReferenceCell->GetLastIpMean(1,3,&IpAct);
	printf( "Chamber[%d]:Gaskontrolle:IpRef1[uA]=%f\n", ChamberId, IpAct * 1.0e6f );
	IpDiff = abs( ( IpAct / ( ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[0] / 1.0e6f ) - 1.0f ) * 100.0f );
	if( IpDiff > ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.AllowableIpVariation )
	{
		printf("1: Gas control niO!\n");
		GasControlOk = false;
	}

	ReferenceCell->GetLastIpMean(2,3,&IpAct);
	printf( "Chamber[%d]:Gaskontrolle:IpRef2[uA]=%f\n", ChamberId, IpAct * 1.0e6f );
	IpDiff = abs( ( IpAct / ( ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[1] / 1.0e6f ) - 1.0f ) * 100.0f );
	if( IpDiff > ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.AllowableIpVariation )
	{
		printf("2: Gas control niO!\n");
		GasControlOk = false;
	}
#endif

	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.GasControlOk = GasControlOk;
		if( GasControlOk == false )
		{
			printf("3: Gas control niO!\n");
			SetPartState( i + 1, SelectGasoline, GasControlFault );
			SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
		}
	}

	return RetVal;
}


void SETrimmingChamber::BmwQualification(ProcessType eType)
{
	SEValueInfo Value;
	// Götz Stefan, 03.05.2023: es soll keinen Teilestatus "Nacharbeit" geben. 
	// Nur:
	//  2: IoDiesel
	//	3: SelectGasoline 
	
	// Auswertung IP Selektion
	for (int iPartNo = 0; iPartNo < CELL_TIEPOINT_COUNT; iPartNo++)
	{
		float fTempVal = 0;

		SetPartState(iPartNo + 1, IoDiesel);

		//contacting test heater side and sensor side ok?
		SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.ContactOk = TrimmCell->IsContactingOk(iPartNo + 1);
		if (SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.ContactOk == false)
		{
			SetPartState(iPartNo + 1, SelectGasoline, ContactRiStabilityNok);
		}

		float fIP450 = 0;
		//Determination of IP450,2 – Method 2 (two-point UP measurement)
		if (eType == ProcessType::IP450Meas2PointUp)
		{
			//Pump current at point of measurement 1
			TrimmCell->GetLastValue(iPartNo + 1, IMT_IPu, 101, &Value);
			float fIPu1 = SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fIpPoint1
				= IpCorrectionOffsetAndDynamic( IpCorrection(Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );
			printf("fIPu1: %f\n", SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fIpPoint1);
			//Pump current at point of measurement 1
			TrimmCell->GetLastValue(iPartNo + 1, IMT_IPu, 102, &Value);
			float fIPu2 = SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fIpPoint2
				= IpCorrectionOffsetAndDynamic( IpCorrection(Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );
			printf("fIPu2: %f\n", SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fIpPoint2);

			//Nernst voltage at point of measurement 1
			TrimmCell->GetLastValue(iPartNo + 1, IMT_URE, 101, &Value);
			float fUNernst1 = Value.Value;

			//Nernst voltage at point of measurement 2
			TrimmCell->GetLastValue(iPartNo + 1, IMT_URE, 102, &Value);
			float fUNernst2 = Value.Value;

			fIP450 = CalcIPu4OrUPu4TwoPointMeas(fIPu1,	fIPu2, fUNernst1, fUNernst2,
				     ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.NernstVoltage) * 1e6f;
		}
		else
		{
			TrimmCell->GetLastValue(iPartNo + 1, IMT_IPu, 100, &Value);
			fIP450 = IpCorrectionOffsetAndDynamic(IpCorrection(Value.Value, 
				     ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue)) * 1e6f;
		}

		printf("#IP450: %f mA\n", fIP450);

		if (_isnan(fIP450) == 0)
		{

			float fLowerLimit = ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpSelectionLowerLimit;
			float fUpperLimit = ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpSelectionUpperLimit;
			if (!CheckBounds(fIP450, fLowerLimit, fUpperLimit))
			{
				SetPartState(iPartNo + 1, SelectGasoline, IpOutsideLimit);
			}

			SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.IpEndCheck = fIP450;
			SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.IpEndCheck *= ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.ScaleFactorIp;
			//copy IP4 value in result structure for PLC
			SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ResultData.IpEndCheck = SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.IpEndCheck;

		}
		else
		{
			SetPartState(iPartNo + 1, SelectGasoline, IpOutsideLimit);
		}

		// APE voltage during IPu4 
		if (eType == ProcessType::IP450Meas2PointUp)
		{
			TrimmCell->GetLastValue(iPartNo + 1, IMT_UAPE, 101, &Value);
			float fUape1 = Value.Value;
			printf("fUape1: %f\n", fUape1);
			TrimmCell->GetLastValue(iPartNo + 1, IMT_UAPE, 102, &Value);
			float fUape2 = Value.Value;
			printf("fUape2: %f\n", fUape2);
			TrimmCell->GetLastValue(iPartNo + 1, IMT_URE, 101, &Value);
			float fUNernst1 = Value.Value;
			printf("fUNernst1: %f\n", fUNernst1);
			TrimmCell->GetLastValue(iPartNo + 1, IMT_URE, 102, &Value);
			float fUNernst2 = Value.Value;
			printf("fUNernst2: %f\n", fUNernst2);

			Value.Value = CalcIPu4OrUPu4TwoPointMeas(fUape1, fUape2, fUNernst1, fUNernst2, 
				           ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.NernstVoltage);
		}
		else
		{
			TrimmCell->GetLastValue(iPartNo + 1, IMT_UAPE, 100, &Value);
		}
		printf("UAPE(IP4): %f\n", Value.Value * 1e3f);
		if (_isnan(Value.Value) == 0)
		{
				float fLowerLimit = 0.0f; //mV 
				float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.PumpVoltage * 1.0e3f;

				if (!CheckBounds(Value.Value * 1.0e3f, fLowerLimit, fUpperLimit))
				{
					SetPartState(iPartNo + 1, SelectGasoline, UApeOutsideLimit);
				}
				SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fUApeAtIPu = Value.Value * 1e3f;

		}
		else
		{
			SetPartState(iPartNo + 1, SelectGasoline, UApeOutsideLimit);
		}

		if (eType == ProcessType::IP450MeasUNernstControl)
		{
			// Nernst voltage (target value)
			TrimmCell->GetLastValue(iPartNo + 1, IMT_URE, 100, &Value);
			printf("UN: IMT_URE, 100: %f\n", Value.Value * 1e3f);
			
			if (_isnan(Value.Value) == 0)
			{
					float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fMinNernstVoltage * 1.0e3f;
					float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fMaxNernstVoltage * 1.0e3f;

					if (!CheckBounds(Value.Value * 1.0e3f, fLowerLimit, fUpperLimit))
					{
						SetPartState(iPartNo + 1, SelectGasoline, UNernstOutsideLimit);
					}
					SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fUNernst = Value.Value * 1e3f;
			}
			else
			{
				SetPartState(iPartNo + 1, SelectGasoline, UNernstOutsideLimit);
			}
		}


		//IPref
		TrimmCell->GetLastValue(iPartNo + 1, IMT_IpRE, 101, &Value);
		printf("IPRef: IMT_IpRE, 101: %f\n", Value.Value * 1e6f);

		if (_isnan(Value.Value) == 0)
		{

				float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fIpRefRngMin;
				float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fIpRefRngMax;

				if (!CheckBounds(Value.Value * 1.0e6f, fLowerLimit, fUpperLimit))
				{
					SetPartState(iPartNo + 1, SelectGasoline, IpReRefOutsideLimit);
				}
				SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fIRe1 = Value.Value * 1e6f;
		}
		else
		{
			SetPartState(iPartNo + 1, SelectGasoline, IpReRefOutsideLimit);
		}

		// APE voltage during IPu4 
		if (eType == ProcessType::IP450Meas2PointUp)
		{
			//IPref
			TrimmCell->GetLastValue(iPartNo + 1, IMT_IpRE, 102, &Value);
			printf("IPRef: IMT_IpRE, 102: %f\n", Value.Value * 1e6f);
			if (_isnan(Value.Value) == 0)
			{
					float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fIpRefRngMin;
					float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fIpRefRngMax;
					if (!CheckBounds(Value.Value * 1.0e6f, fLowerLimit, fUpperLimit))
					{
						SetPartState(iPartNo + 1, SelectGasoline, IpReRefOutsideLimit);
					}
					SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fIRe2 = Value.Value * 1e6f;
			}
			else
			{
				SetPartState(iPartNo + 1, SelectGasoline, IpReRefOutsideLimit);
			}
		}

		//UHMax
		TrimmCell->GetLastValue(iPartNo + 1, IMT_UHmax, 0, &Value);
		printf("UHmax: IMT_UHmax, 0: %f\n", Value.Value);

		if (_isnan(Value.Value) == 0)
		{
				float fLowerLimit = 0.1f; // lower tolerance limit 0.0V
				float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamRiRegulaton.RiRegMaxUH;

				if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
				{
					SetPartState(iPartNo + 1, SelectGasoline, UHeaterOutsideLimit);
				}
				SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.fUhMax = Value.Value;
		}
		else
		{
			SetPartState(iPartNo + 1, SelectGasoline, UHeaterOutsideLimit);
		}


		if (eType == ProcessType::IP450Meas2PointUp)
		{

			TrimmCell->GetLastValue(iPartNo + 1, IMT_UAPE, 101, &Value);
			printf("fUape1: %f, %f, %f\n", Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp1Min, 
				ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp1Max);

			if (_isnan(Value.Value) == 0)
			{
				float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp1Min;  //0.755V
				float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp1Max;  //0.765V

				if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
				{
					SetPartState(iPartNo + 1, SelectGasoline, UApeOutsideLimit );
				}
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
				SetPartState(iPartNo + 1, SelectGasoline, UApeOutsideLimit);
			}

			TrimmCell->GetLastValue(iPartNo + 1, IMT_UAPE, 102, &Value);
			printf("fUape2: %f, %f, %f\n", Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp2Min,
				ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp2Max);
			
			if (_isnan(Value.Value) == 0)
			{
				float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp2Min; //0.935V
				float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp2Max; //0.945V
				if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
				{
					SetPartState(iPartNo + 1, SelectGasoline, UApeOutsideLimit);
				}
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
				SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
			}

			if (ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.EvaluateRhh == 1)
			{
				//Heater hot resistance RHh
				TrimmCell->GetLastValue(iPartNo + 1, IMT_RHh, 102, &Value);
				if (_isnan(Value.Value) == 0)
				{
					float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhLowerLimit; //Ohm
					float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhUpperLimit; //Ohm
					if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
					{
						printf("Chamber[%d]:SE%d:Rhh=%f\n", ChamberId, iPartNo + 1, Value.Value);
						SetPartState(iPartNo + 1, SelectGasoline, RhhOutsideLimit);
					}
				}
				else
				{
					SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
					SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
				}
			}

			//Heater power PH
			if (ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.EvaluatePh == 1)
			{
				TrimmCell->GetLastValue(iPartNo + 1, IMT_PH, 102, &Value);
				if (_isnan(Value.Value) == 0)
				{
					float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhLowerLimit; //Watt
					float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhUpperLimit; //Watt
					if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
					{
						printf("Chamber[%d]:SE%d:Ph=%f\n", ChamberId, iPartNo + 1, Value.Value);
						SetPartState(iPartNo + 1, SelectGasoline, PhOutsideLimit);
					}
				}
				else
				{
					SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
					SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
				}
			}
		}
#if 1
		//Heater hot resistance RHk
		TrimmCell->GetLastValue(iPartNo + 1, IMT_RHk, 0, &Value);
		if (_isnan(Value.Value) == 0)
		{
			float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fRHkMin; //Ohm
			float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fRHkMax; //Ohm
			if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
			{
				printf("Chamber[%d]:SE%d:Rhk=%f\n", ChamberId, iPartNo + 1, Value.Value);
				SetPartState(iPartNo + 1, SelectGasoline, RhkOutsideLimit);
			}
		}
		else
		{
			SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
			SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
		}
#endif
		//Heater hot resistance RHh
		if (ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.EvaluateRhh == 1)
		{
			TrimmCell->GetLastValue(iPartNo + 1, IMT_RHh, 101, &Value);
			if (_isnan(Value.Value) == 0)
			{
				float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhLowerLimit; //Ohm
				float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhUpperLimit; //Ohm
				if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
				{
					printf("Chamber[%d]:SE%d:Rhh=%f\n", ChamberId, iPartNo + 1, Value.Value);
					SetPartState(iPartNo + 1, SelectGasoline, RhhOutsideLimit);
				}
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
				SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
			}
		}

		//Heater power PH
		if (ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.EvaluatePh == 1)
		{
			TrimmCell->GetLastValue(iPartNo + 1, IMT_PH, 101, &Value);
			if (_isnan(Value.Value) == 0)
			{
				float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhLowerLimit; //Watt
				float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhUpperLimit; //Watt
				if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
				{
					printf("Chamber[%d]:SE%d:Ph=%f\n", ChamberId, iPartNo + 1, Value.Value);
					SetPartState(iPartNo + 1, SelectGasoline, PhOutsideLimit);
				}
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
				SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
			}
		}

		// RiMin
		TrimmCell->GetLastValue(iPartNo + 1, IMT_RiMin, 0, &Value);
		if (_isnan(Value.Value) == 0)
		{
			Value.Value = Value.Value - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset;
			float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmChamber.ParamTrimmCell.ParamRiStability.RiStabilityLowerLimit;
			float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmChamber.ParamTrimmCell.ParamRiStability.RiStabilityUpperLimit;
				
			if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
			{
				printf("Chamber[%d]:SE%d:RiMin=%f\n", ChamberId, iPartNo + 1, Value.Value);
				SetPartState(iPartNo + 1, SelectGasoline, RiOutsideLimit);
			}
		}
		else
		{
			SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
			SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
		}

		// RiMax
		TrimmCell->GetLastValue(iPartNo + 1, IMT_RiMax, 0, &Value);
		if (_isnan(Value.Value) == 0)
		{
			Value.Value = Value.Value - ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiOffset;
			float fLowerLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmChamber.ParamTrimmCell.ParamRiStability.RiStabilityLowerLimit;
			float fUpperLimit = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiSelection + ParamTrimmChamber.ParamTrimmCell.ParamRiStability.RiStabilityUpperLimit;
			
			if (!CheckBounds(Value.Value, fLowerLimit, fUpperLimit))
			{
				printf("Chamber[%d]:SE%d:RiMax=%f\n", ChamberId, iPartNo + 1, Value.Value);
				SetPartState(iPartNo + 1, SelectGasoline, RiOutsideLimit);
			}
		}
		else
		{
			SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
			SetPartState(iPartNo + 1, SelectGasoline, ValueNotPlausible);
		}

		SETrimmingValues.SETrimmingValuesTiepoint[iPartNo].ProcessData.TrimmingDone = true;
	}
}
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int DieselQualification( void )                                                                  *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to check diesel qualification of parts.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::DieselQualification( void )
{
	SEValueInfo Value;
	float LastValue;

	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{  

		SetPartState( i + 1, IoDiesel );																			//set part state temporarily io diesel

		//get ip ref sens 1
		ReferenceCell->GetLastValue(1, IMT_IPu, 0, &Value);
		IpRef[0] = IpCorrection( Value.Value, ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);

		//get ip ref sens 2
		ReferenceCell->GetLastValue(2, IMT_IPu, 0, &Value);
		IpRef[1] = IpCorrection( Value.Value, ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);

		//check Ip value for diesel qualification
		TrimmCell->GetLastValue( i + 1, IMT_IPu, 0, &Value );
		if( _isnan( Value.Value )	== 0 )
		{
			if( Value.Value != 0.0f )
			{
				LastValue = IpCorrectionOffsetAndDynamic( IpCorrection( Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );
				LastValue *= ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.ScaleFactorIp;

				if( SETrimmingValues.AssemblyDataCommon.ProcessType == 1 )
				{
					//selection
					if( ( LastValue < ( ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpSelectionLowerLimit / 1e6f ) ) ||
							( LastValue > ( ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpSelectionUpperLimit / 1e6f ) ) )			
					{
						SetPartState( i + 1, SelectGasoline );
					}
				}
				else
				{
					//trimming
					if( ( LastValue < ( ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpTrimmingLowerLimit / 1e6f ) ) ||
							( LastValue > ( ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpTrimmingUpperLimit / 1e6f ) ) )			
					{
						SetPartState( i + 1, SelectGasoline );
					}	
				}
				SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCheck = LastValue * 1e6f;

				
			}
		}
		else
		{
			SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
			SetPartState( i + 1, Nio, ValueNotPlausible );
		}
	
		//check Rhh value for diesel qualification
		TrimmCell->GetLastValue( i + 1, IMT_RHh, 0, &Value );
		LastValue = Value.Value;
		if( _isnan( LastValue ) == 0 )
		{
			if( ( LastValue < ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhLowerLimit ) ||
					( LastValue > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhUpperLimit ) )			
			{
				printf( "Chamber[%d]:SE%d:Rhh=%f\n", ChamberId, i+1, LastValue );
				SetPartState( i + 1, SelectGasoline );
			}
		}
		else
		{
			SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
			SetPartState( i + 1, Nio, ValueNotPlausible );
		}

		//check Ph value for diesel qualification
		TrimmCell->GetLastValue( i + 1, IMT_PH, 0, &Value );
		LastValue = Value.Value;
		if( _isnan( LastValue ) == 0 )
		{
			if( ( LastValue < ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhLowerLimit ) ||
					( LastValue > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhUpperLimit ) )			
			{
				printf( "Chamber[%d]:SE%d:Ph=%f\n", ChamberId, i+1, LastValue);
				SetPartState( i + 1, SelectGasoline );
			}
		}
		else
		{
			SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
			SetPartState( i + 1, Nio, ValueNotPlausible );
		}

		if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.SEPartStatus == IoDiesel )
		{
			SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;	
		}
		else
		{
			if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.SEPartStatus == DeactivatedCondition )
			{
				SetPartState( i + 1, Rework );
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;	
			}
		}
	}

	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingEntryCheck( void )                                                                   *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for check parts at entry of trimming.                                       *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingEntryCheck( void )
{
	int RetVal = 0;
	SEValueInfo Value;
	float LastValue;

	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.SEPartStatus != IoDiesel )
		{
			SetPartState( i + 1, SelectGasoline );

			//get ip ref sens 1
			ReferenceCell->GetLastValue(1, IMT_IPu, 0, &Value);
			IpRef[0] = IpCorrection( Value.Value, ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);

			//get ip ref sens 2
			ReferenceCell->GetLastValue(2, IMT_IPu, 0, &Value);
			IpRef[1] = IpCorrection( Value.Value, ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);

			//check Ip value for diesel qualification
			TrimmCell->GetLastValue( i + 1, IMT_IPu, 0, &Value );
			if( _isnan( Value.Value )	== 0 )
			{
				if( Value.Value != 0.0f )
				{
					LastValue = IpCorrectionOffsetAndDynamic( IpCorrection( Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );
					if( ( LastValue < ( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.StartIpLowerLimit / 1e6f ) ) ||
							( LastValue > ( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.StartIpUpperLimit / 1e6f ) ) )			
					{
						SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
						SetPartState( i + 1, Nio, IpAboveRangeAtStart );
					}
				}
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
				SetPartState( i + 1, Nio, ValueNotPlausible );
			}

			//check Rhh value
			TrimmCell->GetLastValue( i + 1, IMT_RHh, 0, &Value);
			LastValue = Value.Value;
			if( _isnan( LastValue ) == 0 )
			{
				if( ( LastValue < ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhLowerLimit ) &&
						( LastValue > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.RhhUpperLimit ) )
				{
					SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
					SetPartState( i + 1, Rework, RhhOutsideLimit );
				}
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
				SetPartState( i + 1, Nio, ValueNotPlausible );
			}
	
			//check Ph value
			TrimmCell->GetLastValue( i + 1, IMT_PH, 0, &Value);
			LastValue = Value.Value;
			if( _isnan( LastValue ) == 0 )
			{
				if( ( LastValue < ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhLowerLimit ) &&
						( LastValue > ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PhUpperLimit ) )			
				{
					SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
					SetPartState( i + 1, Rework, PhOutsideLimit );
				}
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
				SetPartState( i + 1, Nio, ValueNotPlausible );
			}
	
			TrimmCell->GetLastValue( i + 1, IMT_IPu, 0, &Value);
			if( _isnan( Value.Value )	== 0 )
			{
				if( Value.Value != 0.0f )
				{
					LastValue = IpCorrectionOffsetAndDynamic( IpCorrection( Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );

					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpLowerLimit = ( ( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.EndTestLowerTolarance / 100.0f ) + 1.0f ) * 
														( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) );
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpUpperLimit = ( ( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.EndTestUpperTolarance / 100.0f ) + 1.0f ) * 
														( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) );
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpRawLimit = ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) - 
																			( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) - LastValue ) *
																			( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLimitRawTrim / 100.0f );
					if( ( LastValue < SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpLowerLimit ) ||
							( LastValue > SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpUpperLimit ) )
					{
						if( ( LastValue > SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpRawLimit ) &&
								( LastValue < ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinIpForTrimming / 1.0e6f ) ) )			
						{
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
							SetPartState( i + 1, Rework, NotTrimmable );
						}
					}
					else
					{
						SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
						SetPartState( i + 1, IoGasoline );
					}
				}

			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
				SetPartState( i + 1, Nio, ValueNotPlausible );
			}
		}

		if( ( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.SEPartStatus == Dummy ) ||
				( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.SEPartStatus == Deactivated ) )
		{
			SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;	
		}

	}
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingEntryCheck( void )                                                                   *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for check parts at end of trimming.                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingEndCheck( void )
{
	int RetVal = 0;
	SEValueInfo Value;
	FILE *JournalDataFile = NULL;

	//stability check
	for( int j = 0; j < ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.StabilityMeasCount; j++ )
	{
		for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
		{
			//get ip trimming cell 
			TrimmCell->GetLastValue(i + 1, IMT_IPu, 0, &Value);

			if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.SEPartStatus != IoDiesel )
			{
				if( _isnan( Value.Value )	== 0 )
				{
					if( Value.Value != 0.0f )
					{
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpEndCheck = IpCorrectionOffsetAndDynamic( IpCorrection( Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );
					}
				}
				else
				{
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpEndCheck = 0;	
				}

				SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCheck = SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpEndCheck * 1e6f;
			}
		}
		if( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.StabilityMeasCount > 1 )
		{
			Sleep( (DWORD) ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.StabilityWaitTime );
		}
	}

	//end check
	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		if( ( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.SEPartStatus != IoDiesel ) &&
				( SETrimmingValues.AssemblyDataCommon.ProcessType != 1 ) )
		{
			if( ( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpEndCheck >= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpLowerLimit ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpEndCheck <= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpUpperLimit ) )
			{
				SetPartState( i + 1, IoGasoline );	
			}
			else
			{
				SetPartState( i + 1, Nio, IpOutsideLimit );
			}

			//check trimming min crossing count
			for( int j = 0; j < SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCountEntire - 1; j++ )
			{
				 if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCutCount[ j ]	 < SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCutCount[ j + 1 ] )
				 {
						if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCrossingCount[ j ] < ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinPassageLastCut ) 
						{
						 SetPartState( i + 1, Nio, MinCrossingCount );
						 break;
						}	
				 }
			}

			//check min cut count below
			if( ( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCountRaw <= 2 ) && 
				  ( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCountRaw != 0 ) )
			{
				SetPartState( i + 1, Nio, MinCutCountBelow );
			}

		}
	}

	return RetVal;
}

void WriteValueToPartsJournal(float Value)
{}

int SETrimmingChamber::SaveIpSelectionPartsJournal(ProcessType Type)
{
#if defined _INDUTRON_PRINT_MORE
	printf("##SaveIpSelectionPartsJournal( %d )\n", Type);
#endif

	int RetVal = 0;
	FILE* JournalDataFile = NULL;
	char TempString[255];


	for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
	{
		if ( ( SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus != Dummy ) ||
			 ( SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus != Deactivated ) )
		{
			int TypeNo = 0;

			switch (SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus)
			{
			case Nio:
				TypeNo = 0;
				break;

			case IoDiesel:
				TypeNo = SETrimmingValues.AssemblyDataCommon.TypeNoDs;
				break;

			case IoGasoline:
				TypeNo = SETrimmingValues.AssemblyDataCommon.TypeNoGs;
				break;

			case SelectGasoline:
			case Rework:
				TypeNo = SETrimmingValues.AssemblyDataCommon.TypeNo;
				break;

			default:
				;
				break;
			}

			//-- write statistics file
			sprintf_s(TempString, "%s%03d%04d_%1d.txt", DATA_SAVE_LOCATION, SETrimmingValues.AssemblyDataCommon.TypeNo, SETrimmingValues.AssemblyDataCommon.Charge, SETrimmingValues.AssemblyDataCommon.PartCharge);
			fopen_s(&JournalDataFile, TempString, "r");
			
			if (JournalDataFile == NULL)
			{
				fopen_s(&JournalDataFile, TempString, "a+");

				if (JournalDataFile != NULL)
				{

					const char* cHeader_IP4Sel =
						"Typ;"
						"Chargennr;"
						"WT-Nr;"
						"WT-Pos;"
						"Datum Uhrzeit;"
						"Kammer;"
						"Messmethode;"
						"Pruefergebnis;"
						"RHkalt;"
						"UHmax;"
						"IP450;"
						"UP450;"
						"UN;"
						"IP,Ref;"
						"Rhheiss;"
						"PH;"
						"Ri;"
						"RiMin;"
						"RiMax;"
						"IP450,2Pkt;"
						"UP450,2Pkt;"
						"IP1;"
						"UP1;"
						"UN1;"
						"IP,Ref1;"
						"Rhheiss1;"
						"PH1;"
						"Ri1;"
						"IP2;"
						"UP2;"
						"UN2;"
						"IP,Ref2;"
						"Rhheiss2;"
						"PH2;"
						"Ri2;"
						"RiMin;"
						"RiMax;"
						"IP,ref.sensor1_vor;"
						"IP,ref.sensor1_nach;"
						"IP,ref.sensor2_vor;"
						"IP,ref.sensor2_nach;"
						"Ref1;"
						"Ref2;"
						"IP untere Grenze Selektion;"
						"IP obere Grenze Selektion;"
						"Ipref, target;"
						"Ri, target;"
						"Ph - SRi;"
						"Uhmax, Regler;"
						"Heizzeit;"
						"UN, target;"
						"UP, max;"
						"k - Wert;"
						"Abs. Zeitoffset Pumpspannung UP1 ein;"
						"UP1 Zielwert;"
						"UP1 min;"
						"UP1 max;"
						"Integrationszeit IP1 und UN1;"
						"UP2 Zielwert;"
						"UP2 min;"
						"UP2 max;"
						"Wartezeit vor IP2 und UN2 Messung;"
						"Integrationszeit IP2 und UN2;"
						"Refsens beacht;"
						"dIP / IP, max;"
						"P - Vac;"
						"P - Atmos;"
						"P - Soll;"
						"P - Ist;"
						"Leckrate;"
						"Massenstrom;"
						"IP - Offset;"
						"SE in Selektionstoleranz;"
						"SE außer Selektionstoleranz (IP450);"
						"Pumpstrom (IP450) nicht stabil;"
						"Kont. HZ/SE oder Ri;"
						"Heizerkaltwiderstand;"
						"Innenwiderstand AC;"
						"Heizerspannung;"
						"Nernstspannung;"
						"Pumpspannung;"
						"Referenzstrom (Ip,ref);"
						"Heizleistung;"
						"Heizerheißwiderstand;"
						"Leckagerate;"
						"Evakuierungsdruck;"
						"Durchfluss;"
						"Druck;"
						"Gasüberwachung;"
						"Referenzsonde;"
						"Plausibilitätsprüfung;"
						"Software Version;"
						"\n";

					fprintf_s(JournalDataFile, cHeader_IP4Sel);

					const char* cUnits_IP4Sel =
						";"
						";"
						";"
						";"
						";"
						";"
						";"
						";"
						"Ohm;"
						"V;"
						"uA;"
						"mV;"
						"mV;"
						"uA;"
						"Ohm;"
						"Watt;"
						"Ohm;"
						"Ohm;"
						"Ohm;"
						"uA;"
						"mV;"
						"uA;"
						"mV;"
						"mV;"
						"uA;"
						"Ohm;"
						"Watt;"
						"Ohm;"
						"uA;"
						"mV;"
						"mV;"
						"uA;"
						"Ohm;"
						"Watt;"
						"Ohm;"
						"Ohm;"
						"Ohm;"
						"uA;"
						"uA;"
						"uA;"
						"uA;"
						";"
						";"
						"uA;"
						"uA;"
						"uA;"
						"Ohm;"
						"Watt;"
						"V;"
						"s;"
						"mV;"
						"mV;"
						"mbar;"
						"s;"
						"mV;"
						"mV;"
						"mV;"
						"s;"
						"mV;"
						"mV;"
						"mV;"
						"s;"
						"s;"
						";"
						"%%;"
						"mbar;"
						"mbar;"
						"mbar;"
						"mbar;"
						"mbar/s;"
						"l/min;"
						"uA;"
						";"";"";"";"";"";"";"";"";"";"";"";"";"";"";"";"";"";"";"";"
						"\n";

					fprintf_s(JournalDataFile, cUnits_IP4Sel);

					fflush(JournalDataFile);

					fclose(JournalDataFile);

					JournalDataFile = NULL;
				}
			}
			else
			{
				fflush(JournalDataFile);
				fclose(JournalDataFile);		//close file
				JournalDataFile = NULL;
			}

			fopen_s(&JournalDataFile, TempString, "a+");

			if (JournalDataFile != NULL)
			{
				SYSTEMTIME	Time;
				GetLocalTime(&Time);

				SEValueInfo Value;
				//typ number
				fprintf_s(JournalDataFile, "%03d;", TypeNo);
				//charge number
				fprintf_s(JournalDataFile, "%06d;", SETrimmingValues.AssemblyDataCommon.Charge);
				//wpc number
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.AssemblyDataCommon.WpcNo);
				//wpc pos
				fprintf_s(JournalDataFile, "%d;", i + 1);
				//date time
				fprintf_s(JournalDataFile, "%02d.%02d.%02d %02d:%02d:%02d;", Time.wDay, Time.wMonth, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond);
				//chamber
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.AssemblyDataCommon.ChamberNumber);
				
				//process type
				if ( Type == ProcessType::IP450MeasUNernstControl)
				{
					fprintf_s(JournalDataFile, "Un;");
				}
				else //if ( Type == ProcessType::Ip4Measure2PointUp )
				{
					fprintf_s(JournalDataFile, "2Pkt;");
				}
				
				//nio rework reason
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartNioReworkReason); 

				//heater cold resistance
				TrimmCell->GetLastValue(i + 1, IMT_RHk, 0, &Value);
				if ( _isnan(Value.Value) == 0 )
				{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value); //last rhk
				}
				else
				{
					fprintf_s(JournalDataFile, "NaN;"); //last rhk
				}
				
				//UHmax
				if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUhMax) == 0)
				{
					fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUhMax);
				}
				else
				{
					fprintf_s(JournalDataFile, "NaN;");
				}

				// Nernst voltage control
				if( Type == ProcessType::IP450MeasUNernstControl)
				{
					//IP450 
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCheck) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCheck);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//UP450
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUApeAtIPu) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUApeAtIPu);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//UN
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUNernst) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUNernst); // 		
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//IP,Ref
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIRe1) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIRe1);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//Rhh
					TrimmCell->GetLastValue(i + 1, IMT_RHh, 101, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//Ph
					TrimmCell->GetLastValue(i + 1, IMT_PH, 101, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//Riac
					TrimmCell->GetLastValue(i + 1, IMT_RiAC, 0, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//min Riac (value of ri monitoring)
					TrimmCell->GetLastValue(i + 1, IMT_RiMin, 0, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//max riac (value of ri monitoring)
					TrimmCell->GetLastValue(i + 1, IMT_RiMax, 0, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//no entry for 2Pkt values
					for (int i = 0; i < 18; i++)
					{
						fprintf_s(JournalDataFile, ";");
					}
				}
				else //if ( Type == ProcessType::Ip4Measure2PointUp )
				{
					//no entry for UNernst values
					for (int i = 0; i < 9; i++)
					{
						fprintf_s(JournalDataFile, ";");
					}
					// point of measurement 1
					//IP450,2Pkt
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCheck) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCheck);
					}
					else
					{
					}
					//UP450,2Pkt
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUApeAtIPu) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fUApeAtIPu);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//IP1 (pump current point 1)
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIpPoint1) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIpPoint1 * 1e6f);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//UP1 (pump voltage point 1)
					TrimmCell->GetLastValue(i + 1, IMT_UAPE, 101, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", Value.Value * 1e3f);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//UN1 (nernst voltage point 1)
					TrimmCell->GetLastValue(i + 1, IMT_URE, 101, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", Value.Value * 1e3f);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//IP,Ref1 (reference pump current point 1)
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIRe1) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIRe1);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//RHh1 (heater hot resistance point 1)
					TrimmCell->GetLastValue(i + 1, IMT_RHh, 101, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.3f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//PH1 (heater power point 1)
					TrimmCell->GetLastValue(i + 1, IMT_PH, 101, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.3f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//Ri1 
					TrimmCell->GetLastValue(i + 1, IMT_RiAC, 101, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
				
					// point of measurement 2
					//IP2 (pump current point 2)
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIpPoint1) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIpPoint2 * 1e6f);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//UP2 (pump voltage point 2)
					TrimmCell->GetLastValue(i + 1, IMT_UAPE, 102, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", Value.Value * 1e3f);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//UN2 (nernst voltage point2)
					TrimmCell->GetLastValue(i + 1, IMT_URE, 102, &Value);
					if (_isnan(Value.Value) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", Value.Value * 1e3f);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//IP,Ref2
					if (_isnan(SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIRe2) == 0)
					{
						fprintf_s(JournalDataFile, "%.1f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.fIRe2);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//RHhe
					TrimmCell->GetLastValue(i + 1, IMT_RHh, 102, &Value);
					if (_isnan(Value.Value) == 0)
					{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//PH2
					TrimmCell->GetLastValue(i + 1, IMT_PH, 102, &Value);
					if (_isnan(Value.Value) == 0)
					{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//Ri2
					TrimmCell->GetLastValue(i + 1, IMT_RiAC, 102, &Value);
					if (_isnan(Value.Value) == 0)
					{
					fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//min riac from ri monitoring
					TrimmCell->GetLastValue(i + 1, IMT_RiMin, 0, &Value);
					if (_isnan(Value.Value) == 0)
					{
					fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
					//max riac from ri monitoring
					TrimmCell->GetLastValue(i + 1, IMT_RiMax, 0, &Value);
					if (_isnan(Value.Value) == 0)
					{
					fprintf_s(JournalDataFile, "%.2f;", Value.Value);
					}
					else
					{
						fprintf_s(JournalDataFile, "NaN;");
					}
				}

				//ip reference 1 before process
				fprintf_s(JournalDataFile, "%.1f;", IpRefBeforeProcess[0]); 
				//ip reference 1 after process
				fprintf_s(JournalDataFile, "%.1f;", IpRefAfterProcess[0]);
				//ip reference 2 before process
				fprintf_s(JournalDataFile, "%.1f;", IpRefBeforeProcess[1]);	
				//ip reference 2 after process
				fprintf_s(JournalDataFile, "%.1f;", IpRefAfterProcess[1]); 

				//reference 1
				fprintf_s(JournalDataFile, "%.1f/%.1f/%.1f//%.3f/%.3f/%.0f;", 	
					//reference 1: ip setpoint
					ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[0],	
					//reference 1: ip ref
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.IpRef,		
					//reference 1: ri
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.HeatingRi,		
					//reference 1: nernst voltage
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.NernstVoltage,	
					//reference 1: pump voltage		
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.PumpVoltage,
					//K-Value
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);
			    
				//reference 2
				fprintf_s(JournalDataFile, "%.1f/%.1f/%.1f//%.3f/%.3f/%.0f;", 		
					//reference 2: ip setpoint
					ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[1],
					//reference 2: ip ref
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.IpRef,	
					//reference 2: ri
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.HeatingRi,	
					//reference 2: nernst voltage
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.NernstVoltage,	
					//reference 2: pump voltage	
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.PumpVoltage,
				    //K-Value ADV probe
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);
	
				//parameter from type data:

				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpSelectionLowerLimit);
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamDieselQualification.IpSelectionUpperLimit);

				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.IpRef); //ip ref

				if (SETrimmingValues.AssemblyDataCommon.ProcessType == 1)
				{
					fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiTrimming); //heating 
				}
				else
				{
					fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiSelection); //heating
				}
                //parameter from type data 
				//heating power
				fprintf_s(JournalDataFile, "%.2f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingPower);
				//heating voltage
				fprintf_s(JournalDataFile, "%.2f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingVoltage); 
				//heating time
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingTime); 
				//nernst voltage
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.NernstVoltage *1e3f); 
				//pump voltage
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.PumpVoltage * 1e3f); 
				//k value
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue); 
				//type data 2 point measurement
				fprintf_s(JournalDataFile, "%.2f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fTimeStartUp1);
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp1Target * 1e3f);
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp1Min * 1e3f);
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp1Max * 1e3f);
				fprintf_s(JournalDataFile, "%.2f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fIntTimeMeasPoint1);
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp2Target * 1e3f);
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp2Min * 1e3f);
				fprintf_s(JournalDataFile, "%.1f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fUp2Max * 1e3f);
				fprintf_s(JournalDataFile, "%.2f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fWaitingTimeIp2Un2Meas);
				fprintf_s(JournalDataFile, "%.2f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.fIntTimeMeasPoint2);
				//ip correction mode
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpCorrectionMode); 
				//ip stable
				fprintf_s(JournalDataFile, "%.2f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpStable);  
				//evacuated pressure
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.AssemblyDataCommon.EvacuatedPressure);
				//pressure atmosphere
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.AssemblyDataCommon.AtmospherePressure); 
				//pressure setpoint
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint); 
				//actual pressure chamber
				fprintf_s(JournalDataFile, "%.3f;", ChamberPressureActual); 
				//leackage rate
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.AssemblyDataCommon.LeakageRate);
				//mass flow			
				fprintf_s(JournalDataFile, "%.3f;", ChamberFlowActual); 
				//ip offset GS 
				fprintf_s(JournalDataFile, "%.1f;", ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationTrimmingProcess.IpOffsetGasoline);

				fprintf_s(JournalDataFile, "0;1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;");

				FileVersionGet(TempString, sizeof(TempString), "SELaserTrimming.exe");
				//sw version
				fprintf_s(JournalDataFile, "%s;", TempString);		

				fprintf_s(JournalDataFile, "\n");

				fflush(JournalDataFile);

				fclose(JournalDataFile);
				JournalDataFile = NULL;
			}
			//-- write statistics file end
		}

	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SavePartsJournal( void )                                                                     *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to save the statistics to local hard disc.                                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::SavePartsJournal(void)
{
	int RetVal = 0;
	FILE* JournalDataFile = NULL;
	char TempString[255];

	for (int i = 0; i < CELL_TIEPOINT_COUNT; i++)
	{
		if ((SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus != Dummy) ||
			(SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus != Deactivated))
		{
			int TypeNo = 0;

			switch (SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus)
			{
			case Nio:
				TypeNo = 0;
				break;

			case IoDiesel:
				TypeNo = SETrimmingValues.AssemblyDataCommon.TypeNoDs;
				break;

			case IoGasoline:
				TypeNo = SETrimmingValues.AssemblyDataCommon.TypeNoGs;
				break;

			case SelectGasoline:
			case Rework:
				TypeNo = SETrimmingValues.AssemblyDataCommon.TypeNo;
				break;

			default:
				;
				break;
			}

			//-- write statistics file
			sprintf_s(TempString, "%s%03d%04d_%1d.txt", DATA_SAVE_LOCATION, SETrimmingValues.AssemblyDataCommon.TypeNo, SETrimmingValues.AssemblyDataCommon.Charge, SETrimmingValues.AssemblyDataCommon.PartCharge);

			fopen_s(&JournalDataFile, TempString, "r");
			if (JournalDataFile == NULL)
			{
				fopen_s(&JournalDataFile, TempString, "a+");
				if (JournalDataFile != NULL)
				{
#ifdef LOGGING_OLD_FORMAT
					fprintf_s(JournalDataFile, "Typ;Chargennr;Code;Seriennummer;WT-Nr;WT-Pos;Datum Uhrzeit;Kammer;Ref1;"
						"Ref2;Breite;Länge;Spalt;Vorhalt fein;Startabst grob;Startabst fein;Step1 grob;Step1 fein;Ipref;"
						"Ri;Pheiz;Uheiz;Heizzeit;Unernst;Pause;Upump;MRG O2;Luft O2;K-Wert;"
						"PSoll trim;PSoll mess;Refsens beacht;Grenze IP stabil;min-Zeit-cuts;Wartezeit stabil;Rauschgrenze;max Anz überf;dIP Schichtend;Grob-Grenze;"
						"Fein-Grenze;Interpol Grob;Interpol Fein; 1/X C Grob;1/X C Fein;L o 1/X;Tol Endtest;P-Laser vor;P-Laser innen;P-Laserprozess;"
						"P-Vac;P-Atmos;P-Soll;P-Ist;Leckrate;Massenstrom;Vorschub;IP-Start;IP-End;IP-Soll;"
						"Anz. Schnitte;Aver Anz. Überfahrten;Error_Nummer;WT-Info;IP_Ref1_vor;IP_Ref2_vor;IP_Ref1_nach;IP_Ref2_nach;Stab. Uhrzeit;Stab. Druck;"
						"Stab. IP;...\n");
					fprintf_s(JournalDataFile, ";;;;;;;;;"
						";mm;mm;mm;mm;mm;mm;mm;mm;µA;"
						"Ω;W;V;s;V;s;V;Vol%%;Vol%%;mbar;"
						"mbar;mbar;;%%;ms;ms;µA;;%%;%%;"
						"%%;;;;;01;%%;W;W;;"
						"mbar;mbar;mbar;mbar;mbar/s;l/min;mm/s;µA;µA;µA;"
						";;;;µA;µA;µA;µA;;mbar;"
						"µA;...\n");
#else
					fprintf_s(JournalDataFile, "Typ;Chargennr;Code;Seriennummer;WT-Nr;WT-Pos;Datum Uhrzeit;Kammer;Ref1;"
						"Ref2;Breite;Länge;Spalt;Vorhalt fein;Startabst grob;Startabst fein;Step1 grob;Step1 fein;Ipref;"
						"Ri;Pheiz;Uheiz;Heizzeit;Unernst;Pause;Upump;MRG O2;Luft O2;K-Wert;"
						"PSoll trim;PSoll mess;Refsens beacht;Grenze IP stabil;min-Zeit-cuts;Wartezeit stabil;Rauschgrenze;max Anz überf;dIP Schichtend;Grob-Grenze;"
						"Fein-Grenze;Interpol Grob;Interpol Fein; 1/X C Grob;1/X C Fein;L o 1/X;Tol Endtest;P-Laser vor;P-Laser innen;P-Laserprozess;"
						"P-Vac;P-Atmos;P-Soll;P-Ist;Leckrate;Massenstrom;Vorschub;IP-Start;IP-End;IP-Soll;"
						"Anz. Schnitte;Aver Anz. Überfahrten;Error_Nummer;WT-Info;IP_Ref1_vor;IP_Ref2_vor;IP_Ref1_nach;IP_Ref2_nach;Stab. Uhrzeit;Stab. Druck;"
						"Stab. IP;Stab. vorher Uhrzeit;RH;PH;Ri;IP-Offset GS;rel. IP-Kor;IP-Offset DS;dIP AS?;dt U-Ri-Reg.;"
						"E U-Ri-Reg.;dIH/dt;IH_t1;dRH/dt;RH_t1;RHk;Diff_min;Abs. Lochrand;Zahl zus Schn.;Hub Grobabgl.;"
						"tiefe Schn. fein;dIP AS fein?;min Ueberf. fein;SW Ver;Rep.Rate aktiv;min. Rep.Rate;Rep.Rate;min. Laser Ampl.;Laser Ampl.;min.Anz.LetztGrob.\n");
					fprintf_s(JournalDataFile, ";;;;;;;;;"
						";mm;mm;mm;mm;mm;mm;mm;mm;µA;"
						"Ohm;W;V;s;V;s;V;Vol%%;Vol%%;mbar;"
						"mbar;mbar;;%%;ms;ms;µA;;%%;%%;"
						"%%;;;;;01;%%;W;W;;"
						"mbar;mbar;mbar;mbar;mbar/s;l/min;mm/s;µA;µA;µA;"
						";;;;µA;µA;µA;µA;;mbar;"
						"µA;;Ohm;Watt;Ohm;µA;;µA;%%;s;"
						"VAs;A/s;A;Ohm/s;Ohm;Ohm;mm;mm;;%%;"
						";;%%;;;;Hz;Hz;;\n");
#endif
					fflush(JournalDataFile);
					fclose(JournalDataFile);
					JournalDataFile = NULL;
				}
			}
			else
			{
				fflush(JournalDataFile);
				fclose(JournalDataFile);																				//close file
				JournalDataFile = NULL;
			}

			fopen_s(&JournalDataFile, TempString, "a+");
			if (JournalDataFile != NULL)
			{
				SYSTEMTIME	Time;
				GetLocalTime(&Time);
				SEValueInfo Value;


				fprintf_s(JournalDataFile, "%03d;", TypeNo);																								  //typ number
				fprintf_s(JournalDataFile, "%06d;", SETrimmingValues.AssemblyDataCommon.Charge);						  //charge number
				fprintf_s(JournalDataFile, "%s;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SerialCode);                 //serial code
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SerialNumber);							 //serial number
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.AssemblyDataCommon.WpcNo);							    //wpc number
				fprintf_s(JournalDataFile, "%d;", i + 1);																										  //wpc pos
				fprintf_s(JournalDataFile, "%02d.%02d.%02d %02d:%02d:%02d;", Time.wDay, Time.wMonth, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond); //date time
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.AssemblyDataCommon.ChamberNumber);			    //chamber
				fprintf_s(JournalDataFile, "%.3f/%.3f/%.3f//%.2f/%.2f;", 																			//reference 1
					ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[0],	//reference 1: ip setpoint
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.IpRef,																	//reference 1: ip ref
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.HeatingRi,															//reference 1: ri
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.NernstVoltage,													//reference 1: nernst voltage
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.PumpVoltage);													//reference 1: pump voltage		

				fprintf_s(JournalDataFile, "%.3f/%.3f/%.3f//%.2f/%.2f;", 																			//reference 2
					ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[1],	//reference 2: ip setpoint
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.IpRef,																	//reference 2: ip ref
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.HeatingRi,															//reference 2: ri
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.NernstVoltage,													//reference 2: nernst voltage
					ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.PumpVoltage);													//reference 2: pump voltage	
				fprintf_s(JournalDataFile, "%.3f;", ImageProcessing[i].Diameter);														//width
				fprintf_s(JournalDataFile, "%.3f;", ImageProcessing[i].Diameter);														//length
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Fissure); //fissure
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.HoldBackPositionFineTrim); //hold back position fine
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.StartDistanceRawTrim); //start distance raw trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.StartDistanceFineTrim); //start distance fine trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Step1RawTrim); //step 1 raw trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Step1FineTrim); //step 1 fine trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.IpRef); //ip ref
				if (SETrimmingValues.AssemblyDataCommon.ProcessType == 1)
				{
					fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiSelection); //heating 
				}
				else
				{
					fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingRiTrimming); //heating
				}
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingPower); //heating power
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingVoltage); //heating voltage
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.HeatingTime); //heating time
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.NernstVoltage); //nernst voltage
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.PauseTime); //pause time
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.PumpVoltage); //pump voltage
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.O2Mrg); //o2 mrg
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.O2Air); //o2 air
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue); //k value

				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint); //pressure setpoint
				fprintf_s(JournalDataFile, "0;");																	//not used
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpCorrectionMode); //ip correction mode
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpStable);   //ip stable
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.MinCutTime); //min cut time
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.StabilityWaitTime); //stability wait time
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpNoise); //delta ip noise
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MaxPassage); //max passage
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLayerEnd); //delta ip layer end
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLimitRawTrim); //delta ip limit raw trimming

				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLimitFineTrim); //delta ip limit fine trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.InterpolationFactorRawTrim); //interpolation factor raw trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.InterpolationFactorFineTrim); //interpolation factor fine trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.FaktorCRawTrim); //factor c raw trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.FaktorCFineTrim); //factor c fine trimming
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.Linear1X); //linear or 1/x
				fprintf_s(JournalDataFile, "%.3f;", (abs(ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.EndTestUpperTolarance) + abs(ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.EndTestUpperTolarance)) / 2.0f);
				fprintf_s(JournalDataFile, "0;"); //laser power upper sensor
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.AssemblyDataCommon.LaserPower); //laser power lower sensor
				fprintf_s(JournalDataFile, "%.3f;", (100.0f / SETrimmingValues.AssemblyDataCommon.AdjustPollutePercent) * ParamTrimmChamber.ParamTrimmProcess.ParamLaser.LaserPower[i]); //laser power process

				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.AssemblyDataCommon.EvacuatedPressure); //evacuated pressure
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.AssemblyDataCommon.AtmospherePressure); //pressure atmosphere
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint); //pressure setpoint
				fprintf_s(JournalDataFile, "%.3f;", ChamberPressureActual); //actual pressure chamber
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.AssemblyDataCommon.LeakageRate); //leackage rate
				fprintf_s(JournalDataFile, "%.3f;", ChamberFlowActual); //mass flow
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamLaser.ScannerSpeed); //laser speed
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpStart); //ip first cut
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCut); //ip last cut
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint); //ip setpoint

				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.CutCount); //cut count
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.AverageCrossingCount); //cut count
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartNioReworkReason); //nio rework reason
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SEPartStatus); //part status
				fprintf_s(JournalDataFile, "%.3f;", IpRefBeforeProcess[0]); //ip reference 1 before process
				fprintf_s(JournalDataFile, "%.3f;", IpRefBeforeProcess[1]); //ip reference 1 before process
				fprintf_s(JournalDataFile, "%.3f;", IpRefAfterProcess[0]); //ip reference 2 after process
				fprintf_s(JournalDataFile, "%.3f;", IpRefAfterProcess[1]); //ip reference 2 after process
				fprintf_s(JournalDataFile, "0;"); //not used ( formally: stability time )
				fprintf_s(JournalDataFile, "0;"); //not used ( formally: stability pressure )
				fprintf_s(JournalDataFile, "%.3f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCheck); //ip end check

#ifndef LOGGING_OLD_FORMAT
				fprintf_s(JournalDataFile, "0;"); //not used ( formally: stability time previous )
				TrimmCell->GetLastValue(i + 1, IMT_RHh, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.2f;", Value.Value); //last rhh
				}

				TrimmCell->GetLastValue(i + 1, IMT_PH, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.2f;", Value.Value); //last ph
				}

				TrimmCell->GetLastValue(i + 1, IMT_RiAC, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.2f;", Value.Value); //last riac
				}
				fprintf_s(JournalDataFile, "%.3f;", ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationTrimmingProcess.IpOffsetGasoline); //ip offset gs
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpCorrectionSource); //ip correction source
				fprintf_s(JournalDataFile, "%.3f;", ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationTrimmingProcess.IpOffsetDiesel); //ip offset ds
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLayerEndScrap); //delta layer end scrap
				TrimmCell->GetLastValue(i + 1, IMT_dtRiReg, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value); //last ri reg dt
				}
				TrimmCell->GetLastValue(i + 1, IMT_EH, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value); //last eh
				}
				TrimmCell->GetLastValue(i + 1, IMT_dIHdt, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value); //last eh
				}
				fprintf_s(JournalDataFile, "0;");																	//not used (formally: ih t1)
				TrimmCell->GetLastValue(i + 1, IMT_dRHdt, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value); //last rh dt
				}
				fprintf_s(JournalDataFile, "0;");																	//not used (formally: rh t1)
				TrimmCell->GetLastValue(i + 1, IMT_RHk, 0, &Value);
				if (_isnan(Value.Value) == 0)
				{
					fprintf_s(JournalDataFile, "%.3f;", Value.Value); //last rhk
				}
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance); //min cut distance
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinDistanceToEdgeForAdditionalCuts); //min distance for additional cuts
				fprintf_s(JournalDataFile, "0;");	//additional cuts allowed (todo bz)
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinDeviationRawTrim); //min deviation
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeepCutsFineTrim); //deep cuts fine trimming
				fprintf_s(JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLastFineTrim); //delta ip last fin trimming
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinPassageFineTrim); //min passage fine trimming
				FileVersionGet(TempString, sizeof(TempString), "SELaserTrimming.exe");
				fprintf_s(JournalDataFile, "%s;", TempString);	//sw version
				fprintf_s(JournalDataFile, "0;");	//not used (formally: use lasermonitor)
				fprintf_s(JournalDataFile, "0;");	//not used (formally: lasermonitor)
				fprintf_s(JournalDataFile, "0;");	//not used (formally: lasermonitor)
				fprintf_s(JournalDataFile, "0;");	//not used (formally: lasermonitor)
				fprintf_s(JournalDataFile, "0;");	//not used (formally: lasermonitor)
				fprintf_s(JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinPassageLastCut); //min passage last cut
				/*fprintf_s( JournalDataFile, "%d;", ParamTrimmChamber.ParamTrimmProcess.ParamLaser.WobbleEnable ); //wobble enable
				fprintf_s( JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamLaser.WobbleFrequency ); //wobble frequency
				fprintf_s( JournalDataFile, "%.3f;", ParamTrimmChamber.ParamTrimmProcess.ParamLaser.WobbleTransversal ); //wobble amp
				fprintf_s( JournalDataFile, "0;" );																//not used (formally: wobble ramp)
				fprintf_s( JournalDataFile, "0;" );																//not used (formally: wobble repeat)
				fprintf_s( JournalDataFile, "0;" );																//not used (formally: heating image processing)*/
#endif

				fprintf_s(JournalDataFile, "\n");
				fflush(JournalDataFile);
				fclose(JournalDataFile);
				JournalDataFile = NULL;
			}
			//-- write statistics file end


			//-- write cut file
			sprintf_s(TempString, "%s%03d%04d_%1d_cuts.txt", DATA_SAVE_LOCATION, SETrimmingValues.AssemblyDataCommon.TypeNo, SETrimmingValues.AssemblyDataCommon.Charge, SETrimmingValues.AssemblyDataCommon.PartCharge);

			fopen_s(&JournalDataFile, TempString, "r");
			if (JournalDataFile == NULL)
			{
				fopen_s(&JournalDataFile, TempString, "a+");
				if (JournalDataFile != NULL)
				{
					fprintf_s(JournalDataFile, "Typ;Chargennr;Laminat;SE-Nr;WT-Nr;WT-Pos;Datum;Uhrzeit;Bediener;[StartX;StartY;IP-ist;P-Kammer;Schnitt-Nr;Überfahrt];[...]\n");
					fflush(JournalDataFile);
					fclose(JournalDataFile);
					JournalDataFile = NULL;
				}
			}
			else
			{
				fflush(JournalDataFile);
				fclose(JournalDataFile);																				//close file
				JournalDataFile = NULL;
			}

			fopen_s(&JournalDataFile, TempString, "a+");
			if (JournalDataFile != NULL)
			{
				SYSTEMTIME	Time;
				GetLocalTime(&Time);

				fprintf_s(JournalDataFile, "%03d;", TypeNo);																	//typ number
				fprintf_s(JournalDataFile, "%06d;", SETrimmingValues.AssemblyDataCommon.Charge);						//charge number
				fprintf_s(JournalDataFile, "%s;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SerialCode);	//serial code
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.SerialNumber);	//serial number
				fprintf_s(JournalDataFile, "%d;", SETrimmingValues.AssemblyDataCommon.WpcNo);							 //wpc number
				fprintf_s(JournalDataFile, "%d;", i + 1);																											//wp pos
				fprintf_s(JournalDataFile, "%02d.%02d.%02d;", Time.wDay, Time.wMonth, Time.wYear);				    //date
				fprintf_s(JournalDataFile, "%02d:%02d:%02d;", Time.wHour, Time.wMinute, Time.wSecond);				//time
				fprintf_s(JournalDataFile, "0;");																															//user
				for (int j = 0; j <= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1; j++)
				{
					fprintf_s(JournalDataFile, "%.2f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingCutXPos[j]);
					fprintf_s(JournalDataFile, "%.2f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingCutYPos[j]);
					fprintf_s(JournalDataFile, "%.2f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingCutIp[j]);
					fprintf_s(JournalDataFile, "%.2f;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingCutChamberPressure[j]);
					fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingCutCount[j]);
					fprintf_s(JournalDataFile, "%d;", SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingCrossingCount[j]);
				}
				fprintf_s(JournalDataFile, "\n");
				fflush(JournalDataFile);
				fclose(JournalDataFile);
				JournalDataFile = NULL;
			}
			//-- write cut file
		}

	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SaveNormalJournal( void )                                                                    *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to save the statistics of normal to local hard disc.                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::SaveNormalJournal( void )
{
	int RetVal = 0;
	FILE *JournalDataFile = NULL;
	int FileHandler;
	char TempString[255];
	char TempString2[255];
	bool NewFile = false ;

	sprintf_s(TempString, "%sGBR_Messungen.txt", DATA_SAVE_LOCATION );

	//open file
	if( _sopen_s( &FileHandler, TempString, _O_RDONLY, _SH_DENYNO, _S_IREAD ) == 0 )
	{
		//check file length
		if( _filelength( FileHandler ) >= 5000000)
		{
			NewFile = true;
		}
		_close( FileHandler );																								//close file
	}

	//check new file need
	if( NewFile == true )		
	{
		SYSTEMTIME	Time;
		GetLocalTime( &Time );

		sprintf_s(TempString2, "%sGBR_Messungen_bis_%04d%02d%02d.txt", DATA_SAVE_LOCATION, Time.wYear, Time.wMonth, Time.wDay );
		rename(TempString, TempString2);																			//rename old file
	}
	
	fopen_s( &JournalDataFile, TempString, "r" );
	if( JournalDataFile == NULL )
	{
		fopen_s( &JournalDataFile, TempString, "a+" );
		if( JournalDataFile != NULL )
		{
			fprintf_s( JournalDataFile, "GbR-Typ;DatumUhrzeit;WT-Pos;"
										"RiDcpUG;RiDcpOG;RiDcp;RiDcnUG;RiDcnOG;RiDcn;IpContUG;IpContOG;IpCont;IhUG;IhOG;Ih;RhhUG;RhhOG;Rhh;"
										"ContaktUG;ContaktOG;Contakt;RhhUG;RhhOG;Rhh;IhUG;IhOG;Ih;RhKaltUG;RhkaltOG;RhKalt;"
										"IgrkUG;IgrkOG;Igrk;IlUG;IlOG;Il;RiDcnUG;RiDcnOG;RiDcn;IpreUG;IpreOG;Ipre;RiacUG;RiacOG,Riac;RiacStatUG;RiacStatOG;RiacStat;"
										"UnUG;UnOG;Un;"
										"UpUG;UpOG;Up;IpUG;IpOG;Ip;"
										"IlMaxUG;IlMaxOG;IlMax;IaIpnUG;IaIpnOG;IaIpn;Ip2UG;Ip2OG;Ip2;Un2UG;Un2OG;Un2;Ip3UG;Ip3OG;Ip3;Un3UG;Un3OG;Un3;Ip4UG;Ip4OG;Ip4;Un4UG;Un4OG;Un4;\n" );
			fprintf_s( JournalDataFile, ";;;"
										"Ohm;Ohm;Ohm;Ohm;Ohm;Ohm;µA;µA;µA;mA;mA;mA;Ohm;Ohm;Ohm;"
										"1;1;1;Ohm;Ohm;Ohm;A;A;A;Ohm;Ohm;Ohm;"
										"µA;µA;µA;µA;µA;µA;Ohm;Ohm;Ohm;µA;µA;µA;Ohm;Ohm;Ohm;Ohm;Ohm;Ohm;"
										"V;V;V;"
										"V;V;V;mA;mA;mA;"
										"µA;µA;µA;mA;mA;mA;mA;mA;mA;mV;mV;mV;mA;mA;mA;mV;mV;mV;mA;mA;mA;mV;mV;mV;\n" ); 
			fflush(	JournalDataFile );
			fclose(	JournalDataFile );
			JournalDataFile = NULL;
		}
	}
	else
	{
		fflush(	JournalDataFile );
		fclose(	JournalDataFile );																				//close file
		JournalDataFile = NULL;	
	}

	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		fopen_s( &JournalDataFile, TempString, "a+" );
		if( JournalDataFile != NULL )
		{
			SYSTEMTIME	Time;
			GetLocalTime( &Time );


			switch ( eProcessType )
			{
			case ProcessType::MeasurePositionNormal:
				fprintf_s(JournalDataFile, "Nr 1: Positionsnormal;");
				break;

			case ProcessType::MeasureHeaterNormal:
				fprintf_s(JournalDataFile, "Nr 2: Heizernormal;");
				break;

			case ProcessType::MeasureUniversalNormal:
				fprintf_s(JournalDataFile, "Nr 3: Universalnormal;");
				break;

			case ProcessType::MeasureUnNormal:
				fprintf_s(JournalDataFile, "Nr 4: Un-Normal;");
				break;

			case ProcessType::MeasureIpNormal:
				fprintf_s(JournalDataFile, "Nr 5: Ip-Normal;");
				break;

			case ProcessType::MeasureIlmNormal:
				fprintf_s(JournalDataFile, "Nr 6: Ilm-Mormal;");
				break;
			}

			fprintf_s( JournalDataFile, "%02d.%02d.%02d %02d:%02d:%02d;", Time.wDay, Time.wMonth, Time.wYear, Time.wHour, Time.wMinute, Time.wSecond ); //date time
			fprintf_s( JournalDataFile, "%d;", i + 1 );			
				
			if( eProcessType == ProcessType::MeasurePositionNormal )
			{
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamPositionNormal.ChkSensor[i].LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamPositionNormal.ChkSensor[i].UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataPosition.RiDcp );
				fprintf_s( JournalDataFile, "180,0;" );
				fprintf_s( JournalDataFile, "280,0;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataPosition.RiDcn );
				fprintf_s( JournalDataFile, "0,20;" );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataPosition.IpCont );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataPosition.Ih );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamPositionNormal.ChkHeater[i].LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamPositionNormal.ChkHeater[i].UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataPosition.Rhhot );
			}
			else
			{
				fprintf_s( JournalDataFile, ";;;;;;;;;;;;;;;" );
			}

			if( eProcessType == ProcessType::MeasureHeaterNormal )
			{
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "0,99;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataHeater.ContactOk );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceHot.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceHot.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataHeater.Rhhot );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamHeaterNormal.ChkHeaterCurrent.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamHeaterNormal.ChkHeaterCurrent.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataHeater.Ih );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceCold.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamHeaterNormal.ChkHeaterResistanceCold.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataHeater.RhCold );
			}
			else
			{
				fprintf_s( JournalDataFile, ";;;;;;;;;;;;" );
			}

			if( eProcessType == ProcessType::MeasureUniversalNormal )
			{
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkIgrk.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkIgrk.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataUniversal.Igrk );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkIl.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkIl.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataUniversal.Il );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkRidc.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkRidc.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataUniversal.RiDcn );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkIpre.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkIpre.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataUniversal.IpRe );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkRiac.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkRiac.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataUniversal.RiAc );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkRiacStat.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUniversalNormal.ChkRiacStat.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataUniversal.RiAcstat );
				
			}
			else
			{
				fprintf_s( JournalDataFile, ";;;;;;;;;;;;;;;;;;" );
			}

			if( eProcessType == ProcessType::MeasureUnNormal )
			{
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUnNormal.ChkNernstVoltage.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamUnNormal.ChkNernstVoltage.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataUn.Un );
				}
			else
			{
				fprintf_s( JournalDataFile, ";;;" );
			}

			if( eProcessType == ProcessType::MeasureIpNormal )
			{
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIpNormal.ChkUp.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIpNormal.ChkUp.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIp.UApe );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIpNormal.ChkIp.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIpNormal.ChkIp.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIp.Ip );
			}
			else
			{
				fprintf_s( JournalDataFile, ";;;;;;" );
			}

			if( eProcessType == ProcessType::MeasureIlmNormal )
			{
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkIlMax.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkIlMax.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.Ilm );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkIaIpn.LowerLimit );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkIaIpn.UpperLimit );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.IaIpn );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkPt3Ip2.LowerLimit );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.Ip2 );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkPt3Un2.LowerLimit );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.Un2 );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.Ip3 );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkPt3Un3.LowerLimit );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.Un3 );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkPt3Ip4.LowerLimit );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.Ip4 );
				fprintf_s( JournalDataFile, "%.3f;", ParamNormalMeasure.ParamIlmNormal.ChkPt3Un4.LowerLimit );
				fprintf_s( JournalDataFile, "0,00;" );
				fprintf_s( JournalDataFile, "%.3f;", SENormalValues[i].ResultDataIlm.Un4 );
			}
			else
			{
				fprintf_s( JournalDataFile, ";;;;;;;;;;;;;;;;;;;;;;;;" );
			}

			fprintf_s( JournalDataFile, "\n" );
			fflush(	JournalDataFile );
			fclose(	JournalDataFile );
			JournalDataFile = NULL;
		}
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int GetNewSerialNumberAndCode( int PartStatus, unsigned int * Number, char Code[7] )             *
 *                                                                                                                         *
 * input:               : int PartStatus : status of part                                                                  *
 *                        unsigned int * Number : pointer to serial number                                                 *
 *                       	char Code[7] : code number                                                                       *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to generate the serial code number.                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::GetNewSerialNumberAndCode( int PartStatus, unsigned int * Number, char Code[7] )
{
	int RetVal = 0;

	char Codes [36] = {
		'0','1','2','3','4','5','6','7','8','9',
		'a','b','c','d','e','f','g','h','i','j',
		'k','l','m','n','o','p','q','r','s','t',
		'u','v','w','x','y','z'};

	unsigned long Divisor = SETrimmingRetainData[ChamberId - 1].SerialNumber;
	unsigned long Remainder;

	for( int i = 0; i < 5; i++ )
	{
		Remainder = Divisor % 36;
		Divisor = Divisor / 36;
		if( Remainder < 36 )
		{
			Code[5 - i] = Codes[Remainder]; 
		}
		else
		{
			RetVal = -1;
			break;
		}
	}

	switch( PartStatus	)
	{
		case Nio:
			Code[0] = 'N';
			break;

		case IoDiesel:
			Code[0] = 'D';
			break;

		case IoGasoline:
			Code[0] = 'G';
			break;

		default:
			;
			break;
	}

	Code[6] = '\0';

	*Number = SETrimmingRetainData[ChamberId - 1].SerialNumber++;

	StoreRetainData();

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int StoreRetainData( void )                                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to store the retain data.                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::StoreRetainData( void )
{
	int RetVal = 0;
	FILE *JournalDataFile = NULL;
	char TempString[255];

	sprintf_s( TempString, "%sConfigRetain_%d.txt", CONFIG_DATA_LOCATION, ChamberId );

	fopen_s( &JournalDataFile, TempString, "w+" );
	if( JournalDataFile != NULL )
	{
		fprintf_s( JournalDataFile, "NextSerialCode=%d\n",
			SETrimmingRetainData[ChamberId - 1].SerialNumber );	
		fflush(	JournalDataFile );
		fclose(	JournalDataFile );																				    //close file
		JournalDataFile = NULL;																						    //set file pointer to zero
	}
	else
	{
		printf( "Chamber[%d]::StoreRetainData: Store Data failed!\n", ChamberId );
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int LoadRetainData( void )                                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to load the retain data.                                                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::LoadRetainData( void )
{
	int RetVal = 0;
	FILE *JournalDataFile = NULL;
	char TempString[255];

	sprintf_s( TempString, "%sConfigRetain_%d.txt", CONFIG_DATA_LOCATION, ChamberId );

	fopen_s( &JournalDataFile, TempString, "r+" );
	if( JournalDataFile != NULL )
	{
		fscanf_s( JournalDataFile, "NextSerialCode=%d\n",
			&SETrimmingRetainData[ChamberId - 1].SerialNumber );
		fflush(	JournalDataFile );
		fclose(	JournalDataFile );																				//close file
		JournalDataFile = NULL;																						//set file pointer to zero
	}
	else
	{
		printf( "Chamber[%d]:LoadRetainData: Load Data failed!\n", ChamberId );
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetPartState( unsigned int Tiepoint, PartStatus Status, NioReworkReason Reason )            *
 *                                                                                                                         *
 * input:               : unsigned int Tiepoint : tiepoint                                                                 *
 *                        PartStatus Status : part status  																																 *
 *                      	NioReworkReason Reason : nio/rework reason                                                       *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set the part state.                                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::SetPartState( unsigned int Tiepoint, PartStatus Status, NioReworkReason Reason )
{
	switch( Status )
	{
		case Dummy:
			;																																		//dummy state should only be set from plc (assembly data)
			break;

		case NotPresent:
			;																																		//not present state should only be set from plc (assembly data)
			break;

		case ReworkOnceAgain:
			;																																		//rework once again state should only be set from plc (assembly data)
			break;

		case Deactivated:
			;																																		//deactivated state should only be set from plc (assembly data)
			break;

		case DeactivatedCondition:
			;																																		//deactivated condition state should only be set from plc (assembly data)
			break;

		case Nio:
			if( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Dummy )	&&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != NotPresent ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Rework ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Nio ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Deactivated ) )
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus = Status;
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartNioReworkReason = Reason;
				printf( "Chamber[%d]:SetPartState:SE%d:Status=%d,Reason=%d\n", ChamberId, Tiepoint, Status, Reason );
			}
			break;

		case Rework:
			if( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Dummy ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Nio ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != NotPresent ) && 
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Rework ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Deactivated ) )
			{
				if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != ReworkOnceAgain )
				{
					SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus = Status;
					SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartNioReworkReason = Reason;
					printf( "Chamber[%d]:SetPartState:SE%d:Status=%d,Reason=%d\n", ChamberId, Tiepoint, Status, Reason );
				}
				else
				{
					SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus = Nio;
					SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartNioReworkReason = ReworkOnceAgainRework;
					printf( "Chamber[%d]:SetPartState:SE%d:Status=%d,Reason=%d\n", ChamberId, Tiepoint, Status, Reason );
				}
			}
			break;

		case SelectGasoline: 
		case NotProcessed:
		case IoDiesel:
		case IoGasoline:
			if( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Dummy ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Nio ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Rework ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != NotPresent ) &&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != Deactivated ) &&
				    (SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus != SelectGasoline) )
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus = Status;
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartNioReworkReason = Reason;

				printf( "Chamber[%d]:SetPartState:.SETrimmingValuesTiepoint%d:Status=%d,Reason=%d\n", ChamberId, Tiepoint, Status, Reason );
			}
			break;

		default:
			;
			break;
	}
}


/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingInitRaw( int Tiepoint )                                                              *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for initializing trimming raw.                                              *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingInitRaw( int Tiepoint )
{
	FILE *JournalDataFile = NULL;
	char TempString[255];

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.CutCount = 0;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.CutCountRaw = 1;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.CrossingCountEntire = 0;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.CrossingCount = 0;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.AdditionalCuts = 0;

	//delete old files
	sprintf_s( TempString, "\\Statistics\\Sensor%d_cut.txt", Tiepoint );	//build file name
	remove( TempString );																									//delete old files

	#ifdef SIMULATION
	if(Tiepoint == 1)
	{
		if( TestDataFile == NULL )
		{
			TestDataFile = _fsopen("TestDataFull.txt", "r", _SH_DENYNO );				  // open new logfile for write
			int TestNumber;
			char TestId;

			do
			{
				fscanf_s( TestDataFile, "%c,%d\n", &TestId, 1, &TestNumber, 1 );
			}while( TestId != 'T' || TestNumber != 12 );
	
		}
	}
	#endif

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition = ImageProcessing[Tiepoint - 1].XPos;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition = ImageProcessing[Tiepoint - 1].YPos;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.ZPosition = ImageProcessing[Tiepoint - 1].ZPos;

	switch( Tiepoint )
	{
		//square 1 (tiepoint 1..5)
		case 1: case 2: case 3: case 4: case 5:																							 		
			SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition -= ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationGeometry.PrePositionSquare[0];
			//SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition -= 0.120f;
			break;

			//square 2 (tiepoint 6..10)
		case 6: case 7: case 8: case 9: case 10:
			SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition += ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationGeometry.PrePositionSquare[1];
			//SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition += 0.095f;
			break;

		//square 3 (tiepoint 11..15)
		case 11: case 12: case 13: case 14: case 15:
			SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition += ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationGeometry.PrePositionSquare[2];
			//SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition += 0.095f;
			break;

		//square 4 (tiepoint 16..20)
		case 16: case 17: case 18: case 19: case 20:
			SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition -= ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationGeometry.PrePositionSquare[3];
			//SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition -= 0.120f;
			break;

		default:

			break;
	}


		
	//x-y coordinate rotation Phi° 
	RotatePosition( Tiepoint, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition,
									&SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut, &SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYActCut, NULL);

	//calculate x start position
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut - 
																								( ImageProcessing[ Tiepoint - 1 ].Diameter / 2 ) +
																								ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.StartDistanceRawTrim;
	//calculate y start position		
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYActCut = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYActCut - 
																								( ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Fissure / 2 );
																																																											
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.ZeroPos = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMin = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition - ( ImageProcessing[ Tiepoint - 1 ].Diameter / 2 );
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMax = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition + ( ImageProcessing[ Tiepoint - 1 ].Diameter / 2 );
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMin = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition - ( ImageProcessing[ Tiepoint - 1 ].Diameter / 2 );
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMax = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition + ( ImageProcessing[ Tiepoint - 1 ].Diameter / 2 );

	RotatePosition( Tiepoint, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMin, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMin,
									&SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMin, &SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMin, NULL);

	RotatePosition( Tiepoint, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMax, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMax,
									&SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMax, &SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMax, NULL);

	if( SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMin > SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMax )
	{
		float Temp = 0;

		Temp = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMax;
		SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMax = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMin;
		SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMin = Temp; 
	}

	if( SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMin > SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMax )
	{
		float Temp = 0;

		Temp = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMax;
		SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMax = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMin;
		SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYMin = Temp; 
	}

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ResultData.SEPartStatus = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].AssemblyDataTiepoint.SEPartStatus;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ResultData.SEPartNioReworkReason = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].AssemblyDataTiepoint.SEPartNioReworkReason;

	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingInitFine( int Tiepoint )                                                             *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function for initializing trimming fine.                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingInitFine( int Tiepoint )
{
	float CutLength;

	CutLength = ImageProcessing[ Tiepoint - 1 ].Diameter / 4 - ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Fissure / 2;

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ResultData.CrossingCountLastRaw = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.CrossingCount; 

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.CutCountFine = 1;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.CrossingCount = 0;
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.AdditionalCuts = 0;

	//x-y coordinate rotation Phi° 
	RotatePosition( Tiepoint, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.XPosition, SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.YPosition,
									&SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut, &SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYActCut, NULL);

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut - 
																								( ImageProcessing[ Tiepoint - 1 ].Diameter / 2 ) +
																								ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.StartDistanceFineTrim;

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYActCut = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosYActCut + 
																								( ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Fissure / 2 ) +
																								( CutLength );

	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXMaxFine = ( SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXLastCut - ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.HoldBackPositionFineTrim );
	SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.ZeroPos = SETrimmingValues.SETrimmingValuesTiepoint[ Tiepoint - 1 ].ProcessData.PosXActCut;
																													 
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void RotatePosition( int Tiepoint, float XPos, float YPos, float *XPosDash, float *YPosDash, 	   *
 *                               float *Phi )                                                                              *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                        float XPos : x position                                                                          *
 *                        float YPos : y position                                                                          *
 *                        float *XPosDash : pointer to x position (rotated)                                                *
 *                        float *YPosDash : pointer to y position (rotated)                                                *
 *        								float *Phi : pointer to angle                                                      * 
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function rotate the position.                                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::RotatePosition( int Tiepoint, float XPos, float YPos, float *XPosDash, float *YPosDash, float *Phi )
{
	float PhiTemp, XPosDashTemp, YPosDashTemp;
	const float Pi = 3.14159265358979323846f;

	switch( Tiepoint )
	{
		//square 1 (tiepoint 1..5)
		case 1: case 2: case 3: case 4: case 5:
			PhiTemp = 0.0;
			break;

		case 6: case 7: case 8: case 9: case 10:
			PhiTemp = 1.5f * Pi;
			break;

		//square 3 (tiepoint 11..15)
		case 11: case 12: case 13: case 14: case 15:
			PhiTemp = Pi;
			break;

		//square 4 (tiepoint 16..20)
		case 16: case 17: case 18: case 19: case 20:
			PhiTemp = 0.5f * Pi;
			break;

		default:

			break;
	}

	//x coordinate rotation Phi° 
	XPosDashTemp = YPos * sin( PhiTemp ) + XPos * cos( PhiTemp );

	//y coordinate rotation Phi°
	YPosDashTemp = YPos * cos( PhiTemp ) - XPos * sin( PhiTemp );

	*XPosDash = XPosDashTemp;
	*YPosDash = YPosDashTemp;

	if( Phi != NULL)
	{
		*Phi = PhiTemp * 180.0f / Pi;
	}

}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void RotateBackPosition( int Tiepoint, float XPos, float YPos, float *XPosDash, float *YPosDash, *
 *                               float *Phi )                                                                              *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                        float XPos : x position                                                                          *
 *                        float YPos : y position                                                                          *
 *                        float *XPosDash : pointer to x position (rotated)                                                *
 *                        float *YPosDash : pointer to y position (rotated)                                                *
 *        								float *Phi : pointer to angle                                                                    * 
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function rotate back the position.                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::RotateBackPosition( int Tiepoint, float XPosDash, float YPosDash, float *XPos, float *YPos, float *Phi )
{
	float PhiTemp, XPosTemp, YPosTemp;
	const float Pi = 3.14159265358979323846f;

	switch( Tiepoint )
	{
		//square 1 (tiepoint 1..5)
		case 1: case 2: case 3: case 4: case 5:
			PhiTemp = 0.0;
			break;

		case 6: case 7: case 8: case 9: case 10:
			PhiTemp = 1.5f * Pi;
			break;

		//square 3 (tiepoint 11..15)
		case 11: case 12: case 13: case 14: case 15:
			PhiTemp = Pi;
			break;

		//square 4 (tiepoint 16..20)
		case 16: case 17: case 18: case 19: case 20:
			PhiTemp = 0.5f * Pi;
			break;

		default:

			break;
	}

	//x coordinate rotation Phi° 
	XPosTemp = - YPosDash * sin( PhiTemp ) + XPosDash * cos( PhiTemp );

	//y coordinate rotation Phi°
	YPosTemp = YPosDash * cos( PhiTemp ) + XPosDash * sin( PhiTemp );

	*XPos = XPosTemp;
	*YPos = YPosTemp;
	if( Phi != NULL )
	{
		*Phi = PhiTemp * 180.0f / Pi;
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int Trimming( void )                                                                             *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to handle the trimming process.                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::Trimming( void )
{
	int RetVal = 0;
	int FuncRetVal = 0;
	SEValueInfo Value;
	float WaitForTrimm;
	FILE *JournalDataFile = NULL;

	if ( ( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.MinCutTime > 
			 ( ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.MeasIntervall * 1000.0f ) ) )
	{
		WaitForTrimm = ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.MinCutTime;
	}
	else 
	{
		WaitForTrimm = ParamTrimmChamber.ParamTrimmCell.ParamMainTrimmingCell.MeasIntervall * 1000.0f;
	}

	//get ip ref sens 1
	ReferenceCell->GetLastValue(1, IMT_IPu, 0, &Value);
	IpRef[0] = IpCorrection( Value.Value, ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);

	//get ip ref sens 2
	ReferenceCell->GetLastValue(2, IMT_IPu, 0, &Value);
	IpRef[1] = IpCorrection( Value.Value, ParamTrimmChamber.ParamRefCell.ParamMainReferenceCell.KValue);

	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone == false )
		{
			//get ip trimming cell 
			TrimmCell->GetLastValue(i + 1, IMT_IPu, 0, &Value);

			if( _isnan( Value.Value )	== 0 )
			{
				if( Value.Value != 0.0f )
				{
					#ifdef _DEBUG
					if( ( Value.Value * 1.0e6f ) < 50.0 && abs( Value.Value * 1.0e6f ) > 1.0f )  
					{
						printf( "Chamber[%d]:Error: Ip<50: Value=%f\n", ChamberId, Value.Value * 1.0e6f);
					}
					#endif
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut = IpCorrectionOffsetAndDynamic( IpCorrection( Value.Value, ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.KValue) );
					#ifdef _DEBUG
					if( ( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingTimeActCut * 1.0e6f ) < 50.0 && abs( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingTimeActCut * 1.0e6f ) > 1.0f )  
					{
						printf( "Chamber[%d]:Error: Ip<50: Value=%f\n", ChamberId, Value.Value * 1.0e6f );
					}
					#endif
					SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingTimeActCut = Value.ReceiveTime;

					if( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingTimeLastCut + WaitForTrimm - 100 <= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingTimeActCut	)
					{
						#ifdef SIMULATION
						if( i == 0 )
						{
							if( TestDataFile != NULL )
							{
								if( fscanf_s( TestDataFile, "%f\n", &SETrimmingValues[i].ProcessData.IpActCut ) != 0 )
								{
									SETrimmingValues[i].ProcessData.IpActCut/=1000000;
								}
								else
								{
									SETrimmingValues[i].ProcessData.IpActCut = 0.0;
									fclose(	TestDataFile );
									TestDataFile = NULL;
								}
							}
							else
							{
								SETrimmingValues[i].ProcessData.IpActCut = 0.0;
							}
						}
						#endif

						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpDelta = ( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut - SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpLastCut );
						SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpDiff = ( ( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut - SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpLastCut ) / SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut ) * 100.0f;
							 
						switch( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase )
						{		 
							case 0:
								//measurement of first ip value
								FuncRetVal = TrimmingPhase0( i + 1 );
								if( FuncRetVal == 1 )
								{
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase++;
								}
								break;
							case 1:
								//cut until resist coating cut through (raw trimming)
								FuncRetVal = TrimmingPhase1( i + 1 );
								if( FuncRetVal == 1 )
								{
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase++;
								}
								if( FuncRetVal == 3)
								{
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.IpEndCut = SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut * 1e6f;
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActPhase = 5;
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
									printf( "Chamber[%d]:Trimming:SE%d:Fertig!\n", ChamberId, i + 1 );
								}
								break;

							case 2:
								//cut until coating end reached	(raw trimming)
								FuncRetVal = TrimmingPhase2( i + 1 ); 
								if( FuncRetVal == 1 )
								{
									TrimmingInitFine( i + 1 );
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase++;
								}
								if( FuncRetVal == 2 )
								{
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCount = 0;
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CutCountRaw++;
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase--;
								}
								if( FuncRetVal == 3)
								{
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.IpEndCut = SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut * 1e6f;
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActPhase = 5;
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
									printf( "Chamber[%d]:Trimming:SE%d:Fertig!\n", ChamberId, i + 1 );
								}
								break;
			
							case 3:	
								//cut until resist coating cut through (fine trimming)
								FuncRetVal = TrimmingPhase3( i + 1 ); 
								if( FuncRetVal == 1 )
								{
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase++;
								}
								if( FuncRetVal == 3)
								{
									SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.IpEndCut = SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut * 1e6f;
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase = 5;		
									SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.TrimmingDone = true;
									printf( "Chamber[%d]:Trimming:SE%d:Fertig!\n", ChamberId, i + 1 );
								}
								break;

							case 4:
								//cut until coating end reached	(fine trimming)
								FuncRetVal = TrimmingPhase4( i + 1 ); 
								if( ( FuncRetVal == 1 ) ||
										( FuncRetVal == 3 ) )
								{
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.IpEndCut = SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.IpActCut * 1e6f;
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActPhase = 5;		
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
									printf( "Chamber[%d]:Trimming:SE%d:Fertig!\n", ChamberId, i + 1 );
								}
								if( FuncRetVal == 2 )
								{
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCount = 0;
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCountFine++;
									SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActPhase--;
								}
								break;

							default:
								SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
								printf( "Chamber[%d]:Trimming:SE%d:Fertig!\n", ChamberId, i + 1 );
								break;

						}

						if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone == false )
						{
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingTimeLastCut = Value.ReceiveTime;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.IpLastCut = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.IpActCut;

							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCount = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCountRaw + SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCountFine;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCount++;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCountEntire++;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCountEntire = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCountEntire%100;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.CutCount = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCount;
							if( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCount != 0 )
							{
								SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.AverageCrossingCount = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCountEntire / ( float )SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCount;
							}

							float StartX, StartY, EndX, EndY, CutLength;
							//rotate back cut start position
							RotateBackPosition( i + 1, SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.PosXActCut, SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.PosYActCut, &StartX, &StartY, NULL);
							CutLength = ImageProcessing[ i ].Diameter / 4 - ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Fissure / 2;

							//rotate back cut end position
							RotateBackPosition( i + 1, SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.PosXActCut, SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.PosYActCut - CutLength, &EndX, &EndY, NULL);

							//check cut direction alternating and crossing count multiple 2
							if( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.CutDirectionAlternating == 1 )	&&
									( ( SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCount % 2 ) == 0 ) )
							{
								float Temp = 0;

								Temp = StartX;
								StartX = EndX;
								EndX = Temp;

								Temp = StartY;
								StartY = EndY;
								EndY = Temp;
							}


							if( ( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase == 1 ) ||
								  ( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.ActPhase == 3 ) )
							{
								//do laser cut
								ComMicroLas->TrimmingCut( StartX, 
																					StartY, 
																					SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ZPosition, 
																					EndX, 
																					EndY, 
																					SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ZPosition,
																				  ( ( ParamTrimmChamber.ParamTrimmProcess.ParamLaser.LaserPowerHeightening + 100.0f ) / 100.0f ) * ( 100.0f / SETrimmingValues.AssemblyDataCommon.AdjustPollutePercent ) * ParamTrimmChamber.ParamTrimmProcess.ParamLaser.LaserPower[ i ] ); 
							}
							else
							{
								//do laser cut
								ComMicroLas->TrimmingCut( StartX, 
																					StartY, 
																					SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ZPosition, 
																					EndX, 
																					EndY, 
																					SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ZPosition,
																					( 100.0f / SETrimmingValues.AssemblyDataCommon.AdjustPollutePercent ) * ParamTrimmChamber.ParamTrimmProcess.ParamLaser.LaserPower[ i ] ); 
							}

							SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.TrimmingCutIp[ SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1] = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.IpActCut * 1e6f;
							SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData.TrimmingCutPos[ SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1 ] = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.PosXActCut;

							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCutXPos[SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1] = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.PosXActCut;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCutYPos[SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1] = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.PosYActCut;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCutIp[SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1] = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.IpActCut * 1e6f;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCutChamberPressure[SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1] = ChamberPressureActual;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCutCount[SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1] = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCount;
							SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingCrossingCount[SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire - 1] = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCount;
						}			 
					}
				}
			}


			RetVal = 1;
			if( ( GetActualSystemTimeMs( ) >= TrimmCell->GetSequenceStartTime() + ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.ProcessTimeout * 1000 ) ) ||
					( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.CrossingCountEntire >= 95  ) )
			{
				SetPartState( i + 1, Nio, TimeoutReached );
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ResultData.IpEndCut = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.IpActCut * 1e6f;
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActPhase = 5;
				SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.TrimmingDone = true;
				printf( "Chamber[%d]:Trimming:SE%d:Fertig!\n", ChamberId, i + 1 );
			}
		}
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingPhase0( int Tiepoint )                                                               *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to handle the trimming phase 0 (premeasurement).                            *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingPhase0( int Tiepoint )
{
	int RetVal = 0; 

	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint -1].ProcessData.IpFineLimit = ( ( 100 + ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLimitFineTrim ) / 100 ) * 
																		( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f );

	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint -1].ProcessData.MinimalDeviationAbs = ( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) - 
																										( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) - SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut ) *
																										( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinDeviationRawTrim / 100.0f ) );

	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint -1].ResultData.IpStart = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut * 1e6f;

	RetVal = 1;
	return RetVal;
}


/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingPhase1( int Tiepoint )                                                               *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to handle the trimming phase 1 (cut resist coating(raw)).                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingPhase1( int Tiepoint )
{
	int RetVal = 0;

	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax = 0.0;

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpDelta > ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpNoise / 1.0e6f ) )
	{		    
		RetVal = 1;
	}

	// Grenzwert erreicht?
	if( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut >= SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpRawLimit ) &&
				( ( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CutCount == 1 )	&&
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount == 0 ) ) ||
					( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CutCount == 2 )	) )
	{
		SetPartState( Tiepoint, Nio, RawLimitReached );	
		RetVal = 3;
	}

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount == ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MaxPassage )
	{
		SetPartState( Tiepoint, Nio, MaxPassageReached );	
		RetVal = 3;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingPhase2( int Tiepoint )                                                               *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to handle the trimming phase 2 (coating end (raw)).                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingPhase2( int Tiepoint )
{
	int RetVal = 0;

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax < SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut)
	{
		SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut;
	}

	if( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpDiff < ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLayerEnd )	&&
			( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount >= ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinPassageLastCut )	)
	{
		if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CutCountRaw == 1 )
		{
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFirstCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut;
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFirstCut;
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut;
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut + ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Step1RawTrim;		
		}
		else
		{
			CalculateNewPos( Tiepoint, true );
		}

		RetVal = 2;	

		if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut > SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint -1].ProcessData.MinimalDeviationAbs )
		{																						
			RetVal = 1;
			if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpDiff < ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLayerEndScrap )
			{
				SetPartState( Tiepoint, Nio, ActiveAreaDemaged );
				RetVal = 3;
			}
		}	

		if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut >= SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFineLimit )
		{
			RetVal = 3;
		}
	}

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount == ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MaxPassage )
	{
		SetPartState( Tiepoint, Nio, MaxPassageReached );	
		RetVal = 3;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingPhase3( int Tiepoint )                                                               *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to handle the trimming phase 3 (cut resist coating(fine)).                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingPhase3( int Tiepoint )
{
	int RetVal = 0;

	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax = 0.0;

	// Grenzwert erreicht?
	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut >= SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFineLimit )
	{
		RetVal = 3;																							 
	}
	else
	{
		if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpDelta > ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpNoise / 1.0e6f ) )
		{		    
			RetVal = 1;
		}
		if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount == ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MaxPassage )
		{
			if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CutCountFine == 1 )
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFirstCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut;
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFirstCut;
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut;	 
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut + ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Step1FineTrim;
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut; 
				CalculateNewPos( Tiepoint, false );
			}
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount = 0;
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CutCountFine++;
		}
	}

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount == ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MaxPassage )
	{
		SetPartState( Tiepoint, Nio, MaxPassageReached );	
		RetVal = 3;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int TrimmingPhase4( int Tiepoint )                                                               *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to handle the trimming phase 4 (coating end (fine)).                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::TrimmingPhase4( int Tiepoint )
{
	int RetVal = 0; 

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax < SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut)
	{
		SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut;
	}													 																	
	
	if( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpDiff < ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.DeltaIpLayerEnd ) &&
			( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount >= ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinPassageLastCut ) )
	{
		if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CutCountFine == 1 )
		{
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFirstCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut;
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFirstCut;
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut;
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut + ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Step1FineTrim;
		}
		else
		{
			CalculateNewPos( Tiepoint, false );
		}

		RetVal = 2;
								
	}

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpActCut >= SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpFineLimit )
	{
		RetVal = 1;
	}	

	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.CrossingCount == ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MaxPassage )
	{
		SetPartState( Tiepoint, Nio, MaxPassageReached );	
		RetVal = 3;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int CalculateNewPos( int Tiepoint, bool RawTrimm )                                               *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                        bool RawTrimm : raw trimming active                                                              *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to calculate new trimming positions.                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::CalculateNewPos( int Tiepoint, bool RawTrimm )
{
	int RetVal = 0;
	float Gradient = 0.0, Distance = 0.0, HoldBack = 0.0, FactorA = 0.0, FactorB = 0.0, FactorC = 0.0, ActPos = 0.0;

	Distance = abs(SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut - SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut);
	if( Distance == 0.0 )
	{
		Distance = ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance;
	}
	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut;	

	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpnLast = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn;
	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpMax;
	
	if(RawTrimm == true)
	{
		HoldBack = ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.InterpolationFactorRawTrim;
		FactorC = ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.FaktorCRawTrim;
	}
	else
	{
		HoldBack = ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.InterpolationFactorFineTrim;
		FactorC = ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.FaktorCFineTrim;
	}
	if( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.Linear1X == FALSE )
	{
		// calc linear
		Gradient = ( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn - SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpnLast ) / Distance );
		SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos = ( ( ( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) - 
																										SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn ) / Gradient ) * HoldBack );
	}
	else 
	{
		// calc 1/X	
		ActPos = abs( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut - SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.ZeroPos );
		Gradient = ( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn - SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.IpnLast ) / Distance );
		FactorA = ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.Ipn - Gradient * ( FactorC - ActPos ) );
		FactorB = ( Gradient * (FactorC - ActPos ) * (FactorC - ActPos ) );
		SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos = ( FactorC - FactorB / ( ( ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.IpSetpoint / 1.0e6f ) - FactorA ) - ActPos ) * HoldBack;
	}
	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos <= ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance )
	{
		SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos = ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance;		
	}
	if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos > 0.350f )
	{
		SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos = 0.350f;		
	}

	SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut + SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos;
		
	if( RawTrimm == true )
	{
		//check position inside hole
		if( ( -SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut + SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXMax ) < 
					ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinDistanceToEdgeForAdditionalCuts	)
		{
			if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.OutsideHole == true )
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.TrimmingDone = true;
				SetPartState( Tiepoint, Nio, OutsideHole );
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.OutsideHole = true;
			}
			//outside hole
			if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.AdditionalCuts < ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.AdditionalCutsNumber )
			{																																																																		 
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXMax + 
																										( ( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.AdditionalCuts * ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance ) - 
																										ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinDistanceToEdgeForAdditionalCuts );			

				if ((-SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut + SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut) < ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance) 
				{
					SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut + ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance;	
				}

				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.AdditionalCuts++;
			}
		}
	}
	else
	{
		if( SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut >= SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXMaxFine )	
		{
			SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos / 1.2f;
			if(SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos < ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance) 
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut + ParamTrimmChamber.ParamTrimmProcess.ParamTrimmingProcess.MinCutDistance;	
			}
			else
			{
				SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXActCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.PosXLastCut + SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ProcessData.DiffPos;	
			}
		}
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SETrimmingChamber( void )                                                                        *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor function.                                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
SETrimmingChamber::SETrimmingChamber( int i )
{
	ReferenceCell = new SEReferenceCell((ChamberId * 2));
	TrimmCell = new SETrimmingCell((ChamberId * 2)-1);

	ProcessStep = EProcessStep::Initialization;
	CurrProcessStep = EProcessStep::Undefined;
	NormalStep = 0;
	StabilityStep = 0;
	ParameterSetOk = false;
	StationParameterSetOk = false;
	NormalParameterSetOk = false;
	ParameterImageProcessingOk = false;  
	AssemblyDataOk = false;
	ChamberPressureActualArrayCount = 0;
	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		ImageProcessing[i].XPos = - 9.281f;
		ImageProcessing[i].YPos = 9.693f;
		ImageProcessing[i].ZPos = 3.0f;
		ImageProcessing[i].Diameter = 1.0f;
	}
	#ifdef SIMULATION
	TestDataFile = NULL;
	#endif
	DeinstallThreadRequest = false;
	ChamberState = INF_EMPTY;

	if( ghMutex[ ChamberId - 1] == NULL )
	{
		ghMutex[ ChamberId - 1]  = CreateMutex( 
			NULL,              // default security attributes
			FALSE,             // initially not owned
			NULL);             // unnamed mutex
		if( ghMutex[ ChamberId - 1]  == NULL ) 
		{
			printf("SEComMicroLas::Connect:create mutex failed!\n");
		}
	}

	_mkdir( DATA_SAVE_LOCATION );	//make directory for measure data
	_mkdir( CONFIG_DATA_LOCATION );	//make directory for config data
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SETrimmingChamber( int ChamberId, SEComMicroLas *ComMicroLas )                                   *
 *                                                                                                                         *
 * input:               : int ChamberId: chamber identifier                                                                *
 *                        SEComMicroLas *ComMicroLas : pointer to microlas communication                                   *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the constructor function (overload).                                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
SETrimmingChamber::SETrimmingChamber( int ChamberId, SEComMicroLas *ComMicroLas, SEComPlc *ComPlc )
{
	ReferenceCell = new SEReferenceCell((ChamberId * 2));
	TrimmCell = new SETrimmingCell((ChamberId * 2)-1);
	this->ChamberId = ChamberId;
	this->ComMicroLas = ComMicroLas;
	ClearAllValues();
	ProcessStep = EProcessStep::Initialization;
		NormalStep = 0;
	StabilityStep = 0;
	ParameterSetOk = false;
	StationParameterSetOk = false;
	NormalParameterSetOk = false;
	ParameterImageProcessingOk = false;
	AssemblyDataOk = false;
	ChamberPressureActualArrayCount = 0;
	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		ImageProcessing[i].XPos = - 9.281f;
		ImageProcessing[i].YPos = 9.693f;
		ImageProcessing[i].Diameter = 1.05f;
	}
	#ifdef SIMULATION
	TestDataFile = NULL;
	#endif
	DeinstallThreadRequest = false;

	_mkdir( DATA_SAVE_LOCATION );	//make directory for measure data
	_mkdir( CONFIG_DATA_LOCATION );	//make directory for config data

	DigitalOut = 0;
	IgnoreNewPressure = false;

	if( ghMutex[ ChamberId - 1] == NULL )
	{
		ghMutex[ ChamberId - 1]  = CreateMutex( 
			NULL,              // default security attributes
			FALSE,             // initially not owned
			NULL);             // unnamed mutex
		if( ghMutex[ ChamberId - 1]  == NULL ) 
		{
			printf("SEComMicroLas::Connect:create mutex failed!\n");
		}
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : ~SETrimmingChamber( void )                                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the destructor function.                                                                 *
 *-------------------------------------------------------------------------------------------------------------------------*/
SETrimmingChamber::~SETrimmingChamber( void )
{
	delete ReferenceCell;
	delete TrimmCell;

	if( SEMeasurementCell::Dllopened == true )
	{
		LSTestDLLClose( LSTestHandle);
		SEMeasurementCell::Dllopened = false;
	}
	if( ghMutex[ChamberId - 1] != NULL )
	{
		CloseHandle( ghMutex[ChamberId - 1] );
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int :Initializing( int Mode )                                                                    *
 *                                                                                                                         *
 * input:               : int Mode : (0=not plausible; 1=only 1st; 2=only 2nd; 3=1st and 2nd )                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the initializing function.                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::Initializing( void )
{
	int RetVal = 0;
	int FuncRetVal = 0;

	LoadRetainData();

	/*
	"LSTestDLLOpen" flags = 0xF000000F
	1111 0000 0000 0000 0000 0000 0000 1111  
	-> bit 31: assignment take place over "MCxAssignCell"  
	-> bit 30: CAN baudrate 500kBaud
	-> bit 29: "ResultIdx" automatically
	-> bit 28: "CellVec" complies test station number 
	-> bit 3 : cell for reference sensors chamber 2
	-> bit 2 : cell for	test chamber 2
	-> bit 1 : cell for reference sensors chamber 1
	-> bit 0 : cell for	test chamber 1
	*/
	if( SEMeasurementCell::Dllopened == false )
	{
		FuncRetVal = LSTestDLLOpen( 0xF0000003, &LSTestHandle );
		printf( "Chamber[%d]:LSTestDLLOpen=0xF0000003\n", ChamberId );	
		if( FuncRetVal >= 0 )
		{
			SEMeasurementCell::Dllopened = true;
			//#ifdef _DEBUG
			SetDLLReportingPath( LSTestHandle,"D:\\LSTestReporting.txt" );
			//#endif
		}
	}

	FuncRetVal = TrimmCell->Initializing( &ParamStationTrimmChamber.ParamStationTrimmCell, &ParamTrimmChamber.ParamTrimmCell, &ParamNormalMeasure );
	if(FuncRetVal != 0)
	{
		RetVal = FuncRetVal;
	}

	FuncRetVal = ReferenceCell->Initializing( &ParamStationTrimmChamber.ParamStationRefCell, &ParamTrimmChamber.ParamRefCell );
	if( (FuncRetVal != 0) && (RetVal != 0) )
	{
		RetVal = FuncRetVal;
	}
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int : CheckFirmware( void )                                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the initializing function.                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::CheckFirmware( void )
{
	int RetVal = 0;
	int FuncRetVal = 0;

	FuncRetVal = TrimmCell->CheckFirmware();
	if( FuncRetVal != 0 )
	{
		RetVal = FuncRetVal;
	}
	else
	{
		FuncRetVal = ReferenceCell->CheckFirmware();
		if( FuncRetVal != 0 )
		{
			RetVal = FuncRetVal;
		}
	}
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int : PerformCalib( void )                                                                      *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the initializing function.                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::PerformCalib(void)
{
	int RetVal = 0;
	int FuncRetVal = 0;

	FuncRetVal = TrimmCell->PerformCalib();
	if (FuncRetVal != 0)
	{
		RetVal = FuncRetVal;
	}
    //MCx
	return RetVal;
}
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int MasterReset( void )                                                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to reset all.                                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::MasterReset( void )
{
	StopProcess( );
	return 0;	
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SetParameter( ParamStationDataTrimmingChamber ParamStationTrimmChamber )                     *
 *                                                                                                                         *
 * input:               : ParamDataTrimmingChamber ParamTrimmChamber : parameter data of trimming chamber                  *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to set the parameter of trimming chamber.                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::SetParameter( ParamStationDataTrimmingChamber ParamStationTrimmChamber )
{
	int RetVal = 0;
	bool ParameterOk = true;

	if( ParameterOk == true )
	{
		memcpy_s( &this->ParamStationTrimmChamber, sizeof( this->ParamStationTrimmChamber ), &ParamStationTrimmChamber, sizeof( ParamStationTrimmChamber ) );
		StationParameterSetOk = true;
	}
	else
	{
		StationParameterSetOk = false;
		RetVal = -1;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SetParameter( ParamDataTrimmingChamber ParamTrimmChamber )                                   *
 *                                                                                                                         *
 * input:               : ParamDataTrimmingChamber ParamTrimmChamber : parameter data of trimming chamber                  *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to set the parameter of trimming chamber.                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::SetParameter( ParamDataTrimmingChamber ParamTrimmChamber )
{
	int RetVal = 0;
	bool ParameterOk = true;

	if( ( ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Step1RawTrim == 0.0 ) &&
			( ParamTrimmChamber.ParamTrimmProcess.ParamGeometry.Step1FineTrim == 0.0 ) )
	{
		ParameterOk = false;
	}

	if( ParameterOk == true )
	{
		memcpy_s( &this->ParamTrimmChamber, sizeof( this->ParamTrimmChamber ), &ParamTrimmChamber, sizeof( ParamTrimmChamber ) );
		ParameterSetOk = true;
	}
	else
	{
		ParameterSetOk = false;
		RetVal = -1;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SetParameter( ParamDataNormalMeasure ParamNormalMeasure)                                     *
 *                                                                                                                         *
 * input:               : ParamDataNormalMeasure ParamNormalMeasure : parameter data of normal measure                     *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to set the parameter of normal measure.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::SetParameter( ParamDataNormalMeasure ParamNormalMeasure )
{
	int RetVal = 0;
	bool ParameterOk = true;

	/*if( ( ParamTrimChamber.ParamTrimmProcess.ParamGeometry.Step1RawTrim == 0.0 ) &&
			( ParamTrimChamber.ParamTrimmProcess.ParamGeometry.Step1FineTrim == 0.0 ) )
	{
		ParameterOk = false;
	}*/

	if( ParameterOk == true )
	{
		memcpy_s( &this->ParamNormalMeasure, sizeof( this->ParamNormalMeasure ), &ParamNormalMeasure, sizeof( ParamNormalMeasure ) );
		NormalParameterSetOk = true;
	}
	else
	{
		NormalParameterSetOk = false;
		RetVal = -1;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SetAssemblyData( AssemblyDataTrimmingChamber AssemblyData )                                  *
 *                                                                                                                         *
 * input:               : AssemblyDataTrimmingChamber AssemblyData : assembly data                                         *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to set the assembly data.                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::SetAssemblyData( AssemblyDataTrimmingChamber AssemblyData )
{
	int RetVal = 0;
	bool ParameterOk = true;
	
	//delete all assembly data
	//repeat every tiepoint (running variable i)
	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		memset( &SETrimmingValues.AssemblyDataCommon, 0, sizeof( SETrimmingValues.AssemblyDataCommon ) );
		memset( &SETrimmingValues.SETrimmingValuesTiepoint[i].AssemblyDataTiepoint, 0, sizeof( SETrimmingValues.SETrimmingValuesTiepoint[i].AssemblyDataTiepoint ) );
	}

	//check assembly data common
	if( ( AssemblyData.AssemblyDataCommon.TypeNo == 0 ) ||
			( AssemblyData.AssemblyDataCommon.Charge == 0 ) ||
			( AssemblyData.AssemblyDataCommon.ProcessType == 0 ) ||
			( AssemblyData.AssemblyDataCommon.WpcNo == 0 ) ||
			( AssemblyData.AssemblyDataCommon.ChamberNumber == 0 ) ||
			( AssemblyData.AssemblyDataCommon.GlobalMachineNumber == 0 ) )
	{
		ParameterOk = false;
		RetVal = -1;	
	}

	//check evacuated pressure
	if( AssemblyData.AssemblyDataCommon.EvacuatedPressure > 100.0f )
	{		
		ParameterOk = false;
		RetVal = -2;	
	}

	//check adjust pollute
	if( ( AssemblyData.AssemblyDataCommon.AdjustPollutePercent < 0 ) || 
			( AssemblyData.AssemblyDataCommon.AdjustPollutePercent > 200 ) )
	{
		ParameterOk = false;
		RetVal = -3;
	}
	else
	{
		//limiting to 100%
		if( AssemblyData.AssemblyDataCommon.AdjustPollutePercent > 100 )
		{
			AssemblyData.AssemblyDataCommon.AdjustPollutePercent = 100;	
		}
	}

	if( ParameterOk == true  )
	{
		SETrimmingValues.AssemblyDataCommon.TypeNo = AssemblyData.AssemblyDataCommon.TypeNo;
		SETrimmingValues.AssemblyDataCommon.TypeNoGs = AssemblyData.AssemblyDataCommon.TypeNoGs;
		SETrimmingValues.AssemblyDataCommon.TypeNoDs = AssemblyData.AssemblyDataCommon.TypeNoDs;
		SETrimmingValues.AssemblyDataCommon.Charge = AssemblyData.AssemblyDataCommon.Charge;
		SETrimmingValues.AssemblyDataCommon.PartCharge = AssemblyData.AssemblyDataCommon.PartCharge;
		SETrimmingValues.AssemblyDataCommon.ProcessType = AssemblyData.AssemblyDataCommon.ProcessType;
		SETrimmingValues.AssemblyDataCommon.WpcNo = AssemblyData.AssemblyDataCommon.WpcNo;
		SETrimmingValues.AssemblyDataCommon.ChamberNumber = AssemblyData.AssemblyDataCommon.ChamberNumber;
		SETrimmingValues.AssemblyDataCommon.GlobalMachineNumber = AssemblyData.AssemblyDataCommon.GlobalMachineNumber;
		SETrimmingValues.AssemblyDataCommon.LaserPower = AssemblyData.AssemblyDataCommon.LaserPower;
		SETrimmingValues.AssemblyDataCommon.LeakageRate = AssemblyData.AssemblyDataCommon.LeakageRate;
		SETrimmingValues.AssemblyDataCommon.AtmospherePressure = AssemblyData.AssemblyDataCommon.AtmospherePressure;
		//repeat every tiepoint (running variable i)
		for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
		{
			SETrimmingValues.SETrimmingValuesTiepoint[i].AssemblyDataTiepoint.SEPartStatus = (PartStatus)AssemblyData.AssemblyDataTiepoint[i].SEPartStatus;
			SETrimmingValues.SETrimmingValuesTiepoint[i].AssemblyDataTiepoint.SEPartNioReworkReason = (NioReworkReason)AssemblyData.AssemblyDataTiepoint[i].SEPartNioReworkReason;
		}

		SETrimmingValues.AssemblyDataCommon.EvacuatedPressure = AssemblyData.AssemblyDataCommon.EvacuatedPressure;

		SETrimmingValues.AssemblyDataCommon.AdjustPollutePercent = AssemblyData.AssemblyDataCommon.AdjustPollutePercent;

		AssemblyDataOk = true;
	}
	else
	{
		AssemblyDataOk = false;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadResult( ResultDataTrimming * ResultTrimming, unsigned int Tiepoint )                     *
 *                                                                                                                         *
 * input:               : ResultDataTrimming * ResultTrimming : pointer to result data                                     *
 *                        unsigned int Tiepoint : tiepoint                                                                 *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to read trimming result data.                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ReadResult( ResultDataTrimming * ResultTrimming, unsigned int Tiepoint )
{
	int RetVal = 0;

	if( ghMutex != NULL )
	{
		if( WaitForSingleObject( ghMutex[ChamberId - 1], 10000) == WAIT_OBJECT_0 ) 
		{
			if( ( Tiepoint >= 1 ) &&
					( Tiepoint <= CELL_TIEPOINT_COUNT ) )
			{
				ResultTrimming->SerialNumber = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SerialNumber;
				strcpy_s( ResultTrimming->SerialCode, SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SerialCode );
				ResultTrimming->SEPartStatus = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartStatus;
				ResultTrimming->IpStart = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.IpStart;
				ResultTrimming->IpEndCut = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.IpEndCut;
				ResultTrimming->IpEndCheck = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.IpEndCheck;
				ResultTrimming->CutCount = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.CutCount;
				ResultTrimming->AverageCrossingCount = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.AverageCrossingCount;
				ResultTrimming->CrossingCountLastRaw = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.CrossingCountLastRaw;
				ResultTrimming->SEPartNioReworkReason = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.SEPartNioReworkReason;
				memcpy(	&ResultTrimming->TrimmingCutIp, &SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.TrimmingCutIp, sizeof( ResultTrimming->TrimmingCutIp ) ); 
				memcpy(	&ResultTrimming->TrimmingCutPos, &SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.TrimmingCutPos, sizeof( ResultTrimming->TrimmingCutPos ) ); 
				ResultTrimming->IpRef[0] = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.IpRef[0];
				ResultTrimming->IpRef[1] = SETrimmingValues.SETrimmingValuesTiepoint[Tiepoint - 1].ResultData.IpRef[1];
			}
			else
			{
				RetVal = -1;
			}
			ReleaseMutex( ghMutex[ChamberId - 1] );
		}
	}


	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadResult( ResultDataPositionNormal * ResultPositionNormal, unsigned int Tiepoint )         *
 *                                                                                                                         *
 * input:               : ResultDataPositionNormal * ResultPositionNormal : pointer to result data                         *
 *                        unsigned int Tiepoint : tiepoint                                                                 *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to read position normal result data.                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ReadResult( ResultDataPositionNormal * ResultPositionNormal, unsigned int Tiepoint )
{
	int RetVal = 0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= CELL_TIEPOINT_COUNT ) )
	{
		ResultPositionNormal->RiDcn = SENormalValues[Tiepoint - 1].ResultDataPosition.RiDcn;
		ResultPositionNormal->RiDcp = SENormalValues[Tiepoint - 1].ResultDataPosition.RiDcp;
		ResultPositionNormal->Rhhot = SENormalValues[Tiepoint - 1].ResultDataPosition.Rhhot;	
		ResultPositionNormal->Ih     = SENormalValues[Tiepoint - 1].ResultDataPosition.Ih;
		ResultPositionNormal->IpCont = SENormalValues[Tiepoint - 1].ResultDataPosition.IpCont;
		ResultPositionNormal->Status = SENormalValues[Tiepoint - 1].ResultDataPosition.Status;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadResult( ResultDataHeaterNormal * ResultHeaterNormal, unsigned int Tiepoint )             *
 *                                                                                                                         *
 * input:               : ResultDataHeaterNormal * ResultHeaterNormal : pointer to result data                             *
 *                        unsigned int Tiepoint : tiepoint                                                                 *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to read heater normal result data.                                          *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ReadResult( ResultDataHeaterNormal * ResultHeaterNormal, unsigned int Tiepoint )
{
	int RetVal = 0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= CELL_TIEPOINT_COUNT ) )
	{
		ResultHeaterNormal->ContactOk = SENormalValues[Tiepoint - 1].ResultDataHeater.ContactOk;
		ResultHeaterNormal->Rhhot = SENormalValues[Tiepoint - 1].ResultDataHeater.Rhhot;
		ResultHeaterNormal->Ih = SENormalValues[Tiepoint - 1].ResultDataHeater.Ih;
		ResultHeaterNormal->RhCold = SENormalValues[Tiepoint - 1].ResultDataHeater.RhCold;
		ResultHeaterNormal->Status = SENormalValues[Tiepoint - 1].ResultDataHeater.Status;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadResult( ResultDataUniversalNormal * ResultUniversalNormal, unsigned int Tiepoint )       *
 *                                                                                                                         *
 * input:               : ResultDataUniversalNormal * ResultUniversalNormal : pointer to result data                       *
 *                        unsigned int Tiepoint : tiepoint                                                                 *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to read universal normal result data.                                       *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ReadResult( ResultDataUniversalNormal * ResultUniversalNormal, unsigned int Tiepoint )
{
	int RetVal = 0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= CELL_TIEPOINT_COUNT ) )
	{
		ResultUniversalNormal->Igrk = SENormalValues[Tiepoint - 1].ResultDataUniversal.Igrk;
		ResultUniversalNormal->Il = SENormalValues[Tiepoint - 1].ResultDataUniversal.Il;
		ResultUniversalNormal->RiDcn = SENormalValues[Tiepoint - 1].ResultDataUniversal.RiDcn;
		ResultUniversalNormal->IpRe = SENormalValues[Tiepoint - 1].ResultDataUniversal.IpRe;
		ResultUniversalNormal->RiAcstat = SENormalValues[Tiepoint - 1].ResultDataUniversal.RiAcstat;
		ResultUniversalNormal->RiAc = SENormalValues[Tiepoint - 1].ResultDataUniversal.RiAc;
		ResultUniversalNormal->Status = SENormalValues[Tiepoint - 1].ResultDataUniversal.Status;
	}
	return RetVal;	
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadResult( ResultDataUnNormal * ResultUnNormal, unsigned int Tiepoint )                     *
 *                                                                                                                         *
 * input:               : ResultDataUnNormal * ResultUnNormal : pointer to result data                                     *
 *                        unsigned int Tiepoint : tiepoint                                                                 *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to read un normal result data.                                              *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ReadResult( ResultDataUnNormal * ResultUnNormal, unsigned int Tiepoint )
{
	int RetVal = 0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= CELL_TIEPOINT_COUNT ) )
	{
		ResultUnNormal->Un = SENormalValues[Tiepoint - 1].ResultDataUn.Un;
		ResultUnNormal->Status = SENormalValues[Tiepoint - 1].ResultDataUn.Status;
	}

	return RetVal;
}

int SETrimmingChamber::ReadResult( ResultDataIpNormal * ResultIpNormal, unsigned int Tiepoint )
{
	int RetVal = 0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= CELL_TIEPOINT_COUNT ) )
	{
		ResultIpNormal->Ip = SENormalValues[Tiepoint - 1].ResultDataIp.Ip;
		ResultIpNormal->UApe = SENormalValues[Tiepoint - 1].ResultDataIp.UApe;
		ResultIpNormal->Status = SENormalValues[Tiepoint - 1].ResultDataIp.Status;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadResult( ResultDataIlmNormal * ResultIlmNormal, unsigned int Tiepoint )                   *
 *                                                                                                                         *
 * input:               : ResultDataIlmNormal * ResultIlmNormal : pointer to result data                                   *
 *                        unsigned int Tiepoint : tiepoint                                                                 *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to read ilm normal result data.                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ReadResult( ResultDataIlmNormal * ResultIlmNormal, unsigned int Tiepoint )
{
	int RetVal = 0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= CELL_TIEPOINT_COUNT ) )
	{
		ResultIlmNormal->Ilm = SENormalValues[Tiepoint - 1].ResultDataIlm.Ilm;
		ResultIlmNormal->IaIpn = SENormalValues[Tiepoint - 1].ResultDataIlm.IaIpn;
		ResultIlmNormal->UIpn = SENormalValues[Tiepoint - 1].ResultDataIlm.UIpn;
		ResultIlmNormal->Ip2 = SENormalValues[Tiepoint - 1].ResultDataIlm.Ip2;
		ResultIlmNormal->Un2 = SENormalValues[Tiepoint - 1].ResultDataIlm.Un2;
		ResultIlmNormal->Ip3 = SENormalValues[Tiepoint - 1].ResultDataIlm.Ip3;
		ResultIlmNormal->Un3 = SENormalValues[Tiepoint - 1].ResultDataIlm.Un3;
		ResultIlmNormal->Ip4 = SENormalValues[Tiepoint - 1].ResultDataIlm.Ip4;
		ResultIlmNormal->Un4 = SENormalValues[Tiepoint - 1].ResultDataIlm.Un4;
		ResultIlmNormal->Status = SENormalValues[Tiepoint - 1].ResultDataIlm.Status;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ReadCyclic( CyclicDataTrimming * CyclicTrimming, unsigned int * ChamberState )               *
 *                                                                                                                         *
 * input:               : CyclicDataTrimming * CyclicTrimming : pointer to cyclical data                                   *
 *                        unsigned int * ChamberState : pointer to chamber stat                                            *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to read cyclical result data.                                               *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ReadCyclic( CyclicDataTrimming * CyclicTrimming, unsigned int * ChamberState, float * IpRef )
{
	if( ghMutex != NULL )
	{
		if( WaitForSingleObject( ghMutex[ChamberId - 1], 10000) == WAIT_OBJECT_0 ) 
		{
			for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
			{
				CyclicTrimming[ i ].ActCrossingCount = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CrossingCount;
				CyclicTrimming[ i ].ActCutCount = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.CutCount;
				CyclicTrimming[ i ].ActIpValue = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActIp * 1e6f;
				CyclicTrimming[ i ].ActPhase = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActPhase;
				CyclicTrimming[ i ].ActRhhValue = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActRhh;
				CyclicTrimming[ i ].ActRiValue = SETrimmingValues.SETrimmingValuesTiepoint[ i ].ProcessData.ActRi;
			}
			ReleaseMutex( ghMutex[ChamberId - 1] );
			*ChamberState = this->ChamberState;
			IpRef[ 0 ] = SETrimmingValues.SETrimmingValuesTiepoint[ 1 ].ProcessData.ActIpRef[ 0 ] * 1e6f;
			IpRef[ 1 ] = SETrimmingValues.SETrimmingValuesTiepoint[ 1 ].ProcessData.ActIpRef[ 1 ] * 1e6f;
		}
	}

	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool ParameterReadyToStart( void )                                                               *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = not ready                                                                       *
 *                                 true = ready                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to check parameter set ready for start.                                     *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SETrimmingChamber::ParameterReadyToStart( void )
{
	// tod bz
	return true;
	//return ( ParameterSetOk && StationParameterSetOk && NormalParameterSetOk ); 
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool ParameterReadyForTrimming( void )                                                           *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = not ready                                                                       *
 *                                 true = ready                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to check parameter set ready for start trimming.                            *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SETrimmingChamber::ParameterReadyForTrimming( )
{
	bool _retVal = false;

	_retVal = ParameterSetOk && AssemblyDataOk;
	#ifndef IMAGE_PROCESSING_DISABLE
		_retVal = _retVal && ParameterImageProcessingOk;
	#endif

	return ( _retVal ); 
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int InstallThread( LPTHREAD_START_ROUTINE CallbackAddressMeasurementCell,                        * 
 *	                      LPTHREAD_START_ROUTINE CallbackAddressReferenceCell, 	                                           *
 *												LPTHREAD_START_ROUTINE CallbackAddressChamber,																									 *
 *	                      LONG ( *pFCallBackMeasurementCell )( ... ), 																										 *
 *												LONG ( *pFCallBackReferenceCell )( ... ) )																											 *
 *                                                                                                                         *
 * input:               : LPTHREAD_START_ROUTINE CallbackAddressMeasurementCell : thread start routine measurement cell    *
 *                        LONG ( *pFCallBackCell ) (...) : callback function for fast measurement                          *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                               0 = no error                                                                              *
 *                              -1 = parameter error fast measurement callback                                             *
 *                              -2 = thread running or parameter error                                                     *
 *                              -3 = no thread created                                                                     *
 *                                                                                                                         *
 * description:         : This is the function to create the cell handle thread.                                           *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::InstallThread( LPTHREAD_START_ROUTINE CallbackAddressMeasurementCell, 
													LPTHREAD_START_ROUTINE CallbackAddressReferenceCell, 
													LPTHREAD_START_ROUTINE CallbackAddressChamber,
													LONG ( *pFCallBackMeasurementCell )( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx ), 
													LONG ( *pFCallBackReferenceCell )( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx ) )
{
	int RetVal = 0;
	DWORD dwThreadId, dwThrdParam;

	
	if( ( SETrimmingChamber::hThreadHandleProcessChamber[ChamberId - 1] == NULL) &&
			( CallbackAddressChamber != NULL ) )
	{
		//create thread
		//no security, default stack size, default creation
		SETrimmingChamber::hThreadHandleProcessChamber[ChamberId - 1] = CreateThread( NULL,		
																											0,                       
																											CallbackAddressChamber,             
																											&dwThrdParam,               
																											CREATE_SUSPENDED,                         
																											&dwThreadId);  
		printf( "Chamber[%d]:InstallThread:CreateThread=%lx\n", ChamberId, dwThreadId );
	}

	TrimmCell->InstallThread(CallbackAddressMeasurementCell, pFCallBackMeasurementCell );
	ReferenceCell->InstallThread(CallbackAddressReferenceCell, pFCallBackReferenceCell );

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void DeinstallThread( void )                                                                     *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to exit chamber handle thread.                                              *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::DeinstallThread( void )
{

	DWORD ExitCode = 0;
	DeinstallThreadRequest = true;
	do
	{
		Sleep(100);
		GetExitCodeThread( SETrimmingChamber::hThreadHandleProcessChamber[ChamberId - 1], &ExitCode);
	}while( ExitCode == STILL_ACTIVE );
			
	printf( "Chamber[%d]:DeinstallThread:GetExitCodeThread=%d\n", ChamberId, ExitCode );

	DeinstallThreadRequest = false;

	if( SETrimmingChamber::hThreadHandleProcessChamber[ChamberId - 1] != NULL)
	{
		SETrimmingChamber::hThreadHandleProcessChamber[ChamberId - 1] = NULL;
		printf( "Chamber[%d]:Thread terminated!\n", ChamberId );
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool DeinstallThreadRequested( void )                                                            *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = deinstall not requested                                                         *
 *                                 true = deinstall requested                                                              *
 *                                                                                                                         *
 * description:         : This is the function to check the deinstall thread request.                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SETrimmingChamber::DeinstallThreadRequested( void )
{
	return DeinstallThreadRequest;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SETrimmingChamber::StartProcess( ProcessType Type )                                          *
 *                                                                                                                         *
 * input:               : ProcessType Type : type of process to start                                                      *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to start the process.                                                       *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::StartProcess( ProcessType Type )
{
	int RetVal = 0;

	ProcessType TypeTemp = Type;;
	eProcessType = ProcessType::Undefined; //RH: ggf. PorcessType bereits hier setzten, 
	//dann muss nicht in switch case noch einmal gesetzen werden

	DigitalOut = 0;
	DigitalIn = 0;
	IgnoreNewPressure = false;

	ClearAllValues();

	if( TypeTemp == ProcessType::MeasureTrimmingSelection ) 
	{
		if( SETrimmingValues.AssemblyDataCommon.ProcessType == 1 )
		{
			TypeTemp = ProcessType::MeasureSelection;
		}
		else
		{
			TypeTemp = ProcessType::MeasureTrimming;
		}	
	}

	printf("##TypeTemp: %d\n", TypeTemp);
	switch( TypeTemp )
	{
		case ProcessType::MeasureTrimming:			// measure and trimming
		case ProcessType::MeasureSelection:			// measure and selection
		case ProcessType::IP450MeasUNernstControl: //10: IP4 measurement under Nernst voltage control  
		case ProcessType::IP450Meas2PointUp:       //11: IP4 measurement via 2-point UP measurement
		{
			// chamber status 
			ChamberState = INF_GENERATE_TEST_SEQUENCE;

			// test sequence for reference cell
			ReferenceCell->GenerateAndTransmittSequence( TypeTemp );
			ReferenceCell->StartSequence();

			// test sequence for measurement cell
			TrimmCell->GenerateAndTransmittSequence( TypeTemp );
			TrimmCell->StartSequence();

			//ProcessTrimmingActive = true;
			eProcessType = TypeTemp;
			ChamberState = INF_START_TEST_SEQUENCE;
			break;
		}
		case ProcessType::MeasurePositionNormal:
		case ProcessType::MeasureHeaterNormal:
		case ProcessType::MeasureUniversalNormal:
		case ProcessType::MeasureUnNormal:
		case ProcessType::MeasureIpNormal:
		case ProcessType::MeasureIlmNormal:

			// test sequence calibration board for wiring and position check
			TrimmCell->GenerateAndTransmittSequence( TypeTemp );
			TrimmCell->StartSequence();

			eProcessType = TypeTemp;
			break;

		default:
			RetVal = -1; 
			break;
	}

	if( SETrimmingChamber::hThreadHandleProcessChamber[ChamberId - 1] != NULL )
	{
		RetVal = ResumeThread( SETrimmingChamber::hThreadHandleProcessChamber[ChamberId - 1] );
		if( RetVal >= 0 )
		{
			printf( "Chamber[%d]:Thread resume=%d\n", ChamberId, RetVal );
		}
		else
		{
			printf( "Chamber[%d]:Thread resume=%d;%d\n", ChamberId, RetVal, GetLastError() );
		}
	}
	else
	{
		printf( "Chamber[%d]:Thread not resumed!\n", ChamberId );
	}
	
	return 0;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : bool SETrimmingChamber::ProcessFinished( void )                                                  *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : bool : return value                                                                              *
 *                                 false = process running                                                                 *
 *                                 true = process finished                                                                 *
 *                                                                                                                         *
 * description:         : This is the function to check process finished.                                                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
bool SETrimmingChamber::ProcessFinished( void )
{
	bool TrimmingFinished = true;

	for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
	{
		TrimmingFinished &= SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData.EntireProcessDone; 	
	}

	return TrimmingFinished;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int ClearAllValues( void )                                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to clear all chamber values.                                                *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::ClearAllValues( void )
{
	int RetVal = 0;

	printf("ClearAllValues Chamber Start\n");
	if( WaitForSingleObject( ghMutex[ChamberId - 1], 10000) == WAIT_OBJECT_0 ) 
	{
		try
		{
			ProcessStep = EProcessStep::Initialization;
			CurrProcessStep = EProcessStep::Undefined;
			NormalStep = 0;
			StabilityStep = 0;
			ChamberPressureActual = 0.0f;
			memset( &ChamberPressureActualArray, 0, sizeof( ChamberPressureActualArray ) );
			ChamberPressureActualArrayCount = 0;
			ChamberFlowActual = 0.0f;
			ChamberState = INF_EMPTY;

			for( int i = 0; i < CELL_TIEPOINT_COUNT; i++ )
			{
				memset( &SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData, 0, sizeof( SETrimmingValues.SETrimmingValuesTiepoint[i].ProcessData ) );
				memset( &SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData, 0, sizeof( SETrimmingValues.SETrimmingValuesTiepoint[i].ResultData ) );
				memset( &SENormalValues[i], 0, sizeof( SENormalValues[i] ) );
			}

			memset( &IpRefCorrectionZero, 0, sizeof( IpRefCorrectionZero ) );
			memset( &IpRef, 0, sizeof( IpRef ) );
			memset( &IpRefBeforeProcess, 0, sizeof( IpRefBeforeProcess ) );
			memset( &IpRefAfterProcess, 0, sizeof( IpRefAfterProcess ) );
		}
		catch(...)
		{
			printf("Exception!\n");	
		}
		ReleaseMutex( ghMutex[ChamberId - 1] );
	}

	printf("ClearAllValues Chamber End\n");
	
	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int StopProcess( void )                                                                          *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to stop the process.                                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::StopProcess( void )
{
	int RetVal = 0;

	DigitalOut = 0;
	DigitalIn = 0;

	DeinstallThread();
	
	TrimmCell->StopSequence();
	TrimmCell->DeinstallThread();
	ReferenceCell->StopSequence();
	ReferenceCell->DeinstallThread();

	return RetVal;
}
	
/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SETrimmingCell * GetTrimmingCell( void )                                                         *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : SETrimmingCell * : pointer to trimming cell                                                      *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to trimming cell.                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
SETrimmingCell * SETrimmingChamber::GetTrimmingCell( void )
{
	return TrimmCell;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : SEReferenceCell * GetReferenceCell( void )                                                       *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : SEReferenceCell * : pointer to reference cell                                                    *
 *                                                                                                                         *
 * description:         : This is the function to get the pointer to reference cell.                                       *
 *-------------------------------------------------------------------------------------------------------------------------*/
SEReferenceCell * SETrimmingChamber::GetReferenceCell(void)
{
	return ReferenceCell;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetActualPressure( float ActualPressure )                                                   *
 *                                                                                                                         *
 * input:               : float ActualPressure : actual chamber pressure                                                   *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set the actual chamber pressure.                                         *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::SetActualPressure( float ActualPressure )
{
	float PressureSum = 0.0f;
	if( IgnoreNewPressure == false )
	{
		/*Original-->*///ChamberPressureActual = ActualPressure; 
		/*Original-->*///Änderung "IgnoreNewPressure = true" einkommentieren 

		/*Variante1-->*///previous value
		/*Variante1-->*///ChamberPressureActualArray[ ChamberPressureActualArrayCount ] = ActualPressure;
		/*Variante1-->*///ChamberPressureActual = ChamberPressureActualArray[ ( ChamberPressureActualArrayCount + 7 ) % 10 ];
		/*Variante1-->*///ChamberPressureActualArrayCount = ( ChamberPressureActualArrayCount + 1 ) % 10;
		
		/*Variante2-->*///mean value last 4
		/*Variante2-->*/ChamberPressureActualArray[ ChamberPressureActualArrayCount ] = ActualPressure;
		/*Variante2-->*/ChamberPressureActualArrayCount = ( ChamberPressureActualArrayCount + 1 ) % 4;
		/*Variante2-->*/for( int i = 0; i < 4; i++ )
		/*Variante2-->*/{
		/*Variante2-->*/	PressureSum += ChamberPressureActualArray[ i ];
		/*Variante2-->*/}
		/*Variante2-->*/ChamberPressureActual = PressureSum / 4.0f;

		/*Variante3-->*///previous value mean last 3
		/*Variante3-->*///ChamberPressureActualArray[ ChamberPressureActualArrayCount ] = ActualPressure;
		/*Variante3-->*///PressureSum += ChamberPressureActualArray[ ( ChamberPressureActualArrayCount + 9 ) % 10 ];
		/*Variante3-->*///PressureSum += ChamberPressureActualArray[ ( ChamberPressureActualArrayCount + 8 ) % 10 ];
		/*Variante3-->*///PressureSum += ChamberPressureActualArray[ ( ChamberPressureActualArrayCount + 7 ) % 10 ];
		/*Variante3-->*///PressureSum += ChamberPressureActualArray[ ( ChamberPressureActualArrayCount + 6 ) % 10 ];
		/*Variante3-->*///ChamberPressureActual = PressureSum / 4.0f;
		/*Variante3-->*///ChamberPressureActualArrayCount = ( ChamberPressureActualArrayCount + 1 ) % 10;

		
		/*for( int i = 9; i >= 0; i-- )
		{
			printf("Pressure %d=%f\n",i-9 ,ChamberPressureActualArray[ ( ChamberPressureActualArrayCount + i ) % 10 ] );
		}*/
	}
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetActualFlow( float ActualFlow )                                                           *
 *                                                                                                                         *
 * input:               : float ActualFlow : actual chamber flow                                                           *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set the actual chamber flow.                                             *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::SetActualFlow( float ActualFlow )
{
	ChamberFlowActual = ActualFlow;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : float IpCorrection( float IpMeasured )                                                           *
 *                                                                                                                         *
 * input:               : float IpMeasured : measured ip value (raw value)                                                 *
 *                                                                                                                         *
 * output:              : float: corrected ip value                                                                        *
 *                                                                                                                         *
 * description:         : This is the function to correct the measured ip value with specific parameters.                  *
 *-------------------------------------------------------------------------------------------------------------------------*/
float SETrimmingChamber::IpCorrection( float IpMeasured, float fKValue)
{
	float RetVal = 0.0;
	float PartialPressureMrg = 0.0, PartialPressureO2 = 0.0;


	switch( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpCorrectionMode )
	{
		case 0:																																				 							 																																		
			PartialPressureMrg = ( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.O2Mrg * ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint ) / 100.0f;
			PartialPressureO2 = ( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.O2Air * SETrimmingValues.AssemblyDataCommon.EvacuatedPressure / 100.0f ) + 
													( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.O2Mrg * ( ChamberPressureActual - SETrimmingValues.AssemblyDataCommon.EvacuatedPressure ) / 100.0f );
			RetVal = IpMeasured  * ( PartialPressureO2 / PartialPressureMrg ) * 
							 ( ChamberPressureActual / (fKValue + ChamberPressureActual ) ) *
							 ( ( fKValue + ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint ) / ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.PSetpoint );
			if(RetVal == 0.0)
			{
				printf( "Chamber[%d]:Evacuated=%f, Actual=%f\n", ChamberId, SETrimmingValues.AssemblyDataCommon.EvacuatedPressure, ChamberPressureActual );
			}
			
			break;

		case 1:
			RetVal = IpMeasured * ( ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[0] / IpRef[0] );
			break;

		case 2:
			RetVal = IpMeasured * ( ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[1] / IpRef[1] );
			break;

		case 3:
			RetVal = IpMeasured * ( ( ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[0] + ParamStationTrimmChamber.ParamStationRefCell.ParamStationMainReferenceCell.IpSetpointRef[1] ) / ( IpRef[0] + IpRef[1] ) );
			break;

		default:

			break;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : int SetPositionImageProcessing ( int Tiepoint, float XPos, float YPos, float ZPos,               *
 *                              float Diameter )                                                                           *
 *                                                                                                                         *
 * input:               : int Tiepoint : tiepoint                                                                          *
 *                        float XPos : x-position                                                                          *
 *                        float YPos : y-position                                                                          *
 *                        float ZPos : z-position                                                                          *
 *                        float Diameter : diameter of hole                                                                *
 *                                                                                                                         *
 * output:              : int : return value                                                                               *
 *                                 0 = no error                                                                            *
 *                                                                                                                         *
 * description:         : This is the function to det the positions of image processing.                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
int SETrimmingChamber::SetPositionImageProcessing ( int Tiepoint, float XPos, float YPos, float ZPos, float Diameter )
{
	int RetVal = 0;

	if( ( Tiepoint >= 1 ) &&
			( Tiepoint <= CELL_TIEPOINT_COUNT ) &&
			( abs( XPos ) < 50.0f ) &&
			( abs( YPos ) < 50.0f ) )
	{
		ImageProcessing[Tiepoint - 1].XPos = XPos;
		ImageProcessing[Tiepoint - 1].YPos = YPos;
		ImageProcessing[Tiepoint - 1].ZPos = ZPos;
		ImageProcessing[Tiepoint - 1].Diameter = Diameter;
		ParameterImageProcessingOk = true;
	}
	else
	{
		ParameterImageProcessingOk = false;
		RetVal = -1;
	}

	return RetVal;
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : void SetDigitalIn( unsigned int DigitalIn )                                                      *
 *                                                                                                                         *
 * input:               : unsigned int DigitalIn : digital in flag                                                         *
 *                                                                                                                         *
 * output:              : void                                                                                             *
 *                                                                                                                         *
 * description:         : This is the function to set the digital trigger flags (input from plc communication).            *
 *-------------------------------------------------------------------------------------------------------------------------*/
void SETrimmingChamber::SetDigitalIn( unsigned int DigitalIn )
{
	this->DigitalIn = DigitalIn;
}


/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : unsigned int GetDigitalOut( void )                                                               *
 *                                                                                                                         *
 * input:               : void                                                                                             *
 *                                                                                                                         *
 * output:              : unsigned int DigitalOut : digital out flag                                                       *
 *                                                                                                                         *
 * description:         : This is the function to get the digital trigger flags (output to plc communication).             *
 *-------------------------------------------------------------------------------------------------------------------------*/
unsigned int SETrimmingChamber::GetDigitalOut( void )
{
	return DigitalOut; 
}

/*-------------------------------------------------------------------------------------------------------------------------*
 * function name        : float IpCorrectionOffsetAndDynamic( float IpMeasured, bool OffsetGasoline)                       *
 *                                                                                                                         *
 * input:               : float IpMeasured : measured ip value                                                             *
 *                                                                                                                         *
 * output:              : float : corrected ip value                                                                       *
 *                        bool : offset gasoline or diesel                                                                 *
 *                                                                                                                         *
 * description:         : This is the function to correct the measured ip value with reference sensors.                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
float SETrimmingChamber::IpCorrectionOffsetAndDynamic( float IpMeasured, bool OffsetGasoline)
{
	float RetVal = 0.0;
																	
	// Offset für Übereinstimmung Laserabgleichanlage und Stuttgart
	if( OffsetGasoline == true )
	{ 
		if ((getProcessType() == ProcessType::IP450Meas2PointUp) ||
			(getProcessType() == ProcessType::IP450MeasUNernstControl))
		{
			RetVal = IpMeasured + (ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationTrimmingProcess.IpOffsetGasoline / 1.0e6f);
		}
		else
		{
			RetVal = IpMeasured - (ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationTrimmingProcess.IpOffsetGasoline / 1.0e6f);
		}
	}
	else
	{
		RetVal = IpMeasured - ( ParamStationTrimmChamber.ParamStationTrimmProcess.ParamStationTrimmingProcess.IpOffsetDiesel / 1.0e6f );
	}

				 
	// Dynamische Korrektur wegen unerklärlicher Variation der Messwerte am Anfang des Prozesses 
	switch( ParamTrimmChamber.ParamTrimmCell.ParamIpMeasurement.IpCorrectionSource )
	{
		// nichts machen
	case 0:
		break;
		// Referenzsensor 1 benutzen
	case 1:
		RetVal +=	IpRefCorrectionZero[0] - IpRef[0];
		break;
	case 2:
		RetVal +=	IpRefCorrectionZero[1] - IpRef[1];
		break;
	case 3:
		RetVal +=	( ( IpRefCorrectionZero[0] - IpRef[0] ) + ( IpRefCorrectionZero[1] - IpRef[1] ) ) / 2.0f;
		break;

	}

	return RetVal;
}