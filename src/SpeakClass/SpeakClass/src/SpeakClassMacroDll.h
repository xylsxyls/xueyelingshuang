#pragma once

#ifdef _SpeakClassAPI
#define SpeakClassAPI _declspec(dllimport)
#else
#define SpeakClassAPI _declspec(dllexport)
#endif