#pragma once

#ifdef _LibuvTcpAPI
#define LibuvTcpAPI _declspec(dllimport)
#else
#define LibuvTcpAPI _declspec(dllexport)
#endif