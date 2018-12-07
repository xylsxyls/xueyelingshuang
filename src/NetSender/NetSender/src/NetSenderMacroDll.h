#pragma once

#ifdef _NetSenderAPI
#define NetSenderAPI _declspec(dllimport)
#else
#define NetSenderAPI _declspec(dllexport)
#endif