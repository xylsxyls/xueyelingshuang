#pragma once

#ifdef _ThreadWindowAPI
#define ThreadWindowAPI _declspec(dllimport)
#else
#define ThreadWindowAPI _declspec(dllexport)
#endif