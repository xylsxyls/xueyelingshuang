#pragma once
#include "Cjson/CjsonAPI.h"
#include "CAceHttpServerMacro.h"
#include <afxwin.h>

//默认报头有两个，Content-Type和Content-Length
class Cjson;
class CHttpString{
public:
	CString str;

public:
	void InitClient(CString strIP,int port,CString ContentType = "application/json;charset=UTF-8");
	//1表示成功处理
	void InitServer(int flag,CString ContentType = "application/json;charset=UTF-8");
	//添加报头
	void AddMessage(CString strHead,CString strValue);
	//1表示post发送
	void ProtocolStyle(CString strProtocol,int flag);
	//获取URI
	CString GetProtocol();
	//获取方法名
	CString GetStyle();
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