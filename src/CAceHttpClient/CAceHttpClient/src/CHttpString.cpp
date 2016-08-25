#include <SDKDDKVer.h>
#include "CHttpString.h"

void CHttpString::init(CString strIP,int port,CString ContentType){
	CString strPort = "";
	strPort.Format("%d",port);

	str = " http://" + strIP + ":" + strPort + " HTTP/1.1\r\nContent-Type:" + ContentType + "\r\nContent-Length:\r\n\r\n";
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

void CHttpString::SetJson(Cjson json){
	int nLeft = str.Find("\r\n\r\n");
	str.Delete(nLeft + 4,str.GetLength() - nLeft - 4);
	CString strJson = json.toCString("","");
	str = str + strJson;
	CString strLength = "";
	strLength.Format("%d",strJson.GetLength());
	//删除\r\nContent-Length:到下一个\r\n之间的东西
	nLeft = str.Find("\r\nContent-Length:");
	nLeft = nLeft + 17;
	int nRight = str.Find("\r\n",nLeft);
	str.Delete(nLeft,nRight - nLeft);
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