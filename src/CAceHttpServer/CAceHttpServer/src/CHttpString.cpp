#include <SDKDDKVer.h>
#include "CHttpString.h"

void CHttpString::InitClient(CString strIP,int port,int DataStyle,int nCharset){
	CString strPort = "";
	strPort.Format("%d",port);

	CString strContentType = "";
	if(DataStyle == 1){
		//"application/json;charset=UTF-8"
		strContentType = "application/json;";
	}
	if(nCharset == 1){
		strContentType = strContentType + "charset=ANSI";
	}
	else if(nCharset == 2){
		strContentType = strContentType + "charset=UTF-8";
	}
	str = " http://" + strIP + ":" + strPort + " HTTP/1.1\r\nContent-Type:" + strContentType + "\r\nContent-Length:\r\n\r\n";
	return;
}

void CHttpString::InitServer(int flag,int DataStyle,int nCharset){
	CString strContentType = "";
	if(DataStyle == 1){
		//"application/json;charset=UTF-8"
		strContentType = "application/json;";
	}
	if(nCharset == 1){
		strContentType = strContentType + "charset=ANSI";
	}
	else if(nCharset == 2){
		strContentType = strContentType + "charset=UTF-8";
	}
	str = "HTTP/1.1 200 OK\r\nContent-Type:" + strContentType + "\r\nContent-Length:\r\n\r\n";
	return;
}

void CHttpString::AddMessage(CString strHead,CString strValue){
	CString strMessage = strHead + ":" + strValue;
	str.Insert(str.Find("\r\n\r\n"),strMessage);
	return;
}

void CHttpString::ProtocolStyle(CString strProtocol,int flag){
	int nRight = str.Find(" HTTP/1.1\r\n");
	int nLeft = str.Find(":");
	nLeft = str.Find(":",nLeft + 1);
	while(!(str[nLeft] >= '0' && str[nLeft] <= '9')) nLeft++;
	while(str[nLeft] >= '0' && str[nLeft] <= '9') nLeft++;
	str.Delete(nLeft,nRight - nLeft);
	if(flag == 1){
		str.Insert(nLeft,strProtocol);
		str = "POST" + str;
	}
	return;
}

CString CHttpString::GetProtocol(){
	int nRight = str.Find(" HTTP/1.1\r\n");
	int nLeft = str.Find(":");
	nLeft = str.Find(":",nLeft + 1);
	while(!(str[nLeft] >= '0' && str[nLeft] <= '9')) nLeft++;
	while(str[nLeft] >= '0' && str[nLeft] <= '9') nLeft++;
	return str.Mid(nLeft,nRight - nLeft);
}

CString CHttpString::GetStyle(){
	int nRight = str.Find(" ");
	return str.Mid(0,nRight);
}

void CHttpString::SetData(CString strData){
	int nLeft = str.Find("\r\n\r\n");
	str.Delete(nLeft + 4,str.GetLength() - nLeft - 4);
	str = str + strData;
	SetLength(strData.GetLength());
	return;
}

void CHttpString::SetLength(int length){
	//删除\r\nContent-Length:到下一个\r\n之间的东西
	int nLeft = str.Find("\r\nContent-Length:");
	nLeft = nLeft + 17;
	int nRight = str.Find("\r\n",nLeft);
	str.Delete(nLeft,nRight - nLeft);
	CString strLength = "";
	strLength.Format("%d",length);
	str.Insert(nLeft,strLength);
	return;
}

Cjson CHttpString::GetJsonData(){
	int nLeft = str.Find("\r\n\r\n");
	nLeft = nLeft + 4;
	Cjson json;
	json.LoadJson(str.Mid(nLeft,str.GetLength() - nLeft));
	return json;
}

CString CHttpString::GetContentType(){
	int nLeft = str.Find("\r\nContent-Type:");
	nLeft = nLeft + 15;
	int nRight = str.Find("\r\n",nLeft);
	return str.Mid(nLeft,nRight - nLeft);
}

int CHttpString::GetContentLength(){
	int nLeft = str.Find("\r\nContent-Length:");
	nLeft = nLeft + 17;
	int nRight = str.Find("\r\n");
	return atoi(str.Mid(nLeft,nRight - nLeft));
}

CString CHttpString::GetData(){
	int nLeft = str.Find("\r\n\r\n");
	nLeft = nLeft + 4;
	int nRight = str.Find("\r\n");
	return str.Mid(nLeft,nRight - nLeft);
}

int CHttpString::GetCharset(){
	CString strContentType = GetContentType();
	int nLeft = strContentType.Find("charset=");
	nLeft = nLeft + 8;
	CString strChatset = "";
	strChatset = strContentType.Mid(nLeft,strContentType.GetLength() - nLeft);
	strChatset.Replace(" ","");
	strChatset.Replace("\t","");
	strChatset.Replace("\r","");
	strChatset.Replace("\n","");
	if(strChatset == "ANSI") return 1;
	else if(strChatset == "UTF-8") return 2;
	else return -1;
}