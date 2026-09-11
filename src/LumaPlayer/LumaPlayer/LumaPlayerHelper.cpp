#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerHelper.h"
#include "Config.h"
#include "CStringManager/CStringManagerAPI.h"
#include <QPainter>
#include <algorithm>
#include <cctype>
#include <string>

int LumaPlayerHelper::clampInt(int value, int minValue, int maxValue)
{
	if (value < minValue)
	{
		return minValue;
	}
	if (value > maxValue)
	{
		return maxValue;
	}
	return value;
}

int64_t LumaPlayerHelper::clampTime100ns(int64_t value, int64_t minValue, int64_t maxValue)
{
	if (value < minValue)
	{
		return minValue;
	}
	if (value > maxValue)
	{
		return maxValue;
	}
	return value;
}

bool LumaPlayerHelper::isPlayingState(int32_t state)
{
	return state == LumaPlayerCoreCStatePlaying;
}

bool LumaPlayerHelper::isOpenedState(int32_t state)
{
	return state == LumaPlayerCoreCStateStopped ||
		state == LumaPlayerCoreCStatePlaying ||
		state == LumaPlayerCoreCStatePaused ||
		state == LumaPlayerCoreCStateSeeking ||
		state == LumaPlayerCoreCStateEnded;
}

bool LumaPlayerHelper::hasDebugArgument(const QStringList& arguments)
{
	for (int32_t index = 1; index < arguments.size(); ++index)
	{
		if (arguments.at(index).compare(QStringLiteral("debug"), Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	return false;
}

QString LumaPlayerHelper::formatTime(int64_t time100ns, bool withMillisecond)
{
	if (time100ns < 0)
	{
		time100ns = 0;
	}
	int64_t totalSeconds = time100ns / g_config.m_second100ns;
	int64_t hour = totalSeconds / 3600;
	int64_t minute = (totalSeconds / 60) % 60;
	int64_t second = totalSeconds % 60;
	if (!withMillisecond)
	{
        return QString::fromStdString(CStringManager::Format(g_config.m_timeTextFormat.c_str(),
            static_cast<int>(hour), static_cast<int>(minute), static_cast<int>(second)));
	}
	int64_t millisecond = (time100ns / g_config.m_millisecond100ns) % 1000;
    return QString::fromStdString(CStringManager::Format(g_config.m_preciseTimeTextFormat.c_str(),
        static_cast<int>(hour), static_cast<int>(minute), static_cast<int>(second), static_cast<int>(millisecond)));
}

void LumaPlayerHelper::paintLoadIcon(QPainter& painter, const QRect& rect, bool hover)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(hover ? g_config.m_buttonHoverColor : g_config.m_buttonColor);
    painter.drawRoundedRect(rect, g_config.m_cornerRadius, g_config.m_cornerRadius);
    QPen pen(g_config.m_iconColor);
    pen.setWidthF(g_config.m_iconStroke);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.translate(rect.topLeft());
    painter.scale(rect.width() / g_config.m_iconCanvasSize, rect.height() / g_config.m_iconCanvasSize);
    painter.drawPath(g_config.m_loadIcon);
    painter.restore();
}