#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerHelper.h"
#include "Config.h"
#include <QPainter>
#include <algorithm>
#include <cctype>
#include <string>

QString LumaPlayerHelper::qcn(const wchar_t* text)
{
	return QString::fromWCharArray(text);
}

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

bool LumaPlayerHelper::hasDebugArgument(int argc, char* argv[])
{
	if (argv == nullptr)
	{
		return false;
	}
	for (int index = 1; index < argc; ++index)
	{
		if (argv[index] == nullptr)
		{
			continue;
		}
		std::string argument(argv[index]);
		std::transform(argument.begin(), argument.end(), argument.begin(), [](unsigned char value) -> char
		{
			return static_cast<char>(std::tolower(value));
		});
		if (argument == "debug")
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
		return QString("%1:%2:%3").arg(hour, 2, 10, QChar('0')).arg(minute, 2, 10, QChar('0')).arg(second, 2, 10, QChar('0'));
	}
	int64_t millisecond = (time100ns / g_config.m_millisecond100ns) % 1000;
	return QString("%1:%2:%3.%4").arg(hour, 2, 10, QChar('0')).arg(minute, 2, 10, QChar('0')).arg(second, 2, 10, QChar('0')).arg(millisecond, 3, 10, QChar('0'));
}

void LumaPlayerHelper::paintLoadIcon(QPainter& painter, const QRect& rect, bool hover)
{
	painter.save();
	painter.setPen(Qt::NoPen);
	painter.setBrush(hover ? QColor(255, 255, 255, 38) : QColor(255, 255, 255, 8));
	painter.drawRoundedRect(rect, 3, 3);
	QPen pen(QColor(245, 245, 245, 220));
	pen.setWidth(2);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	QRect tray(rect.left() + 4, rect.top() + 11, rect.width() - 8, 5);
	painter.drawRect(tray);
	painter.drawLine(QPoint(rect.center().x(), rect.top() + 4), QPoint(rect.center().x(), rect.top() + 12));
	painter.drawLine(QPoint(rect.center().x(), rect.top() + 4), QPoint(rect.center().x() - 4, rect.top() + 8));
	painter.drawLine(QPoint(rect.center().x(), rect.top() + 4), QPoint(rect.center().x() + 4, rect.top() + 8));
	painter.restore();
}