#include "stdafx.h"
#include "CTCPClient.h"
#include "ChatTestDlg.h"
#include "RegDlg.h"
#include "ChatDlg.h"

void CTCPClient::receive(char *pData,int length){
	/*
	CString strMessageRes = pData;
	Cjson jsonRsp;
	jsonRsp.LoadJson(strMessageRes);

	//登录反馈消息号10002
	if(jsonRsp["ID"].toValue().nValue == 10002){
		if(jsonRsp["strRsp"].toValue().strValue == "登录成功"){
			//登录界面隐藏，显示聊天界面
			pChatTestDlg->ShowWindow(SW_HIDE);
			//打开主界面
			pChatTestDlg->OpenMainDlg();
		}
		else AfxMessageBox(jsonRsp["strRsp"].toValue().strValue);
	}
	//注册反馈消息号10003
	else if(jsonRsp["ID"].toValue().nValue == 10003){
		if(jsonRsp["strRsp"].toValue().strValue == "注册成功"){
			//销毁注册界面
			pRegDlg->EndDialog(0);
			pRegDlg = NULL;
		}
		else AfxMessageBox(jsonRsp["strRsp"].toValue().strValue);
	}*/
}

Cjson CTCPClient::ReceiveReqJson(Cjson jsonReq){
	if(MsgIDReq == 10007) return MessageOpenChatWindowAndShow(jsonReq);
	return Cjson();
}

void CTCPClient::ReceiveRspJson(Cjson jsonRsp,Cjson jsonCheckPackage){
	//登录反馈消息号10002
	if(MsgIDRsp == 10000) MessageLoginRsp(jsonRsp,jsonCheckPackage);
	//注册反馈消息号10003
	else if(MsgIDRsp == 10001) MessageRegisterRsp(jsonRsp,jsonCheckPackage);
	else if(MsgIDRsp == 10002) MessageShowText(jsonRsp,jsonCheckPackage);
	else if(MsgIDRsp == 10003) MessageAddList(jsonRsp,jsonCheckPackage);
	else if(MsgIDRsp == 10004) MessageShowFriendList(jsonRsp,jsonCheckPackage);
	else if(MsgIDRsp == 10005) MessageDeleteFriendList(jsonRsp,jsonCheckPackage);
	else if(MsgIDRsp == 10006) MessageShowOne(jsonRsp,jsonCheckPackage);
	return;
}

void CTCPClient::MessageLoginRsp(Cjson jsonRsp,Cjson jsonCheckPackage){
	Cjson *pjsonRsp = new Cjson;
	*pjsonRsp = jsonRsp;
	Cjson *pjsonCheckPackage = new Cjson;
	*pjsonCheckPackage = jsonCheckPackage;
	::PostMessage(jsonCheckPackage.GetHwnd(1),10000,(WPARAM)pjsonRsp,(LPARAM)pjsonCheckPackage);
	return;
}

void CTCPClient::MessageRegisterRsp(Cjson jsonRsp,Cjson jsonCheckPackage){
	::SendMessage(jsonCheckPackage.GetHwnd(1),10001,(WPARAM)&jsonRsp,0);
	return;
}

extern HWND MainHwnd;
//因为要向所有人发送，所以会出现别的客户端找不到的寄存包的情况
void CTCPClient::MessageShowText(Cjson jsonRsp,Cjson jsonCheckPackage){
	::SendMessage(MainHwnd,10002,(WPARAM)&jsonRsp,0);
	return;
}

void CTCPClient::MessageAddList(Cjson jsonRsp,Cjson jsonCheckPackage){
	::SendMessage(MainHwnd,10003,(WPARAM)&jsonRsp,0);
	return;
}

void CTCPClient::MessageShowFriendList(Cjson jsonRsp,Cjson jsonCheckPackage){
	::SendMessage(jsonCheckPackage.GetHwnd(1),10004,(WPARAM)&jsonRsp,0);
	return;
}

void CTCPClient::MessageDeleteFriendList(Cjson jsonRsp,Cjson jsonCheckPackage){
	::SendMessage(MainHwnd,10005,(WPARAM)&jsonRsp,0);
	return;
}

void CTCPClient::MessageShowOne(Cjson jsonRsp,Cjson jsonCheckPackage){
	::SendMessage(jsonCheckPackage.GetHwnd(1),10006,(WPARAM)&jsonRsp,0);
	return;
}

map<CString,HWND> mapChatHwnd;

Cjson CTCPClient::MessageOpenChatWindowAndShow(Cjson jsonReq){
	CString strUser = jsonReq["User"].toValue().strValue;
	CString strChatUser = jsonReq["ChatUser"].toValue().strValue;
	CString strNickName = jsonReq["NickName"].toValue().strValue;
	CString strText = jsonReq["text"].toValue().strValue;
	//必须会主线程才能创建界面
	::SendMessage(MainHwnd,10008,(WPARAM)&jsonReq,0);
	//创建窗口之后
	Cjson json;
	json["NickName"] = strNickName;
	json["text"] = strText;
	::SendMessage(mapChatHwnd[strUser],10006,(WPARAM)&json,0);
	return Cjson();
}