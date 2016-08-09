#include "CMessage.h"
#include <stdlib.h>

CMessage::CMessage(int length){
	this->length = length;
	pBuf = (char *)calloc(length,1);
}

CMessage::CMessage(const CMessage& message){
	length = message.length;
	pBuf = (char *)calloc(length,1);
	memcpy(pBuf,message.pBuf,length);
}

CMessage::~CMessage(){
	free(pBuf);
}

CMessage CMessage::operator = (CString str){
	if(str.GetLength() >= length) return *this;

	memcpy(pBuf,(LPSTR)(LPCTSTR)str,str.GetLength());
	length = str.GetLength() + 1;
	return *this;
}