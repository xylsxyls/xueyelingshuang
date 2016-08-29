#pragma once
#include "Cjson/CjsonAPI.h"
#include <afxwin.h>
#include "CAceHttpServerMacro.h"

//默认报头有两个，Content-Type和Content-Length
class Cjson;
class CAceHttpServerAPI CHttpString{
public:
	CString str;

public:
	//"application/json;charset=UTF-8"
	//1表示json，1表示ANSI字符集，2表示UTF-8字符串
	void InitClient(CString strIP,int port,int DataStyle,int nCharset);
	//1表示成功处理返回200OK
	void InitServer(int flag,int DataStyle,int nCharset);
	//添加报头
	void AddMessage(CString strHead,CString strValue);
	//1表示post发送
	void ProtocolStyle(CString strProtocol,int flag);
	//获取URI
	CString GetProtocol();
	//获取方法名
	CString GetStyle();
	//添加内容
	void SetData(CString strData);
	//添加长度
	void SetLength(int length);
	//获取内容
	Cjson GetJsonData();
	//获取类型
	CString GetContentType();
	//获取长度
	int GetContentLength();
	//获取内容，返回实际接收长度
	CString GetData();
	//获取字符集
	int GetCharset();
};