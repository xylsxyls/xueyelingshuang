#pragma once

#ifdef _QtCoreAPI
#define QtCoreAPI _declspec(dllimport)
#else
#define QtCoreAPI _declspec(dllexport)
#endif