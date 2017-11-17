#pragma once

#ifdef _CStopWatchAPI
#define CStopWatchAPI _declspec(dllimport)
#else
#define CStopWatchAPI _declspec(dllexport)
#endif