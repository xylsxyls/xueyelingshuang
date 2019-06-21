#pragma once

#ifdef _DialogManagerAPI
#define DialogManagerAPI _declspec(dllimport)
#else
#define DialogManagerAPI _declspec(dllexport)
#endif