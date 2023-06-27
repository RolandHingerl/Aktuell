/** \file
* Projekt:  A4000 = Planarsonde                                      Datei: IMT_BasTypeDef.H *
* Version:  V1.00                                                          Datum: 03.09.2009 *
* \brief   Übergreifende Definitionen der IMT-Basistypen                                    *
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
  ENUMITEM2( IMT_Report,8 ),/**< DLL: Dummy Messwerttyp für Reportingabruf */
  ENUMITEM2( IMT_TimeEvent,9 ),/**< DLL: Time-Event */
/** Allgemeine Messwerttypen (10) */
/** \todo: Evtl. ab 1 => Übergreifende ab 100!?! (Ü: Trotzdem 999 interne Sub-Messtypen möglich!) */
  ENUMITEM2( IMT_Temp,10 ),    /**< Karten-Temperatur (ADS1258) */
  ENUMITEM ( IMT_HP34401A ),   /**< Pseudo-Meßwerttyp für Meßwert über HP 34401A */
  ENUMITEM ( IMT_dt ),         /**< Zeitdifferenz allgemein */
  ENUMITEM ( IMT_CutOff ),     /**< Abschaltgrund (OVS) */
  ENUMITEM( IMT_TS ), /**< TimeStamp (Zeitpunkt) von Messungen, wird überall verwendet, wo Zeitstempel als Messwert geliefert werden.*/ 
  ENUMITEM ( IMT_Status ),    /**< Status Bits eines Geräts  */
/** Übergreifende Messwerttypen (100) */
/** Heizerseite (100) */
  ENUMITEM2( IMT_HSCONT,100 ), /**< Pseudo-Meßwerttyp für heizerseitige Kontaktierüberprüfung */
  ENUMITEM ( IMT_UHReg ),      /**< Interner Heizerspannungsmesswert am Reglerausgang */
  ENUMITEM ( IMT_UHout ),      /**< Heizerspannung am Kartenausgang */
  ENUMITEM ( IMT_UHS ),        /**< Heizerspannung am Sense-Eingang */
  ENUMITEM ( IMT_UHmin ),      /**< UH-Minimum */
  ENUMITEM ( IMT_UHmax ),      /**< UH-Maximum */
  ENUMITEM ( IMT_IH ),         /**< Heizerstrom */
  ENUMITEM ( IMT_IHAVC ),      /**< Heizerstrom bei AVC */
  ENUMITEM ( IMT_RHh ),        /**< Heizerheißwiderstandsmeßwert (Pseudo-Typ) */
  ENUMITEM ( IMT_RHk ),        /**< Heizerkaltwiderstandsmeßwert (Pseudo-Typ) */
  ENUMITEM ( IMT_PH ),         /**< 110 Heizleistungsmeßwert (Pseudo-Typ) */
  ENUMITEM ( IMT_UL ),         /**< Leckstromprüfspannung MOD: */
  ENUMITEM ( IMT_IL ),         /**< Leckstrommesswert */
  ENUMITEM ( IMT_EH ),         /**< Heizenergiemesswert (Pseudo-Typ) */
  ENUMITEM ( IMT_dtRiReg ),    /**< Zeitintervallwert Umschaltung UH- auf Ri-Regelung */
  ENUMITEM ( IMT_dIHdt ),      /**< Steigung dIH/dt nach UH-Sprung */
  ENUMITEM ( IMT_dRHdt ),      /* Steigung dRH/dt nach UH-Sprung */
  ENUMITEM ( IMT_dtUH ),       /* Zeitintervallwert bei UH-Fensterüberwachung */
  ENUMITEM ( IMT_PHmin ),      /* PH-Minimum */
  ENUMITEM ( IMT_PHmax ),      /* PH-Maximum */
  ENUMITEM ( IMT_dtPH ),       /* Zeitintervallwert bei PH-Fensterüberwachung */
  ENUMITEM ( IMT_ULmin ),      /* UL-Minimum */
  ENUMITEM ( IMT_ULmax ),      /* UL-Maximum */
  ENUMITEM ( IMT_dtUL ),       /* Zeitintervallwert bei UL-Fensterüberwachung */
  ENUMITEM ( IMT_QUH ),        /**< KontHZ: Quotient UHS / UHReg */
  ENUMITEM ( IMT_QHS ),        /**< KontHZ: Quotient UHS / UREmax */
  ENUMITEM ( IMT_UHFreq ),	   /**< Mittelwert Frequenz bei UH getaktet */
  ENUMITEM ( IMT_UHFreqMin ),	   /**< Minimum Frequenz bei UH getaktet */
  ENUMITEM ( IMT_UHFreqMax ),	   /**< Maximum Frequenz bei UH getaktet */
//  ENUMITEM ( IMT_RHkTC ),      /* Heizerkaltwiderstandsmesswert, temperaturkompensiert */
/** \todo Evtl. zwischen ILN und ILM unterscheiden!?! */
/** Sensorseite (150) */
  ENUMITEM2( IMT_SSCONT,150 ), /**< Pseudo-Meßwerttyp für sensorseitige Kontaktierüberprüfung */
/* Lamdasensor (200) */
/** Heizerseite (200) **/
/** Sensorseite (250) **/
  ENUMITEM2( IMT_URE,250 ),    /* Spannungsmeßwert URE (SE-ADC) */
  ENUMITEM ( IMT_UN ),         /* Nernstspannungsmeßwert (SE-ADC) */
//Ü: Differenzmesswerte (z.B. UN4-UN3) laufen auch über Grund-Messtyp.
  ENUMITEM ( IMT_UElyt ),      /* Spannungsmeßwert ELYT (SE-ADC) */
  ENUMITEM ( IMT_UAPE ),       /**< Bipolare Spannung an APE (SE-ADC) */
  ENUMITEM ( IMT_IPr ),        /* Aktivierstrommeßwert (SE-ADC) */
  ENUMITEM ( IMT_IAC ),        /* 255!!! Strommeßwert zur RiAC-Bestimmung (SE-ADC) */
  ENUMITEM ( IMT_IpRE ),       /**< Referenzpumpstrom */
//P: Worin liegt der Unterschied zwischen IpRE und IpN?
  ENUMITEM ( IMT_IpN ),        /* Pumpsstrom Nernstzelle */
  ENUMITEM ( IMT_IgRK ),       /* Grenzstrom Referenzkanal */
  ENUMITEM ( IMT_IPu ),        /**< APE-Pumpstrommesswert */
  ENUMITEM ( IMT_RiAC ),       /**< RiAC-Meßwert (Pseudo-Typ) */
  ENUMITEM ( IMT_RiDCn ),      /* RiDC-Messwert Nernstzelle */
  ENUMITEM ( IMT_RiDCp ),      /* RiDC-Messwert Pumpzelle */
  ENUMITEM ( IMT_Rel ),        /* Rel-Meßwert (Pseudo-Typ) */
  ENUMITEM ( IMT_dTPr ),       /* Zeitintervallwert IA-Fensterüberwachung*/
  ENUMITEM ( IMT_KSAPERE ),    /* Kurzschlusserkennung APE-RE */
  ENUMITEM ( IMT_L1W ),        /* Übertragungsfunktionsprüfung */
  ENUMITEM ( IMT_RiB ),        /* RiB-Messung */
  ENUMITEM ( IMT_RiPuls ),     /* RiPuls-Messung: */
  ENUMITEM ( IMT_RiPulsUN ),   /* RiPuls-Messung: UN (Uvor,Upuls,Unach) */
  ENUMITEM ( IMT_dtElecCap ),  /* Zeitwert RE-Elektrodenkapazität */
  ENUMITEM ( IMT_ElecCap ),    /* RE-Elektrodenkapazität */
  ENUMITEM ( IMT_RiACp ),      /* Ri_P-Meßwert (Pseudo-Typ) */
  ENUMITEM ( IMT_UPmin ),      /* UP,min bei Fensterüberwachung */
  ENUMITEM ( IMT_UPmax ),      /* UP,max bei Fensterüberwachung */
  ENUMITEM ( IMT_dtUP ),       /* UP,dt bei Fensterüberwachung */
  ENUMITEM ( IMT_IPuMin ),     /* IPu,min bei Fensterüberwachung */
  ENUMITEM ( IMT_IPuMax ),     /* IPu,max bei Fensterüberwachung */
  ENUMITEM ( IMT_dtIPu ),      /* IPu,dt bei Fensterüberwachung */
  ENUMITEM ( IMT_IpREmin ),    /* IpRE,min bei Fensterüberwachung */
  ENUMITEM ( IMT_IpREmax ),    /* IpRE,max bei Fensterüberwachung */
  ENUMITEM ( IMT_dtIpRE ),     /* IpRE,dt bei Fensterüberwachung */
  ENUMITEM ( IMT_UNmin ),      /**< UN,min bei Fensterüberwachung */
  ENUMITEM ( IMT_UNmax ),      /**< UN,max bei Fensterüberwachung */
  ENUMITEM ( IMT_dtUN ),       /**< UN,dt bei Fensterüberwachung */
  ENUMITEM ( IMT_RiMin ),      /* Ri,min bei Fensterüberwachung */
  ENUMITEM ( IMT_RiMax ),      /* Ri,max bei Fensterüberwachung */
  ENUMITEM ( IMT_dtRi ),       /* Ri,dt bei Fensterüberwachung */
  ENUMITEM ( IMT_IpNmin ),     /* IpN,min bei Fensterüberwachung */
  ENUMITEM ( IMT_IpNmax ),     /* IpN,max bei Fensterüberwachung */
  ENUMITEM ( IMT_dtIpN ),      /* IpN,dt bei Fensterüberwachung */
  ENUMITEM( IMT_dUNdtMin ), /**< dUN/dt min */
  ENUMITEM( IMT_dUNdtMax ), /**< dUN/dt max */
 /* ENUMITEM( IMT_dtdUNdt ), *< Dauer dUN/dt im Fenster*/
/* Partikelsensor (300) */
/** Heizerseite (300) **/
  ENUMITEM2( IMT_SCHTM,300 ),  /**< Pseudo-Meßwerttyp für Kurzschlusserkennung HTM */
  ENUMITEM ( IMT_RTM ),        /* Temperaturmäanderwiderstand */
  ENUMITEM ( IMT_TMTemp ),     /* Temperatur aus TM-Regelung, ausgerechnet aus RTM-Messwert */
/** Sensorseite (350) **/
  ENUMITEM2( IMT_ULide,350 ),  /* Leckstromprüfspannung MOD: */
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
  ENUMITEM ( IMT_TempRFM ),    /* Temperatur während RFM-Messung */
  ENUMITEM ( IMT_RFMCONT ),    /* Kontaktierüberprüfung RFM */
  ENUMITEM ( IMT_RInpPSTM ),   /* Eingangswiderstand PSTM32 */
  ENUMITEM ( IMT_UideMin ),     /**< Min bei UIDE Fensterüberwachung  */
  ENUMITEM ( IMT_UideMax),     /**< Max bei UIDE Fensterüberwachung  */
  ENUMITEM ( IMT_dtUide),     /**< Dauer UIDE im Fenster  */
  ENUMITEM ( IMT_ILMide ),    /**< Maximaler Leckstrom IDE, wegen Messanschlägen nötig*/
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
  ENUMITEM2( IMT_IHV,940 ),    /* Strom bei Hochspannungsprüfung */
  ENUMITEM ( IMT_UHV ),        /* Spannung bei Hochspannungsprüfung */
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

