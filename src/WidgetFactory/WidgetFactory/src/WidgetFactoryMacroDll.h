#pragma once

#ifdef _WidgetFactoryAPI
#define WidgetFactoryAPI _declspec(dllimport)
#else
#define WidgetFactoryAPI _declspec(dllexport)
#endif