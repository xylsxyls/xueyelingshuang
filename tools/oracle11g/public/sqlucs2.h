/*
ORACLE, Copyright (c) 1998, 2000 ORACLE Corporation
ORACLE Utilities, Copyright (c) 1981, 1982, 1983, 1986, 1990, 1991,, 1998, 2000 1995,
 1998 ORACLE Corp
 
Restricted Rights
This program is an unpublished work under the Copyright Act of the
United States and is subject to the terms and conditions stated in
your  license  agreement  with  ORACORP  including  retrictions on
use, duplication, and disclosure.
 
Certain uncopyrighted ideas and concepts are also contained herein.
These are trade secrets of ORACORP and cannot be  used  except  in
accordance with the written permission of ORACLE Corporation.
*/

#ifndef SQLUCS2_ORACLE
#define SQLUCS2_ORACLE

#ifndef ORATYPES
#  ifndef S_ORACLE
#    include <oratypes.h>
#  endif  /* S_ORACLE */
#endif  /* ORATYPES */

struct uvarchar  {ub2 len;  utext arr[1];};
typedef struct uvarchar uvarchar;

struct ulong_varchar  {ub4 len;  utext arr[1];};
typedef struct ulong_varchar ulong_varchar;

#endif  /* SQLUCS2_ORACLE */
