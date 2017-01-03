#include "Cini.h"
#include <windows.h>

Cini::Cini(string iniPath){
	this->iniPath = iniPath;
}

string Cini::ReadIni(string key,int MaxLength,string strSection){
	char *szText = new char[MaxLength];
	GetPrivateProfileString(strSection.c_str(),key.c_str(),"-1",szText,MaxLength,iniPath.c_str());
	string strText = szText;
	delete[] szText;
	return strText;
}

void Cini::WriteIni(string key,string text,string strSection){
	WritePrivateProfileString(strSection.c_str(),key.c_str(),text.c_str(),iniPath.c_str());
	return;
}