#include "Com.h"
#include <windows.h>

Com::Com()
{
	CoInitialize(NULL);
}

Com::~Com()
{
	CoUninitialize();
}

