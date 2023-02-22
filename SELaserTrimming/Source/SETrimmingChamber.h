/*----------------------------------------------------------------------------------------------------------*
 *  modulname       : SEMeasurement.h                                                                       *
 *----------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                      *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                             *
 *----------------------------------------------------------------------------------------------------------*
 *  description:                                                                                            *
 *  ------------                                                                                            *
 *  Header file for SEMeasurement.                                                                          *
 *----------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                          *
 * ---------|------------|----------------|---------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                               *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                    *
 *----------------------------------------------------------------------------------------------------------*/
#pragma once
#include "SEMeasurementCell.h"
#include "SEComMicroLas.h"
#include "SEComPlc.h"
#include "SEHelpFunctions.h"


#ifndef _SE_TRIMMING_CHAMBER
	//---------------------------------------------------------------------------------------------------------
	#define _SE_TRIMMING_CHAMBER
	//---------------------------------------------------------------------------------------------------------
	#ifdef _SE_TRIMMING_CHAMBER_INTERNAL
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
	#ifdef _SE_TRIMMING_CHAMBER_INTERNAL
		#define DLL_DECL		DLL_EXPORT
		#define CALLTYPE		__cdecl
	#else
		#define DLL_DECL		DLL_IMPORT
		#define CALLTYPE		__cdecl
	#endif

//-- Global defines -----------------------------------------------------------------------------------------
#define CHAMBER_COUNT	2																										//chamber count
#define CHAMBER1	0																								    		//chamber count
#define CHAMBER2	1																										    //chamber count

#define REF_CELL_COUNT 2																									//reference cell count
#define REF_CELL1 1																												//reference cell 1
#define REF_CELL2	2																												//reference cell 2


#define SQUARE1 1																													//square 1
#define SQUARE2 2																													//square 2
#define SQUARE3 3																													//square 3
#define SQUARE4 4																													//square 4
#define SQUARE_COUNT 4																										//square count

#define MODE_CHAMBER1_ONLY 1                                              //only chamber 1
#define MODE_CHAMBER2_ONLY 2                                              //only chamber 2
#define MODE_CHAMBER1_AND_CHAMBER2 3                                      //chamber 1 and chamber 2

#ifdef DEBUG_OUTPUT_FILE
	#define printf( ... ) dprintf( TRUE, TRUE, ##__VA_ARGS__ );
#endif

#define INF_EMPTY 100																											//inf empty
#define INF_GENERATE_TEST_SEQUENCE 101																		//inf generate test sequence
#define INF_START_TEST_SEQUENCE 102																				//inf start test sequence
#define INF_WAIT_RI_STABILITY	103																					//inf wait ri stability
#define INF_WAIT_IP_STABILITY 104																					//inf wait ip stability
#define INF_TRIMMING_ACTIVE 105																						//inf trimming active
#define INF_TRIMMING_FINISHED 106																					//inf trimming finished
#define INF_LABELING 107																									//inf labeling
#define INF_COOLING_PASSIVE 108																						//inf cooling passive
#define INF_COOLING_ACTIVE 109																						//inf cooling active

#define DATA_SAVE_LOCATION                           "D:\\SELasertrimming\\Statistics\\"
#define CONFIG_DATA_LOCATION                         "D:\\SELasertrimming\\Config\\"

#define NONE_FLAG 0x00000000																							//none flag 
#define COOLING_ACTIVE_FLAG 0x00000001																		//cooling active flag
#define HEATING_FINISHED_FLAG 0x00000002																	//heating finished flag

//-- Global enum -------------------------------------------------------------------------------------------

//part status
enum PartStatus
{
	Nio = -1,																																//nio
	NotPresent = 0,																													//not present
	NotProcessed = 1,																												//not processed
	IoDiesel = 2,																														//io diesel
	SelectGasoline = 3,																											//select gasoline
	IoGasoline = 4,																													//io gasoline
	Dummy = 5,																															//dummy
	Rework = 6,																															//rework
	ReworkOnceAgain = 7,																										//rework once again
	Deactivated = 8,																												//deactivated
	DeactivatedCondition = 9																								//deactivated condition
};

//nio or rework reason
enum NioReworkReason
{
	LeackageRateOutside = 13,																								//leackage rate outside (all)
	EvacuatedPressureOutside = 12,																					//evacuated pressure outside (all)
	FlowOutside = 11,																												//flow outside (all)
	PressureOutside = 10,																									  //pressure outside (all)
	ImageProcessing = 9,																										//image processing io part (mae se)
	ImageProcessingBox = 8,																									//image processing nio part (mae se)
	NotTrimmable = 7,																												//not trimmable (se)
	PhOutsideLimit = 6,																											//ph outside (se)
	RhhOutsideLimit = 5,																										//rhh outside (se)
	GasControlFault = 4,																										//gas control fault (all)
	NoStability = 3,																												//no stability (se)
	ContactRefNok = 2,																											//contact reference sensor nio (all)
	ContactRiStabilityNok = 1,																							//contact and ri stability nio (se)
	NoReason = 0,																														//no reason - empty message (all,mae se,se)
	ActiveAreaDemaged = -1,																									//active area demaged (se)
	RisingTooLow = -2,																											//rising ip too low (se)
	IpOutsideLimit = -3,																										//ip ouside (se)
	ValueNotPlausible = -4,																									//value not plausible (se)
	TimeoutReached = -5,																										//timeout reached (se)
	RawLimitReached = -6,																										//raw limit reached (se)
	MaxPassageReached = -7,																									//max passage reached (se)
	OutsideHole = -8,																												//outside hole (se)
	IpAboveRangeAtStart = -9,																								//ip above range at start (se)
	MinCrossingCount = -10,																									//min crossing count violated (se)
	ReworkOnceAgainRework = -11,                                            //rework once again rework state (se)
	MinCutCountBelow = -12                                                  //min cut count below (se)
};

//retain data
struct SETrimmingRetain
{
	unsigned int SerialNumber;																							//store serial number
};


//trimming info struct
struct ProcessDataTrimming
{
	bool ContactOk;																													//contact ok
	bool RiStability;																												//ri stability
	bool ContactRefOk;																											//contact ref ok
	bool IpStability;																												//ip stability
	bool GasControlOk;																											//gas control
	bool TrimmingDone;																											//trimming done

	float IpLastStability;																									//ip last stability measurement
	float IpLastStabilityUnscaled;																					//ip last stability measurement unscaled

	bool EntireProcessDone;																									//entire process done

	unsigned int ActPhase;																									//actual phase at trimming process

	float XPosition;																												//x position image processing  
	float YPosition;																												//y position image processing
	float ZPosition;																												//z position image processing  
	float TrimmingDiameter;                                                 //trimming width

	float IpEndCheck;																												//ip at end check
	
	float IpRawLimit;																												//ip raw limit
	float IpFineLimit;																											//ip fine limit

	float MinimalDeviationAbs;																							//min deviation 

	float IpLowerLimit;																											//ip lower limit
	float IpUpperLimit;																											//ip upper limit

	float IpActCut;																													//ip at actual cut
	long long TrimmingTimeActCut;																						//trimming time at actual cut
																																			
	float IpLastCut;																												//ip last cut
	long long TrimmingTimeLastCut;																					//trimming time at last cut

	float IpDiff;																														//ip difference
	float IpDelta;																													//ip delta

	float IpFirstCut;																												//ip first cut
	float Ipn;																															//ipn
	float IpnLast;																													//ipn last 
	float IpMax;																														//ip max

	float ZeroPos;																													//zero position
	float DiffPos;																													//difference position

	bool OutsideHole;																												//outside hole flag
	unsigned int AdditionalCuts;																						//additional cuts

	float PosXMax;																													//max x pos at trimming
	float PosXMin;																													//min x pos at trimming
	float PosYMax;																													//max y pos at trimming
	float PosYMin;																													//min y pos at trimming
	float PosXMaxFine;																											//max x pos at fine trimming

	float PosXActCut;																												//actual cut position x-axis
	float PosYActCut;																												//actual cut position y-axis

	float PosXLastCut;																											//last cut position x-axis
	float PosYLastCut;																											//last cut position y-axis

	int CrossingCount; 																						          //crossing count
	int CrossingCountEntire;                                                //entire crossing count           
	unsigned int CutCount;																									//cut count over all
	unsigned int CutCountRaw;																								//cut count at raw trimming
	unsigned int CutCountFine;																							//cut count at fine trimming

	float ActRhh;																														//actual rhh
	float ActIp;																														//actual ip
	float ActRi;																														//actual ri

	float ActIpRef[2];                                       								//actual ip reference cell

	float TrimmingCutXPos[100];																							//trimming cut x position
	float TrimmingCutYPos[100];																							//trimming cut y position
	float TrimmingCutIp[100];																								//trimming ip
	float TrimmingCutChamberPressure[100];																	//trimming chamber pressure
	int TrimmingCutCount[100];																							//trimming cut count
	int TrimmingCrossingCount[100];																					//trimming crossing count
};


//trimming info struct
struct SETrimmingInfoTiepoint
{
	AssemblyDataTrimmingTiepoint AssemblyDataTiepoint;											//assembly data tiepoint specific
	ResultDataTrimming ResultData;																					//result data trimming
	ProcessDataTrimming ProcessData;																				//process data trimming
};

//trimming info struct
struct SETrimmingInfo
{
	AssemblyDataTrimmingCommon AssemblyDataCommon;													//assembly data common
	SETrimmingInfoTiepoint SETrimmingValuesTiepoint[CELL_TIEPOINT_COUNT];		//trimming info each tiepoint
};

//normal info
struct SENormalInfo
{
	ResultDataPositionNormal ResultDataPosition;	//result data position normal
	ResultDataHeaterNormal ResultDataHeater;	    //result data heater normal
	ResultDataUniversalNormal	ResultDataUniversal;//result data universal normal
	ResultDataUnNormal ResultDataUn;	//result data un normal
	ResultDataIpNormal ResultDataIp;	//result data ip normal
	ResultDataIlmNormal ResultDataIlm;	//result data ilm normal
};

//image processing
struct DataImageProcessing
{
	float XPos;																															//x position
	float YPos;																															//y position
	float ZPos;																															//z position
	float Diameter;																													//diameter
};

//-- Global structures --------------------------------------------------------------------------------------

/*-------------------------------------------------------------------------------------------------------------------------*
 * class name           : SETrimmingChamber                                                                                *
 *                                                                                                                         *
 * description:         : This is the base class of trimming chamber.                                                      *
 *-------------------------------------------------------------------------------------------------------------------------*/
class SETrimmingChamber
{

private:
	//-- members
	//input members
	int ChamberId;																													//chamber id
	bool DeinstallThreadRequest;																						//request deinstall threads

	bool IgnoreNewPressure;																									//ignore new pressure value at stability measurement
	bool StationParameterSetOk;																							//station parameter	data plausible
	bool ParameterSetOk;																										//trimming parameter data plausible
	bool NormalParameterSetOk;																							//normal parameter data plausible
	bool AssemblyDataOk;																										//assembly data plausible
	bool ParameterImageProcessingOk;																				//image processing data plausible																				
	bool ProcessTrimmingActive;																							//process trimming active
	bool ProcessNormalActive;																								//process normal measurement active

	__int64	StabilityStartTime;																							//stability start time
	__int64 CoolingStartTime;																								//cooling start time
	

	int ProcessStep;																												//process step
	int NormalStep;																												  //normal step
	int StabilityStep;																											//stability step
	
	unsigned int NormalNumber;																							//number of normal insert
	unsigned int ChamberState;																							//chamber state
	float ChamberPressureActual;																						//actual chamber pressure
	float ChamberPressureActualArray[4];																		//actual chamber pressure array
	int ChamberPressureActualArrayCount;                            		    //actual chamber pressure array count
	float ChamberFlowActual;																								//actual chamber flow
	float ChamberLaserPowerActual;                                          //actual chamber laser power

	unsigned int DigitalIn;																									//digital trigger flags from plc
	unsigned int DigitalOut;																								//digital trigger flags to plc
	
	ParamStationDataTrimmingChamber ParamStationTrimmChamber;								//parameter data trimming chamber
	ParamDataTrimmingChamber ParamTrimmChamber;															//parameter data trimming chamber
	ParamDataNormalMeasure ParamNormalMeasure;															//parameter data normal measurement
	DataImageProcessing ImageProcessing[CELL_TIEPOINT_COUNT];								//image processing data
	SETrimmingInfo SETrimmingValues;																				//trimming info values
	SENormalInfo SENormalValues[CELL_TIEPOINT_COUNT];												//normal info values
	
	float IpRefCorrectionZero[REF_CELL_COUNT];															//ip reference cell zero correction 
	float IpRef[REF_CELL_COUNT];																						//ip reference cell

	float IpRefBeforeProcess[REF_CELL_COUNT];																//ip reference cell before measurement
	float IpRefAfterProcess[REF_CELL_COUNT];																//ip reference cell after measurement
	
	SETrimmingCell * TrimmCell;																							//pointer to laser trimming cell	
	SEReferenceCell * ReferenceCell;																				//pointer to reference cell	
	SEComMicroLas * ComMicroLas;																						//pointer to microlas communication
	SEComPlc * ComPlc;			          																			//pointer to plc communication

	#ifdef SIMULATION
	FILE *TestDataFile;
	#endif
	
public:
	static SETrimmingRetain SETrimmingRetainData[CHAMBER_COUNT];	//read retain data
	SETrimmingChamber();	//constructor
	SETrimmingChamber( int ChamberId, SEComMicroLas *ComMicroLas, SEComPlc *ComPlc );    //constructor (overload)
	~SETrimmingChamber();		//destructor
	int Initializing( void );	//initialisation
	int CheckFirmware( void );	//check and update firmware
	int MasterReset( void );	//master reset
	int SetParameter( ParamStationDataTrimmingChamber ParamStationTrimmChamber ); //set parameter for trimming chamber
	int SetParameter( ParamDataTrimmingChamber ParamTrimmChamber );					//set parameter for trimming chamber
	int SetParameter( ParamDataNormalMeasure ParamNormalMeasure );					//set parameter for normal measure
	int SetAssemblyData( AssemblyDataTrimmingChamber AssemblyData );				//set assembly data
	//read result at trimming
	int ReadResult( ResultDataTrimming * ResultTrimming, unsigned int Tiepoint );
	//read result at position normal measurement
	int ReadResult( ResultDataPositionNormal * ResultPositionNormal, unsigned int Tiepoint );
	//read result at heater normal measurement
	int ReadResult( ResultDataHeaterNormal * ResultHeaterNormal, unsigned int Tiepoint );
	//read result at universal normal measurement
	int ReadResult( ResultDataUniversalNormal * ResultUniversalNormal, unsigned int Tiepoint );
	//read result at un normal measurement
	int ReadResult( ResultDataUnNormal * ResultUnNormal, unsigned int Tiepoint );
	//read result at ip normal measurement
	int ReadResult( ResultDataIpNormal * ResultIpNormal, unsigned int Tiepoint );
	//read result at ílm normal measurement
	int ReadResult( ResultDataIlmNormal * ResultIlmNormal, unsigned int Tiepoint );
	//read cyclical data
	int ReadCyclic( CyclicDataTrimming * CyclicTrimming, unsigned int * ChamberState, float * IpRef );
	bool ParameterReadyToStart( );		//check ready to start
	bool ParameterReadyForTrimming( );	//check ready for trimming
	//install threads
	int InstallThread( LPTHREAD_START_ROUTINE CallbackAddressMeasurementCell, 
										 LPTHREAD_START_ROUTINE CallbackAddressReferenceCell, 
										 LPTHREAD_START_ROUTINE CallbackAddressChamber,
										 LONG ( *pFCallBackMeasurementCell )( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx ), 
										 LONG ( *pFCallBackReferenceCell )( WORD wFUIdx,WORD wPartIdx,int eMainType,float rMVal,DWORD tsMTime,LONG lResIdx ) );
	void DeinstallThread();     //deinstall threads
	int StartProcess(ProcessType Type);																			//start porcess
	bool ProcessFinished( void );																						//check process finished
	int ClearAllValues( void );																							//clear all values
	int StopProcess( void );																								//stop process
	SETrimmingCell * GetTrimmingCell(void);																	//get address of trimming cell
	SEReferenceCell * GetReferenceCell(void);																//get address of reference cell
	void SetActualPressure( float ActualPressure );													//set actual pressure 
	void SetActualFlow( float ActualFlow );																	//set actual flow
	//set positions form image processing
	int SetPositionImageProcessing ( int Tiepoint, float XPos, float YPos, float ZPos, float Diameter );
	void SetDigitalIn( unsigned int DigitalIn );														//set digital input	(handshake plc)
	unsigned int GetDigitalOut( void );																			//get digital output (handshake plc)
	float IpCorrection( float IpMeasured );																	//ip correction												
	float IpCorrectionOffsetAndDynamic( float IpMeasured, bool OffsetGasoline = true);	//ip correction and dynamics
	bool DeinstallThreadRequested( void );																	//check deinstall thread request
	void HandleProcess(void);																								//handle process
	bool ChamberAllCellsReady();																						//check all cells at chamber ready
	bool IpStabilityCheck( int Tiepoint, int Count );												//ip stability check
	int GasControl( void ); 																								//gas control
	int DieselQualification();																							//diesel qualification
	int TrimmingEntryCheck();																								//trimming entry check
	int TrimmingEndCheck();																									//trimming end check
	int SavePartsJournal();																									//save part journal
	int SaveNormalJournal();																								//save normal journal
	//get bnew serial number and code
	int GetNewSerialNumberAndCode( int PartStatus, unsigned int *Number, char Code[6] );
	int StoreRetainData( void );																						//store retain data																												 
	int LoadRetainData( void );																							//load retain data
	//set part state
	void SetPartState( unsigned int Tiepoint, PartStatus Status, NioReworkReason Reason = NoReason );
	int TrimmingInitFine( int Tiepoint );																		//trimming init fine
	int TrimmingInitRaw( int Tiepoint );																		//trimming init raw
	int Trimming();																													//trimming
	int TrimmingPhase0( int Tiepoint );																			//trimming phase 0 (measure)
	int TrimmingPhase1( int Tiepoint );																			//trimming phase 1 (resist coating (raw))
	int TrimmingPhase2( int Tiepoint );																			//trimming phase 2 (coating end	(raw))
	int TrimmingPhase3( int Tiepoint );																			//trimming phase 3 (resist coating (fine))
	int TrimmingPhase4( int Tiepoint );																			//trimming phase 4 (resist coating (fine))
	int CalculateNewPos( int Tiepoint, bool RawTrimm );											//calculate new position
	//rotate position (square basic)
	void RotatePosition( int Tiepoint, float XPos, float YPos, float *XPosDash, float *YPosDash, float *Phi );
	//rotate position bach (square basic)
	void RotateBackPosition( int Tiepoint, float XPos, float YPos, float *XPosDash, float *YPosDash, float *Phi );
	static HANDLE hThreadHandleProcessChamber[CHAMBER_COUNT];								//static thread handle for each process chamber
	static HANDLE ghMutex[CHAMBER_COUNT]; 
};

	//---------------------------------------------------------------------------------------------------------
	#ifndef _SE_TRIMMING_CHAMBER_INTERNAL
		#undef DLL_DECL
		#undef CALLTYPE
	#endif
	//---------------------------------------------------------------------------------------------------------
	#undef EXTERN
#endif






