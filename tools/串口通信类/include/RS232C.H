/*
	使用示例：+++++++++++++++++++++++++++++++++++++++++++
	#include "RS232.H"
	打开端口#############################################
	if(!Rs232.SetOpenState(9600, 8, NOPARITY, ONESTOPBIT))
		return -1;
	if(!Rs232.Open(1))
		return -1;
	发送数据#############################################
	BOOL bRet = Rs232.SendString((LPBYTE)send, 5);
	if(!bRet) return -1;
	接收数据#############################################
	BOOL bRet = Rs232.GetString(recv, 2, &size, 1000);
	if(!bRet) return -1;
	关闭数据#############################################
	Rs232.Close();	
	+++++++++++++++++++++++++++++++++++++++++++++++++++++
 */



#ifndef _RS_232C_H_
#define _RS_232C_H_

#include <stdio.h>
#include <mmsystem.h>


#define BUFFMAX		1492

int WriteCOMLog(char*pKey, char* pData, UINT uLen, bool bSend, bool bHex=true);

// #####################################################################
///$$$$$$$$ 串口操作类
// #####################################################################

/*******************************************************************************

 RS232C	Open Close
 RTS	RtsOn RtsOff IsRts
 DTR	DtrOn DtrOff IsDtr

*******************************************************************************/
class CRs232c
{
public:
	CRs232c( void );
	CRs232c( char* pKey);
	~CRs232c( void );
private:
	HANDLE			m_hComm;
	COMMTIMEOUTS	m_CommTime;
	DCB				m_Dcb;
	int				m_nPort;				// Port No(1-4)
	int				m_nOpnSpeed;			// Speed
	int				m_nOpnData;				// DataBit
	int				m_nOpnParity;			// Parity
	int				m_nOpnStop;				// StopBit
	int				m_nLen;					// Text Length
	BYTE			RsSendbuff[BUFFMAX];	// Send Buffer
	BYTE			RsReceivebuff[BUFFMAX];	// Recieve Buffer
	CString			m_strKey;

public:
	// Check Port
	int EnumPorts( char* chPorts );

	// RS232C Open State
	BOOL SetOpenState( int nSpeed = 9600,
		int nData = 8, int nParity = NOPARITY, int nStop = ONESTOPBIT );

	// RS232C Open
	BOOL Open( int nPort = 1 );
	// RS232C Close
	void Close( void );

	// DCB Data
	BOOL SetDcb( void );

	// Open Check
	BOOL IsOpen( void ) { return m_hComm != NULL; }

	// Get Open Port No
	int OpeningPort( void );

//////// RTS Control ////////
	// RTS ON
	BOOL RtsOn( void );
	// RTS OFF
	BOOL RtsOff( void );
	// RTS Status
	BOOL IsRts( void );

//////// DTR Control ////////
	// DTR ON
	BOOL DtrOn( void );
	// DTR OFF
	BOOL DtrOff( void );
	// DTR Status
	BOOL IsDtr( void );

//////// Send/Recieve ////////
	// Send(char)
	BOOL Sendchar( BYTE buff );

	// Send(text)
	BOOL SendString( LPBYTE buff, int nLen );

	// file(RS232C)write
	BOOL WriteGeneric( LPBYTE buff, int nLen );

	// Receive(text)  daguoc注：不阻塞，无数据会立刻返回
	BOOL GetString( LPBYTE buff, int ReceiveLen, int* nSize );
	// Receive(text)  daguoc ADD 超时返回
	BOOL GetString( LPBYTE buff, int ReceiveLen, int* nSize, UINT nTimeOut );
	// 
	BOOL IsString( void );
	// 清除发送接收缓冲区数据
	BOOL ClearSR();

//////// Change CTS and DSR ////////
	BOOL SetCts( BOOL bValue );
	BOOL SetDsr( BOOL bValue );

	// Read Send Text length
	int GetSendLen( void ){ return m_nLen; }
	// Write Send Text Length
	void SetSendLen( int n = 0 ){ m_nLen = n; }
	
	// Get Send Buffer
	LPBYTE GetSendBuff( void ){ return RsSendbuff; }
	// Get Receive Buffer
	LPBYTE GetReceiveBuff( void ){ return RsReceivebuff; }

//////// Timer ////////
	// Timeup
	void SetTimOver( BOOL Flag = FALSE ){ m_nTimeOver = Flag; }
	// Get Timer status
	BOOL GetTimOver( void ){ return m_nTimeOver; }

	// Start Timer
	BOOL TimerSet( UINT mTime );

	// Stop Timer
	void TimerReset( void );

//////// Timeup Event ////////
	static void CALLBACK OnTimer_Over( UINT nID, UINT nMsg, DWORD dwUser, DWORD dw1, DWORD dw2 );

protected:
	// Timer ID
	UINT m_nTimerID;				// Timer ID
	TIMECAPS m_timeCaps;			// Timer Inf.
	BOOL m_nTimeOver;				// Timeup Flag
};

extern CRs232c Rs232;

// #####################################################################
///$$$$$$$$ 并口操作类
// #####################################################################
class CParallel
{
public:
	CParallel( void );
	CParallel( char* pKey );
	~CParallel( void );

	BOOL Open(int nPort = 1);
	void Close( void );
	BOOL IsOpen( void ) { return m_hLPT != INVALID_HANDLE_VALUE; }
	// Send(text)
	BOOL SendString( LPBYTE buff, int nLen );
	// Receive(text)  daguoc ADD 超时返回
	BOOL GetString( LPBYTE buff, int ReceiveLen, int* nSize, UINT nTimeOut );
	
	// Timeup
	void SetTimOver( BOOL Flag = FALSE ){ m_nTimeOver = Flag; }
	// Get Timer status
	BOOL GetTimOver( void ){ return m_nTimeOver; }
	// Start Timer
	BOOL TimerSet( UINT mTime );
	// Stop Timer
	void TimerReset( void );
	//////// Timeup Event ////////
	static void CALLBACK OnTimer_Over( UINT nID, UINT nMsg, DWORD dwUser, DWORD dw1, DWORD dw2 );
	
private:
	HANDLE			m_hLPT;
	// Timer ID
	UINT m_nTimerID;				// Timer ID
	TIMECAPS m_timeCaps;			// Timer Inf.
	BOOL m_nTimeOver;				// Timeup Flag
	CString m_strKey;
};



//extern CParallel Parallel;



#endif // _RS_232C_H_

