#include "Cini.h"

Cini::Cini(CString iniPath){
	this->iniPath = iniPath;
}

CString Cini::ReadIni(CString key,int MaxLength,CString strSection){
	char *szText = new char[MaxLength];
	GetPrivateProfileString(strSection,key,"-1",szText,MaxLength,iniPath);
	CString strText = szText;
	delete[] szText;
	return strText;
}

void Cini::WriteIni(CString key,CString text,CString strSection){
	WritePrivateProfileString(strSection,key,text,iniPath);
	return;
}