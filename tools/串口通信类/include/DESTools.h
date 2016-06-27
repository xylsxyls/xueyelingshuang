// SimCardIssueDevice.h: interface for the SimCardIssueDevice class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DESTOOLS_H__A202BFB1_5FCA_496F_9946_3D04E2056B88__INCLUDED_)
#define AFX_DESTOOLS_H__A202BFB1_5FCA_496F_9946_3D04E2056B88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include <string>
using namespace std;



#define		EDDL_DES_NOTHISKEY		1

class CDESTools  
{
public:
	CDESTools();
	virtual ~CDESTools();
	
 BYTE DES_SetKey (BYTE sw1, BYTE sw2, BYTE *pkey);
 void DES_DEA (BYTE *in, BYTE *out, BYTE *key);
 void DES_UDEA (BYTE *in, BYTE *out, BYTE *key);
 void DES_DES (BYTE *in, BYTE *out /* packed 64-bit INPUT/OUTPUT blocks */);
 void DES_3DEA(char *s_in,  char *r_out, char *key);
 void DES_3UDEA(char *s_in,  char *r_out, char *key);
 void Pack8 (BYTE *ascii, BYTE *binary);
 void Unpack8 (BYTE *ascii, BYTE *binary);
 int Des_JavaString(BYTE* in, UINT iinlen, BYTE* out, BYTE* Key);
 int Des_UJavaString(BYTE* in, UINT iinlen, BYTE* out, BYTE* Key);

public:	
	//int WriteToCard(char* lpszData,char* Message);
	
private:	
	//int Comm2CardIssue(const BYTE* cmds, int cmdLen, BYTE* recvs, int* recvLen) ;
	//void CharToBcd(char * Cbuf, BYTE * Bbuf,short *blen);


private:
	typedef struct
	{
		BYTE bpKey[8];
	}key_stru;
	
	key_stru ksKeyTable[50];

#ifdef FASTER
  static unsigned short KS[16][4];  /* Key schedule of 16 48-bit subkeys
                              generated from 64-bit key */
#else
  BYTE  KS[16][48];	/* Key schedule of 16 48-bit subkeys
                              generated from 64-bit key */
#endif

protected:	
	DWORD			m_APrtocol;
	DWORD			m_dwProtocol;
};

#endif // !defined(AFX_SIMCARDISSUEDEVICE_H__A202BFB1_5FCA_496F_9946_3D04E2056B88__INCLUDED_)
