#pragma once

#ifdef _QtCoreAPI
#define QtCoreAPI _declspec(dllexport)
#else
#define QtCoreAPI _declspec(dllimport)
#endif