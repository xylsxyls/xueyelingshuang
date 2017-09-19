#include "ConfigInfo.h"
#include "Rect/RectAPI.h"
#include "DmSoft/DmSoftAPI.h"

ConfigInfo::ConfigInfo()
{
	m_screenWidth = DmSoft::GetScreenWidth();
	if (m_screenWidth == 0)
	{
		ASSERT(0);
	}
	m_screenHeight = DmSoft::GetScreenHeight();
	if (m_screenHeight == 0)
	{
		ASSERT(0);
	}

	int32_t nineOffset = RECT_NINE_1 - 1;
	for (int32_t index = 1; index <= 9; ++index)
	{
		(*this)[index + nineOffset] = GetRectForNine(index);
	}


}

ConfigInfo::~ConfigInfo()
{

}

ConfigInfo& ConfigInfo::Instance()
{
	static ConfigInfo configInfo;
	return configInfo;
}

xyls::Rect ConfigInfo::GetRectForNine(int32_t id)
{
	int32_t width = m_screenWidth / 3.0;
	int32_t height = m_screenHeight / 3.0;
	return xyls::Rect(xyls::Point(((id - 1) % 3) * width,
									((id - 1) / 3) * height),
									width,
									height);
}