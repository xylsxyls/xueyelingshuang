#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0600
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <mmsystem.h>
#include <propidl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>

#ifndef _REFERENCE_TIME_DEFINED
#define _REFERENCE_TIME_DEFINED
typedef LONGLONG REFERENCE_TIME;
#endif
