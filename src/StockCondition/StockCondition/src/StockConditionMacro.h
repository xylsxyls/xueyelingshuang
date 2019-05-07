#pragma once

#ifdef _StockConditionAPI
#define StockConditionAPI _declspec(dllimport)
#else
#define StockConditionAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef StockConditionAPI
#define StockConditionAPI 
#endif