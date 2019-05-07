#pragma once

#ifdef _StockConditionAPI
#define StockConditionAPI _declspec(dllimport)
#else
#define StockConditionAPI _declspec(dllexport)
#endif