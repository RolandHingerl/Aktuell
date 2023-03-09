/**********************************************************************************************/
/* Projekt:  SETestDLL für indutron SE-Prüftechnik                         Datei: SETestDLL.H */
/* Version:  V2.59                                                         Datum: 12.08.2013  */
/* Inhalt:   Definition der externen Schnittstelle                                            */
/*   (c) 2003-2013 indutron Industrieelektronik GmbH, D-83026 Rosenheim                       */
/**********************************************************************************************/

#ifndef _setestdll_h_
#define _setestdll_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

#include "SEconstants.h"

#if !defined(EXPORT)
#define EXPORT __declspec (dllexport)    /* Macro for DLL functions */
#endif

#if !defined(CALLTYPE)
#define CALLTYPE
#endif

#if defined(SETPOINTRESIDX) /* ResIdx bei Stellgrößen? */
 #define SPRESIDX ,LONG ResultIdx
#else
 #define SPRESIDX
#endif


/*...prototypes..............................................................*/


EXPORT LONG CALLTYPE TFlwCapRETimeIn(HANDLE handle,float TestTime,float EvalStartVolt,float TurnOffVolt,float MaxTime,LONG ResultIdxTime);
EXPORT LONG CALLTYPE TFlwCapRECapIn(HANDLE handle,float TestTime,float PumpCurr,LONG ResultIdxIP4,float SampleRate,LONG ResultIdxCap);
EXPORT LONG CALLTYPE TFlwCapREIn(HANDLE handle,float TestTime,float PumpCurr,LONG ResultIdxIP4,float SampleRate,float EvalStartVolt,float TurnOffVolt,float MaxTime,LONG ResultIdxTime,LONG ResultIdxCap);
EXPORT LONG CALLTYPE TFlwALKIn(HANDLE handle,float TestTime,LONG Flags,float PumpCurr,LONG ResultIdxIP4,float PumpCurrOffs,float TotalOffs,float SampleRate,float EvalStartVolt,float TurnOffVolt,float MaxTime,LONG ResultIdxStartTime,LONG ResultIdxStoppTime,LONG ResultIdxCap,LONG ResultIdxIp,LONG ResultIdxIpMin,LONG ResultIdxIpMax);
EXPORT LONG CALLTYPE TFlwChOffMaskSet(HANDLE handle,float TestTime,DWORD ChMask SPRESIDX);
EXPORT LONG CALLTYPE TFlwChSelect(HANDLE handle,float TestTime,DWORD ChMask SPRESIDX);
EXPORT LONG CALLTYPE TFlwChSPValSet(HANDLE handle,float TestTime,LONG Type,float rScale,float Val01,float Val02,float Val03,float Val04,float Val05,float Val06,float Val07,float Val08,float Val09,float Val10,float Val11,float Val12,float Val13,float Val14,float Val15,float Val16,float Val17,float Val18,float Val19,float Val20 SPRESIDX);
EXPORT LONG CALLTYPE TFlwContChkHS(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwContChkSS(HANDLE handle,float TestTime,float RiACMax,float PumpCurrMin,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwContChkSSExt(HANDLE handle,float TestTime,LONG Flags,float RiACMin,float RiACMax,float PumpCurrMin,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwCycLeakCurr(HANDLE handle,float StartTime,float EndTime,float Period,LONG ResultIdxILM,LONG ResultIdxIL);
EXPORT LONG CALLTYPE TFlwDIHdtIn(HANDLE handle,float TestTime,float t1,float dt,LONG ResultIdxIH1,LONG ResultIdxDIHdt);
EXPORT LONG CALLTYPE TFlwDRHdtIn(HANDLE handle,float TestTime,float t1,float dt,LONG ResultIdxRH1,LONG ResultIdxDRHdt);
EXPORT LONG CALLTYPE TFlwExtSyncIn(HANDLE handle,float TestTime,long lEvFlags,float rTimeOut,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwDynTestCurrAPESet(HANDLE handle,float TestTime,float rFacA,float rOffsB SPRESIDX);
EXPORT LONG CALLTYPE TFlwDynTestCurrRESet(HANDLE handle,float TestTime,float rFacA,float rOffsB SPRESIDX);
EXPORT LONG CALLTYPE TFlwFinalCurrRefChIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwGenerate(HANDLE handle);
EXPORT LONG CALLTYPE TFlwHeaterColdResIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterColdResInExt(HANDLE handle,float TestTime,LONG Flags,float HeaterVolt,float MeasDly,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterColdResCompIn(HANDLE handle,float TestTime,LONG Flags,float MinColdRes,float MaxHeatEnergy,float MaxHeaterVolt,float AmbientTemp,float Alpha,float QUHloLim,float QUHhiLim,float QHSloLim,float QHShiLim,LONG ResIdxRHkTemp,LONG ResIdxRHk,LONG ResIdxKontHZ,LONG ResIdxKontUHS,LONG ResIdxKontQUH,LONG ResIdxKontRHk,LONG ResIdxKontQHS);
EXPORT LONG CALLTYPE TFlwHeaterColdResCompIn2(HANDLE handle, float kfTestTime, LONG kuFlags, float MinColdRes, float MaxHeatEnergy, float MaxHeaterVolt, float AmbientTemp,
	float Alpha, float QUHloLim, float QUHhiLim, float QHSloLim, float QHShiLim,
	float kfMinColdResSwitchOff, float kfMaxColdResSwitchOff,
	LONG ResIdxRHkTemp, LONG ResIdxRHk, LONG ResIdxKontHZ, LONG ResIdxKontUHS, LONG ResIdxKontQUH, LONG ResIdxKontRHk, LONG ResIdxKontQHS);
EXPORT LONG CALLTYPE TFlwHeaterCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterCurrSet(HANDLE handle,float TestTime,float HeaterCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterCurrLimSet(HANDLE handle,float TestTime,float HeaterCurrLim SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterEnergyIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterHotResIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterPowIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterPowIntvIn(HANDLE handle,float TestTime,float Interval,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterPowerSet(HANDLE handle,float TestTime,float HeaterPower,float HeaterStartVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterPowerSetExt(HANDLE handle,float TestTime,float HeaterPower,float HeaterStartVolt,float MaxHeaterVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterRelaisSet(HANDLE handle,float TestTime,DWORD RelaisOn SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterResSet(HANDLE handle,float TestTime,float HeaterRes,float HeaterStartVolt,float MaxHeaterVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltClockingSet(HANDLE handle,float TestTime,LONG Mode,float HeaterVolt,float OnTime,float OffTime SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterVoltSet(HANDLE handle,float TestTime,float HeaterVolt,float RampTime SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltSetExt(HANDLE handle,float TestTime,float HeaterVolt,float RampTime,LONG Flags SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltSetNoRamp(HANDLE handle, float TestTime, float HeaterVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltSupvSet(HANDLE handle, float TestTime, float HeatVoltMin, float HeatVoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltPowDepSet(HANDLE handle,float TestTime,LONG Flags,float FinalPow,float MaxHeaterVolt,float RampTime,LONG ResIdxUHstart SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltSupvRelSet(HANDLE handle,float TestTime,float HeatVoltMin,float HeatVoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwHLineResCal(HANDLE handle,float TestTime SPRESIDX);
EXPORT LONG CALLTYPE TFlwIPOVAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwIPOVREIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwL1wIn(HANDLE handle,float TestTime,float Amplitude,float Frequency,float PeriodsBeforeMeas,float MeasTimeUN,float MeasPeriods,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwLastError(HANDLE handle);
EXPORT LONG CALLTYPE TFlwLeakCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSet(HANDLE handle,float TestTime,float LeakCurrVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSetExt(HANDLE handle,float TestTime,float LeakCurrVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSupvSet(HANDLE handle, float TestTime, float LeakCurrVoltMin, float LeakCurrVoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSupvIn(HANDLE handle,float TestTime,LONG ResultIdxMin,LONG ResultIdxMax,LONG ResultIdxTime);
EXPORT LONG CALLTYPE TFlwMaxHeaterCurrSet(HANDLE handle,float TestTime,float MaxHeaterCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwMaxHeaterVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMaxLeakCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMaxMaxLeakCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMeasSupvSet(HANDLE handle,float TestTime,int/*eMMWIDX*/ eType,float LoLim,float HiLim SPRESIDX);
EXPORT LONG CALLTYPE TFlwMeasSupvIn(HANDLE handle,float TestTime,int/*eMMWIDX*/ eType,LONG ResultIdxMin,LONG ResultIdxMax,LONG ResultIdxTime);
EXPORT LONG CALLTYPE TFlwMinHeaterVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwNernstVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwNew(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE TFlwOVSHRegSet(HANDLE handle,float TestTime,LONG Flags,float UHMeasDly,float NotifyRHh,float NotifyRiAC,float TurnOffRHh,float TurnOffRiAC,float FaultRHh,float FaultRiAC,float IAC,float Freq,float MeasDly,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwOVSGetResults(HANDLE handle,float TestTime,LONG ResIdxUHstart,LONG ResIdxtRHNfy,LONG ResIdxtRiNfy,LONG ResIdxtRH,LONG ResIdxtRi,LONG ResIdxEH,LONG ResIdxRH,LONG ResIdxRi);
EXPORT LONG CALLTYPE TFlwOVSGetResults2(HANDLE handle, float TestTime, LONG ResIdxtRHh, LONG ResIdxRiACn, LONG ResIdxRiACp, LONG ResIdxIP4, LONG ResIdxPHh);
EXPORT LONG CALLTYPE TFlwPumpCurrAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpCurrNernstIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpCurrREIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpCurrRESet(HANDLE handle,float TestTime,float PumpCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpCurrREDynSet(HANDLE handle,float TestTime,float Offset,float Factor SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpVoltAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpVoltAPESet(HANDLE handle,float TestTime,float PumpVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpVoltREIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpVoltRESet(HANDLE handle,float TestTime,float PumpVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpVoltREDynSet(HANDLE handle,float TestTime,float PumpVoltOffs,float PumpCurrFac,float PumpVoltMin,float PumpVoltMax,float RiMax,float PumpVoltRiMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpVoltRampAPEStart(HANDLE handle,float TestTime,float PumpVoltInc,float Period,LONG ResultIdxIp,LONG ResultIdxUN);
EXPORT LONG CALLTYPE TFlwPumpVoltRampREStart(HANDLE handle,float TestTime,float PumpVoltInc,float Period,LONG ResultIdxIpN);
EXPORT LONG CALLTYPE TFlwRelIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwRiACConfig(HANDLE handle,float TestTime,float IAC,float Freq,float MeasDly);
EXPORT LONG CALLTYPE TFlwRiACIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwRiACInExt(HANDLE handle,float TestTime,float IAC,float Freq,float MeasDly,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwRiBIn(HANDLE handle,float TestTime,float AmplitudePl,float PulseOnTimePl,float MeasTimePl,float AmplitudeMi,float PulseOnTimeMi,float MeasTimeMi,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwRiDCnIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwRiPIDRegSet(HANDLE handle,float TestTime,float Ri,float IAC,float Freq,float MeasDly,float MaxUH,float Kp,float Ki,float Kd,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwRiPIDRegSet2(HANDLE handle,float TestTime,float Ri,float MaxUH,float Kp,float Ki,float Kd,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwRiPulseIn(HANDLE handle,float TestTime,float Amplitude,float Frequency,float PulseOnTime,float MeasPeriods,float MeasTimeBef,float MeasTimePuls,float MeasTimeAft,LONG ResultIdxUvor,LONG ResultIdxUpuls,LONG ResultIdxUnach,LONG ResultIdxRiPuls);
EXPORT LONG CALLTYPE TFlwRiPulseConfig(HANDLE handle,float TestTime,float Amplitude,float Frequency,float PulseOnTime,float MeasPeriods/*,LONG ResultIdx*/);
EXPORT LONG CALLTYPE TFlwRiPulseUvorIn(HANDLE handle,float TestTime,float MeasTimeBef,LONG ResultIdxUvor);
EXPORT LONG CALLTYPE TFlwRiPulseUpulsIn(HANDLE handle,float TestTime,float MeasTimePuls,LONG ResultIdxUpuls);
EXPORT LONG CALLTYPE TFlwRiPulseUnachIn(HANDLE handle,float TestTime,float MeasTimeAft,LONG ResultIdxUnach);
EXPORT LONG CALLTYPE TFlwRiPulseRiPulsIn(HANDLE handle,float TestTime,LONG ResultIdxRiPuls);
EXPORT LONG CALLTYPE TFlwRiRegSet(HANDLE handle,float TestTime,float Ri,float MaxUH,float Kp,float Ki,float Kd,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwRiRegSupv(HANDLE handle,float TestTime,LONG Mode,float RiMin,float RiMax,LONG MinResIdx,LONG MaxResIdx);
EXPORT LONG CALLTYPE TFlwRiPIDSnapRegConfig(HANDLE handle,float TestTime,float UH,float PH,float MaxSnapTime,float RiSnapLo,float RiSnapHi);
EXPORT LONG CALLTYPE TFlwRiPIDSnapRegSet2(HANDLE handle,float TestTime,float Ri,float MaxUH,float Kp,float Ki,float Kd,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwRiPIDSnapRegSet(HANDLE handle,float TestTime,float UH,float PH,float MaxSnapTime,float RiSnapLo,float RiSnapHi,float Ri,float IAC,float Freq,float MeasDly,float MaxUH,float Kp,float Ki,float Kd,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwRiPIDSnapRegSet3(HANDLE handle,float TestTime,long Flags,float UH,float PH_RampTime,float MaxSnapTime,float RiSnapLo,float RiSnapHi,float Ri,float RiPhLo,float RiPhHi,float IAC,float Freq,float MeasDly,float MaxUH,float Kp,float Ki,float Kd,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwDtEHRiIn(HANDLE handle,float TestTime,LONG ResultIdxDt,LONG ResultIdxEH);
EXPORT LONG CALLTYPE TFlwRiSnapEHIn(HANDLE handle,float TestTime,LONG ResultIdxEH);
EXPORT LONG CALLTYPE TFlwRiSnapDtIn(HANDLE handle,float TestTime,LONG ResultIdxDt);
EXPORT LONG CALLTYPE TFlwRiSyncSens(HANDLE handle,float TestTime,float MeasDlyMin SPRESIDX);
EXPORT LONG CALLTYPE TFlwSensorRelaisSet(HANDLE handle,float TestTime,DWORD RelaisOn SPRESIDX);
EXPORT LONG CALLTYPE TFlwSwitchFromRiToPHReg(HANDLE handle,float TestTime,float PHOffset,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTemperatureIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTestCurrAPESet(HANDLE handle,float TestTime,float TestCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrAPESet2(HANDLE handle,float TestTime,float TestCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTestCurrRESet(HANDLE handle,float TestTime,float TestCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrRESet2(HANDLE handle,float TestTime,float TestCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrWdwAPESet(HANDLE handle,float TestTime,float TestCurrMin,float TestCurrMax,float VoltMin,float VoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrWdwEnd(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTestCurrWdwRESet(HANDLE handle,float TestTime,float TestCurrMin,float TestCurrMax,float VoltMin,float VoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrWdwRESet2(HANDLE handle,float TestTime,float TestCurrMin,float TestCurrMax,float VoltMin,float VoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwTimeEvent(HANDLE handle,float StartTime,float EndTime,float Period,float UserVal1,float UserVal2,float UserVal3,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwUAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwUNPIRegSet(HANDLE handle,float TestTime,float UN,float MaxUP,float Kp,float Ki,float MeasDly,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwMeasFLO(HANDLE handle,float TestTime, float kfDuration, float kfMinIp, float kfStabilityLimit, float kfIpUG, float fIpOG 								 , float fStartAveraging, float fStartDelayRiMMW, float fRiUG, float fRiOG,
								 float kfStartDelayContSs, LONG ResultIdxIpStab, LONG ResultIdx_t_Stab, 
								 LONG ResultIdx_tFLO_UG, LONG tIdx_tFLO_OG, LONG ResultIdx_t63, LONG ResultIdxIpMax, 
								 LONG ResultIdx_t_Max, LONG ResultIdx_RiAC_FLO);
EXPORT LONG CALLTYPE TFlwMeasUHRampSupvSet(HANDLE handle, float TestTime, int kiFlags, float fMin, float fMax, float kfEvalDuration SPRESIDX);
EXPORT LONG CALLTYPE TFlwMeasUHRampSupvIn(HANDLE handle,float TestTime,int eType,LONG ResultIdxMaxvalue,LONG ResultIdxTimeMaxvalue,
										  LONG ResultIdxTimeInRange);
EXPORT LONG CALLTYPE TFlwMeasStoredValue(HANDLE handle,float TestTime,int eType,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwCycMeasPrefix(HANDLE handle, LONG Flags, float Period, LONG Count);
EXPORT LONG CALLTYPE TFlwCycMeasPrefix2(HANDLE handle, LONG Flags, float Period, LONG Count, float rIntegrIntv);
EXPORT LONG CALLTYPE TFlwMeasMinMaxSet(HANDLE handle,float TestTime, DWORD kuFlags,int/*eMMWIDX*/ eType, float kfDuration, float kfInterval SPRESIDX);  
EXPORT LONG CALLTYPE TFlwMeasMinMaxIn(HANDLE handle,float kfTestTime, DWORD kuFlags, int/*eMMWIDX*/ ki_eType, LONG ResultIdxMin,LONG ResultIdxMax,
									 LONG ResultIdxTSMin, LONG ResultIdxTSMax);
EXPORT LONG CALLTYPE TFlwGetTS(HANDLE handle,float kfTestTime, WORD kuIndex, LONG kuResultIdxTS);

EXPORT LONG CALLTYPE TFlwMeasAverage(HANDLE handle,float kfTestTime, DWORD kuFlags, int/*eIMTBasTypes*/ ki_eIMTBasType, float kfInterval, float kfFrequency, LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMeasAverage2(HANDLE handle, float kfTestTime, DWORD kuFlags, 
	int/*eIMTBasTypes*/ ki_eIMTBasType1,
	int/*eIMTBasTypes*/ ki_eIMTBasType2,
	float kfIntegrationTime, float kfFrequency, LONG ResultIdx1, LONG ResultIdx2);
EXPORT LONG CALLTYPE TFlwHeaterRHh_PH_In(HANDLE handle,float kfTestTime, DWORD kuFlags, float kfInterval, LONG ResultIdxRHh, LONG ResultIdxPH);

EXPORT LONG CALLTYPE TFlwGenAndReport(HANDLE handle,const char *szReportPath,DWORD uEffStartTime);


#ifdef __cplusplus
}
#endif

#endif /* _setestdll_h_ */




