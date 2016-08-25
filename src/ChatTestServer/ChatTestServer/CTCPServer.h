#pragma once
#include "CAceTCPServer/CAceTCPServerAPI.h"

class CMysql;
class CTable;
class Cjson;
class CTCPServer : public CAceTCPServer{
public:
	CMysql* pMysql;
	CTable* pTable;
	map<CString,ACE_SOCK_Stream*> mapPeer;
	CTCPServer();
	~CTCPServer();
	virtual void receive(char* pData,int length,ACE_SOCK_Stream* ppeer);
	virtual Cjson ReceiveReqJson(Cjson jsonReq,int MsgID,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	virtual void ReceiveRspJson(Cjson jsonRsp,int MsgID,Cjson jsonCheckPackage);

	Cjson MessageLoginReq(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	Cjson MessageRegisterReq(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	Cjson MessageSendTextToAll(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	Cjson MessageUserToAll(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	Cjson MessageRefreshList(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	Cjson MessageDeleteList(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	Cjson MessageChat(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer);
	
};