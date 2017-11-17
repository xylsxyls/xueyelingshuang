#pragma once

#ifdef _CBigNumberAPI
#define CBigNumberAPI _declspec(dllimport)
#else
#define CBigNumberAPI _declspec(dllexport)
#endif