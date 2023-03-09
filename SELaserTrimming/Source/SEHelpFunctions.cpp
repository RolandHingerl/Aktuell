/*-------------------------------------------------------------------------------------------------------------------------*
 *  modulname       : SEComMicroLas.cpp                                                                                    *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  project         : SE Lasertrimming                                                                                     *
 *  responsible     : BaP/TEF3.49 - Bätz Oliver                                                                            *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  description:                                                                                                           *
 *  ------------                                                                                                           *
 *  This module contains control and evaluation routines for the communication with micro las.                             *
 *-------------------------------------------------------------------------------------------------------------------------*
 *  version | date       | author         | description of changes                                                         *
 * ---------|------------|----------------|------------------------------------------------------------------------------- *
 *  V xx.yy | dd.mm.yyyy | -              | -                                                                              *
 *  V 00.10 | 27.10.2011 | BaP/TEF3.49-BZ | init version                                                                   *
 *-------------------------------------------------------------------------------------------------------------------------*/
	#include <windows.h>
	#include <math.h>

__int64 GetActualSystemTimeMs( void )
{
	__int64	ActualSystemTicks;																							//actual system ticks
	__int64	ActualSystemFrequency;																					//actual system frequency

	QueryPerformanceCounter((LARGE_INTEGER*)&ActualSystemTicks);					  //get actual system ticks
	QueryPerformanceFrequency((LARGE_INTEGER*)&ActualSystemFrequency);			//get actual system frequency
	return (__int64)((float)ActualSystemTicks / (float)ActualSystemFrequency * 1000.0f);		
}

float Round( float Input, int Digits )
{
	
	float X;

	if( Input >= 0 )
	{
		X = (float) ( (int) ( Input * pow( 10.0f, Digits ) + 0.5 ) / pow( 10.0f, Digits ) );
	}
	else
	{
		X = (float) ( (int) ( Input * pow( 10.0f, Digits ) - 0.5 ) / pow( 10.0f, Digits ) );
	}
		
	return X; 
}

bool CheckBounds(float fRealVal, float fLowerLimit, float fUpperLimit)
//Prüft REAL-Wert auf Bereich
//Anm.: NaN berücksichtigt (ergibt iO).
{
	return( !( (fRealVal < fLowerLimit) || (fRealVal > fUpperLimit) ) ); //return result
}
