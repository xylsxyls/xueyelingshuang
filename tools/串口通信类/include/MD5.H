// Md5.h : main header file for the MD5 DLL
//

#if !defined(AFX_MD5_H__1A8C58C0_BD3D_45AF_933B_2E5A103F1DC2__INCLUDED_)
#define AFX_MD5_H__1A8C58C0_BD3D_45AF_933B_2E5A103F1DC2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern "C" void MD5String(unsigned char *pSource,unsigned int iLen, char* pMd5);
extern "C" bool MD5File(char *pFile, char* pMd5);

#endif // !defined(AFX_MD5_H__1A8C58C0_BD3D_45AF_933B_2E5A103F1DC2__INCLUDED_)
