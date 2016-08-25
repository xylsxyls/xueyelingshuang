#pragma once
#include "Cjson/CjsonAPI.h"
#include <afxwin.h>

//默认报头有两个，Content-Type和Content-Length
class Cjson;
class CHttpString{
public:
	CString str;

public:
	void init(CString strIP,int port,CString ContentType = "application/json;charset=UTF-8");
	//添加报头
	void AddMessage(CString strHead,CString strValue);
	//1表示post发送
	void ProtocolStyle(CString strProtocol,int flag);
	//添加内容
	void SetJson(Cjson json);
	//获取内容
	Cjson GetJsonData();
	//获取类型
	CString GetContentType();
	//获取长度
	int GetContentLength();
	//获取内容，返回实际接收长度
	CString GetData();
};