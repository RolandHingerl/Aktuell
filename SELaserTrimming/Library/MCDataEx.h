/* MCDataEx.h
*/

#ifndef _MCDATAEX_H_
#define _MCDATAEX_H_

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

	typedef struct TFlwResultTS { /* Messstatus, mit Messzeitpunkt zu jedem Messwert */
		LONG TFlowEnd; /* TRUE -> Prüfablauf beendet und alle Messwerte abgeholt */
		float TestTime; /* Aktueller Prüfablaufzeitpunkt */
		LONG ResultIdx; /* (Externer) Messindex (0 = kein Messwert) */
		MEASINF atMInf[20]; /* Messwerte + Zeitstempel */
	} TFLWRESULTTS;

/*...prototypes..............................................................*/
	EXPORT LONG CALLTYPE GetDLLVersion(void);
	EXPORT LONG CALLTYPE LSTestDLLOpen(/*DWORD*/LONG Flags, HANDLE* phandle);
	EXPORT LONG CALLTYPE LSTestDLLClose(HANDLE handle);
	EXPORT LONG CALLTYPE MeasSplittResIdx(HANDLE handle, LONG* plMainIdx, LONG* plSubIdx, LONG* plLocIdx, LONG lResIdx);
	EXPORT LONG CALLTYPE SetDLLReportingPath(HANDLE handle, char* szLogPath);
	EXPORT LONG CALLTYPE MCxAssignCell(HANDLE handle, LONG CellNo, LONG McListLow, LONG McListHigh);
	EXPORT LONG CALLTYPE MCxAssignMcUnit(HANDLE handle, LONG McUnitNo, BYTE* pabyNodeIdList);
	EXPORT LONG CALLTYPE MCxInfo(HANDLE handle, LONG CellNo, LONG MCNo, MCINFO* ptInfo);
	EXPORT USHORT CALLTYPE MCxInfoGetType(HANDLE handle, LONG CellNo, LONG MCNo);
	EXPORT USHORT CALLTYPE MCxInfoGetSerialNumber(HANDLE handle, LONG CellNo, LONG MCNo);
	EXPORT float CALLTYPE MCxInfoGetMainSoftVer(HANDLE handle, LONG CellNo, LONG MCNo);
	EXPORT float CALLTYPE MCxInfoGetModSoftVer(HANDLE handle, LONG CellNo, LONG MCNo);
	EXPORT LONG CALLTYPE MCxInit(HANDLE handle, LONG Cell);
	EXPORT LONG CALLTYPE MCxInitRL(HANDLE handle, LONG Cell, float RL1, float RL2, float RL3, float RL4, float RL5, float RL6, float RL7, float RL8, float RL9, float RL10, float RL11, float RL12, float RL13, float RL14, float RL15, float RL16, float RL17, float RL18, float RL19, float RL20);
	EXPORT LONG CALLTYPE MCxTFlowResult(HANDLE handle, LONG Cell, TFLWRESULT* pTFlwResult);
	EXPORT LONG CALLTYPE MCxTFlowResultTS(HANDLE handle, LONG CellSpec, TFLWRESULTTS* pTFlwResultTS);
	EXPORT LONG CALLTYPE MCxTFlowMultiResultTS(HANDLE handle, LONG CellSpec, TFLWRESULTTS* pTFlwResultTS, LONG* plDsCnt);
	EXPORT LONG CALLTYPE MCxFwUpload(HANDLE handle, LONG CellSpec, LONG Flags, char* szHexFile);
	EXPORT LONG CALLTYPE MCxLastError(HANDLE handle, LONG CellSpec);
	EXPORT LONG CALLTYPE MCxStatus(HANDLE handle, LONG Cell);
	EXPORT LONG CALLTYPE MCxCalib(HANDLE handle, LONG Cell);
	EXPORT LONG CALLTYPE MCxChSel(HANDLE handle, LONG CellSpec, LONG ChMask);
	EXPORT LONG CALLTYPE MCxTFlowTx(HANDLE handle, LONG Cell);
	EXPORT LONG CALLTYPE MCxStartSEQ(HANDLE handle, LONG CellSpec);
	EXPORT LONG CALLTYPE MCxStop(HANDLE handle, LONG Cell);
	EXPORT LONG CALLTYPE MCxEvent(HANDLE handle, LONG CellSpec, LONG EvFlags);
	EXPORT LONG CALLTYPE MCxCycResCallback(HANDLE handle, LONG CellSpec, LONG(*pFCallBack)(WORD wFUIdx, WORD wPartIdx,/*eIMTBasTypes*/int eMainType, float rMVal, DWORD tsMTime, LONG lResIdx));
	EXPORT LONG CALLTYPE MCxTFlowResChk(HANDLE handle, LONG Cell);
	EXPORT float CALLTYPE MCxTFlowResMeasRes(HANDLE handle, LONG Cell, LONG Idx);
	EXPORT LONG CALLTYPE MCxTFlowResMeasResType(HANDLE handle, LONG Cell, LONG Idx);
	EXPORT LONG CALLTYPE MCxTFlowResResultIdx(HANDLE handle, LONG Cell);
	EXPORT LONG CALLTYPE MCxTFlowResTFlowEnd(HANDLE handle, LONG Cell);
	EXPORT float CALLTYPE MCxTFlowResTestTime(HANDLE handle, LONG Cell);
	EXPORT LONG CALLTYPE MCxReport(HANDLE handle, LONG CellSpec, LONG Flags, BYTE* psFileName);
	EXPORT LONG CALLTYPE MCxSetLineResistance(HANDLE handle, LONG CellSpec, int ketElecType, float RL1, float RL2, float RL3, float RL4, float RL5, float RL6, float RL7, float RL8, float RL9, float RL10, float RL11, float RL12, float RL13, float RL14, float RL15, float RL16, float RL17, float RL18, float RL19, float RL20);
#if defined (LSS_API)
	EXPORT LONG CALLTYPE MCxLSS(HANDLE handle, LONG CellSpec, LONG kiFlags, DWORD kuCANID, DWORD kuSerial_Number,
		DWORD kuProduct_Code, DWORD kuRevision_Number, DWORD kuVendor_ID);
#endif
#if defined (DM_API)
	EXPORT LONG CALLTYPE MCxStartDM(HANDLE handle, LONG CellSpec);
#endif
#if defined (MULTIF_API)
	EXPORT LONG CALLTYPE MCxContacting(HANDLE handle, LONG CellSpec, LONG Close);
	EXPORT LONG CALLTYPE MCxExcIO(HANDLE handle, LONG CellSpec, DWORD* pdwInpMirr, struct ASYNCOUTPINF* pOutpDat);
	EXPORT LONG CALLTYPE MCxMultifErr(HANDLE handle, LONG CellSpec);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _setestflow_h_ */




