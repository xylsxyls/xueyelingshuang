#pragma once

#ifdef _LogSenderAPI
#define LogSenderAPI _declspec(dllimport)
#else
#define LogSenderAPI _declspec(dllexport)
#endif