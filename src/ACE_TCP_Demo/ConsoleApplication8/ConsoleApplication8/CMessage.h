#pragma once
#include <afxwin.h>

class CMessage{
public:
	char* pBuf;
	int length;

public:
	CMessage(int length);
	CMessage(const CMessage& message);
	~CMessage();

public:
	CMessage operator = (CString str);

};