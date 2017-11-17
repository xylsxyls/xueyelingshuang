#pragma once

#ifdef _DmSoftAPI
#define DmSoftAPI _declspec(dllimport)
#else
#define DmSoftAPI _declspec(dllexport)
#endif