#pragma once

#ifdef _JsonCppAPI
#define JSON_API _declspec(dllimport)
#else
#define JSON_API _declspec(dllexport)
#endif