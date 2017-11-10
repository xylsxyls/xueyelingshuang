#pragma once
#include <string>
#include <QColor>
#include <stdint.h>
#include <vector>

class QssHelper
{
public:
	static std::wstring QColorToWString(const QColor& color);

	static bool GetPicHeight(const std::wstring& path, int32_t count, std::vector<int32_t>& vecHeight);
};