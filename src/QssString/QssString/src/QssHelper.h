#pragma once
#include <string>
#include <QColor>
#include <vector>
#include "QssStringMacro.h"

#define GetQColor QssHelper::GetQColorWithDefault
#define GetInt QssHelper::GetIntWithDefault

/** 设置QSS使用的类，该类只用于内部调用
*/
class QssStringAPI QssHelper
{
public:
	static std::wstring QColorToWString(const QColor& color);

	static bool GetPicHeight(const std::wstring& path, qint32 count, std::vector<qint32>& vecHeight);

	template <typename QControl>
	static void init(const std::wstring& itemName, QControl* control);

	static QColor GetQColorWithDefault(const QColor& dstColor, const QColor& srcColor);

	static qint32 GetIntWithDefault(const qint32& dstInt, const qint32& srcInt);

};