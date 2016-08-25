#pragma once
#include "CAceTCPClient/CAceTCPClientAPI.h"
#include <map>
using namespace std;



class CTCPClient : public CAceTCPClient{
	virtual void receive(char *pData,int length);
	virtual Cjson ReceiveReqJson(Cjson jsonReq,int MsgID);
	virtual void ReceiveRspJson(Cjson jsonRsp,int MsgID,Cjson jsonCheckPackage);

	void MessageLoginRsp(Cjson jsonRsp,Cjson jsonCheckPackage);
	void MessageRegisterRsp(Cjson jsonRsp,Cjson jsonCheckPackage);
	void MessageShowText(Cjson jsonRsp,Cjson jsonCheckPackage);
	void MessageAddList(Cjson jsonRsp,Cjson jsonCheckPackage);
	void MessageShowFriendList(Cjson jsonRsp,Cjson jsonCheckPackage);
	void MessageDeleteFriendList(Cjson jsonRsp,Cjson jsonCheckPackage);
	void MessageShowOne(Cjson jsonRsp,Cjson jsonCheckPackage);

	Cjson MessageOpenChatWindowAndShow(Cjson jsonReq);
	
	
};