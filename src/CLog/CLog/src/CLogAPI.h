#pragma once
#define _CLogAPI
#include "CLog.h"

#ifdef _DEBUG
#pragma comment(lib,"CLogd.lib")
#else
#pragma comment(lib,"CLog.lib")
#endif

const string moduleName = "";
const int sizekb = -1;

#ifdef _DEBUG
const int LOGLEVEL = CLog::logLevel::INFO | CLog::logLevel::DBG | CLog::logLevel::WARNING | CLog::logLevel::ERR | CLog::logLevel::FATAL;
#else
const int LOGLEVEL = CLog::logLevel::INFO | CLog::logLevel::ERR | CLog::logLevel::FATAL;
#endif

//?不能放在库的.h中初始化，因为所有在头文件中定义的全局变量都被编译到动态库中去了，之后再改头文件参数无效
const int logFile = CLog::PrivateCreateLogApi(LOGLEVEL, moduleName, sizekb);