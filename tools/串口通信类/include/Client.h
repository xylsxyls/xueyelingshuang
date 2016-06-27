#pragma once

#include "TCPServer.h"
#include "BasePack.h"

#define SOCKET_TIMEOUT		120000
#define WAITMSG_TIMEOUT		150000

#define MAX_PACK_SIZE	5120

#define SERVER_SOCKET_MSG		(WM_USER + 302)

#include <vector>

class CTCPServer;
class CClient
{
public:
	CClient(void);
	~CClient(void);

protected:
	int			m_nSocket;
	HANDLE		m_hSocketRecvEvent;
	HANDLE		m_hSocketSendEvent;
	HANDLE		m_hSocketThread;
	CTCPServer*	m_pParent;
	BOOL		m_bExit;
	std::vector<LPBASE_MSG_PACK>	m_apSendMsg;
	CRITICAL_SECTION	m_SendSection;
	HWND		m_ctrlParent;

public:
	BOOL		AttachSocket(int nSocket, HWND ctrlParent);
	DWORD		ExecuteSocketContent();
	void		SetParent(CTCPServer* pParent){m_pParent = pParent;}
	void		CloseClientSocket();
	BOOL		AddSendMsg(LPBASE_MSG_PACK pPack);
	int			GetSocket(){return m_nSocket;}

protected:
	static	DWORD	WINAPI	ExecuteSocketMsg(LPVOID	pParam);
	BOOL		ExecuteClientMsg(LPBASE_MSG_PACK pPack);
	void		ExitThread();
	void		ClearAllParam();
};

