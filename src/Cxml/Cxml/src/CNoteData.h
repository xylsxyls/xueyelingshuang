#pragma once
#include <afxwin.h>

class CNoteData{
public:
	int nPosition;
	CString strNoteData;

	CNoteData();
	CNoteData(int nPosition,CString strNoteData);
};