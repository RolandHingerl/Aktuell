/*----------------------------------------------------------------------------------------------------------*
 *  modulname       : SEComPlc.h                                                                       *
 *----------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                      *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                             *
 *----------------------------------------------------------------------------------------------------------*
 *  description:                                                                                            *
 *  ------------                                                                                            *
 *  Header file for SEComPlc.                                                                          *
 *----------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                          *
 * ---------|------------|----------------|---------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                               *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                    *
 *----------------------------------------------------------------------------------------------------------*/

#pragma once

//#include "SETrimmingChamber.h"

#ifndef _SE_COM_PLC
  //---------------------------------------------------------------------------------------------------------
  #define _SE_COM_PLC
  //---------------------------------------------------------------------------------------------------------
  #ifdef _SE_COM_PLC_INTERNAL
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
  #ifdef _SE_COM_PLC_INTERNAL
    #define DLL_DECL		DLL_EXPORT
    #define CALLTYPE		__cdecl
  #else
    #define DLL_DECL		DLL_IMPORT
    #define CALLTYPE		__cdecl
  #endif
  
//-- Global defines -----------------------------------------------------------------------------------------
#define IP_SERVER "192.168.212.33"	 //plc ip address
#define PORT_PLC_CHAMBER1 19554		 //plc port chamber 1
#define PORT_PLC_CHAMBER2 19556		 //plc	port chamber 2

#define ACTION_COUNT 32	//action count

#define NO_CMD           0	//no command
#define SET_PARAMETER    1	//command set parameter
#define GET_RESULT       2	//command get result
#define SET_GET_CYCLICAL 3	//command get cyclical
#define SET_ACTION       4	//command set action
#define GET_STATE        5	//command get state
#define GLOBAL_ERROR     6	//command global error
#define CMD_COUNT        7	//entir command count

#define FLT_LASERTRIMMING_FIRST -139000000	//first error number

#define CHAMBER_COUNT	2	//chamber count

#define CELL_TIEPOINT_COUNT   20	//cell tiepoint count
       
//-- fault global (-139000000..139000099 ) ----------------------------------------------------------------
#define FLT_MICROLAS_CONNECT      -139000000   //se lasertrimming process: microlas connection disrupted
#define FLT_ADVOV_NOT_READY       -139000001   //se lasertrimming process: not all advov card ready
#define FLT_PLC_CONNECT           -139000002   //se lasertrimming process: plc connection disruped
#define FLT_BOOT_NOT_FINISHED     -139000003   //se lasertrimming process: boot not finished 
#define FLT_MICROLAS_ERROR        -139000004   //se lasertrimming process: microlas error 
#define FLT_ADVOV_UPDATE_MAIN     -139000005   //se lasertrimming process: advov card update main faulty 
#define FLT_ADVOV_UPDATE_MODULE   -139000006   //se lasertrimming process: advov card update module faulty 
#define FLT_NO_CHAMBER_ACTIVE     -139000007   //se lasertrimming process: no chamber active
#define FLT_NO_PARAMETER_DATA     -139000008   //se lasertrimming process: no parameter data
#define FLT_MICROLAS_DRILLING_APP -139000009   //se lasertrimming process: microlas error drilling application 

#define FLT_MICROLAS_MICROLAS_FIRST  -139000010  //se lasertrimming process: microlas error drilling application	first
#define FLT_MICROLAS_MICROLAS_COMMON -139000010  //se lasertrimming process: microlas error common
//{ FLT_MICROLAS_MICROLAS_FIRST -1 ... FLT_MICROLAS_MICROLAS_FIRST - 59 } //se lasertrimming process: microlas error codes
#define FLT_MICROLAS_MICROLAS_LAST -139000069  	 //se lasertrimming process: microlas error drilling application	last

//-- fault parameter (-139000100..139000149 ) --------------------------------------------------------------
#define FLT_PARAMETER_DATA_TRIMMING_OUTSIDE_LIMIT -139000100   					  //se lasertrimming process: parameter data trimming outside limit
#define FLT_PARAMETER_DATA_NORMAL_OUTSIDE_LIMIT -139000101   							//se lasertrimming process: parameter data normal outside limit
#define FLT_ASSEMBLY_NOT_PLAUSIBLE -139000102   											    //se lasertrimming process: assembly data not plaubible
#define FLT_EVACUATED_PRESSURE_NOT_PLAUSIBLE -139000103   								//se lasertrimming process: evacuated pressure not plaubible
#define FLT_LASER_POWER_ADJUST_NOT_PLAUSIBLE -139000104   								//se lasertrimming process: laser power adjustment not plausible
#define FLT_IMAGEPROCESSING_DATA_OUTSIDE_LIMIT -139000105   							//se lasertrimming process: imageprocessing data not plausible
#define FLT_MICROLAS_DATA_NOT_PLAUSIBLE -139000106   							        //se lasertrimming process: microlas data not plausible
#define FLT_STATION_PARAMETER_DATA_TRIMMING_OUTSIDE_LIMIT -139000107   		//se lasertrimming process: station parameter data outside limit
#define FLT_MICROLAS_STATION_DATA_NOT_PLAUSIBLE -139000108   						  //se lasertrimming process: microlas station data not plausible

//-- fault result (-139000150..139000199 ) -----------------------------------------------------------------
#define FLT_RESULT_DATA_REQUEST_OUTSIDE_LIMIT -139000150   							  //se lasertrimming process: result data request outside limit

//-- fault cyclical (-139000200..139000249 ) ---------------------------------------------------------------


//-- fault action (-139000250..139000299 ) -----------------------------------------------------------------
#define FLT_ACTION_START_NOT_ALLOWED -139000250														//se lasertrimming process: action start not allowed 

//-- fault state (-139000300..139000349 ) ------------------------------------------------------------------
#define FLT_MICROLAS_SCANNER -139000300																		//se lasertrimming process: fault at microlas control 
#define FLT_ACTION_START_CONDITIONS -139000301														//se lasertrimming process: action start conditions not complied
#define FLT_ADVOV_OVERTEMP -139000302   																  //se lasertrimming process: advov overtemperature

#define FLT_LASERTRIMMING_LAST -139000499																	//last error number

#ifdef DEBUG_OUTPUT_FILE
  #define printf( ... ) dprintf( TRUE, TRUE, ##__VA_ARGS__ );
#endif

//-- Global enum --------------------------------------------------------------------------------------------
  
#include "NormalNumber.h"

//-- Global structures --------------------------------------------------------------------------------------

//parameter microlas scanner move absolute
struct ParamDataScannerMoveAbs
{
  float XPos;	//x-position [mm]
  float YPos;	//y-position [mm]
  float ZPos;	//z-position [mm]
};

//parameter trimming cut 
struct ParamDataScannerTrimmingCut
{
  float StartXPos;	 //start x-position [mm]																											
  float StartYPos;	 //start y-position [mm]
  float StartZPos;	 //start z-position [mm]
  float EndXPos;	 //end x-position [mm]
  float EndYPos;	 //end y-position [mm]
  float EndZPos;	 //end z-position [mm]
  float LaserPower;	 //laser power [mW]
};

//parameter image processing
struct ParamDataImageProcessing
{
  unsigned int Tiepoint; //tiepoint
  float XPos;	  //x-position [mm]
  float YPos;	  //y-position [mm]
  float ZPos;	  //z-position [mm]
  float Diameter; //diameter of hole [mm]
};

//parameter laser point
struct ParamDataScannerLaserPoint
{
  float XPos;	//x-position [mm]
  float YPos;	//y-position [mm]
  float ZPos;	//z-position [mm]
  float LaserPower;	//laser power [mW]
  float LaserTime;	//duration [ms]
};

//parameter write text
struct ParamDataScannerWriteText
{
  char Code[7];	//test to write
  float XPos;	//x-position [mm]
  float YPos;	//y-position [mm]
  float ZPos;	//z-position [mm]
  float Angle;		 //angle [°]
  float LaserPower;	 //laser power [mW]
};

//parameter data reference cell
struct ParamStationDataMainReferenceCell
{
  float IpSetpointRef[2];    //ip setpoint reference sense
};

//parameter station data reference cell
struct ParamStationDataReferenceCell
{
  float ParamHeaterCabelResistance[2];                                    //heater cable resistance [Ω]
  ParamStationDataMainReferenceCell ParamStationMainReferenceCell;
};

//parameter station data trimming cell
struct ParamStationDataTrimmingCell
{
  float ParamHeaterCabelResistance [ CELL_TIEPOINT_COUNT ];	//heater cable resistance [Ω]
};

//geometry data
struct ParamStationDataGeometry
{
  float PrePositionSquare[4]; //pre position square [mm]
};
//trimm data
struct ParamStationDataProcess 
{	
  float IpOffsetGasoline;	//ip offset gasoline [µA] (formally: IP_offset)
  float IpOffsetDiesel;	//ip offset diesel [µA] (formally: IP_offset)
};

//laser data
struct ParamStationDataLaser
{
  unsigned int RTC5SerialNumber;	//rtc5 serial number
  char CorrectionFile [16]; 		//correction file
  unsigned int CalibrationFactorXY; //calib factor xy
  unsigned int CalibrationFactorZ; 	//calib factor z
  float ZCorrA;  //z correction a
  float ZCorrB;  //z correction b
  float ZCorrC;  //z correction c
  unsigned int JumpSpeed;	 //jump speed
  float LaserOnDelay; 		//laser on delay
  float LaserOffDelay;		//laser off delay
  float JumpDelay; 			//jump delay
  float MarkDelay; 			//mark delay
  float PolygonDelay; 		//polygon delay
  unsigned int LaserControlType; //laser control type
  unsigned int LaserPowerDACNo;  //laser dac number
  float LaserPowerScale;		 //laser power scale
  float LaserPowerDelay;		 //laser power delay
};

//trimming process
struct ParamStationDataTrimmingProcess
{
  ParamStationDataGeometry ParamStationGeometry;													//geometry data
  ParamStationDataProcess	ParamStationTrimmingProcess;										//trimming process data
  ParamStationDataLaser	ParamStationLaser;																//laser data
};

//trimming chamber data
struct ParamStationDataTrimmingChamber
{
  ParamStationDataReferenceCell ParamStationRefCell;										//reference cell
  ParamStationDataTrimmingCell ParamStationTrimmCell;										//trimming cell
  ParamStationDataTrimmingProcess	ParamStationTrimmProcess;							//process data
};

//parameter data reference cell
struct ParamDataMainReferenceCell
{
  unsigned int HeatingMode;		 //heating mode (formally: ReferenceHeatingContinuous)
  float HeatingVoltage;			//heating voltage (formally: U_Heiz)
  float HeatingRi;				//internal resistance (formally: Ri)
  float HeatingPower;			//heating power at permanent heating
  float HeatingTime;			//heating time (formally: HeatTime)
  unsigned int TestGasOverRef;	//test gas over reference sense (formally: _Ref_pruefen)
  float AllowableIpVariation;	//allowable ip variation reference sense (formally: d_IP_Ref)
  float IpRef;		 //ip ref (formally: IPref)   
  unsigned int EnableNernstRegulation;	//enable nernst regulation (formally: NernstRegulation)
  float NernstVoltage;	//nernst voltage (formally: U_Nernst)   
  float NernstRegulationStartDelay;		//nernst regulation start delay 
  float PumpVoltage;				//pump voltage (formally: U_Pump)
  float NegativePumpPulsVoltage;	//pump voltage pulse (not used)
  float NegativePumpPulsDelay;		//pump voltage pulse delay (not used)
  float NegativePumpPulsDuration;	//pump voltage pulse duration (not used)
  float KValue; //K-Value for referene probe [mbar]
};

//parameter data reference cell
struct ParamDataReferenceCell
{
  ParamDataMainReferenceCell ParamMainReferenceCell; //main parameter reference cell	
};

//parameter data trimming cell
struct ParamDataMainTrimmingCell
{
  int ProcessType;				//ProcessType (10: Nernst voltage control, 11: 2-point measurement)
  float IpRef;					//ip ref [µA] (formally: Ip_Ref)
  float fIpRefRngMin;
  float fIpRefRngMax;
  float fSeRHkMin;
  float fRHkVMax;
  float fRHkMin;
  float fRHkMax;
  float HeatingRiTrimming;		    //Ri at trimming [Ω] (formally: Ri)
  float HeatingRiSelection;			//Ri at diesel selection [Ω]
  float HeatingRiOffset;			//ri offset because of line rersistance sensor [Ω]
  float HeatingPower;				//heater power [W] (formally: Heiz_Power)
  float HeatingVoltage;				//heater voltage [V] (formally: U_Heiz)
  float HeatingTime;				//heating duration [s] (formally: Heiz_Time)
  float HeaterRampDuration;			//heating ramp duration [s] (formally: HeaterRampDuration)
  float NernstVoltage;				//nernst voltage [V] (formally: U_Nernst)
  float fMinNernstVoltage;			//nernst voltage [V] min 
  float fMaxNernstVoltage;			//nernst voltage [V] max
  float PauseTime;					//pause time [s] (formally: Pausenzeit_H_L)
  float StartDelay;					//start delay [s] (formally: Startverzoegerung)
  float PumpVoltage;				//pump voltage  [V] (formally: U_Pump)
  float MeasIntervall;				//meas intervall [s] (formally: Messintervall)
  unsigned int HeatingType;			//heating type (formally: _Heiz_Leistung)
  unsigned int MeasFLOEnable;		//meas uh-reg to ri-reg (formally: bMeasureFLO)
  float RiTrigger;					//ri trigger [Ω] (formally: RiTrigger)
  float DeltaTihMeas;				//duration di/dt und dr/dt [ms] (formally: deltaTIHMeas)
  float PhDisturbedRi;				//heater power at disturbed ri reglation [W] (formally: fPHwhenRiRegFailed)
  float TimeoutDisturbedRi;			//time to recognize disturbed ri-regulation [s] (formally: fTimeoutForRiRegFailed)
  float IntegrationTime;			//integartion time 
  //type data for 2-point measurement
  float fTimeStartUp1;				//APE pump voltage start time point of measurement 1
  float fUp1Target;
  float fUp1Min;
  float fUp1Max;
  float fIntTimeMeasPoint1;
  float fUp2Target;
  float fUp2Min;
  float fUp2Max;
  float fWaitingTimeIp2Un2Meas;
  float fIntTimeMeasPoint2;
};

//parameter data stability
struct ParamDataRiStability 
{
  float RiStabilityUpperLimit;	//ri stability upper limit [Ω]
  float RiStabilityLowerLimit;	//ri stability lower limit [Ω]
  float RiStabilityDuration;	//ri stability max duration [s]
};

//parameter data ip measurement
struct ParamDataIpMeasurement 
{
  float O2Mrg;			 //o2 contend test gas [Vol%] (formally: O2_MRG)"       
  float O2Air;			 //o2 contend air [Vol%] (formally: O2_Luft)"     
  float KValue;			 //k-value [mbar] (formally: K_Wert)"   
  float PPreHeating;     //pre heating pressure setpoint	[mbar] 
  float PSetpoint;		 //pressure setpoint [mbar] (formally: P_Soll)"     
  float	PMaxAllowedDiff; //max allowed pressure difference [mbar]
  float FlowSetpoint;	 //chamber flow setpoint [ccm/min]
  float FlowMaxAllowedDiff; //chamber flow max allowed difference [ccm/min]
  float EvacuatedPressureAllowedMax;  //max allowed evacuated pressure [mbar]
  float LeackageRateAllowedMax; //max allowed leackage rate [mbar/s]
  int IpCorrectionMode;	 //ip correction mode (formally: _Refsens)"       
  float IpStable;		 //ip stable limit [%] (formally: IP_Stabil)"       
  float IpStableTime1;	 //ip stable time 1 (multiple 1,0s) [s] (formally: Zeit1_IP_stabil)"       
  float IpStableTime2;	 //ip stable time 2 (multiple 1,0s) [s] (formally: Zeit2_IP_stabil)"     
  float IpStableTime3;	 //ip stable time 3 (multiple 1,0s) [s] (formally: Zeit3_IP_stabil)"      
  float MinCutTime;		 //min cut time [ms] (formally: Schnitte_Zeit)"
  int StabilityMeasCount;	//number of stability measurements after trimming [1] (formally: _Anz_Stab_Mess)"        
  float StabilityWaitTime;	//stability wait time between measurements [ms] (formally: Warte_Zeit)"       
  unsigned int EvaluateRhh;	//evaluation rhh [1] (formally: EvaluateRHh)       
  float RhhLowerLimit;	//rhh lower limit [Ω] (formally: RHh_lowerLimit)       
  float RhhUpperLimit;	//rhh upper limit [Ω] (formally: RHh_upperLimit)       
  unsigned int EvaluatePh;	//evaluation ph [1] (formally: EvaluatePH)      
  float PhLowerLimit;   //power lower limit [W] (formally: PH_lowerLimit)       
  float PhUpperLimit; 	//power upper limit [0;50] [W] (formally: PH_upperLimit)       
  unsigned int EnableNernstRegulation;	//enable nernst regulation (0=off, 1=on) (formally: NernstRegulation)        
  float NernstRegulationStartDelay;			//start delay nernst regulation [s] (formally: NernstRegulationStartDelay)      
  int IpCorrectionSource;	//ip correction source (0=none, 1=ref1, 2=ref2, 3=ref1+ref2) (formally: IPCorrectionSource)       
  float StartIpLowerLimit;	//start ip lower limit [µA] (formally: StartIP_lowerLimit)      
  float StartIpUpperLimit;	//start ip upper limit [µA] (formally: StartIP_upperLimit)             
  float MaxPassageLastRawTrim;	//max passage last raw trimming cut [1] (formally: fStatisticsIndicator)

};

//parameter data ri regulation
struct ParamDataRiRegulaton 
{
  float RiRegMaxUH;	//max heater voltage at ri regulation [V]
  float RiRegKp;	//pid-regulator kp-value [1]
  float RiRegKi;	//pid-regulator ki-value [1]
  float RiRegKd;	//pid-regulator kd-value [1]
  float Freq;		//frequency [Hz]
  float Amplitude;	//amplitude [µA]
  float Delay;		//delay [ms]
  float Ta;
};

//parameter data trimming cell
struct ParamDataTrimmingCell
{
  ParamDataMainTrimmingCell ParamMainTrimmingCell;	//main parameter trimming cell 
  ParamDataRiStability	ParamRiStability;			//parameter ri stability
  ParamDataIpMeasurement ParamIpMeasurement;		//parameter ip measurement
  ParamDataRiRegulaton ParamRiRegulaton;			//parameter ri regulation
};

//geometry data
struct ParamDataGeometry
{
  float Fissure;	//fissure [mm] (formally: Spalt Gob-Fein)    
  float HoldBackPositionFineTrim;	//hold back position fine trimming [mm] (formally: d_max_fein )        
  float StartDistanceRawTrim;		//start distance raw trimming [mm] (formally: Start_grob)       
  float StartDistanceFineTrim;		//start distance fine trimming [mm] (formally: Start_fein)        
  float Step1RawTrim;		//step 1 raw trimming [mm] (formally: Step1_grob)       
  float Step1FineTrim;		//step 1 fine trimming [mm] (formally: Step1_fein)   
};

//trimm data
struct ParamDataProcess 
{				    
  float IpSetpoint;					//ip setpoint [µA] (formally: IP_soll)
  float	MinIpForTrimming;			//min. ip for trimming [µA]
  unsigned int MaxPassage;			//max passage [1] (formally: _max_Schnitte)
  float DeltaIpNoise;				//delta ip noise [µA] (formally: d_IP_Rauschen)
  float DeltaIpLayerEnd;			//delta ip layer end [%] (formally: d_IP_Schichtende)
  float DeltaIpLimitRawTrim;		//delta ip raw limit [%] (formally: d_IP_soll_grob)
  float DeltaIpLimitFineTrim;		//delta ip fine limit [%] (formally: d_IP_soll_fein)
  float InterpolationFactorRawTrim;	//interpolation factor raw trimming [1] (formally: Faktor_grob)
  float InterpolationFactorFineTrim;//interpolation factor fine trimming [1] (formally: Faktor_fein)
  float FaktorCRawTrim;		//c-factor raw trimming [1] (formally: Faktor_C_grob)
  float FaktorCFineTrim;	//c-factor fine trimming [1] (formally: Faktor_C_fein)
  unsigned int Linear1X;	//linear or 1/x function (0=linear;1=1/x) (formally: _Linear_1_X)
  float EndTestUpperTolarance;	//end test upper tolerance [%] (formally: Upper_Tol_Endtest)
  float EndTestLowerTolarance;	//end test lower tolerance [%] (formally: Lower_Tol_Endtest)
  float DeltaIpLayerEndScrap;	//delta ip layer end scrap [%] (formally: d_IP_Schichtende_Ausschuss)
  unsigned int CheckMaxCountCrossingReached;//check of max crossing count [1] (formally: CheckMaxCountCrossingReached)
  float MinCutDistance;					//min. cut distance [mm] (formally: Diff_min_X)
  unsigned int AdditionalCutsNumber;	//number of additional cuts [1] (formally: AdditionalCutsNumber)
  float MinDistanceToEdgeForAdditionalCuts;	//min. distance to edge for additional cuts [mm] (formally: MinimalDistanceToEdgeForAdditionalCuts)
  float MinDeviationRawTrim;				//min. deviation raw trimming [%] (formally: MinimalDeviation)
  int MinPassageLastCut;					//min. crossing count at all cuts except last cut fine trimming [1] (formally: MinCrossingsLastCutCoarse)
  unsigned int DeepCutsFineTrim;			//deep cuts at fine trimming (formally: DeepCutsDuringFineTrim)
  float DeltaIpLastFineTrim;				//delta ip last fin trimming [%] (formally: d_IP_Last_fein)
  int MinPassageFineTrim;					//min. passage fine trimming (formally: _min_Ueberfahrten_fein)
  unsigned int ProcessTimeout;				//process timeout (entire action)
  unsigned int CutDirectionAlternating;     //cut direction alternating (0=off,1=on)
  unsigned int CoolingType;		//cooling type (0=off,1=time,2=rhh)
  int CoolingTimePassive;       //cooling time passive
  int CoolingTimeActive;        //cooling time active
  float CoolingRhhPasssive;		//rhh value for cooling passive
  float CoolingRhhActive;		//rhh value for cooling active
  int CoolingTimeoutRhhPasssive;//cooling timeout for cooling passive
  int CoolingTimeoutRhhActive;	//cooling timeout for cooling active
  int Labeling;		//labeling (0=off,1=on)
};

//laser data
struct ParamDataLaser
{
  float ScannerSpeed;			//scanner speed [mm/s]
  unsigned int WobbleEnable;	//wobble enable (0=off, 1=on)
  float WobbleFrequency;		//wobble frequency [Hz] 
  float WobbleTransversal;		//wobble transversal [mm]
  float WobbleLongitudinal;		//wobble longitudinal	[mm]
  float LaserPower[ CELL_TIEPOINT_COUNT ];	//laser power [W]
  float LaserPowerHeightening;     //laser power heightening [%]
};

//diesel qualification
struct ParamDataDieselQualification 
{
  float ScaleFactorIp;          //scale factor for ip calculation diesel [1]
  float IpTrimmingLowerLimit;	//ip lower limit at trimming process [µA] (formally: IP_lowerLimit)
  float IpTrimmingUpperLimit;	//ip upper limit at trimming process [µA] (formally: IP_upperLimit)
  float IpSelectionLowerLimit;	//ip lower limit at selection process [µA] (formally: IP_lowerLimit)
  float IpSelectionUpperLimit;	//ip upper limit at selection process [µA] (formally: IP_upperLimit)
};

//trimming process
struct ParamDataTrimmingProcess
{
  ParamDataGeometry ParamGeometry;			//geometry data
  ParamDataProcess	ParamTrimmingProcess;	//trimming process data
  ParamDataLaser	ParamLaser;				//laser data
  ParamDataDieselQualification ParamDieselQualification;	//diesel qualification data
};

//trimming chamber data
struct ParamDataTrimmingChamber
{
  ParamDataReferenceCell ParamRefCell;	//reference cell
  ParamDataTrimmingCell ParamTrimmCell;	//trimming cell
  ParamDataTrimmingProcess ParamTrimmProcess;	 //process data
};


//check type
struct CheckType
{
  float LowerLimit;		//lower limit
  float UpperLimit;		//upper limit
};

//parameter data position normal
struct ParamDataPositionNormal
{
  CheckType ChkSensor[CELL_TIEPOINT_COUNT];		//sensor side
  CheckType ChkHeater[CELL_TIEPOINT_COUNT];		//heater side
};


//parameter heater normal
struct ParamDataHeaterNormal
{
  float HeaterVoltageSetpointHot;		//heater voltage setpoint hot
  float HeaterVoltageSetpointCold;		//heater voltage setpoint cold
  CheckType ChkHeaterCurrent;			//check heater current
  CheckType ChkHeaterResistanceHot;		//check heater resistance hot
  CheckType ChkHeaterResistanceCold;	//check heater resistance cold
};

//parameter un normal
struct ParamDataUnNormal
{
  float HeaterVoltageSetpoint;		//heater voltage setpoint
  CheckType  ChkNernstVoltage;		//check nernst voltage
};

//parameter universal normal
struct ParamDataUniversalNormal
{
  float UlSetpoint;		//ul setpoint
  float IgrkUpNSetpoint;		//igrk upn setpoint	  
  float RidcnHeaterVoltageSetpoint;		//ridcn heater voltage 
  float RidcnUpNSetpoint;	//ridcn upn setpoint
  float IpreHeaterVoltageSetpoint;	//ipre heater voltage setpoint
  float IpreIpNSetpoint;			//ipre ipn setpoint
  float RiacHeaterVoltageSetpoint;	//riac heater voltage setpoint

  CheckType ChkIl;	//check il

  CheckType ChkRiacStat;	//check riac stat

  CheckType ChkRiac;	//check riac
  
  CheckType ChkRidc;	//check ridc

  CheckType ChkIgrk;	//check igrk

  CheckType ChkIpre;	//check ipre
};

//parameter ip normal
struct ParamDataIpNormal
{
  float HeaterVoltageSetpoint;		//heater voltage setpoint [V]
  float IpPumpVoltageSetpoint;		//pump voltage setpoint
  CheckType ChkUp; //check up
  CheckType ChkIp; //check ip
};

//parameter ilm normal
struct ParamDataIlmNormal
{
  float Pt3IpReSetpoint;//pt3 ipre setpoint
  float Pt3Up2Setpoint;	//pt3 up2 setpoint
  float Pt3Up3Setpoint; //pt3 up3 setpoint
  float Pt3Up4Setpoint;	//pt3 up4 setpoint
  CheckType ChkIlMax;	//check il max
  CheckType ChkPt3Ip2;	//check ip2
  CheckType ChkPt3Ip4;	//check ip4
  CheckType ChkPt3Un2;	//check un2
  CheckType ChkPt3Un3;	//check un3
  CheckType ChkPt3Un4;	//check un4
  CheckType ChkIaIpn;	//check iaipn
  CheckType ChkUIpn;    //check uipn
  
};

//parameter data normal measure
struct ParamDataNormalMeasure
{
  ParamDataPositionNormal ParamPositionNormal;	//position normal
  ParamDataHeaterNormal	ParamHeaterNormal;		//heater normal
  ParamDataUnNormal ParamUnNormal;				//un normal
  ParamDataUniversalNormal ParamUniversalNormal;//universal normal
  ParamDataIpNormal	ParamIpNormal;				//ip normal
  ParamDataIlmNormal ParamIlmNormal;			//ilm normal
};

//assembly data common 
struct AssemblyDataTrimmingCommon
{
  unsigned int TypeNo;	//type number
  unsigned int TypeNoGs;	//type number
  unsigned int TypeNoDs;	//type number
  unsigned int Charge;	//charge number
  unsigned int PartCharge;	//part charge number
  int ProcessType;		//process type (1=selection gs/ds, 2=trimming gs, 3=trimming gs/ds)
  unsigned int WpcNo;	//wpc number
  unsigned int ChamberNumber;	 //chamber number
  unsigned int GlobalMachineNumber;	//global machine number
  float EvacuatedPressure;	//evacuated pressure [mbar]
  float AdjustPollutePercent;	//adjustment pollution protection glass [%]	(0%=max pollution; 100%=no pollution; 200%=not plausible (plc zero line wrong))
  float LaserPower;		//laser power sensor [mW]
  float LeakageRate;	//leakage rate [mbar/s]
  float AtmospherePressure;	//atmosphere pressure [mbar]
};

//assembly data each tiepoint
struct AssemblyDataTrimmingTiepoint
{
  int SEPartStatus;	 //part status
  int SEPartNioReworkReason; //nio or rework reason
};

//assembly data every chamber
struct AssemblyDataTrimmingChamber
{
  AssemblyDataTrimmingCommon AssemblyDataCommon;//common data
  AssemblyDataTrimmingTiepoint AssemblyDataTiepoint[CELL_TIEPOINT_COUNT];	//tiepoint specific data
};

//result data trimming
struct ResultDataTrimming
{
  unsigned int SerialNumber;//serial number
  char SerialCode[7];		//serial code
  float IpStart;	//start ip
  float IpEndCut;   //ip at end of last cut 
  float IpEndCheck;	//ip at end check
  int CutCount;	    //count of cuts
  float AverageCrossingCount; //avg crossing count
  int CrossingCountLastRaw;	  //crossing count at last raw trimming cut
  int SEPartStatus;		//part status
  int SEPartNioReworkReason; //nio or rework reason
  float TrimmingCutPos[100]; //cut position every crossing
  float TrimmingCutIp[100];	 //ip every crossing
  float IpRef[2];  //ip value reference cell at beginning process
};

//result data position normal
struct ResultDataPositionNormal
{
  float RiDcp;	//RiDcp	[Ω]
  float RiDcn;	//RiDcn [Ω]
  float IpCont;	//IpCont [mA]
  float Ih;		//heater current [A]
  float Rhhot;	//heater resistance hot [Ω]
  int Status;	//status code (0=io,1=value1 nio,2=value2 nio,4=value3 nio,8=value4 nio,...)
};

//result data heater normal
struct ResultDataHeaterNormal
{
  float ContactOk;	//contact ok
  float RhCold;		//heater resistance cold [Ω]
  float Ih;			//heater current [A]
  float Rhhot;		//heater resistance hot [Ω]
  int Status;		//status code (0=io,1=value1 nio,2=value2 nio,4=value3 nio,8=value4 nio,...)
};

//result data universal normal
struct ResultDataUniversalNormal
{
  float Igrk;	//igrk [µA]
  float Il;	    //leak current [µA]
  float RiDcn;	//RiDcn [Ω]
  float IpRe;	//IpRe [µA]
  float RiAc;	//RiAc [Ω]
  float RiAcstat;	//RiAcStat [Ω]
  int Status;	//status code (0=io,1=value1 nio,2=value2 nio,4=value3 nio,8=value4 nio,...)
};

//result data un normal
struct ResultDataUnNormal
{
  float Un;		//nernst voltage [mV]
  int Status;	//status code (0=io,1=value1 nio,2=value2 nio,4=value3 nio,8=value4 nio,...)
};

//result data ip normal
struct ResultDataIpNormal
{
  float UApe;  //UApe [V]
  float Ip;	   //pump current [µA]
  int Status;  //status code (0=io,1=value1 nio,2=value2 nio,4=value3 nio,8=value4 nio,...)
};

//result data ilm normal
struct ResultDataIlmNormal
{
  float Ilm;	//leak current max [µA]
  float IaIpn;	//IaIpn [mA]
  float UIpn;	//UIpn [V]
  float Ip2;	//Ip2 [mA]
  float Un2;	//Un2 [mV]
  float Ip3;	//Ip3 [mA]
  float Un3;	//Un3 [mV]
  float Ip4;	//Ip4 [mA]
  float Un4;	//Un4 [mV]
  int Status;	//status code (0=io,1=value1 nio,2=value2 nio,4=value3 nio,8=value4 nio,...)
};

//cyclical data
struct CyclicDataTrimming
{
  float ActIpValue;		//actual ip value
  float ActRiValue;		//actual ri value
  float ActRhhValue;	//actual rhh value
  int ActCrossingCount; //crossing count
  unsigned int ActCutCount;		//cut count over all
  unsigned int ActPhase;		//actual phase at trimming process
};

//trimming parameter request from plc 
struct PlcRequestStationTrimming
{
  char Id;
  char SubId;
  ParamStationDataTrimmingChamber ParamStationTrimmingChamber;
};

//trimming parameter request from plc 
struct PlcRequestParameterTrimming
{
  char Id;
  char SubId;
  ParamDataTrimmingChamber ParamTrimmingChamber;
};

//normal measure parameter request from plc 
struct PlcRequestParameterNormalMeasure
{
  char Id;
  char SubId;
  ParamDataNormalMeasure ParamNormalMeasure;
};

//scanner move abs parameter request from plc 
struct PlcRequestParameterMoveAbs
{
  char Id;
  char SubId;
  ParamDataScannerMoveAbs ParamScannerMoveAbs;
};

//trimming cut parameter request from plc 
struct PlcRequestParameterTrimmingCut
{
  char Id;
  char SubId;
  ParamDataScannerTrimmingCut ParamScannerTrimmingCut;
};

//image processing parameter request from plc 
struct PlcRequestParameterImageProcessing
{
  char Id;
  char SubId;
  ParamDataImageProcessing ParamImageProcessing;
};

//laser point parameter request from plc 
struct PlcRequestParameterLaserPoint
{
  char Id;
  char SubId;
  ParamDataScannerLaserPoint ParamScannerLaserPoint;
};

//write text parameter request from plc 
struct PlcRequestParameterWriteText
{
  char Id;
  char SubId;
  ParamDataScannerWriteText ParamScannerWriteText;
};

//trimming assambly parameter request from plc 
struct PlcRequestParameterTrimmingAssembly
{
  char Id;
  char SubId;
  AssemblyDataTrimmingChamber ParamAssembly;
};

//normal number parameter request from plc 
struct PlcRequestParameterNormalNumber
{
  char Id;
  char SubId;
  unsigned int NormalNumber;
};

//parameter response to plc
struct PlcResponseParameter
{
  char Id;
  int RetVal;
};

//result request from plc 
struct PlcRequestResult
{
  char Id;
  char SubId;
  unsigned int Tiepoint;
};

//trimming result response to plc
struct PlcResponseResultTrimming
{
  char Id;
  char SubId;
  int RetVal;
  ResultDataTrimming ResultTrimming;		
};

//position normal result response to plc
struct PlcResponseResultPositionNormal
{
  char Id;
  char SubId;
  int RetVal;
  ResultDataPositionNormal ResultPositionNormal;		
};

//heater normal result response to plc
struct PlcResponseResultHeaterNormal
{
  char Id;
  char SubId;
  int RetVal;
  ResultDataHeaterNormal ResultHeaterNormal;		
};

//universal normal result response to plc
struct PlcResponseResultUniversalNormal
{
  char Id;
  char SubId;
  int RetVal;
  ResultDataUniversalNormal ResultUniversalNormal;		
};

//un normal result response to plc
struct PlcResponseResultUnNormal
{
  char Id;
  char SubId;
  int RetVal;
  ResultDataUnNormal ResultUnNormal;		
};

//ip normal result response to plc
struct PlcResponseResultIpNormal
{
  char Id;
  char SubId;
  int RetVal;
  ResultDataIpNormal ResultIpNormal;		
};

//ilm normal result response to plc
struct PlcResponseResultIlmNormal
{
  char Id;
  char SubId;
  int RetVal;
  ResultDataIlmNormal ResultIlmNormal;		
};

//cyclic request from plc 
struct PlcRequestCyclic
{
  char Id;
  float ChamberPressureActual;
  float ChamberFlowActual;
  float ChamberLaserPowerActual;
  unsigned int DigitalIn;
};

//cyclic response to plc
struct PlcResponseCyclic
{
  char Id;
  int RetVal;
  unsigned int ApplicationState;
  unsigned int ChamberState;
  unsigned int DigitalOut;
	float ActIpRef[2];                                       				
  CyclicDataTrimming CyclicTrimming[CELL_TIEPOINT_COUNT];
};

//state request from plc 
struct PlcRequestState
{
  char Id;
};

//state response to plc
struct PlcResponseState
{
  char Id;
  int RetVal;
  bool ActionStarted[32];
  bool ActionFinished[32];
};

//action request from plc
struct PlcResponseAction
{
  char Id;
  int RetVal;
};

//action response to plc
struct PlcRequestAction
{
  char Id;
  bool ActionStart[32];
};


//error response to plc
struct PlcResponseError
{
  char Id;
  int RetVal;
};


/*-------------------------------------------------------------------------------------------------------------------------*
 * class name           : SEComPlc                                                                                         *
 *                                                                                                                         *
 * description:         : This is the class for communication with plc.                                                    *
 *-------------------------------------------------------------------------------------------------------------------------*/
class SEComPlc
{

private:
	//-- members
	//input members
	int ChamberId;		    //chamber id
	bool SocketConnected;	//socket connected
	SOCKET ServerSocket;	//server socket
	SOCKET AcceptSocket;	//accept socket
	int ActualErrorCode[CMD_COUNT];	//actual error code
	bool Ready;		  //ready
	bool BootReady;	  //boot ready flag
	bool ParameterState;	//parameter state
	unsigned int ChamberState;	//chamber state

  //parameterdata 
  ParamStationDataTrimmingChamber ParamStationTrimmingChamber;	//trimming chamber station data
  ParamDataTrimmingChamber ParamTrimmingChamber;		//trimming chamber
  ParamDataNormalMeasure ParamNormalMeasure;			//normal measure
  ParamDataScannerMoveAbs ParamScannerMoveAbs;			//scanner move absolute 
  ParamDataScannerTrimmingCut ParamScannerTrimmingCut;	//scanner trimming cut
  ParamDataImageProcessing ParamImageProcessing;		//image processing
  ParamDataScannerLaserPoint ParamScannerLaserPoint;	//scanner laser point
  ParamDataScannerWriteText ParamScannerWriteText;		//scanner write text
  AssemblyDataTrimmingChamber AssemblyData;				//assembly data
  NormalNumber eNormalNumber;	//normal number

  //result data
  ResultDataTrimming ResultTrimming;	//trimming chamber
  ResultDataPositionNormal ResultPositionNormal;	//position normal
  ResultDataHeaterNormal ResultHeaterNormal;		//heater normal
  ResultDataUniversalNormal ResultUniversalNormal;	//universal normal
  ResultDataUnNormal ResultUnNormal;	//un normal
  ResultDataIpNormal ResultIpNormal;	//ip normal
  ResultDataIlmNormal ResultIlmNormal;	//ilm normal

  //cyclic data
  CyclicDataTrimming CyclicTrimming[CELL_TIEPOINT_COUNT];	//cyclical data

  //action flag data
  bool ActionStart[32];	  //action start flag
  bool ActionRisingEdge[32];  //rising edge flag
  bool ActionFallingEdge[32]; //falling edge flag
  bool ActionStarted[32];	  //action started flag
  bool ActionFinished[32];	  //action finished flag
  bool ActionAbort[32];	//action abort flag

  float ChamberPressureActual;	 //actual chamber pressure
  float ChamberFlowActual;		 //actual chamber flow
  float ChamberLaserPowerActual; //actual chamber laser power

  unsigned int DigitalIn;	//digital in 
  unsigned int DigitalOut;	//digital out

	float ActIpRef[2]; 

  //output members
  

  //-- memberfunctions
public:
  //-- memberfunctions
  SEComPlc();	 //constructor
  SEComPlc( int ChamberId );  //constructor (overloaded)
  ~SEComPlc();		     //destructor
  int Initializing( void );		//initialization
  ParamStationDataTrimmingChamber GetParamStationDataTrimming( void );		//get trimming parameter data
  ParamDataTrimmingChamber GetParamDataTrimming( void );		//get trimming parameter data
  ParamDataNormalMeasure GetParamDataNormalMeasure( void ); 	//get normal measure data
  ParamDataScannerMoveAbs GetParamDataMoveAbs( void );			//get parameter scanner move abolute
  ParamDataScannerTrimmingCut GetParamDataTrimmingCut( void );	//get parameter data scanner trimming cut 
  ParamDataImageProcessing GetParamDataImageProcessing( void );	//get parameter image processing
  ParamDataScannerLaserPoint GetParamDataLaserPoint( void );	//get parameter data scanner laser point
  ParamDataScannerWriteText GetParamDataWriteText( void );		//get parameter data scanner write text
  float GetParamDataAdjustPollute( void );	//get parameter adjust pollute
  NormalNumber GetNormalNumber( void );		//get normal number
  AssemblyDataTrimmingChamber GetAssemblyData( void );	//get assembly data

  ResultDataTrimming * GetResultDataTrimmingAddress( void );//get address result data trimming
  ResultDataPositionNormal * GetResultDataPositionNormalAddress( void );	//get address result data position normal
  ResultDataHeaterNormal * GetResultDataHeaterNormalAddress( void );	//get address result data heater normal
  ResultDataUniversalNormal * GetResultDataUniversalNormalAddress( void );//get address result data universal normal
  ResultDataUnNormal * GetResultDataUnNormalAddress( void );	//get address result data un normal
  ResultDataIpNormal * GetResultDataIpNormalAddress( void );	//get address result data ip normal
  ResultDataIlmNormal * GetResultDataIlmNormalAddress( void );	//get address result data ilm normal

  CyclicDataTrimming * GetCyclicDataTrimmingAddress( void );	//get address result data cyclical

  unsigned int * GetChamberStateAddress( void );	//get address chamber state

	float * GetIpRefValuesAddress( void );	  	//get address chamber state

  int Connect( void );		//connect to plc
  int Disconnect( void );	//disconnect from plc

  static HANDLE hThreadHandleComPlc[CHAMBER_COUNT];	//thread handle

  int RecvSend( void );		//receive and send 
  int InstallThread( LPTHREAD_START_ROUTINE CallbackAddress ) ;		//install thread

  void MasterReset();	//master reset

  float GetActualPressure( void );	//get actual chamber pressure
  float GetActualFlow( void );			//get actual chamber flow
	float GetActualLaserPower( void );		//get actual chamber laser power

  unsigned int GetDigitalIn( void );	//get digital in flags
  void SetDigitalOut( unsigned int DigitalOut );	//set digital out flags

  bool GetActionFlag( char Flag );	//get action flag
  bool GetRisingEdge( char Flag );		//get rising edge
  bool GetFallingEdge( char Flag );	//get falling edge
  bool GetAbortFlag( char Flag );	//get abort flag
  void WriteActionStartedFlag( char Flag, bool State );	//write action start flag
  void WriteActionFinishedFlag( char Flag, bool State );//write action finished flag
  void WriteActionAbortFlag( char Flag, bool State );	//write action abort flag
  void SetErrorCode( unsigned int Type, int ErrorCode );//set error code
  int ReadErrorCode( unsigned int Type );	//read error code
  void SetBootReady( void );	//set boot ready
  void SetReady( void );//set ready

  int (*ParameterOverTaken)( unsigned int Type );	//parameter overtaken function
  int (*ResultRead)( unsigned int Type, unsigned int Tiepoint );		//read reasult function
  void (*ActionFlagChanged)( void );												//action flag changed
  void (*CyclicalReceived)( void );			//cyclical data received
  

 /*-------------------------------------------------------------------------------------------------------------------------*
  * function name        : void InstallOverTakenCallback( int ( *Callbackfunc )( unsigned int SubId ) )                     *
  *                                                                                                                         *
  * input:               : int ( *Callbackfunc )( unsigned int SubId ) : callback function                                  *
  *                                                                                                                         *
  * output:              : void                                                                                             *
  *                                                                                                                         *
  * description:         : This is the function to install the parameter overtaken callback function.                       *
  *-------------------------------------------------------------------------------------------------------------------------*/
  void InstallOverTakenCallback( int ( *Callbackfunc )( unsigned int SubId ) )
  {
    ParameterOverTaken = Callbackfunc;
  }

  /*------------------------------------------------------------------------------------------------------------------------*
  * function name        : void InstallResultReadCallback( int ( *Callbackfunc )( unsigned int SubId, unsigned int Tiepoint ) )*
  *                                                                                                                         *
  * input:               : int ( *Callbackfunc )( unsigned int SubId, unsigned int Tiepoint ) : callback function           *
  *                                                                                                                         *
  * output:              : void                                                                                             *
  *                                                                                                                         *
  * description:         : This is the function to install the result read callback function.                               *
  *-------------------------------------------------------------------------------------------------------------------------*/
  void InstallResultReadCallback( int ( *Callbackfunc )( unsigned int SubId, unsigned int Tiepoint ) )
  {
    ResultRead = Callbackfunc;
  }
  
  /*------------------------------------------------------------------------------------------------------------------------*
  * function name        : void InstallActionFlagChangedCallback( void ( *Callbackfunc )( void ) )                          *
  *                                                                                                                         *
  * input:               : void ( *Callbackfunc )( void ) : callback function                                               *
  *                                                                                                                         *
  * output:              : void                                                                                             *
  *                                                                                                                         *
  * description:         : This is the function to install the action flag changed callback function.                       *
  *-------------------------------------------------------------------------------------------------------------------------*/
  void InstallActionFlagChangedCallback( void ( *Callbackfunc )( void ) )
  {
    ActionFlagChanged = Callbackfunc;
  }

  /*------------------------------------------------------------------------------------------------------------------------*
  * function name        : void InstallCyclicalReceivedCallback( void ( *Callbackfunc )( void ) )                           *
  *                                                                                                                         *
  * input:               : void ( *Callbackfunc )( void ) : callback function                                               *
  *                                                                                                                         *
  * output:              : void                                                                                             *
  *                                                                                                                         *
  * description:         : This is the function to install the cyclical received callback function.                         *
  *-------------------------------------------------------------------------------------------------------------------------*/
  void InstallCyclicalReceivedCallback( void ( *Callbackfunc )( void ) )
  {
    CyclicalReceived = Callbackfunc;
  }	


};

//---------------------------------------------------------------------------------------------------------
  #ifndef _SE_COM_PLC_INTERNAL
    #undef DLL_DECL
    #undef CALLTYPE
  #endif
  //---------------------------------------------------------------------------------------------------------

// Analogous to Enum "TYPE MeasuringResultSubCmdEnum" in PLC software
#if 1
enum PLCMeasuringResultCmd
{
	UNDEFINED				= 0,
	RESULT_SELECTION		= 1,
	RESULT_POSITION_NORMAL	= 2,
	RESULT_HEATER_NORMAL	= 3,
	RESULT_UNIVERSAL_NORMAL = 4,
	RESULT_UN_NORMAL		= 5,
	RESULT_IP_NORMAL		= 6,
	RESULT_ILL_NORMAL		= 7,
};
#endif
  #undef EXTERN
#endif


