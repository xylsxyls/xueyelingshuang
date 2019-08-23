#pragma once

#ifdef _FiniteDequeAPI
#define FiniteDequeAPI _declspec(dllimport)
#else
#define FiniteDequeAPI _declspec(dllexport)
#endif