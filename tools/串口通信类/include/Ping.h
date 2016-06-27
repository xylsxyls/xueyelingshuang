// Ping.h: interface for the CPing class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PING_H__025E9743_4CF6_4E4B_8F55_06E80B80E3E4__INCLUDED_)
#define AFX_PING_H__025E9743_4CF6_4E4B_8F55_06E80B80E3E4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>


#define ICMP_ECHOREPLY	0
#define ICMP_ECHOREQ	8

// IP Header -- RFC 791
typedef struct tagIPHDR
{
	u_char  VIHL;			// Version and IHL
	u_char	TOS;			// Type Of Service
	short	TotLen;			// Total Length
	short	ID;				// Identification
	short	FlagOff;		// Flags and Fragment Offset
	u_char	TTL;			// Time To Live
	u_char	Protocol;		// Protocol
	u_short	Checksum;		// Checksum
	struct	in_addr iaSrc;	// Internet Address - Source
	struct	in_addr iaDst;	// Internet Address - Destination
}IPHDR, *PIPHDR;


// ICMP Header - RFC 792
typedef struct tagICMPHDR
{
	u_char	Type;			// Type
	u_char	Code;			// Code
	u_short	Checksum;		// Checksum
	u_short	ID;				// Identification
	u_short	Seq;			// Sequence
	char	Data;			// Data
}ICMPHDR, *PICMPHDR;


#define REQ_DATASIZE 32		// Echo Request Data size

// ICMP Echo Request
typedef struct tagECHOREQUEST
{
	ICMPHDR icmpHdr;
	DWORD	dwTime;
	char	cData[REQ_DATASIZE];
}ECHOREQUEST, *PECHOREQUEST;


// ICMP Echo Reply
typedef struct tagECHOREPLY
{
	IPHDR	ipHdr;
	ECHOREQUEST	echoRequest;
	char    cFiller[256];
}ECHOREPLY, *PECHOREPLY;

class CPing  
{
public:
	CPing();
	virtual ~CPing();

public:
	
public:
	int			Ping(LPCTSTR lpszIP, LPCTSTR lpszName = "");// 0 ≥…π¶ ∑Ò‘Ú  ß∞‹
	int			telnet(LPCTSTR lpszIP, UINT uPort, UINT uTimeOut=3000);
	
protected:
	
protected:
	int			SendEchoRequest(SOCKET s, LPSOCKADDR_IN lpstToAddr);
	DWORD		RecvEchoReply(SOCKET s, LPSOCKADDR_IN lpsaFrom, u_char *pTTL);
	int			WaitForEchoReply(SOCKET s);
	u_short		in_cksum(u_short *addr, int len);
		
public:
	
};


#endif // !defined(AFX_PING_H__025E9743_4CF6_4E4B_8F55_06E80B80E3E4__INCLUDED_)
