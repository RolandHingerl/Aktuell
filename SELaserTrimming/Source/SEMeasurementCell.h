/*----------------------------------------------------------------------------------------------------------*
 *  modulname       : SEMeasurementCell.h                                                                   *
 *----------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                      *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                             *
 *----------------------------------------------------------------------------------------------------------*
 *  description:                                                                                            *
 *  ------------                                                                                            *
 *  Header file for SEMeasurementCell.                                                                      *
 *----------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                          *
 * ---------|------------|----------------|---------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                               *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                    *
 *----------------------------------------------------------------------------------------------------------*/

#pragma once
#include <vector>
#include <list>
#include <math.h>
#include <locale.h>


#include "SEHelpFunctions.h"
#include "SEComPlc.h"
#include "..\Library\IMT_BasTypeDef.h"
#include "ProcessTypes.h"

static HANDLE LSTestHandle;														//handle for LSTestDll (must be a global variable)


#ifndef _SE_MEASUREMENT_CELL
	//---------------------------------------------------------------------------------------------------------
	#define _SE_MEASUREMENT_CELL
	//---------------------------------------------------------------------------------------------------------
	#ifdef _SE_MEASUREMENT_CELL_INTERNAL
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
	#ifdef _SE_MEASUREMENT_CELL_INTERNAL
		#define DLL_DECL		DLL_EXPORT
		#define CALLTYPE		__cdecl
	#else
		#define DLL_DECL		DLL_IMPORT
		#define CALLTYPE		__cdecl
	#endif
	
//-- Global defines ----------------------------------------------------------------------------------------
//#define SIMULATION 1																										
//#define STATUS_SIMULATION 1
#define MICRO_LAS_SIMULATION 1
//#define PLC_SIMULATION 1
//#define REF_SIMULATION 1
#define DEBUG_OUTPUT_FILE 1
//#define LOGGING_OLD_FORMAT 1
//#define CHAMBER1_DISABLE 1
#define CHAMBER2_DISABLE 1
#define IMAGE_PROCESSING_DISABLE 1

#define CELL_COUNT			   4	//cell count
#define CELL_TIEPOINT_COUNT   20	//cell tiepoint count

#define ADVOV_FW_MAIN		3.800	//advov firmware version main 
#define ADVOV_FW_MODULE		3.800	//advov firmware version module
#define FW_LOCATION         "D:\\SELasertrimming\\AdvovFw\\"

#ifdef DEBUG_OUTPUT_FILE
#define printf( ... ) dprintf( TRUE, TRUE, ##__VA_ARGS__ );
#endif

// indutron defines:
#define _INDUTRON_PRINT_MORE  // additional test outputs

//-- Global enum --------------------------------------------------------------------------------------------
//cell types
enum CellType
{
	eTrimmCell	   = 0x0000000F, //trimming cell (mclist)
	eReferenceCell = 0x00000001	 //reference cell (mclist)
};

#if 0
//normal number
enum NormalNumber
{
	PositionNormal	= 1,  //position normal
	HeaterNormal	= 2,  //heater normal
	UniversalNormal = 3,  //universal normal
	UnNormal		= 4,  //un normal
	IpNormal		= 5,  //ip normal
	IlmNormal		= 6,  //ilm normal
};
#endif

//-- Global structures --------------------------------------------------------------------------------------
//SE measurement values
struct SEValueInfo
{
	long long ReceiveTime;		//receive time from ADVOV card
	long long ReceiveTimePC;	//receive time from pc
	float Value;				//measurement value									
};

//heater control parameter
struct HeaterControlParam
{
	float RampTime;		//ramp time [s]
	float HeatVolt;		//heater voltage [V]
	float HeatPow;		//heater power [W]
	float MaxHeatVolt;	//max heater voltage at regulation [V]
	float HeatTime;		//heating time [s]
	float CooldownTime;	//cooldown time [s]
};

//test sequence parameter
struct TestSequenceParam
{
	HeaterControlParam Heater[2];	//heater control parameter
	float PumpCurrentRE;			//pump current 
	float PumpVoltADV;				//pump voltage
	float RiSetPoint;				//ri setpoint
	float UNRegDelay;				//un regulation delay
	float NernstVoltADV;			//nernst voltage 
	float FastContinue;				//fast continue
	float LowPassConst;				//low pass const
	float MeasureFLODelay;			//meas uh-reg to ri-reg delay
	float MeasureFLO;				//meas uh-reg to ri-reg	enable
	float RiTrigger; 				//ri trigger point
	float DeltaTIHMeas;				//delta tih measure
	float RiRegMaxUH;				//max heater voltage at ri regulation
	float RiRegKp;					//pid-regulator kp-value
	float RiRegKi;					//pid-regulator ki-value
	float RiRegKd;					//pid-regulator kd-value
	float PHwhenRiRegFailed;		//heater power at ri regulation failed
	float TimeoutForRiRegFailed;	//timeount at ri regulation failed
	float NegPulseTime;				//negative pulse time
	float NegPulseDelay;			//negative pulse delay
	float NegPulseVolt;				//negative pulse voltage
};






/*-------------------------------------------------------------------------------------------------------------------------*
 * class name           : SEMeasurementCell                                                                                *
 *                                                                                                                         *
 * description:         : This is the base class of measurement cell.                                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
class SEMeasurementCell
{
private:
	//-- private members

	//-- private memberfunctions
protected:
	//-- protected members
	int CellId;			//cell id
	bool DeinstallThreadRequest;	//deinstall thread request
	
	ParamStationDataTrimmingCell *ParamStationTrimmCell;	//parameter for trimming cell
	ParamDataTrimmingCell *ParamTrimmCell;	//parameter for trimming cell
	ParamDataReferenceCell *ParamRefCell;	//parameter for reference cell
	ParamStationDataReferenceCell *ParamStationRefCell;		//parameter for reference cell
	ParamDataNormalMeasure *ParamNormal;	//parameter for normal measure

	
	
	bool SequenceFinished;
	ProcessType ProcessTypeLocal;
	float AvgCardTemperature;
		
	//cyclic measurement values
	std::vector <SEValueInfo> RiValues[CELL_TIEPOINT_COUNT];	//vector of measured Ri values (ident=260)
	SEValueInfo RiValues1[CELL_TIEPOINT_COUNT];	//measured Ri values (ident=260)
	SEValueInfo RiValues2[CELL_TIEPOINT_COUNT];	//measured Ri values (ident=260)
	
	SEValueInfo RiMinValues[CELL_TIEPOINT_COUNT]; // measured RiMin values(ident = 285)
	SEValueInfo RiMaxValues[CELL_TIEPOINT_COUNT]; // measured RiMax values(ident = 286)

	// IMT_IAPE
	std::vector <SEValueInfo> IpValues[CELL_TIEPOINT_COUNT];	//vector of measured Ip values (ident=259)
	SEValueInfo IpValues1[CELL_TIEPOINT_COUNT];	//measured Ip values (ident=259)
	SEValueInfo IpValues2[CELL_TIEPOINT_COUNT];	//measured Ip values (ident=259)
	SEValueInfo IpValues3[CELL_TIEPOINT_COUNT];	//measured Ip values (ident=259)
	SEValueInfo Ip4Average[CELL_TIEPOINT_COUNT];//measured Ip values (ident=259, SubIndex=100)

	SEValueInfo IPuPoint1[CELL_TIEPOINT_COUNT];  //measured Ip values (ident=259, SubIndex=101)
	SEValueInfo IPuPoint2[CELL_TIEPOINT_COUNT]; //measured Ip values (ident=259, SubIndex=102)

    std::vector <SEValueInfo> RhhValues[CELL_TIEPOINT_COUNT]; //vector of measured Rhh values (ident=108)
	std::vector <SEValueInfo> PhValues[CELL_TIEPOINT_COUNT];  //vector of measured Ph  values (ident=110)

	//acyclic values
	SEValueInfo UhValues[CELL_TIEPOINT_COUNT];	  //measured Uh values (ident=103) 
	SEValueInfo UhMinValues[CELL_TIEPOINT_COUNT]; //measured Uh min values (ident=104) 
	SEValueInfo UhMaxValues[CELL_TIEPOINT_COUNT]; //measured Uh max values (ident=105)

	SEValueInfo TemperatureValues[CELL_TIEPOINT_COUNT];	//measured temperature values (ident=10)
	SEValueInfo RhkValues[CELL_TIEPOINT_COUNT];	   //measured Rhk values (ident=109)
	SEValueInfo EhValues[CELL_TIEPOINT_COUNT];	   //measured Eh values (ident=113)
    SEValueInfo RiRegValues[CELL_TIEPOINT_COUNT];  //measured RiReg values (ident=114)
	SEValueInfo RhDtValues[CELL_TIEPOINT_COUNT];   //measured RhDt values (ident=116)
	SEValueInfo IhDtValues[CELL_TIEPOINT_COUNT];   //measured IhDt values (ident=115)
	SEValueInfo HSContValues[CELL_TIEPOINT_COUNT]; //measured AdvHSCont values (ident=100)
	SEValueInfo SSContValues[CELL_TIEPOINT_COUNT]; //measured AdvSSCont values (ident=150)
	SEValueInfo IhValues[CELL_TIEPOINT_COUNT];	   //measured Ih values (ident=106)

	//normal measurement
	SEValueInfo UapeValues[CELL_TIEPOINT_COUNT];  //measured Uape values (ident=253)
	SEValueInfo UapeValues2[CELL_TIEPOINT_COUNT]; //measured Uape values (ident=253)
	SEValueInfo UapeAverage[CELL_TIEPOINT_COUNT]; //measured Uape value (ident=253, Subindex = 100)
	SEValueInfo UapePoint1[CELL_TIEPOINT_COUNT]; //measured Uape value (ident=253, Subindex = 101)
	SEValueInfo UapePoint2[CELL_TIEPOINT_COUNT]; //measured Uape value (ident=253, Subindex = 102)

	SEValueInfo UnValues[CELL_TIEPOINT_COUNT];	  //measured Un values (ident=251)
	SEValueInfo UnValues1[CELL_TIEPOINT_COUNT];	//measured Un values (ident=251)
	SEValueInfo UnValues2[CELL_TIEPOINT_COUNT];	//measured Un values (ident=251)
	SEValueInfo UnValues3[CELL_TIEPOINT_COUNT];	//measured Un values (ident=251)

	SEValueInfo IpReValues[CELL_TIEPOINT_COUNT];  //measured IpRe values (ident=256)
	SEValueInfo IpRe20uA[CELL_TIEPOINT_COUNT];  //measured IpRe values (ident=256)

	SEValueInfo UReValues[CELL_TIEPOINT_COUNT];	  //measured URe values (ident=250)
	SEValueInfo UReValues2[CELL_TIEPOINT_COUNT];  //measured URe values (ident=250), for average measurement
	
	SEValueInfo URePoint1[CELL_TIEPOINT_COUNT];  //measured IpRe values (ident=256)
	SEValueInfo URePoint2[CELL_TIEPOINT_COUNT];  //measured IpRe values (ident=256)

	SEValueInfo IgRkValues[CELL_TIEPOINT_COUNT];  //measured IgRk values (ident=258)
	SEValueInfo IlValues[CELL_TIEPOINT_COUNT];	  //measured Il values (ident=112)
	SEValueInfo RiDCnValues[CELL_TIEPOINT_COUNT]; //measured RiDCn values (ident=261)
	SEValueInfo IprValues[CELL_TIEPOINT_COUNT];	  //measured IPr values (ident=254)

	//-- protected memberfunctions
	//execute heating function
	int ExecHeating(int HeatPhase, float StartTime, float EndTime, float CooldownTime, 
	                float HeatVolt, float RampTime, float HeatPow, float MaxHeatVolt, float * ExecTime);
	int ClearAllValues( void );																					 		//clear all values
	int ClearValues(int Tiepoint, eIMTBasTypes eImtType);												//clear values of one type
	//add value	
	int AddValue(int Tiepoint, eIMTBasTypes eImtType, int Index, SEValueInfo Value);
  
public:
	
	//-- public members
	static bool Dllopened;																									//static flag LSTestDll opened
	static HANDLE hThreadMeasInMeasurementCellChamber[CELL_COUNT];					//static thread handle for each measurement cell
	__int64	SequenceStartTime;		//sequence start time [ms]
	//-- public memberfunctions
	SEMeasurementCell(){};	//constructor
	SEMeasurementCell(int CellId);	//constructor (overloaded)
	~SEMeasurementCell(){};	//destructor
	//install thread
	int InstallThread( LPTHREAD_START_ROUTINE CallbackAddressCell, 
	                    LONG ( *pFCallBackCell )( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx ) );
	void DeinstallThread( void );	//deinstall thread
	//get last inserted value
	int GetLastValue(int Tiepoint, eIMTBasTypes eImtType, int Index, SEValueInfo * Value );
	__int64 GetSequenceStartTime( void );	//get sequence start time
	
	int StartSequence( void );	//start sequence
	int StopSequence( void );	//stop sequence
	
	bool SequenceStarted;
	bool ReadDataAdvovActive;
	
	bool IsContactingOk (int Tiepoint ); //contacting ok
	bool IsRiStable( int Tiepoint );	 //ri regulation stable

	bool AllCardsReady( void );	 //all cards ready
	bool DeinstallThreadRequested( void ); //deinstall thread requested

	//handle measurement values
	int HandleTestSequenceValues(WORD wFUIdx,WORD wPartIdx,/*eIMTBasTypes*/int eMainType, int SubIndex,
	                             float rMVal,float tsMTime,LONG lResIdx);

	void SetSequenceFinished( bool SequenceFinished );	//set sequence finished
	bool IsSequenceFinished( void );	//check sequence finished
	bool IsCardTemperatureOk( void );	//check card temperature
	bool IsSequenceStarted( void );
	bool IsReadDataAdvovActive( void );
	void SetReadDataAdvov( bool Active ); 

};

/*-------------------------------------------------------------------------------------------------------------------------*
 * class name           : SETrimmingCell : public SEMeasurementCell                                                        *
 *                                                                                                                         *
 * description:         : This is the derived class for measurement trimming cells.                                        *
 *-------------------------------------------------------------------------------------------------------------------------*/
class SETrimmingCell : public SEMeasurementCell
{
private:
	//-- private members

	//-- private memberfunctions
	int GenerateTestSequence( TestSequenceParam Parameter ); //generate test sequence
	int GenerateTestSequence_Ip4UNernstControl( TestSequenceParam Parameter );
	int GenerateTestSequence_Ip4TwoPointMeasUp( TestSequenceParam Parameter );
	int GeneratePositionNormalContactSequence( void );	     //generate contact check sequence
	int GeneratePositionNormalSequence( void );	    //generate position normal sequence
	int GenerateHeaterNormalSequence( void );	    //generate heater normal sequence
	int GenerateUniversalNormalSequence( void );	//generate universal normal sequence
	int GenerateIlmNormalSequence( void );	//generate ilm sequence
	int GenerateUnNormalSequence( void );	//generate un normal sequence
	int GenerateIpNormalSequence( void );	//generate ip normal sequence
	int GenerateCoolingSequence( void );    //generate cooling sequence

protected:
	//-- protected members

	//-- protected memberfunctions	
	
  
public:
	//-- public members
	__int64 DurIp4Meas;
	//-- public memberfunctions
	SETrimmingCell(){};	//constructor										
	SETrimmingCell(int CellId ) : SEMeasurementCell( CellId ){};  //constructor (overload)															
	~SETrimmingCell(){};  //destructor
	
	int Initializing( ParamStationDataTrimmingCell *ParamStationTrimmCell, ParamDataTrimmingCell *ParamTrimmCell, ParamDataNormalMeasure *ParamNormal ); //initialization
	int CheckFirmware( void );
	int GenerateAndTransmittSequence( ProcessType Type );									  //genaerate and transmitt sequence
};


/*-------------------------------------------------------------------------------------------------------------------------*
 * class name           : SEReferenceCell : public SEMeasurementCell                                                       *
 *                                                                                                                         *
 * description:         : This is the derived class for measurement reference cells.                                       *
 *-------------------------------------------------------------------------------------------------------------------------*/
class SEReferenceCell : public SEMeasurementCell
{
private:
	//-- private members

	//-- private memberfunctions	
	int GenerateTestSequence( TestSequenceParam Parameter );	//generate test sequenc for laser trimming
	int GenerateTestSequence_Ip4UNernstControl( TestSequenceParam Parameter );  // generate test sequenc for IP4 selection with Nernst voltage control
	int GenerateTestSequence_Ip4TwoPointMeasUp( TestSequenceParam Parameter );     // generate test sequenc for IP4 selection with 2-point measurement of the Nerst voltage

public:
	//-- public members

	//-- private memberfunctions
	SEReferenceCell() {};	//constructor										
	SEReferenceCell(int CellId) : SEMeasurementCell(CellId) {};	//constructor (overload)															
	~SEReferenceCell() {};	//destructor														

	int Initializing(ParamStationDataReferenceCell* ParamStationRefCell, ParamDataReferenceCell* ParamRefCell); //initialization
	int CheckFirmware(void);
	int GenerateAndTransmittSequence(ProcessType Type);	//generate and transmitt sequence
	int GetLastIpMean(int Tiepoint, unsigned int MeanCount, float* IpReference);		//get Ip reference value

};

//---------------------------------------------------------------------------------------------------------
	#ifndef _SE_MEASUREMENT_CELL_INTERNAL
		#undef DLL_DECL
		#undef CALLTYPE
	#endif
	//---------------------------------------------------------------------------------------------------------
	#undef EXTERN
#endif


