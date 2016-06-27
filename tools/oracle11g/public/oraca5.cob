      *    
      *****************************************************************
      *                                                               *
      *               O  R  A  C  A                                   *
      *                                                               *
      * User setable flags:                                           *
      *                                                               *
      *   ORACCHF : .NE. 0 if "do cursor cache consistency check"     *
      *             (note: ORADBGF must be .NZ.).                     *
      *   ORADBGF : .NE. 0 if "do DEBUG checks". This is a "master"   *
      *             debug flag used to turn all DEBUG options on      *
      *             or off.                                           *
      *   ORAHCHF : .NE. 0 if "check heap consistency" (ORADBGF must  *
      *             be .NZ.). Useful to help determine if someone     *
      *             is erroneously clobbering memory that the PCC     *
      *             "owns". MUST BE SET BEFORE 1ST CONNECT. Subsequent*
      *             attempts to change ORAHCHF are ignored.           *
      *   ORASTXTF: Saved SQL stmt text flag. Used to control whether *
      *             text of current SQL stmt is saved in ORASTXT.     *
      *             (Note: ORASTXTF is independent of ORADBGF).       *
      *                                                               *
      *             0 = Don't save SQL stmt (default).                *
      *             1 = Save SQL stmt on SQLERROR.                    *
      *             2 = Save SQL stmt on SQLWARNING/SQLERROR.         *
      *             3 = Always save SQL stmt.                         *
      *                                                               *
      *   Cursor cache statistics. Set after COMMIT or ROLLBACK. Each *
      *   CONNECT'd DATABASE has its own set of statistics.           *
      *                                                               *
      *   ORAHOC  : Highest Max Open OraCursors requested. Highest    *
      *             value for MAXOPENCURSORS by any CONNECT to this   *
      *             DATABASE.                                         *
      *   ORAMOC  : Max Open OraCursors required. Specifies the max nr*
      *             of OraCursors required to run this pgm. Can be    *
      *             higher than ORAHOC if working set (MAXOPENCURSORS)*
      *             was set too low, thus forcing the PCC to expand   *
      *             the cache.                                        *
      *   ORACOC  : Current nr of OraCursors used.                    *
      *   ORANOR  : Nr of OraCursor cache reassignments. Can show the *
      *             degree of "thrashing" in the cache. Optimally,    *
      *             this nr should be kept as low as possible (time   *
      *             vs space optimization).                           *
      *   ORANPR  : Nr of SQL stmt "parses".                          *
      *   ORANEX  : Nr of SQL stmt "executes". Optimally, the rela-   *
      *             tionship of ORANEX to ORANPR should be kept as    *
      *             high as possible.                                 *
      *                                                               *
      * MODIFIED                                                      *
      *   losborne   03/09/94 - COMP-5 version of ORACA               *
      *   Clare      02/20/86 - Add heap consistency check.           *
      *   Clare      03/12/86 - ORACA cuc statistics.                 *
      *****************************************************************
       01  ORACA.
           05  ORACAID               PIC X(8).
           05  ORACABC               PIC S9(9) COMP-5.
      *
      *    Flags which are setable by User.
      *
           05  ORACCHF               PIC S9(9) COMP-5.
           05  ORADBGF               PIC S9(9) COMP-5.
           05  ORAHCHF               PIC S9(9) COMP-5.
           05  ORASTXTF              PIC S9(9) COMP-5.
      *
      *    Saved len and text of current SQL stmt.
      *
           05  ORASTXT.
               49 ORASTXTL           PIC S9(4) COMP-5.
               49 ORASTXTC           PIC X(70).
      *
      *    Saved len and text of filename containing current SQL stmt.
      *
           05  ORASFNM.
               49 ORASFNML           PIC S9(4) COMP-5.
               49 ORASFNMC           PIC X(70).
      *
      *    Saved line nr in current filename containing SQL stmt.
      *
           05  ORASLNR               PIC S9(9) COMP-5.
      *
      *    Cursor cache statistics.
      *
           05  ORAHOC                PIC S9(9) COMP-5.
           05  ORAMOC                PIC S9(9) COMP-5.
           05  ORACOC                PIC S9(9) COMP-5.
           05  ORANOR                PIC S9(9) COMP-5.
           05  ORANPR                PIC S9(9) COMP-5.
           05  ORANEX                PIC S9(9) COMP-5.
      *
      *    END ORACA
