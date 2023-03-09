/**********************************************************************************************/
/* Projekt:  SETestDLL für indutron SE-Prüftechnik                         Datei: SETestDLL.H */
/* Version:  V2.51                                                         Datum: 12.08.2010  */
/* Inhalt:   Definition der externen Schnittstelle                                            */
/*   (c) 2003-2010 indutron Industrieelektronik GmbH, D-83026 Rosenheim                       */
/**********************************************************************************************/

#ifndef _setestdll_h_
#define _setestdll_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

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

/*------------------ Defines ------------------------------------------------*/
#define RESIDXPARTLEN 1000L /* -> 3stellige Teilnummernbereiche */
#define EFFRESIDXMS(main,sub) ((((((long)(main))*RESIDXPARTLEN)+((long)(sub)))))
#define EFFRESIDX(main,sub,inc) ((((((long)(main))*RESIDXPARTLEN)+((long)(sub)))*RESIDXPARTLEN)+(inc))
#define RESIDXMS(ResIdx) ((ResIdx)/RESIDXPARTLEN) /* Main+Sub abspalten */

typedef struct mcinfo { /* Prüfrechner-Information */
  unsigned short wType; /* Kartentyp */
  char sType[16]; /* Kartentyp in ASCII */
  unsigned short wSerialNo; /* Seriennummer */
//P: Evtl. Firmware-Version auch als Integer zurückliefern!?!
  float          rMainSoftVer; /* Firmware-Version (Hauptkarte) */
  float          rModSoftVer; /* Firmware-Version (Module) */
} MCINFO; /**/

typedef struct TFlwResult { /* Messstatus */
  LONG TFlowEnd; /* TRUE -> Prüfablauf beendet und alle Messwerte abgeholt */
  float TestTime; /* Aktueller Prüfablaufzeitpunkt */
  LONG ResultIdx; /* (Externer) Messindex (0 = kein Messwert) */
  float MeasRes[20]; /* Messwerte */
 } TFLWRESULT;

#ifndef MDATASETDEF
#define MDATASETDEF
typedef struct MeasInfStruct
{
  float rMVal; /* Messwert */
  DWORD tsMTime; /* Messzeitpunkt */
} MEASINF;
#endif

typedef struct TFlwResultTS { /* Messstatus, mit Messzeitpunkt zu jedem Messwert */
  LONG TFlowEnd; /* TRUE -> Prüfablauf beendet und alle Messwerte abgeholt */
  float TestTime; /* Aktueller Prüfablaufzeitpunkt */
  LONG ResultIdx; /* (Externer) Messindex (0 = kein Messwert) */
  MEASINF atMInf[20]; /* Messwerte + Zeitstempel */
 } TFLWRESULTTS;

#if 0
typedef struct ASYNCINPINF {  /* Asynchrone Informationen (MULTIF) (PDO3TX) */
 DWORD dwInpMirr; /* Eingangszustände */
 struct ASYNCERRINF { /* Fehlerinfo (Basisbewegung) */
   BYTE  byIdx; /* Index der Basisbewegung */
   BYTE  byReserve; /* Füll-Byte */
   short iLstErr; /* Fehlerinfo (Basisbewegung) */
 } ASYNCINPINF;
};
#endif

/*...prototypes..............................................................*/
EXPORT LONG CALLTYPE GetDLLVersion(void);
EXPORT LONG CALLTYPE SETestDLLOpen(/*DWORD*/LONG Flags,HANDLE *phandle);
EXPORT LONG CALLTYPE SETestDLLClose(HANDLE handle);
EXPORT LONG CALLTYPE LSTestDLLOpen(/*DWORD*/LONG Flags,HANDLE *phandle);
EXPORT LONG CALLTYPE LSTestDLLClose(HANDLE handle);
EXPORT LONG CALLTYPE MCxAssignCell(HANDLE handle,LONG CellNo,LONG McListLow,LONG McListHigh);
//EXPORT LONG CALLTYPE MCxAsyncStatus(HANDLE handle,LONG CellSpec,struct ASYNCINPINF *pDat);
EXPORT LONG CALLTYPE MCxBmStatus(HANDLE handle,LONG CellSpec,LONG *plBmIdx,LONG *plStatus);
EXPORT LONG CALLTYPE MCxCalib(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MCxChSel(HANDLE handle,LONG CellSpec,LONG ChMask);
EXPORT LONG CALLTYPE MCxContacting(HANDLE handle,LONG CellSpec,LONG Close);
EXPORT LONG CALLTYPE MCxCycResCallback(HANDLE handle,LONG CellSpec,LONG (*pFCallBack)(WORD wFUIdx,WORD wPartIdx,/*eIMTBasTypes*/int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx));
EXPORT LONG CALLTYPE MCxEvent(HANDLE handle,LONG CellSpec,LONG EvFlags);
EXPORT LONG CALLTYPE MCxFwUpload(HANDLE handle,LONG CellSpec,LONG Flags,char *szHexFile);
EXPORT LONG CALLTYPE MCxInfo(HANDLE handle,LONG CellNo,LONG MCNo,MCINFO *ptInfo);
EXPORT LONG CALLTYPE MCxInit(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MCxInitIO(HANDLE handle,LONG CellSpec);
EXPORT LONG CALLTYPE MCxInitRL(HANDLE handle,LONG Cell,float RL1,float RL2,float RL3,float RL4,float RL5,float RL6,float RL7,float RL8,float RL9,float RL10,float RL11,float RL12,float RL13,float RL14,float RL15,float RL16,float RL17,float RL18,float RL19,float RL20);
EXPORT LONG CALLTYPE MCxLastError(HANDLE handle,LONG CellSpec);
EXPORT LONG CALLTYPE MCxMultifErr(HANDLE handle,LONG CellSpec);
EXPORT LONG CALLTYPE MCxRdInpMirr(HANDLE handle,LONG CellSpec,DWORD *pdwInpMirr);
EXPORT LONG CALLTYPE MCxReport(HANDLE handle,LONG CellSpec,LONG Flags,BYTE *psFileName);
EXPORT LONG CALLTYPE MCxStart(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MCxStartSEQ(HANDLE handle,LONG CellSpec);
EXPORT LONG CALLTYPE MCxStartDM(HANDLE handle,LONG CellSpec);
EXPORT LONG CALLTYPE MCxStatus(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MCxStop(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MCxTFlowResChk(HANDLE handle,LONG Cell);
EXPORT float CALLTYPE MCxTFlowResMeasRes(HANDLE handle,LONG Cell,LONG Idx);
EXPORT LONG CALLTYPE MCxTFlowResMeasResType(HANDLE handle,LONG Cell,LONG Idx);
EXPORT LONG CALLTYPE MCxTFlowResResultIdx(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MCxTFlowResTFlowEnd(HANDLE handle,LONG Cell);
EXPORT float CALLTYPE MCxTFlowResTestTime(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MCxTFlowResult(HANDLE handle,LONG Cell,TFLWRESULT *pTFlwResult);
EXPORT LONG CALLTYPE MCxTFlowResultTS(HANDLE handle,LONG CellSpec,TFLWRESULTTS *pTFlwResultTS);
EXPORT LONG CALLTYPE MCxTFlowTx(HANDLE handle,LONG Cell);
EXPORT LONG CALLTYPE MeasSplittResIdx(HANDLE handle,LONG *plMainIdx,LONG *plSubIdx,LONG *plLocIdx,LONG lResIdx);
EXPORT LONG CALLTYPE SETestDLLSetOutpWdw(HWND hOutpWnd);
EXPORT LONG CALLTYPE SetDLLReportingPath(HANDLE handle,char *szLogPath);
EXPORT LONG CALLTYPE TFlwAIIn(HANDLE handle,LONG CardIdx,float TestTime,LONG Input,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwAOSet(HANDLE handle,LONG CardIdx,float TestTime,LONG Output,float Value,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwAVCSet(HANDLE handle,float TestTime,float MinHeatupCurrent,float MaxHeatupCurrent,float HeaterVoltageOV,float MaxVoltageReduction,float TemperatureFlatnessConst,float maxTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwBBSTempIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwBBSTempSet(HANDLE handle,float TestTime,float Temperature SPRESIDX);
EXPORT LONG CALLTYPE TFlwCapRETimeIn(HANDLE handle,float TestTime,float EvalStartVolt,float TurnOffVolt,float MaxTime,LONG ResultIdxTime);
EXPORT LONG CALLTYPE TFlwCapRECapIn(HANDLE handle,float TestTime,float PumpCurr,LONG ResultIdxIP4,float SampleRate,LONG ResultIdxCap);
EXPORT LONG CALLTYPE TFlwCapREIn(HANDLE handle,float TestTime,float PumpCurr,LONG ResultIdxIP4,float SampleRate,float EvalStartVolt,float TurnOffVolt,float MaxTime,LONG ResultIdxTime,LONG ResultIdxCap);
EXPORT LONG CALLTYPE TFlwChOffMaskSet(HANDLE handle,float TestTime,DWORD ChMask SPRESIDX);
EXPORT LONG CALLTYPE TFlwChSel(HANDLE handle,float TestTime,DWORD ChMask SPRESIDX);
EXPORT LONG CALLTYPE TFlwChSelect(HANDLE handle,float TestTime,DWORD ChMask SPRESIDX);
EXPORT LONG CALLTYPE TFlwChSPValSet(HANDLE handle,float TestTime,LONG Type,float rScale,float Val01,float Val02,float Val03,float Val04,float Val05,float Val06,float Val07,float Val08,float Val09,float Val10,float Val11,float Val12,float Val13,float Val14,float Val15,float Val16,float Val17,float Val18,float Val19,float Val20 SPRESIDX);
EXPORT LONG CALLTYPE TFlwActiveChSel(HANDLE handle,float TestTime,DWORD ChMask SPRESIDX);
EXPORT LONG CALLTYPE TFlwContChkHIT(HANDLE handle,float TestTime,float HeatVolt,float RHkMin,float RHkMax,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwContChkHS(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwContChkSS(HANDLE handle,float TestTime,float RiACMax,float PumpCurrMin,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwContacting(HANDLE handle,float TestTime,LONG Close,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwCycLeakCurr(HANDLE handle,float StartTime,float EndTime,float Period,LONG ResultIdxILM,LONG ResultIdxIL);
EXPORT LONG CALLTYPE TFlwDIIn(HANDLE handle,LONG CardIdx,float TestTime,LONG Input,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwDIHdtIn(HANDLE handle,float TestTime,float t1,float dt,LONG ResultIdxIH1,LONG ResultIdxDIHdt);
EXPORT LONG CALLTYPE TFlwDRHdtIn(HANDLE handle,float TestTime,float t1,float dt,LONG ResultIdxRH1,LONG ResultIdxDRHdt);
EXPORT LONG CALLTYPE TFlwDOSet(HANDLE handle,LONG CardIdx,float TestTime,LONG Output,long Value,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwDynTestCurrRESet(HANDLE handle,float TestTime,float rFacA,float rOffsB SPRESIDX);
EXPORT LONG CALLTYPE TFlwExtSyncIn(HANDLE handle,float TestTime,long lEvFlags,float rTimeOut,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwFinalCurrRefChIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT /*BYTE **/LONG CALLTYPE TFlwGenerate(HANDLE handle);
EXPORT LONG CALLTYPE TFlwHIT(HANDLE handle,float TestTime,LONG Flags,LONG lPosSE1,LONG PulseTimeUsec,LONG PauseTimeMsec,float VH,float VIRdarkMax,float VIRmax/*,LONG CHList*/,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterColdResIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterColdResInHIT(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterColdResInOrig(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterColdResInExt(HANDLE handle,float TestTime,LONG Flags,float HeaterVolt,float MeasDly,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterCurrSet(HANDLE handle,float TestTime,float HeaterCurr SPRESIDX);
//EXPORT LONG CALLTYPE TFlwHeaterEffVoltSet(HANDLE handle,float TestTime,float HeaterVoltEff,float HeaterVolt,float Frequency);
EXPORT LONG CALLTYPE TFlwHeaterEnergyIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterHotResIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterIRLevelIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterPowIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterPowIntvIn(HANDLE handle,float TestTime,float Interval,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterPowerSet(HANDLE handle,float TestTime,float HeaterPower,float HeaterStartVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterPowerSetExt(HANDLE handle,float TestTime,float HeaterPower,float HeaterStartVolt,float MaxHeaterVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterRelaisSet(HANDLE handle,float TestTime,DWORD RelaisOn SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltClockingSet(HANDLE handle,float TestTime,LONG Mode,float HeaterVolt,float OnTime,float OffTime SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwHeaterVoltSet(HANDLE handle,float TestTime,float HeaterVolt,float RampTime SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltSetExt(HANDLE handle,float TestTime,float HeaterVolt,float RampTime,LONG Flags SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltSetNoRamp(HANDLE handle,float TestTime,float HeaterVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterVoltSupvIn(HANDLE handle,float TestTime,LONG ResultIdxMin,LONG ResultIdxMax,LONG ResultIdxTime);
EXPORT LONG CALLTYPE TFlwHeaterPowerSupvSet(HANDLE handle,float TestTime,float HeatPowMin,float HeatPowMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwHeaterPowerSupvIn(HANDLE handle,float TestTime,LONG ResultIdxMin,LONG ResultIdxMax,LONG ResultIdxTime);
EXPORT LONG CALLTYPE TFlwHLineResCal(HANDLE handle,float TestTime SPRESIDX);
EXPORT LONG CALLTYPE TFlwIPOVAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwIPOVREIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwL1wIn(HANDLE handle,float TestTime,float Amplitude,float Frequency,float PeriodsBeforeMeas,float MeasTimeUN,float MeasPeriods,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwLastError(HANDLE handle);
EXPORT LONG CALLTYPE TFlwLeakCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSet(HANDLE handle,float TestTime,float LeakCurrVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSetExt(HANDLE handle,float TestTime,float LeakCurrVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSupvSet(HANDLE handle,float TestTime,float LeakCurrVoltMin,float LeakCurrVoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwLeakCurrVoltSupvIn(HANDLE handle,float TestTime,LONG ResultIdxMin,LONG ResultIdxMax,LONG ResultIdxTime);
EXPORT LONG CALLTYPE TFlwMaxHeaterCurrSet(HANDLE handle,float TestTime,float MaxHeaterCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwMaxHeaterVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMaxLeakCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMaxMaxLeakCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMeasCyclic(HANDLE handle,float TestTime,LONG MeasType,float Period,LONG Count,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwMinHeaterVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwNernstVoltIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwNew(HANDLE handle,LONG Cell);
//EXPORT LONG CALLTYPE TFlwOVSRHkSet(HANDLE handle,float TestTime,float RHk01,float RHk02,float RHk03,float RHk04,float RHk05,float RHk06,float RHk07,float RHk08,float RHk09,float RHk10,float RHk11,float RHk12,float RHk13,float RHk14,float RHk15,float RHk16,float RHk17,float RHk18,float RHk19,float RHk20 SPRESIDX);
EXPORT LONG CALLTYPE TFlwOVSHRegSet(HANDLE handle,float TestTime,LONG Flags,float UHMeasDly,float NotifyRHh,float NotifyRiAC,float TurnOffRHh,float TurnOffRiAC,float FaultRHh,float FaultRiAC,float IAC,float Freq,float MeasDly,float Ta SPRESIDX);
EXPORT LONG CALLTYPE TFlwOVSGetResults(HANDLE handle,float TestTime,LONG ResIdxUHstart,LONG ResIdxtRHNfy,LONG ResIdxtRiNfy,LONG ResIdxtRH,LONG ResIdxtRi,LONG ResIdxEH,LONG ResIdxRH,LONG ResIdxRi);
//EXPORT LONG CALLTYPE TFlwOVSHResTracking(HANDLE handle,float TestTime,float Interval,float HeaterVolt,float TurnOffRatio SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpCurrAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpCurrNernstIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpCurrREIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpCurrRESet(HANDLE handle,float TestTime,float PumpCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpVoltAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpVoltAPESet(HANDLE handle,float TestTime,float PumpVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpVoltREIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwPumpVoltRESet(HANDLE handle,float TestTime,float PumpVolt SPRESIDX);
EXPORT LONG CALLTYPE TFlwPumpVoltRampAPEStart(HANDLE handle,float TestTime,float PumpVoltInc,float Period,LONG ResultIdxIp,LONG ResultIdxUN);
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
EXPORT LONG CALLTYPE TFlwDtEHRiIn(HANDLE handle,float TestTime,LONG ResultIdxDt,LONG ResultIdxEH);
EXPORT LONG CALLTYPE TFlwRiSnapEHIn(HANDLE handle,float TestTime,LONG ResultIdxEH);
EXPORT LONG CALLTYPE TFlwRiSnapDtIn(HANDLE handle,float TestTime,LONG ResultIdxDt);
EXPORT LONG CALLTYPE TFlwRiSyncSens(HANDLE handle,float TestTime,float MeasDlyMin SPRESIDX);
EXPORT LONG CALLTYPE TFlwSensorRelaisSet(HANDLE handle,float TestTime,DWORD RelaisOn SPRESIDX);
EXPORT LONG CALLTYPE TFlwSwitchFromRiToPHReg(HANDLE handle,float TestTime,float PHOffset,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTemperatureIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTestCurrAPERESet(HANDLE handle,float TestTime,float TestCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrAPESet(HANDLE handle,float TestTime,float TestCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTestCurrRESet(HANDLE handle,float TestTime,float TestCurr SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrWdwAPESet(HANDLE handle,float TestTime,float TestCurrMin,float TestCurrMax,float VoltMin,float VoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwTestCurrWdwEnd(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTestCurrWdwRESet(HANDLE handle,float TestTime,float TestCurrMin,float TestCurrMax,float VoltMin,float VoltMax SPRESIDX);
EXPORT LONG CALLTYPE TFlwTimeEvent(HANDLE handle,float StartTime,float EndTime,float Period,float UserVal1,float UserVal2,float UserVal3,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwTubesBath(HANDLE handle,float TestTime,LONG TubesDnBathUp,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwUAPEIn(HANDLE handle,float TestTime,LONG ResultIdx);
EXPORT LONG CALLTYPE TFlwUNPIRegSet(HANDLE handle,float TestTime,float UN,float MaxUP,float Kp,float Ki,float MeasDly,float Ta SPRESIDX);
/* Cyclic Mode dependend functions */
/* Definition of TFlwCycMeasPrefix.Flags */
//H: Low-Wort wird an PK weitergereicht
#define CYCFL_IMMREPORT 0x00010000L /* 1 -> report every result immediately */
EXPORT LONG CALLTYPE TFlwCycMeasPrefix(HANDLE handle,LONG Flags,float Period,LONG Count);
EXPORT LONG CALLTYPE TFlwCycMeasPrefix2(HANDLE handle,LONG Flags,float Period,LONG Count,float rIntegrIntv);
/* Direct Mode dependend functions */
EXPORT LONG CALLTYPE DmHeaterVoltSet(HANDLE handle,LONG CellVec,LONG ChMask,float HeaterVolt,float RampTime,LONG Flags);
EXPORT LONG CALLTYPE DmHeaterPowerSet(HANDLE handle,LONG CellVec,LONG ChMask,float HeaterPower,float HeaterStartVolt,float MaxHeaterVolt,LONG Flags);
EXPORT LONG CALLTYPE DmLeakCurrVoltSet(HANDLE handle,LONG CellVec,LONG ChMask,float LeakCurrVolt);
EXPORT LONG CALLTYPE DmHeaterVoltIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmHeaterCurrIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmHeaterPowIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmHeaterColdResIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmHeaterHotResIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmLeakCurrVoltIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmIntTempIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmPumpVoltAPESet(HANDLE handle,LONG CellVec,LONG ChMask,float PumpVolt);
EXPORT LONG CALLTYPE DmPumpVoltRESet(HANDLE handle,LONG CellVec,LONG ChMask,float PumpVolt);
EXPORT LONG CALLTYPE DmPumpCurrRESet(HANDLE handle,LONG CellVec,LONG ChMask,float PumpCurr);
EXPORT LONG CALLTYPE DmTestCurrAPESet(HANDLE handle,LONG CellVec,LONG ChMask,float TestCurr);
EXPORT LONG CALLTYPE DmTestCurrWdwAPESet(HANDLE handle,LONG CellVec,LONG ChMask,float TestCurrMin,float TestCurrMax,float VoltMin,float VoltMax);
EXPORT LONG CALLTYPE DmTestCurrRESet(HANDLE handle,LONG CellVec,LONG ChMask,float TestCurr);
EXPORT LONG CALLTYPE DmTestCurrWdwRESet(HANDLE handle,LONG CellVec,LONG ChMask,float TestCurrMin,float TestCurrMax,float VoltMin,float VoltMax);
EXPORT LONG CALLTYPE DmRiPIDRegSet(HANDLE handle,LONG CellVec,LONG ChMask,float Ri,float IAC,float Freq,float MeasDly,float MaxUH,float Kp,float Ki,float Kd,float Ta);
EXPORT LONG CALLTYPE DmUNPIDRegSet(HANDLE handle,LONG CellVec,LONG ChMask,float UN,float MaxUP,float Kp,float Ki,float Kd,float Ta);
EXPORT LONG CALLTYPE DmLeakCurrIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmMaxLeakCurrIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmPumpVoltAPEIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmPumpCurrAPEIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmPumpVoltREIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmNernstVoltIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmRiDCnIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmFinalCurrRefChIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmPumpCurrNernstIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmPumpCurrREIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmTestCurrIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmTestCurrWdwEnd(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmUAPEIn(HANDLE handle,LONG CellVec,LONG ResultIdx);
EXPORT LONG CALLTYPE DmL1wIn(HANDLE handle,LONG CellVec,float Amplitude,float Frequency,float PeriodsBeforeMeas,float MeasTimeUN,float MeasPeriods,LONG ResultIdx);
EXPORT LONG CALLTYPE DmRiACIn(HANDLE handle,LONG CellVec,float IAC,float Freq,float MeasDly,LONG ResultIdx);
EXPORT LONG CALLTYPE DmRiRegSupv(HANDLE handle,LONG CellVec,LONG Mode,float RiMin,float RiMax,LONG MinResIdx,LONG MaxResIdx);
EXPORT LONG CALLTYPE DmRiBIn(HANDLE handle,LONG CellVec,float AmplitudePl,float PulseOnTimePl,float MeasTimePl,float AmplitudeMi,float PulseOnTimeMi,float MeasTimeMi,LONG ResultIdx);
EXPORT LONG CALLTYPE DmRiPulseIn(HANDLE handle,LONG CellVec,float Amplitude,float Frequency,float PulseOnTime,float MeasPeriods,float MeasTimeBef,float MeasTimePuls,float MeasTimeAft,LONG ResultIdxUvor,LONG ResultIdxUpuls,LONG ResultIdxUnach,LONG ResultIdxRiPuls);

#ifdef __cplusplus
}
#endif

#endif /* _setestdll_h_ */




