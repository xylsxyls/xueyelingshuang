#include "ControlSubStyle.h"
#include "CStringManager/CStringManagerAPI.h"

#include <vector>

void ControlSubStyle::writeKeyValue(QssString* controlStyle,
									bool hasSubControl,
									const std::wstring& subControl,
									const std::wstring& key,
									const std::wstring& value)
{
	if (controlStyle == nullptr)
	{
		return;
	}
	const bool useSubControl = hasSubControl && subControl.empty() == false;
	controlStyle->addClassName()(useSubControl, subControl).AddKeyValue(key, value);
}

void ControlSubStyle::writeColorStateMap(QssString* controlStyle,
										 bool hasSubControl,
										 const std::wstring& subControl,
										 const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
										 const std::wstring& key)
{
	if (controlStyle == nullptr)
	{
		return;
	}
	const bool useSubControl = hasSubControl && subControl.empty() == false;
	for (auto itStateMap = colorStateMap.begin(); itStateMap != colorStateMap.end(); ++itStateMap)
	{
		for (auto itColor = itStateMap->second.begin(); itColor != itStateMap->second.end(); ++itColor)
		{
			controlStyle->addClassName()(useSubControl, subControl)(itStateMap->first)(itColor->first).AddKeyValue(key, QssHelper::QColorToWString(itColor->second));
		}
	}
}

void ControlSubStyle::writeImageStateMap(QssString* controlStyle,
										 bool hasSubControl,
										 const std::wstring& subControl,
										 const std::wstring& imagePath,
										 qint32 stateCount,
										 const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
										 const std::wstring& key)
{
	if (controlStyle == nullptr)
	{
		return;
	}
	if (stateCount <= 0)
	{
		return;
	}
	std::vector<qint32> vecHeight;
	if (QssHelper::GetPicHeight(imagePath, stateCount, vecHeight) == false)
	{
		return;
	}
	if (static_cast<qint32>(vecHeight.size()) != stateCount + 1)
	{
		return;
	}
	const bool useSubControl = hasSubControl && subControl.empty() == false;
	for (auto itStateMap = imageStateMap.begin(); itStateMap != imageStateMap.end(); ++itStateMap)
	{
		for (auto itImage = itStateMap->second.begin(); itImage != itStateMap->second.end(); ++itImage)
		{
			const qint32 imageNum = itImage->second;
			if (imageNum > stateCount || imageNum < 0)
			{
				continue;
			}
			const qint32 topHeight = imageNum == 0 ? 0 : vecHeight[imageNum - 1];
			const qint32 bottomHeight = vecHeight[stateCount - imageNum];
			const std::wstring imageUrl = CStringManager::Format(L"url(%s) %d 0 %d 0 stretch stretch",
																 imagePath.c_str(),
																 topHeight,
																 bottomHeight);
			controlStyle->addClassName()(useSubControl, subControl)(itStateMap->first)(itImage->first).AddKeyValue(key, imageUrl);
		}
	}
}

void ControlSubStyle::setWidgetKeyValue(QssString* controlStyle,
										const std::wstring& key,
										const std::wstring& value)
{
	writeKeyValue(controlStyle, false, L"", key, value);
}

void ControlSubStyle::setWidgetPxValue(QssString* controlStyle,
									   const std::wstring& key,
									   qint32 valuePx)
{
	setWidgetKeyValue(controlStyle, key, CStringManager::Format(L"%dpx", valuePx));
}

void ControlSubStyle::setWidgetPxSolidValue(QssString* controlStyle,
											const std::wstring& key,
											qint32 valuePx)
{
	setWidgetKeyValue(controlStyle, key, CStringManager::Format(L"%dpx solid", valuePx));
}

void ControlSubStyle::setWidgetColorStateMap(QssString* controlStyle,
											 const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
											 const std::wstring& key)
{
	writeColorStateMap(controlStyle, false, L"", colorStateMap, key);
}

void ControlSubStyle::setWidgetImageStateMap(QssString* controlStyle,
											 const std::wstring& imagePath,
											 qint32 stateCount,
											 const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
											 const std::wstring& key)
{
	writeImageStateMap(controlStyle, false, L"", imagePath, stateCount, imageStateMap, key);
}

void ControlSubStyle::setKeyValue(QssString* controlStyle,
								  const std::wstring& subControl,
								  const std::wstring& key,
								  const std::wstring& value)
{
	writeKeyValue(controlStyle, true, subControl, key, value);
}

void ControlSubStyle::setPxValue(QssString* controlStyle,
								 const std::wstring& subControl,
								 const std::wstring& key,
								 qint32 valuePx)
{
	setKeyValue(controlStyle, subControl, key, CStringManager::Format(L"%dpx", valuePx));
}

void ControlSubStyle::setPxSolidValue(QssString* controlStyle,
									  const std::wstring& subControl,
									  const std::wstring& key,
									  qint32 valuePx)
{
	setKeyValue(controlStyle, subControl, key, CStringManager::Format(L"%dpx solid", valuePx));
}

void ControlSubStyle::setColorStateMap(QssString* controlStyle,
									   const std::wstring& subControl,
									   const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
									   const std::wstring& key)
{
	writeColorStateMap(controlStyle, true, subControl, colorStateMap, key);
}

void ControlSubStyle::setImageStateMap(QssString* controlStyle,
									   const std::wstring& subControl,
									   const std::wstring& imagePath,
									   qint32 stateCount,
									   const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
									   const std::wstring& key)
{
	writeImageStateMap(controlStyle, true, subControl, imagePath, stateCount, imageStateMap, key);
}

void ControlSubStyle::setDescendantKeyValue(QssString* controlStyle,
											const std::wstring& descendant,
											const std::wstring& key,
											const std::wstring& value)
{
	if (controlStyle == nullptr)
	{
		return;
	}
	controlStyle->addClassName()(SPACE, descendant).AddKeyValue(key, value);
}

void ControlSubStyle::setDescendantSubControlKeyValue(QssString* controlStyle,
													  const std::wstring& descendant,
													  const std::wstring& subControl,
													  const std::wstring& key,
													  const std::wstring& value)
{
	if (controlStyle == nullptr)
	{
		return;
	}
	controlStyle->addClassName()(SPACE, descendant)(true, subControl).AddKeyValue(key, value);
}

void ControlSubStyle::setDescendantSubControlPxValue(QssString* controlStyle,
													 const std::wstring& descendant,
													 const std::wstring& subControl,
													 const std::wstring& key,
													 qint32 valuePx)
{
	setDescendantSubControlKeyValue(controlStyle, descendant, subControl, key, CStringManager::Format(L"%dpx", valuePx));
}

void ControlSubStyle::setDescendantSubControlPxSolidValue(QssString* controlStyle,
														  const std::wstring& descendant,
														  const std::wstring& subControl,
														  const std::wstring& key,
														  qint32 valuePx)
{
	setDescendantSubControlKeyValue(controlStyle, descendant, subControl, key, CStringManager::Format(L"%dpx solid", valuePx));
}