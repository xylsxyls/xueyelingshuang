      *    
      * Copyright (c) 1991, 2003, Oracle Corporation.  All rights reserved.  
      *   NAME
      *     sqlda.cob 
      *   DESCRIPTION
      *     Include file for dynamic 4 pro*cobol pgms with COMP
      *   RETURNS
      *
      *   NOTES
      *     This include file contains COMP values for
      *     dynamic 4 programs for byte swapped
      *     platforms.  

       01  BNDDSC.
           02  SQLDNUM             PIC S9(9) COMP VALUE 20.
           02  SQLDFND             PIC S9(9) COMP.
           02  BNDDVAR OCCURS 20 TIMES.
               03 BNDDV            PIC S9(9) COMP.
               03 BNDDFMT          PIC S9(9) COMP.
               03 BNDDVLN          PIC S9(9) COMP.
               03 BNDDFMTL         PIC S9(4) COMP.
               03 BNDDVTYP         PIC S9(4) COMP.
               03 BNDDI            PIC S9(9) COMP.
               03 BNDDH-VNAME      PIC S9(9) COMP.
               03 BNDDH-MAX-VNAMEL PIC S9(4) COMP.
               03 BNDDH-CUR-VNAMEL PIC S9(4) COMP.
               03 BNDDI-VNAME      PIC S9(9) COMP.
               03 BNDDI-MAX-VNAMEL PIC S9(4) COMP.
               03 BNDDI-CUR-VNAMEL PIC S9(4) COMP.
               03 BNDDFCLP         PIC S9(9) COMP.
               03 BNDDFCRCP        PIC S9(9) COMP.
      *
       01  SELDSC.
           02  SQLDNUM             PIC S9(9) COMP VALUE 20.
           02  SQLDFND             PIC S9(9) COMP.
           02  SELDVAR OCCURS 20 TIMES.
               03 SELDV            PIC S9(9) COMP.
               03 SELDFMT          PIC S9(9) COMP.
               03 SELDVLN          PIC S9(9) COMP.
               03 SELDFMTL         PIC S9(4) COMP.
               03 SELDVTYP         PIC S9(4) COMP.
               03 SELDI            PIC S9(9) COMP.
               03 SELDH-VNAME      PIC S9(9) COMP.
               03 SELDH-MAX-VNAMEL PIC S9(4) COMP.
               03 SELDH-CUR-VNAMEL PIC S9(4) COMP.
               03 SELDI-VNAME      PIC S9(9) COMP.
               03 SELDI-MAX-VNAMEL PIC S9(4) COMP.
               03 SELDI-CUR-VNAMEL PIC S9(4) COMP.
               03 SELDFCLP         PIC S9(9) COMP.
               03 SELDFCRCP        PIC S9(9) COMP.

      *
       01  XSELDI.
           03  SEL-DI                  OCCURS 20 TIMES PIC S9(4) COMP.
       01  XSELDIVNAME.
           03  SEL-DI-VNAME            OCCURS 20 TIMES PIC X(80).
       01  XSELDV.
           03  SEL-DV                  OCCURS 20 TIMES PIC X(80)
                                         VALUE SPACES.
       01  XSELDHVNAME.
           03  SEL-DH-VNAME            OCCURS 20 TIMES PIC X(80).
      *
       01  XBNDDI.
           03  BND-DI                  OCCURS 20 TIMES PIC S9(4) COMP.
       01  XBNDDIVNAME.
           03  BND-DI-VNAME            OCCURS 20 TIMES PIC X(80).
       01  XBNDDV.
           03  BND-DV                  OCCURS 20 TIMES PIC X(80).
       01  XBNDDHVNAME.
           03  BND-DH-VNAME            OCCURS 20 TIMES PIC X(80).
      *

       01  NULL-STATUS               PIC S9(4) COMP.
       01  PRECISION                 PIC S9(4) COMP.
       01  SCALE                     PIC S9(4) COMP.
