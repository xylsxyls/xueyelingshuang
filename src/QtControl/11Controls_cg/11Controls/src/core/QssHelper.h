#pragma once
#include <string>
#include <QColor>
#include <stdint.h>
#include <vector>
#include "CoreMacro.h"

#define GetQColor QssHelper::GetQColorWithDefault
#define GetInt QssHelper::GetIntWithDefault

/** 设置QSS使用的类，该类只用于内部调用
*/
class QssHelper
{
public:
	static std::wstring QColorToWString(const QColor& color);

	static bool GetPicHeight(const std::wstring& path, int32_t count, std::vector<int32_t>& vecHeight);

	template <typename QControl>
	static void init(const std::wstring& itemName, QControl* control);

	static QColor GetQColorWithDefault(const QColor& dstColor, const QColor& srcColor);

	static int32_t GetIntWithDefault(const int32_t& dstInt, const int32_t& srcInt);

};