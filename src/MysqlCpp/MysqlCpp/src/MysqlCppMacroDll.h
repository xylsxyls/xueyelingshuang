#pragma once

#ifdef _MysqlCppAPI
#define MysqlCppAPI _declspec(dllimport)
#else
#define MysqlCppAPI _declspec(dllexport)
#endif