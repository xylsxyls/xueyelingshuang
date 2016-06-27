/*
 * $Header: sqlkpr.h 25-aug-2005.21:32:24 subanerj Exp $ 
 */

/* Copyright (c) 1994, 2005, Oracle. All rights reserved.  */
/*
   NAME
     sqlkpr.h - SQLlib K&R PRototypes
   DESCRIPTION
     Defines k&r prototypes for externalized SQLLIB functions used in 
     conjunction with OCI
   PUBLIC FUNCTION(S)
     sqllda: Get Logon Data Area for connection
     sqlld2: Logon Data area form 2 -- with host name
     sqlcda: Cursor data area from result set cursor
     sqlcur: PCC cursor from cursor data area

   NOTES

   MODIFIED   (MM/DD/YY)
    subanerj   08/25/05 - ANSI prototypes; miscellaneous cleanup
    apopat     05/08/02 - [2362423] MVS PE to make lines shorter than 79
    apopat     09/20/00 - [1356099] Clarification on use of sqllda
    lvbcheng   01/16/97 - Fix for b2
    jbasu      02/08/95 - Add prototypes for new thread entry pts
    jbasu      09/21/94 - Update sqlcda,sqlcur; move sqlrcn,sqlfcn to sqlefn.h
    jbasu      09/13/94 - Move XA prototypes to sqlefn.h
    jbasu      08/16/94 - use OCI cda_def type from ocidfn.h instead of sqlold
    jbasu      08/16/94 - Creation
*/

#ifndef SQLKPR
#  define SQLKPR

#ifndef OCIDFN
#  include <ocidfn.h>
#endif

#define SQLLDAGetCurrent(arg1, arg2) sqlldat(arg1, arg2)

void sqllda(Lda_Def *lda);
   /* LDA version 1.   Creates an lda for use in OCI programs.
   **   Assumes that we are logged on through an embedded SQL statement.
   **   The lda is filled in using the connect information from the most 
   **   recently executed embedded SQL statement.  So the ONLY way to use
   **   sqllda() is to call sqllda() immediately after the 
   **   EXEC SQL CONNECT... statement.
   */


void sqlldat(dvoid *sqluga, Lda_Def *lda);
  /* Thread-safe version of sqllda.  Takes an extra parameter sqluga, which is 
   * the SQLLIB runtime context. 
   */

/* **************************************************************************/
/* ** SQLLD2 ** "sqlld2" is an extended version of                          */
/*              the sqllda function used to pass a connection to OCI.       */
/*              "sqlld2" can be used at any time to set up an "lda" for use */
/*              by OCI.  It does not need to called immediately after a SQL */
/*              statement using the particular connection (typically the    */
/*              CONNECT itself).                                            */
/****************************************************************************/

#define SQLDAGetNamed(arg1, arg2, arg3, arg4) sqlld2t(arg1, arg2, arg3, arg4) 

void sqlld2(Lda_Def *lda, text *hname, sb4 *hnamel);
   /* LDa version 2.  Creates an lda for use in OCI programs.  The difference
   **      between sqlld2 and sqllda is that sqlld2 allows specification of
   **      the database host name to determine the connection to use in
   **      establishing the lda. 
   **  hname - buffer containing the "logical" name for this connection.  This
   **            would be the same identifier used in an AT clause in an
   **            embedded SQL statement.  A null ptr indicates the "default"
   **            database used when there is no AT clause in a SQL statement.
   **  hnamel - length of buffer containing hname.
   */

void sqlld2t(dvoid *sqluga, Lda_Def *lda, text *hname, sb4 *hnamel);
   /* Thread-safe version of sqlld2. Takes an extra parameter sqluga, which is 
    * the SQLLIB runtime context. 
    */ 

#define SQLCDAFromResultSetCursor(arg1, arg2, arg3, arg4) \
        sqlcdat(arg1, arg2, arg3, arg4)

void sqlcda(/*_ Cda_Def *cda, dvoid *cur, sword *retval _*/);
/* 
 * Name: sqlcda() - SQLlib result set cursor to a Cursor Data Area in OCI
 * Description: Translates a result set cursor in SQLLIB to a cursor data area
 *              in OCI.  Both of these structs should have been previously 
 *              created/allocated.
 * Input   : cur    -pointer to a result set cursor struct allocated thru Pro*
 * Output  : cda    -pointer to destination cursor data area in OCI
 *           retval -Return value: 0 if no error,SQLLIB error number otherwise
 *
 * Notes: 1.In case of an error, the v2_rc and rc fields of the cda are
 *          populated with the negative and positive error codes respectively.
 *        2.The 'rows processed count' in the cda structure is NOT populated
 *          in this routine.  This field is set to the correct value only
 *          AFTER a fetch is done in OCI using the cda. Same
 *          comment applies to other cda fields like the function type.
 * ===========================================================================
 */
void sqlcdat(/*_ dvoid *sqluga, Cda_Def *cda, dvoid *cur, sword *retval _*/);
 /* Thread-safe version of sqlcda().  Takes an extra parameter sqluga, which 
  * is the SQLLIB runtime context. 
  */ 

#define SQLCDAToResultSetCursor(SQL_SINGLE_RCTX, arg1, arg2, arg3) \
        sqlcurt(arg1, arg2, arg3, arg4) 

void sqlcur(/*_ dvoid *cur, Cda_Def *cda, sword *retval _*/);
/*
 * Name: sqlcur() - SQLlib result set CURsor from an oci cursor data area
 * Description: Translates a cursor data area in OCI to a result set cursor
 *              in SQLLIB.  Both of these structs should have been previously
 *              created/allocated.
 * Input   : cda    - pointer to a cursor data area in OCI
 * Output  : cur    - pointer to a result set cursor struct (previously 
 *                    allocated thru PCC)
 *           retval - Return value: 0 if no error, error code otherwise.
 * Notes   :The sqlca structure for PCC is NOT updated by this routine.  This 
 *          structure gets populated (with error code, rows fetched count etc)
 *          only AFTER a database operation is performed through PCC using the
 *          translated cursor.
 *============================================================================
 */
void sqlcurt(/*_ dvoid *sqluga, dvoid *cur, Cda_Def *cda, sword *retval _*/);
 /* Thread-safe version of sqlcur().  Takes an extra parameter sqluga, which 
  * is the SQLLIB runtime context. 
  */ 

#endif /* SQLKPR */
