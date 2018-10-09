#pragma once
#define _VariantAPI
#include "Variant.h"

#ifdef _DEBUG
#pragma comment(lib,"Variantd.lib")
#else
#pragma comment(lib,"Variant.lib")
#endif