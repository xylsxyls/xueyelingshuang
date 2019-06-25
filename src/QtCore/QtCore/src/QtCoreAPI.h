#pragma once
#define _QtCoreAPI
#include "EO_XmlSax2Parser.h"
#include "HttpRequest.h"
#include "QssHelper.h"
#include "QssString.h"
#include "WidgetFactory.h"

#ifdef _DEBUG
#pragma comment(lib,"QtCored.lib")
#else
#pragma comment(lib,"QtCore.lib")
#endif