#pragma once
#ifdef _WIN32
#define _VariantAPI
#endif
#include "Variant.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Variantd.lib")
#else
#pragma comment(lib,"Variant.lib")
#endif
#endif