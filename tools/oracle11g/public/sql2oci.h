
/*
 * $Header: sql2oci.h 08-may-2002.12:13:40 apopat Exp $
 */

/* Copyright (c) 1996, 2002, Oracle Corporation.  All rights reserved.  */
 
/* NOTE:  See 'header_template.doc' in the 'doc' dve under the 'forms' 
      directory for the header file template that includes instructions. 
*/
 
/* 
   NAME 
     sql2oci.h - SQLLIB functions for V8 OCI interoperability

   DESCRIPTION 
     This file contains SQLLIB functions to obtain OCI environment handle
     and OCI service context in a Pro*c program.

   RELATED DOCUMENTS 
 
   INSPECTION STATUS 
     Inspection date: 
     Inspection status: 
     Estimated increasing cost defects per page: 
     Rule sets: 
 
   ACCEPTANCE REVIEW STATUS 
     Review date: 
     Review status: 
     Reviewers: 
 
   PUBLIC FUNCTION(S) 
     SQLEnvGet()
     SQLSvcCtxGet()

   PRIVATE FUNCTION(S)
     NONE

   EXAMPLES

   NOTES
     <other useful comments, qualifications, etc.>

   MODIFIED   (MM/DD/YY)
   apopat      05/08/02 - [2362423] MVS PE to make lines shorter than 79
   apopat      07/31/99 - [707588] TAB to blanks for OCCS
   apopat      04/06/99 - [563753] Allowing prototypes for C++
   lvbcheng    06/23/98 - Add SQLRowidGet
   sylin       02/25/98 - [600216] Add C++ support
   lvbcheng    05/13/97 - Desupport wrappers
   lvbcheng    01/13/97 - fix non-ANSI SQLNumberFromFloat error
   lvbcheng    01/03/97 - test
   lvbcheng    01/03/97 - Revert back to OCINumber *
   lvbcheng    12/12/96 - Finalize
   lvbcheng    12/06/96 - test
   lvbcheng    12/06/96 - test
   lvbcheng    12/05/96 - Add default context code
   lvbcheng    12/05/96 - Add connection to sqlcpr.h and sql[a|k]pr.h
   lvbcheng    12/05/96 - Remove merge conflicts
   lvbcheng    12/02/96 - Change Interface of sql2oci wrappers
   lvbcheng    10/21/96 - Add OCINumber set procedures
   lvbcheng    10/21/96 - Add vstr wrappers
   jbasu       08/02/96 - Remove private include files
   jbasu       07/05/96 - [8.0] SQLSvcCtxGet() enhancements
   jbasu       06/04/96 - Support for V80 objects
   jbasu       06/04/96 - Creation

*/

#ifndef SQL2OCI_ORACLE
#  define SQL2OCI_ORACLE

#ifndef OCI_ORACLE
#  include <oci.h>
#endif /* OCI_ORACLE */

#ifndef ORL_ORACLE
#  include <orl.h>
#endif /* ORL_ORACLE */

#ifndef SQLCPR_ORACLE
#  include <sqlcpr.h>
#endif /* SQLCPR_ORACLE */

#if defined (__STDC__) || defined (__cplusplus)

/* --------------------- */
/*  Includes for ANSI C  */
/* --------------------- */
#ifdef  __cplusplus
extern "C" {
#endif

#ifndef SQLAPR_ORACLE
#  include <sqlapr.h>
#endif /* SQLAPR_ORACLE */

/* Get Procedures for OCI Env and Svc Ctx */

extern sword SQLEnvGet(dvoid *rctx, OCIEnv **oeh);

extern sword SQLSvcCtxGet(dvoid *rctx, text *dbname, sb4 dbnamelen, 
                          OCISvcCtx **svc);

#ifdef  __cplusplus
}
#endif

#else   /* k&r style - non-ANSI code */

/* -------------------- */
/*  Includes for K&R C  */
/* -------------------- */

#ifndef SQLKPR_ORACLE
#  include <sqlkpr.h>
#endif /* SQLKPR_ORACLE */

/* ------------------------------------ */
/* Get rowid of last exec sql statement */
/* ------------------------------------ */

void SQLRowidGet(/*_ dvoid *rctx, OCIRowid **urid _*/);

/* 
   NAME: SQLRowidGet - SQLLIB Rowid Get

   PARAMETERS:
   rctx (IN): pointer to a sqllib runtime context
   urid (OUT): pointer to an allocated rowid descriptor

   DESCRIPTION:
   Sets urid to be the rowid of the last row inserted,
   updated or selected

   */







/* ------------------------------------------- */
/*  Get 8.0 OCI environment handle from SQLLIB */
/* ------------------------------------------- */

sword SQLEnvGet(/*_ dvoid *rctx, OCIEnv **oeh _*/);
/*
   NAME: SQLEnvGet - SQLLIB Get Environment from the Runtime Context

   PARAMETERS:
   rctx (IN): pointer to a sqllib runtime context
   oeh (IN/OUT): OCIEnv ptr
   
   DESCRIPTION:
   Sets oeh to the OCIEnv corresponding to the runtime context.

   RETURNS:
   status - SQL_SUCCESS on success, SQL_ERROR on failure.

   */

/* ---------------------------------------------- */

/* Get 8.0 OCI Service Context for a database connection in Pro*C */

/* non-threaded, no Pro*C runtime context */
sword SQLSvcCtxGet(/*_ dvoid *rctx, text *dbname, sb4 dbnamelen,
                     OCISvcCtx **svc _*/);
/*
   NAME: SQLSvcCtxGet - SQLLIB Service Context Get

   PARAMETERS:
   rctx (IN): pointer to a sqllib runtime context
   dbname (IN): buffer containing the "logical" name for this connection.
   dbnamelen (IN): length of the dbname buffer
   svc (IN/OUT): address of an OCISvcCtx ptr
   
   DESCRIPTION:
   Sets svc to the OCI Service Context corresponding to the runtime context.

   RETURNS:
   status - SQL_SUCCESS on success, SQL_ERROR on failure.

   NOTES:
   'dbname' is the buffer containing the "logical" name for this connection.
   This  would be the same identifier used in an AT clause in an embedded
   SQL statement.  
   
   'dbnamelen' is the length of buffer containing dbname.
   
   If dbname is a NULL ptr or dbnamelen is 0, then the "default" database
   connection is assumed (as in a SQL statement with no AT clause).
   
   A value of -1 for dbnamelen can be used to indicate that dbname 
   is a NULL-terminated string.
   
   */

#endif /* k&r style - non-ANSI code */

#endif /* SQL2OCI_ORACLE */
