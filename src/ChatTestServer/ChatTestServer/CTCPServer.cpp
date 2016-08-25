#include <SDKDDKVer.h>
#include "CTCPServer.h"
#include "Cjson/CjsonAPI.h"
#include "CMysqlAPI/CMysqlAPI.h"

CTCPServer::CTCPServer(){
	pMysql = new CMysql("127.0.0.1","root","root","chattest",3306);
	//打开数据库，获取数据库指针
	pTable = pMysql->OpenTableInterface("login");
}

CTCPServer::~CTCPServer(){
	delete pMysql;
}

void CTCPServer::receive(char* pData,int length,ACE_SOCK_Stream* ppeer){
	/*
	Cjson jsonReq;
	jsonReq.LoadJson((CString)pData);
	Cjson jsonRsp;
	
	//登录
	if(jsonReq["ID"].toValue().nValue == 10000){
		//登录返回消息，消息号10002
		jsonRsp["ID"] = 10002;
		MessageLogin(jsonReq,jsonRsp);
	}
	//注册
	else if(jsonReq["ID"].toValue().nValue == 10001){
		//登录返回消息，消息号10003
		jsonRsp["ID"] = 10003;
		MessageRegister(jsonReq,jsonRsp);
	}

	CString strMessageRes = "";
	strMessageRes = jsonRsp.toCString("","");
	send((LPSTR)(LPCTSTR)strMessageRes,strMessageRes.GetLength() + 1,ppeer);
	*/
	return;
}

Cjson CTCPServer::ReceiveReqJson(Cjson jsonReq,int MsgID,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	//登录
	if(MsgID == 10000) return MessageLoginReq(jsonReq,pvecSendIPPeer);
	//注册
	else if(MsgID == 10001) return MessageRegisterReq(jsonReq,pvecSendIPPeer);
	else if(MsgID == 10002) return MessageSendTextToAll(jsonReq,pvecSendIPPeer);
	else if(MsgID == 10003) return MessageUserToAll(jsonReq,pvecSendIPPeer);
	else if(MsgID == 10004) return MessageRefreshList(jsonReq,pvecSendIPPeer);
	else if(MsgID == 10005) return MessageDeleteList(jsonReq,pvecSendIPPeer);
	else if(MsgID == 10006) return MessageChat(jsonReq,pvecSendIPPeer);
	else return Cjson();
}

void CTCPServer::ReceiveRspJson(Cjson jsonRsp,int MsgID,Cjson jsonCheckPackage){
	return;
}

Cjson CTCPServer::MessageLoginReq(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	//单个回复
	OnlyRspToReq(pvecSendIPPeer);

	CString strUser = jsonReq["User"].toValue().strValue;
	CString strPassWord = jsonReq["PassWord"].toValue().strValue;

	//把User和通路保存在map里
	mapPeer[strUser] = pvecSendIPPeer->at(0);

	//查看mysql里是否有该用户名，以及密码是否正确
	CRecord select = CField("User") + CField("PassWord");
	CRecord condition = CFieldValue(CField("User"),"=",strUser);
	vector<CRecord> record = pTable->SelectRecord(&condition,0,99999,&select);

	Cjson jsonRsp;
	if(record.size() == 0){
		jsonRsp["strRsp"] = "用户名不存在";
	}
	//用户名正确
	else if(record.size() == 1){
		//密码正确
		CString strFieldName = record.at(0).vecFieldValue.at(1).Field.FieldName;
		CString strValue = record.at(0).vecFieldValue.at(1).strValue;
		if(strFieldName == "PassWord" && strValue == "'" + strPassWord + "'"){
			jsonRsp["strRsp"] = "登录成功";
		}
		else jsonRsp["strRsp"] = "密码错误";
	}
	else jsonRsp["strRsp"] = "有多条相同用户名服务器出错";
	return jsonRsp;
}

Cjson CTCPServer::MessageRegisterReq(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	//单个回复
	OnlyRspToReq(pvecSendIPPeer);

	CString strNewUser = jsonReq["NewUser"].toValue().strValue;
	CString strNewPassWord = jsonReq["NewPassWord"].toValue().strValue;
	//向数据库插入数据
	CRecord record = CFieldValue(CField("User"),strNewUser) + CFieldValue(CField("PassWord"),strNewPassWord)
					 + CFieldValue(CField("NickName"),strNewUser) + CFieldValue(CField("IsOnline"),"0");
	BOOL nResult = pTable->AddRecord(&record);
	Cjson jsonRsp;
	//用户名重复
	if(nResult == 0) jsonRsp["strRsp"] = "用户名已被使用";
	//注册成功
	else jsonRsp["strRsp"] = "注册成功";
	return jsonRsp;
}

Cjson CTCPServer::MessageSendTextToAll(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	CString strUser = jsonReq["User"].toValue().strValue;
	CRecord FieldRecord = CField("NickName");
	CRecord ConditionRecord = CFieldValue(CField("User"),"=",strUser);
	vector<CRecord> vecRecord = pTable->SelectRecord(&ConditionRecord,0,0,&FieldRecord);
	if(vecRecord.size() != 1) AfxMessageBox("数据库有多个相同User");
	Cjson jsonRsp;
	CString strNickName = vecRecord.at(0).vecFieldValue.at(0).strValue;
	strNickName = strNickName.Mid(1,strNickName.GetLength() - 2);
	jsonRsp["NickName"] = strNickName;
	jsonRsp["text"] = jsonReq["text"].toValue().strValue;
	return jsonRsp;
}

Cjson CTCPServer::MessageUserToAll(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	CString strUser = jsonReq["User"].toValue().strValue;
	CRecord SetRecord = CFieldValue(CField("IsOnline"),"=","1");
	CRecord Condition = CFieldValue(CField("User"),"=",strUser);

	pTable->ReviseRecord(&SetRecord,&Condition);
	CRecord FieldRecord = CField("NickName");
	CRecord ConditionRecord = CFieldValue(CField("User"),"=",strUser);
	vector<CRecord> vecRecord = pTable->SelectRecord(&ConditionRecord,0,0,&FieldRecord);
	if(vecRecord.size() != 1) AfxMessageBox("数据库中有两个同名");
	CString NickName = vecRecord.at(0).vecFieldValue.at(0).strValue;
	NickName = NickName.Mid(1,NickName.GetLength() - 2);
	Cjson jsonRsp;
	jsonRsp["User"] = strUser;
	jsonRsp["NickName"] = NickName;
	return jsonRsp;
}

Cjson CTCPServer::MessageRefreshList(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	OnlyRspToReq(pvecSendIPPeer);

	CRecord FieldRecord = CField("User") + CField("NickName");
	CRecord ConditionRecord = CFieldValue(CField("IsOnline"),"=","1") + "and" + CFieldValue(CField("User"),"!=",jsonReq["User"].toValue().strValue);
	vector<CRecord> vecRecord = pTable->SelectRecord(&ConditionRecord,0,0,&FieldRecord);
	Cjson jsonRsp;
	jsonRsp["NameCount"] = (int)vecRecord.size();
	int i = -1;
	while(i++ != vecRecord.size() - 1){
		CString strName = "";
		CString strUser = vecRecord.at(i).vecFieldValue.at(0).strValue;
		strUser = strUser.Mid(1,strUser.GetLength() - 2);
		strName.Format("User%d",i);
		jsonRsp[strName] = strUser;
		CString strNickName = vecRecord.at(i).vecFieldValue.at(1).strValue;
		strNickName = strNickName.Mid(1,strNickName.GetLength() - 2);
		strName.Format("NickName%d",i);
		jsonRsp[strName] = strNickName;
	}
	return jsonRsp;
}

Cjson CTCPServer::MessageDeleteList(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	CString strUser = jsonReq["User"].toValue().strValue;
	CRecord SetRecord = CFieldValue(CField("IsOnline"),"=","0");
	CRecord Condition = CFieldValue(CField("User"),"=",strUser);
	pTable->ReviseRecord(&SetRecord,&Condition);

	mapPeer.erase(strUser);

	Cjson jsonRsp;
	jsonRsp["User"] = strUser;
	return jsonRsp;
}

Cjson CTCPServer::MessageChat(Cjson jsonReq,vector<ACE_SOCK_Stream*>* pvecSendIPPeer){
	OnlyRspToReq(pvecSendIPPeer);

	CString strUser = jsonReq["User"].toValue().strValue;
	CString strChatUser = jsonReq["ChatUser"].toValue().strValue;
	CString strText = jsonReq["text"].toValue().strValue;

	//让对方开一个窗口，并显示信息，10007
	Cjson jsonOpenShowReq;
	jsonOpenShowReq["User"] = strChatUser;
	jsonOpenShowReq["ChatUser"] = strUser;
	jsonOpenShowReq["text"] = strText;
	CRecord FieldRecord = CField("NickName");
	CRecord ConditionRecord = CFieldValue(CField("User"),"=",strUser);
	vector<CRecord> vecRecord = pTable->SelectRecord(&ConditionRecord,0,0,&FieldRecord);
	CString strNickName = vecRecord.at(0).vecFieldValue.at(0).strValue;
	strNickName = strNickName.Mid(1,strNickName.GetLength() - 2);
	jsonOpenShowReq["NickName"] = strNickName;
	ACE_SOCK_Stream* ppeer = mapPeer[strChatUser];
	//可能出现对面下线了但是这边的单聊窗口没有关的情况
	if(ppeer == 0) return Cjson();
	SendReqJson(jsonOpenShowReq,10007,mapPeer[strChatUser]);

	Cjson jsonRsp;
	jsonRsp["text"] = strText;
	jsonRsp["NickName"] = strNickName;
	return jsonRsp;
}