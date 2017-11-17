#pragma once

#ifdef _PointAPI
#define PointAPI _declspec(dllimport)
#else
#define PointAPI _declspec(dllexport)
#endif