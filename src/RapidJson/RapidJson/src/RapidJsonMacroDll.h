#pragma once

#ifdef _RapidJsonAPI
#define RapidJsonAPI _declspec(dllimport)
#else
#define RapidJsonAPI _declspec(dllexport)
#endif