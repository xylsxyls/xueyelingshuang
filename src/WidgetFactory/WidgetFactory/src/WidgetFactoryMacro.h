#pragma once

#ifdef _WidgetFactoryAPI
#define WidgetFactoryAPI _declspec(dllimport)
#else
#define WidgetFactoryAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef WidgetFactoryAPI
#define WidgetFactoryAPI 
#endif