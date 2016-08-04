#pragma once
#include <afxwin.h>
#include "CCheckUSBMacro.h"

class CCheckUSBAPI CCheckUSB{
public:
	HWND h_CheckIn;
	HWND h_CheckOut;

public:
	void init(CCheckUSB *pSon);
	CCheckUSB();
	~CCheckUSB();

public:
	void CheckUSBIn();
	virtual void USBIn(CString diskpath) = 0;
	void CheckUSBOut();
	virtual void USBOut(CString diskpath) = 0;
};