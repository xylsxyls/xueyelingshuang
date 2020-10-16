#pragma once
#ifdef _WIN32
#define _WidgetFactoryAPI
#endif
#include "WidgetFactory.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"WidgetFactoryd.lib")
#else
#pragma comment(lib,"WidgetFactory.lib")
#endif
#endif