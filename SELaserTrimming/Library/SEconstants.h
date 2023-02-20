/** \file 
\brief Konstanten und enums, die sowohl intern als auch extern verwendet werden.
*/

#ifndef _SECONSTANTS_H_
#define _SECONSTANTS_H_

/** Konstanten für Prüfkarten MCx...*/
#define IFL_WAITMCLIST (1L<<31) /**< Mit der Initialisierung warten, bis PR-Liste übergeben */
#define IFL_BAUD500K   (1L<<30) /**< 1 -> 500kBaud, 0 -> 250kBaud */
#define IFL_AUTORESIDX (1L<<29) /**< 1 -> ResIdx wird automatisch gebildet */
#define IFL_CANNETIDX  (1L<<28) /**< 1 -> CAN-Netz 1, 0 -> CAN-Netz 0 */

/* Definitionen zum Firmware-Download über MPDOs */
//#define FWDNLD_ALL        0x80000000L /* Download in alle Knoten */
//H: Download in alle Knoten macht keinen Sinn mehr, da die SE-PL inhomogen sind. Daher generell nicht mehr verwenden.
#define FWDNLD_SEPK       0x40000000L /* Download in alle SEC-Prüfkarten */
#define FWDNLD_SPSCAN     0x20000000L /* Download in alle SPS-CAN-Karten */
#define FWDNLD_PSOVHK     0x20000000L /* Download in alle PSOV-Prüfkarten (HK) */
//A: Gleiches Bit für FWDNLD_PSOVHK wie für FWDNLD_SPSCAN, da nie gleichzeitig in einer Anlage.
#define FWDNLD_HVMUX      0x10000000L /* Download in alle HV-Multiplexer */
#define FWDNLD_ADVHK      0x08000000L /**< Download in alle ADV-Prüfkarten (HK) */
#define FWDNLD_ADVMOD     0x04000000L /**< Download in alle ADV-Prüfkarten (MOD) */
#define FWDNLD_SINGLENODE 0x02000000L /* Download nur in angegebenen Knoten */
#define FWDNLD_NODEAREA   0x01000000L /* Download in angegebenen Knotenbereich */
#define FWDNLD_HIT        0x00800000L /* Download in alle HIT-Elektroniken */
#define FWDNLD_PSM12      0x00400000L /* Download in alle PSOV-Prüfkarten (MOD) */
#define FWDNLD_HVSRC      0x00200000L /* Download in alle HV-Prüfgeräte */
#define FWDNLD_RFM        0x00100000L /* Download in alle RFM-Backen */


#define UPLDFL_COMPRESSED 0x0001 /**< 1 -> komprimierte Übertragung */

#define MWUNDEF ((float)1e-23)

#ifndef MDATASETDEF
#define MDATASETDEF
typedef struct MeasInfStruct
{
	float rMVal; /* Messwert */
	DWORD tsMTime; /* Messzeitpunkt */
} MEASINF;
#endif

/** Elektrodentypen */
enum ELECTYPE {
	_ELEC_RE = 0, /**< RE */
	_ELEC_IPN, /**< IPN */
	_ELEC_APE, /**< APE */
	_ELEC_HPl, /**< H+ */
	_ELEC_HMi, /**< H- */
	_ELEC_TM, /**< TM */
	_ELEC_CNT /**< Anzahl */
};


/** enums und Konstanten für Prüfabläufe TFlw...*/
/** MinMaxWdw-Indizes */
typedef enum MMWIndexEnum {
	MMWIDX_UL = 0, /**< Leckstromprüfspannung */
	MMWIDX_UH, /**< Heizerspannung */
	MMWIDX_PH, /**< Heizleistung */
	MMWIDX_Ri, /**< Ri während Regelung */
	MMWIDX_UP, /**< Pumpspannung APE */
   //MMWIDX_UP und MMWIDX_IPu müssen wegen Schleife direkt aufeinanderfolgen.
	MMWIDX_IPu, /**< Pumpstrom APE */
	MMWIDX_IpRE, /**< Referenzpumpstrom */
	MMWIDX_UN, /**< Nernstspannung RE */
	MMWIDX_IpN, /**< Pumpstrom RE */
	MMWIDX_dUNdtwTS, /**< Gradient Nernstspannung RE mit TimeStamp */
	MMWIDX_UNwTS, /**< Nernstspannung RE mit TimeStamp*/
	MMWIDX_UIDE, /**< UIDE bei PSOV */
	MMWIDX_CNT, /**< Zahl der Einträge in eMMWIDX */
 /** C166 macht 16 Bit große enums, ARM eher 8 Bit, mit #MMWIDX_SIZEDUMMY werden alle Compiler zu 16 Bit Größe gezwungen */
	MMWIDX_SIZEDUMMY = USHRT_MAX
} eMMWIDX;




#define RIREGFL_Ri_P  0x0001 /**< 1 -> Ri_P-Regelung */
#define RIREGFL_RegUH 0x0002 /**< 1 -> UH regeln (und nicht nur steuern) */
#define RIREGFL_IHLIMUHLEAP  0x0004 /**< 1 -> IH-begrenzten UH-Sprung mit UH-Regelung */
#define RIREGFL_NOUHMAXLIM   0x0008 /**< 1 -> keine UH-Begrenzung bei UH- oder PH-Regelung */
#define RIREGFL_NOUHSTARTLIM 0x0010 /**< 1 -> wenn nicht Start mit Ri-Regelung, dann UH-Startspannung nicht begrenzen */
#define RIREGFL_PHLIMABS     0x0020 /**< 1 -> Ri-Grenzen für PH-Mittelwertbildung absolut */
#define RIREGFL_FROMPH       0x0040 /**< 1 -> Übergang von PH auf Ri-Regelung, alte Einstellungen des Reglers verwenden */
#define RIREGFL_CONTLAST    0x0080 /**< 1 -> Übergang auf Ri-Regelung, alte Einstellungen des Reglers verwenden und bei lezter Spannung weitermachen */
#define RIREGFL_RISNAPLASTUH  0x0100 /**< 1 -> Bei Start Ri-Regelung (SnapIn) werden die Parameter so gesetzt, dass der Regler bei letzter Spannung weitermacht */
#define RIREGFL_OLDRHK       0x0200 /**< 1 -> Vor der Messung wird Wert aus alter RHk-Messung übernommen */
#define RIREGFL_PHISMINIMUM 0x0400 /**< 1 ->Angegebener PH-Wert ist Mindestwert für PH-Regelung (für Endmontagelinie) */
#define RIREGFL_LASTUH 0x0800 /**< 1 ->Bei Start Ri-Regelung ohne Snap-In werden die Parameter so gesetzt, dass der Regler bei letzter Spannung weitermacht, die am SE angelegt war. Vor dem Start des Ri-Reglers muss noch RiAC gemessen werden, damit ein alter Wert verfügbar ist.*/

#define _UHIREGELUNG 0x8000 /**< 1 -> Regelgröße UHI, 0 -> Regelgröße UHS */
#define _SOFTRAMP    0x4000 /**< 1 -> Software-Rampe */
#define _FASTRAMP    0x2000 /**< 1 -> Schnellstmögliche Rampe über Schalttransistor (HDSPK = 14æs) */
#define _RAMPREL     0x1000 /**< 1 -> Rampe beginnend ab aktueller Heizerspannung, 0 = UHmin */
#define _LINERESCOMP 0x0800 /**< 1 -> Sprung auf Zielwert (kompensiert), 0 -> Sprung auf Sollwert */
#define _GENSYNC     0x0200 /**< 1 -> Sync. Impulse generieren (PSOV.2) */
#define _UHFLEXT (_UHIREGELUNG|_SOFTRAMP|_FASTRAMP|_RAMPREL|_LINERESCOMP|_GENSYNC)

#define OVSFL_TURNOFFRH    0x0001 /**< 1: Abschaltung bei RH */
#define OVSFL_TURNOFFRiACn 0x0002 /**< 1: Abschaltung bei RiACn */
#define OVSFL_TURNOFFRiACp 0x0004 /**< 1: Abschaltung bei RiACp */
#define OVSFL_TURNOFFIP4   0x0008 /**< 1: Abschaltung bei IP4 */
#define OVSFL_DETECTRH     OVSFL_TURNOFFRH /*0x0010 *//**< 1: Erkennt Überschreitung der RH-Schwelle */
#define OVSFL_DETECTRiACn  OVSFL_TURNOFFRiACn /*0x0020 *//**< 1: Erkennt Unterschreitung der RiACn-Schwelle */
#define OVSFL_DETECTRiACp  OVSFL_TURNOFFRiACp /*0x0040 *//**< 1: Erkennt Unterschreitung der RiACp-Schwelle */
#define OVSFL_DETECTIP4    OVSFL_TURNOFFIP4 /*0x0080 *//**< 1: Erkennt Überschreitung der IP4-Schwelle */
#define OVSFL_TURNOFFRHDEF 0x0100 /**< 1: Abschaltung bei RH-Defekt */
#define OVSFL_TURNOFFRiDEF 0x0200 /**< 1: Abschaltung bei RiAC-Defekt */
#define OVSFL_MEAS_IPU     0x1000 /**< 1: IPu synchron zu RiACn messen  */
#define OVSFL_MEAS_RIACP   0x2000 /**< 1: 0 -> RiACn; 1 -> RiACp  */
#define OVSFL_RHPERCENT    0x4000 /**< 1: RH-Abschaltschwelle wird als Faktor übergeben */
#define OVSFL_1STCYCLE     0x8000 /**< 1: Erster Zyklus (-> RHk speichern) */
#define OVSFL_LARGE_TO_RIACN	   0x0010 /**< 1: RiAC Wert tOVS.tPar.rTurnOffRiACn wird ohne 0,1 Skalierung übertragen*/

#define RHKTFL_KONTHZ   0x01 /**< 1 -> KontHZ implizit */
#define RHKTFL_TEMPCOMP 0x02 /**< 1 -> Temperaturkompensation durchführen */
#define RHKTFL_USEMAXUH 0x04 /**< 1 -> Wenn kein KontHZ, dann trotzdem UH nicht beschränken */
#define RHKTFL_RHUHOFFS 0x08 /**< 1 -> Offset-Abgleich UH-Stellgröße zusammen mit RHk-Messung durchführen */
#define RHKTFL_KOUHOFFS 0x10 /**< 1 -> Offset-Abgleich UH-Stellgröße zusammen mit KontHZ-Messung durchführen */
#define RHKTFL_SWITCHOFF 0x20 /**< 1 -> Wenn RHk außerhalb der Grenzen, wird Kanal abgeschaltet */
#define RHKTFL_KONTGBR 0x40 /**< 1 -> Bei der Kontaktierüberprüfung wird mit 5V #UH_KONTHZ_GBR begonnen, wegen GBRs mit Halbleitern im Heizerkreis. */

#define RHKFL_UHREG    0x0001 /**< 1 -> UH-Regelung, sonst nur Stellung */
#define RHKFL_UHSPOFFS 0x0002 /**< 1 -> Offset-Abgleich UH-Stellgröße durchführen */
#define RHKFL_CHKSCHSS 0x0004 /**< 1 -> Auf Kurzschluss zwischen Heizer- und Sensorseite prüfen */
#define RHKFL_HEATERSTAYSON 0x0008 /**< 1 -> Heizer wird nicht abgeschaltet */
#define RHKFL_OFFSWITHRINT 0x0010 /**< 1 -> interner Widerstand wird auch berücksichtigt */

#define MCYCFL_CHSHIFT 0x0080 /**< 1 -> Kanalversatz, d.h. Kanäle in äqudistanten Zeitabständen einzeln der Reihe nach messen */
#define MCYCFL_FORCE_RIAC    0x0040 /**< 1 -> Zyklische Messung RiAC ohne laufenden RiAC-Regler machen. */
#define MCYCFL_STOP    0x0001 /**< 1 -> PR, nicht dll! stoppt zyklische Messungen mit dem übergebenem ::MType. Bisher nur mit RiAC getestet.*/

#define MUNI_FL_CYCLIC_RIACEXT 0x2000 /* 1 -> zyklischer Messauftrag für RiACext -> Parameter für RiAC-Messung angegeben */

#define KONTSEFL_DONTTURNOFF 0x0001 /**< 1 -> Heizer wird nicht abgeschaltet */  
#define KONTSEFL_DELAYED 0x0002 /**< 1 -> bei Aufruf werden Parameter gespeichert, Auswertung erfolgt während eines längeren 
		                      Ablaufs, z.B. FLO  */  
													
	/**{ Für _PA_ChValSet: 
  schon vergeben: 108 IMT_RHh, 109  IMT_RHk, 259 eigentlich 3 IMT_IPu, 257 eigentlich 1 IMT_IpN, 301 eigentlich 45 IMT_RTM */
#define CHVALSET_UHOFFSET (4)
#define CHVALSET_PHETVOFFSET (5)
/**}*/

/**{ Für RHh Messung mit Flags */
#define RHHFL_FOR_ETV    0x8000 /**< 1 -> RHh-Messung für dynamische ETV. In den niedrigwertigen Bits ist dann die Nummer kodiert: 
                             RHHFL_FORETV | 1 -> 1. Messung
							 RHHFL_FORETV | 2 -> 2. Messung*/
/**}*/
/**{ Für PH Messung mit Flags */
#define PHFL_FOR_ETV    0x8000 /**< 1 -> PH-Messung für dynamische ETV. In den niedrigwertigen Bits ist dann die Nummer kodiert: 
                             PHFL_FORETV | 1 -> 1. Messung
							 PHFL_FORETV | 2 -> 2. Messung*/
/**}*/

/**{ Für Flags bei TFlwSetAdjustValue()  */
 #define ADJSET_RELATIV 0x0001 /**< 1 -> übergebene Werte sind relativ zu den schon vorhandenen*/
 #define ADJSET_STORE 0x0002  /**< 1 -> Werte werden gespeichert */
 #define ADJSET_ISAWT 0x0040  /**< 1 -> in eType übertragener Typ ist AWT, nicht MWT */
 #define ADJSET_ISCONFIG 0x0080  /**< 1 -> in eType übertragener Typ ist für Modulkonfiguration */
 #define ADJSET_RESET 0x0100  /**< 1 -> in eType übertragener Typ wird auf default-Wert zurückgesetzt */
/**}*/

/** Maximale Anzahl an Resultindizes pro TFlw-Aufruf,
vorher Max. 8 Ergebnisse pro Messwert, jetzt 10, wegen Ufett Einzelmesswerte*/
#define MAXRESULTSPERMEAS 10 

#endif
