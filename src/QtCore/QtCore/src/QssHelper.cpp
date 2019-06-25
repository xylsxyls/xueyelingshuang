#include "QssHelper.h"
#include "core/coreAPI.h"
#include <QPixmap>

std::wstring QssHelper::QColorToWString(const QColor& color)
{
	return CStringManager::Format(L"rgba(%d,%d,%d,%d)", color.red(), color.green(), color.blue(), color.alpha());
}

bool QssHelper::GetPicHeight(const std::wstring& path, qint32 count, std::vector<qint32>& vecHeight)
{
	if (count == 0)
	{
		return false;
	}

	QString imgPath = QString::fromStdWString(path.c_str());
	QPixmap imgPixmap;
	if (imgPixmap.load(imgPath) == false)
	{
		return false;
	}

	qint32 imgHeight = imgPixmap.height();
	qint32 stepHeight = imgHeight / count;

	if (imgHeight % count != 0)
	{
		return false;
	}

	qint32 height = 0;
	vecHeight.clear();
	while (height <= imgHeight)
	{
		vecHeight.push_back(height);
		height += stepHeight;
	}

	if (vecHeight.size() != count + 1)
	{
		return false;
	}

	return true;
}

template <typename QControl>
void QssHelper::init(const std::wstring& itemName, QControl* control)
{
	control->m_className = typeid(QControl).name();
	qint32 count = 6;
	while (count-- != 0)
	{
		control->m_className.pop_front();
	}
	control->m_itemName = itemName;
}

QColor QssHelper::GetQColorWithDefault(const QColor& dstColor, const QColor& srcColor)
{
	return (dstColor == QColor(0, 0, 0, 0)) ? srcColor : dstColor;
}

qint32 QssHelper::GetIntWithDefault(const qint32& dstInt, const qint32& srcInt)
{
	return (dstInt == -1) ? srcInt : dstInt;
}