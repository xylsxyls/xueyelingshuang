#pragma once
#define _CCheckUSBAPI
#include "CCheckUSB.h"

#ifdef NDEBUG
#pragma comment(lib,"CCheckUSB.lib")
#else
#pragma comment(lib,"CCheckUSBd.lib")
#endif