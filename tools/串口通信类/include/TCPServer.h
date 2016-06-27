#pragma once

#ifndef _VIEWER_TCPSERVER_H_
#define _VIEWER_TCPSERVER_H_

#include "Client.h"
#include "BasePack.h"

#include <vector>

class CClient;

class CTCPServer
{
public:
	CTCPServer();
	~CTCPServer(void);

public:


protected:
	static DWORD WINAPI	TCPServerThreadProc(LPVOID pParam);
	DWORD WINAPI TCPServerThreadContent(LPVOID pParam);

	static DWORD WINAPI	DealRequestThreadProc(LPVOID pParam);
	DWORD WINAPI DealRequestThreadContent(LPVOID pParam);

protected:
	bool m_bExit;
	CString m_strLastErr;
	std::vector<CClient*> m_apClient;

	HANDLE m_hTCPServer;
	HANDLE	m_hTCPServerEvent;

	HANDLE m_hDealRequest;
	HANDLE	m_hDealRequestEvent;
private:
	/************************************************************************/
	/*                               socket                                 */
	/************************************************************************/ 
private:
	CRITICAL_SECTION m_AcceptSection;
	u_short m_nServPort;
	SOCKET m_nServSocket;
	HANDLE m_hAcceptEvent;
	HANDLE	m_hListenThread;
	HWND	m_hWnd;

private:
	static DWORD WINAPI ListenThreadProc(LPVOID pParam);
	virtual DWORD		ListenThreadContent();

public:
	bool InitTCPServer(HWND hWnd,u_short uServerPort);
	void ClientExit(CClient* pClient);
	BOOL AddSendMsg(LPBASE_MSG pMsg, int nSocketSendTo);
	CString GetLastError() {return m_strLastErr;}

};



#endif