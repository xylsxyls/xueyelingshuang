#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include <QMetaType>
#include "BigNumber/BigNumberAPI.h"

struct SolutionWidgetParam
{
	IntDateTime m_date;
	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> m_buyStock;
	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> m_sellStock;

	SolutionWidgetParam()
	{
		m_date.clear();
	}
};
Q_DECLARE_METATYPE(SolutionWidgetParam);