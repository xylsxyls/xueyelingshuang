#pragma once
#include <afxwin.h>
#include "CAceTCPServerMacro.h"

class CAceTCPServerAPI CLocalIPPort{
public:
	CString IP;
	int port;

public:
	CLocalIPPort();
};