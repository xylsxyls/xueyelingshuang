//------------------------------------------------------------------------------
// File: project.h
//
// Desc: DirectShow sample code - header file for TxtPlayer sample
//       Master header file that includes all the other header files used by the
//       project.  This enables compiled headers to work using build.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <windows.h>
#include <winuser.h>
#include <commdlg.h>

#include <stdarg.h>
#include <stdio.h>

#include <streams.h>
#include <atlbase.h>
#include <mmreg.h>
#include <commctrl.h>
#include <strsafe.h>


#ifndef NUMELMS
   #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#include "app.h"
#include "vcdplyer.h"
#include "resource.h"
#include "persist.h"
