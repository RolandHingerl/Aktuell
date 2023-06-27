/** \file
* Projekt:  A4000 = Planarsonde                                      Datei: IMT_BasTypeDef.H *
* Version:  V1.00                                                          Datum: 03.09.2009 *
* \brief   �bergreifende Definitionen der IMT-Basistypen                                    *
*   \copyright 1999-2009 indutron Industrieelektronik GmbH, D-83026 Rosenheim                       *
**********************************************************************************************/

#undef ENUMITEM
#undef ENUMITEM2
#undef BEGIN_ENUM
#undef END_ENUM

#ifndef GENERATE_ENUM_STRINGS
  #define ENUMITEM( element ) element
  #define ENUMITEM2( element, val ) element = val
  #define BEGIN_ENUM( ENUM_NAME ) typedef enum tag##ENUM_NAME
  #define END_ENUM( ENUM_NAME ) ENUM_NAME; \
          extern const char* GetString##ENUM_NAME(enum tag##ENUM_NAME index);
#else
  typedef struct { const char * desc; int type;} EnumDesc_t;
  #define ENUMITEM( element ) { #element, (int)(element) }
  #define ENUMITEM2( element, val ) { #element, val }
  #define BEGIN_ENUM( ENUM_NAME ) EnumDesc_t gs_##ENUM_NAME [] =
  #define END_ENUM( ENUM_NAME ) ; const char* GetString##ENUM_NAME(enum tag##ENUM_NAME index) \
    { for (int i = 0; i < sizeof(gs_##ENUM_NAME)/sizeof(EnumDesc_t); i++) { \
      if ((int)index == gs_##ENUM_NAME [i].type) return gs_##ENUM_NAME [i].desc; } \
      return "Unknown Enum type!!"; }
#endif

  /** Messarten. */
BEGIN_ENUM(eIMTBasTypes)
{
  /** \note ::tageIMTBasTypes Definitionen werden auch im ADV-Projekt in meLS_ImtTypeToMWT() verwendet. */
  ENUMITEM2( IMT_INTERN,0 ),   /**< Interne (reservierte) Messwerttypen */
  ENUMITEM2( IMT_Report,8 ),/**< DLL: Dummy Messwerttyp f�r Reportingabruf */
  ENUMITEM2( IMT_TimeEvent,9 ),/**< DLL: Time-Event */
/** Allgemeine Messwerttypen (10) */
/** \todo: Evtl. ab 1 => �bergreifende ab 100!?! (�: Trotzdem 999 interne Sub-Messtypen m�glich!) */
  ENUMITEM2( IMT_Temp,10 ),    /**< Karten-Temperatur (ADS1258) */
  ENUMITEM ( IMT_HP34401A ),   /**< Pseudo-Me�werttyp f�r Me�wert �ber HP 34401A */
  ENUMITEM ( IMT_dt ),         /**< Zeitdifferenz allgemein */
  ENUMITEM ( IMT_CutOff ),     /**< Abschaltgrund (OVS) */
  ENUMITEM( IMT_TS ), /**< TimeStamp (Zeitpunkt) von Messungen, wird �berall verwendet, wo Zeitstempel als Messwert geliefert werden.*/ 
  ENUMITEM ( IMT_Status ),    /**< Status Bits eines Ger�ts  */
/** �bergreifende Messwerttypen (100) */
/** Heizerseite (100) */
  ENUMITEM2( IMT_HSCONT,100 ), /**< Pseudo-Me�werttyp f�r heizerseitige Kontaktier�berpr�fung */
  ENUMITEM ( IMT_UHReg ),      /**< Interner Heizerspannungsmesswert am Reglerausgang */
  ENUMITEM ( IMT_UHout ),      /**< Heizerspannung am Kartenausgang */
  ENUMITEM ( IMT_UHS ),        /**< Heizerspannung am Sense-Eingang */
  ENUMITEM ( IMT_UHmin ),      /**< UH-Minimum */
  ENUMITEM ( IMT_UHmax ),      /**< UH-Maximum */
  ENUMITEM ( IMT_IH ),         /**< Heizerstrom */
  ENUMITEM ( IMT_IHAVC ),      /**< Heizerstrom bei AVC */
  ENUMITEM ( IMT_RHh ),        /**< Heizerhei�widerstandsme�wert (Pseudo-Typ) */
  ENUMITEM ( IMT_RHk ),        /**< Heizerkaltwiderstandsme�wert (Pseudo-Typ) */
  ENUMITEM ( IMT_PH ),         /**< 110 Heizleistungsme�wert (Pseudo-Typ) */
  ENUMITEM ( IMT_UL ),         /**< Leckstrompr�fspannung MOD: */
  ENUMITEM ( IMT_IL ),         /**< Leckstrommesswert */
  ENUMITEM ( IMT_EH ),         /**< Heizenergiemesswert (Pseudo-Typ) */
  ENUMITEM ( IMT_dtRiReg ),    /**< Zeitintervallwert Umschaltung UH- auf Ri-Regelung */
  ENUMITEM ( IMT_dIHdt ),      /**< Steigung dIH/dt nach UH-Sprung */
  ENUMITEM ( IMT_dRHdt ),      /* Steigung dRH/dt nach UH-Sprung */
  ENUMITEM ( IMT_dtUH ),       /* Zeitintervallwert bei UH-Fenster�berwachung */
  ENUMITEM ( IMT_PHmin ),      /* PH-Minimum */
  ENUMITEM ( IMT_PHmax ),      /* PH-Maximum */
  ENUMITEM ( IMT_dtPH ),       /* Zeitintervallwert bei PH-Fenster�berwachung */
  ENUMITEM ( IMT_ULmin ),      /* UL-Minimum */
  ENUMITEM ( IMT_ULmax ),      /* UL-Maximum */
  ENUMITEM ( IMT_dtUL ),       /* Zeitintervallwert bei UL-Fenster�berwachung */
  ENUMITEM ( IMT_QUH ),        /**< KontHZ: Quotient UHS / UHReg */
  ENUMITEM ( IMT_QHS ),        /**< KontHZ: Quotient UHS / UREmax */
  ENUMITEM ( IMT_UHFreq ),	   /**< Mittelwert Frequenz bei UH getaktet */
  ENUMITEM ( IMT_UHFreqMin ),	   /**< Minimum Frequenz bei UH getaktet */
  ENUMITEM ( IMT_UHFreqMax ),	   /**< Maximum Frequenz bei UH getaktet */
//  ENUMITEM ( IMT_RHkTC ),      /* Heizerkaltwiderstandsmesswert, temperaturkompensiert */
/** \todo Evtl. zwischen ILN und ILM unterscheiden!?! */
/** Sensorseite (150) */
  ENUMITEM2( IMT_SSCONT,150 ), /**< Pseudo-Me�werttyp f�r sensorseitige Kontaktier�berpr�fung */
/* Lamdasensor (200) */
/** Heizerseite (200) **/
/** Sensorseite (250) **/
  ENUMITEM2( IMT_URE,250 ),    /* Spannungsme�wert URE (SE-ADC) */
  ENUMITEM ( IMT_UN ),         /* Nernstspannungsme�wert (SE-ADC) */
//�: Differenzmesswerte (z.B. UN4-UN3) laufen auch �ber Grund-Messtyp.
  ENUMITEM ( IMT_UElyt ),      /* Spannungsme�wert ELYT (SE-ADC) */
  ENUMITEM ( IMT_UAPE ),       /**< Bipolare Spannung an APE (SE-ADC) */
  ENUMITEM ( IMT_IPr ),        /* Aktivierstromme�wert (SE-ADC) */
  ENUMITEM ( IMT_IAC ),        /* 255!!! Stromme�wert zur RiAC-Bestimmung (SE-ADC) */
  ENUMITEM ( IMT_IpRE ),       /**< Referenzpumpstrom */
//P: Worin liegt der Unterschied zwischen IpRE und IpN?
  ENUMITEM ( IMT_IpN ),        /* Pumpsstrom Nernstzelle */
  ENUMITEM ( IMT_IgRK ),       /* Grenzstrom Referenzkanal */
  ENUMITEM ( IMT_IPu ),        /**< APE-Pumpstrommesswert */
  ENUMITEM ( IMT_RiAC ),       /**< RiAC-Me�wert (Pseudo-Typ) */
  ENUMITEM ( IMT_RiDCn ),      /* RiDC-Messwert Nernstzelle */
  ENUMITEM ( IMT_RiDCp ),      /* RiDC-Messwert Pumpzelle */
  ENUMITEM ( IMT_Rel ),        /* Rel-Me�wert (Pseudo-Typ) */
  ENUMITEM ( IMT_dTPr ),       /* Zeitintervallwert IA-Fenster�berwachung*/
  ENUMITEM ( IMT_KSAPERE ),    /* Kurzschlusserkennung APE-RE */
  ENUMITEM ( IMT_L1W ),        /* �bertragungsfunktionspr�fung */
  ENUMITEM ( IMT_RiB ),        /* RiB-Messung */
  ENUMITEM ( IMT_RiPuls ),     /* RiPuls-Messung: */
  ENUMITEM ( IMT_RiPulsUN ),   /* RiPuls-Messung: UN (Uvor,Upuls,Unach) */
  ENUMITEM ( IMT_dtElecCap ),  /* Zeitwert RE-Elektrodenkapazit�t */
  ENUMITEM ( IMT_ElecCap ),    /* RE-Elektrodenkapazit�t */
  ENUMITEM ( IMT_RiACp ),      /* Ri_P-Me�wert (Pseudo-Typ) */
  ENUMITEM ( IMT_UPmin ),      /* UP,min bei Fenster�berwachung */
  ENUMITEM ( IMT_UPmax ),      /* UP,max bei Fenster�berwachung */
  ENUMITEM ( IMT_dtUP ),       /* UP,dt bei Fenster�berwachung */
  ENUMITEM ( IMT_IPuMin ),     /* IPu,min bei Fenster�berwachung */
  ENUMITEM ( IMT_IPuMax ),     /* IPu,max bei Fenster�berwachung */
  ENUMITEM ( IMT_dtIPu ),      /* IPu,dt bei Fenster�berwachung */
  ENUMITEM ( IMT_IpREmin ),    /* IpRE,min bei Fenster�berwachung */
  ENUMITEM ( IMT_IpREmax ),    /* IpRE,max bei Fenster�berwachung */
  ENUMITEM ( IMT_dtIpRE ),     /* IpRE,dt bei Fenster�berwachung */
  ENUMITEM ( IMT_UNmin ),      /**< UN,min bei Fenster�berwachung */
  ENUMITEM ( IMT_UNmax ),      /**< UN,max bei Fenster�berwachung */
  ENUMITEM ( IMT_dtUN ),       /**< UN,dt bei Fenster�berwachung */
  ENUMITEM ( IMT_RiMin ),      /* Ri,min bei Fenster�berwachung */
  ENUMITEM ( IMT_RiMax ),      /* Ri,max bei Fenster�berwachung */
  ENUMITEM ( IMT_dtRi ),       /* Ri,dt bei Fenster�berwachung */
  ENUMITEM ( IMT_IpNmin ),     /* IpN,min bei Fenster�berwachung */
  ENUMITEM ( IMT_IpNmax ),     /* IpN,max bei Fenster�berwachung */
  ENUMITEM ( IMT_dtIpN ),      /* IpN,dt bei Fenster�berwachung */
  ENUMITEM( IMT_dUNdtMin ), /**< dUN/dt min */
  ENUMITEM( IMT_dUNdtMax ), /**< dUN/dt max */
 /* ENUMITEM( IMT_dtdUNdt ), *< Dauer dUN/dt im Fenster*/
/* Partikelsensor (300) */
/** Heizerseite (300) **/
  ENUMITEM2( IMT_SCHTM,300 ),  /**< Pseudo-Me�werttyp f�r Kurzschlusserkennung HTM */
  ENUMITEM ( IMT_RTM ),        /* Temperaturm�anderwiderstand */
  ENUMITEM ( IMT_TMTemp ),     /* Temperatur aus TM-Regelung, ausgerechnet aus RTM-Messwert */
/** Sensorseite (350) **/
  ENUMITEM2( IMT_ULide,350 ),  /* Leckstrompr�fspannung MOD: */
  ENUMITEM ( IMT_ILide ),      /* Leckstrommesswert */
  ENUMITEM ( IMT_UidePl ),     /*  */
  ENUMITEM ( IMT_UideMi ),     /*  */
  ENUMITEM ( IMT_UidePlMi ),   /*  */
//  ENUMITEM ( IMT_UidePlPk ),   /* OBSOLET */
  ENUMITEM ( IMT_TMR0 ),       /* Aus RTMkalt ermittelter R0 */
  ENUMITEM ( IMT_UpkIdeMi ),   /*  */
  ENUMITEM ( IMT_IidePl ),     /*  */
  ENUMITEM ( IMT_IideMi ),     /*  */
  ENUMITEM ( IMT_IideMiPl ),   /* OBSOLET, nein unten umdefineirt  */
  ENUMITEM ( IMT_IidePlPk ),   /* OBSOLET, nein unten umdefineirt */
  ENUMITEM ( IMT_Ride ),       /*  */
  ENUMITEM ( IMT_RFM ),        /**< RFM-Messung  */
  ENUMITEM ( IMT_TempRFM ),    /* Temperatur w�hrend RFM-Messung */
  ENUMITEM ( IMT_RFMCONT ),    /* Kontaktier�berpr�fung RFM */
  ENUMITEM ( IMT_RInpPSTM ),   /* Eingangswiderstand PSTM32 */
  ENUMITEM ( IMT_UideMin ),     /**< Min bei UIDE Fenster�berwachung  */
  ENUMITEM ( IMT_UideMax),     /**< Max bei UIDE Fenster�berwachung  */
  ENUMITEM ( IMT_dtUide),     /**< Dauer UIDE im Fenster  */
  ENUMITEM ( IMT_ILMide ),    /**< Maximaler Leckstrom IDE, wegen Messanschl�gen n�tig*/
/* Multifunktionskarte (900) */
  ENUMITEM2( IMT_DI,900 ),     /*  */
  ENUMITEM ( IMT_AI ),         /*  */
/* HIT-Elektronik (920) */
  ENUMITEM2( IMT_IRlvl,920 ),  /*  */
  ENUMITEM ( IMT_TempBBS ),    /*  */
  ENUMITEM ( IMT_dtPulse ),    /*  */
  ENUMITEM ( IMT_Energy ),     /**< Vom SE aufgenommene Energie in Ws bei HIT */
  ENUMITEM ( IMT_TempIRD ),    /**< Temperatur IR-Diode bei HIT  */
/* HSP-Elektronik (940) */
  ENUMITEM2( IMT_IHV,940 ),    /* Strom bei Hochspannungspr�fung */
  ENUMITEM ( IMT_UHV ),        /* Spannung bei Hochspannungspr�fung */
/* Fremde Messwerttypen (1000) */
  ENUMITEM2( IMT_PLC,1000 ),   /*  */
  ENUMITEM ( IMT_NUM ) /* Anzahl, muss letzter Eintrag sein! */
}
END_ENUM(eIMTBasTypes)

#if !defined(GENERATE_ENUM_STRINGS)
  #define IMT_Upk IMT_ULide
  //#define IMT_Ipk IMT_ILide
  #define IMT_IideIntegral IMT_IideMiPl /* Iide-Stromintegral */
  #define IMT_IideResponse IMT_IidePlPk /* Eigendiagnosestrom */
#endif

