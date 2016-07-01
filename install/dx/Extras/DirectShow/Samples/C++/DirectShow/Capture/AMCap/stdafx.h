// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__08E7CDC6_7F58_11D2_8CF1_00A0C9441E20__INCLUDED_)
#define AFX_STDAFX_H__08E7CDC6_7F58_11D2_8CF1_00A0C9441E20__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif

#define _ATL_APARTMENT_THREADED
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <atlbase.h>
extern CComModule _Module;

// C4995'function': name was marked as #pragma deprecated
//
// The version of atlcom.h which shipped with Visual Studio .NET 
// 2003 uses some deprecated functions.  Warning C4995 is disabled 
// because the file cannot be changed and we do not want to display 
// warnings which the user cannot fix.
#pragma warning( push )
#pragma warning( disable : 4995 )

#include <atlcom.h>

#pragma warning( pop )

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__08E7CDC6_7F58_11D2_8CF1_00A0C9441E20__INCLUDED)
