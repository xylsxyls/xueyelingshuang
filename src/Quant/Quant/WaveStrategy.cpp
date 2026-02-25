#include "WaveStrategy.h"
#include <algorithm>
#include <iostream>
#include "Util.h"
#include "CStringManager/CStringManagerAPI.h"

// 操作类型枚举定义
enum
{
	OPERATE, // 操作模式：上午卖出下午买入
	HOLD,    // 持有模式：上午不卖，下午买入
	GIVE_UP  // 放弃模式：如果当前空仓：当天不买入；如果当前满仓：当天下午卖出（已经买入则第二天早晨卖出）
};

WaveStrategy::WaveStrategy() :
	m_openPrice(0),
	m_closePrice(0),
	m_hangSellPrice(0),
	m_hangSellObserveTime(ObserveTime::COUNT),
	m_directSellPrice(0),
	m_directSellObserveTime(ObserveTime::COUNT),
	m_hangBuyPrice(0),
	m_hangBuyObserveTime(ObserveTime::COUNT),
	m_directBuyPrice(0),
	m_directBuyObserveTime(ObserveTime::COUNT),
	m_giveUpHangSellPrice(0),
	m_giveUpHangSellObserveTime(ObserveTime::COUNT),
	m_giveUpDirectSellPrice(0),
	m_giveUpDirectSellObserveTime(ObserveTime::COUNT),
	m_aObserveTime(ObserveTime::COUNT),
	m_bObserveTime(ObserveTime::COUNT),
	m_holdCent(0),
	m_giveUpCent(0),
	m_aPrice(0),
	m_bPrice(0),
	m_chaseCent(0),
	m_cutCent(0),
	m_isNormal(false),
	m_canTrade(false),
	m_hasFirstBuy(false),
	m_isUpdateBuyParam(false),
	m_isUpdateSellParam(false)
{
	// 初始化策略模式和名称
	m_mode = StrategyMode::WAVE;
	m_modeName = "Wave";
}

WaveStrategy::~WaveStrategy()
{
	// 析构函数，无需特殊清理
}

bool WaveStrategy::fillCheckParam()
{
	// 第四步：解析策略参数，包括各种时间点和差价阈值
	m_hangSellObserveTime = (ObserveTime)m_strategyParam[0]; // 挂卖价时间点
	m_directSellObserveTime = (ObserveTime)m_strategyParam[1]; // 直卖价时间点
	m_hangBuyObserveTime = (ObserveTime)m_strategyParam[2]; // 挂买价时间点
	m_directBuyObserveTime = (ObserveTime)m_strategyParam[3]; // 直买价时间点
	m_giveUpHangSellObserveTime = m_hangBuyObserveTime; // 放弃挂卖价时间点
	m_giveUpDirectSellObserveTime = m_directBuyObserveTime; // 放弃直卖价时间点
	m_aObserveTime = (ObserveTime)m_strategyParam[4]; // 观察价A时间点
	m_bObserveTime = (ObserveTime)m_strategyParam[5]; // 观察价B时间点
	m_holdCent = m_strategyParam[6]; // 观察价B-A大于等于多少时当天直买，第二天早上不卖
	m_giveUpCent = m_strategyParam[7]; // 观察价B-A小于等于多少时，当天不买，如果当前持有则当天下午卖出，如果已经买入则第二天早晨卖出
	m_chaseCent = m_strategyParam[8]; // 当天早上卖出后反追差价
	m_cutCent = m_strategyParam[9]; // 割肉差价
	m_isNormal = (m_strategyParam[10] == 1); // 是否为常规走势
	m_tradeCount.resize(4);
	m_tradeCount[0] = 0;
	m_tradeCount[1] = 0;
	m_tradeCount[2] = 0;
	m_tradeCount[3] = 0;

	// 第五步：验证时间点顺序和参数逻辑，如果无效则返回false
	if ((int32_t)m_directSellObserveTime < (int32_t)m_hangSellObserveTime ||
		(int32_t)m_directSellObserveTime - (int32_t)m_hangSellObserveTime > 3 ||
		(int32_t)m_directBuyObserveTime < (int32_t)m_hangBuyObserveTime ||
		(int32_t)m_directBuyObserveTime - (int32_t)m_hangBuyObserveTime > 3 ||
		(int32_t)m_hangBuyObserveTime <= ((int32_t)m_directSellObserveTime + 1) ||
		m_bObserveTime <= m_aObserveTime ||
		(int32_t)m_bObserveTime > (int32_t)m_hangBuyObserveTime ||
		m_giveUpCent >= m_holdCent)
	{
		return false;
	}

	return true;
}

bool WaveStrategy::onTradingDay(uint32_t date)
{
	// 第一步：检查策略参数是否有效，如果无效则记录错误并返回false
	if (!isStrategyParamValid())
	{
		RCSend("策略参数无效: isStrategyParamValid");
		return false;
	}

	// 第二步：获取第一只股票代码（目前只支持单股票，未来可扩展）
	const std::string& stock = m_vecStock[0];
	// 更新当天日期
	m_date = date;

	// 第三步：获取当天分时数据，如果为空则记录错误并返回false
	const std::vector<int32_t>& dayInfo = m_spMarket->getStockData(stock, m_date);
	if (dayInfo.empty())
	{
		RCSend("当天分时数据为空: dayInfo empty, date = %u", m_date);
		return false;
	}

	// 第六步：获取各种价格指标，包括开盘价和收盘价
	m_openPrice = dayInfo[(int32_t)Overall::OPEN]; // 开盘价（9:25数据）
	m_closePrice = dayInfo[(int32_t)Overall::CLOSE]; // 收盘价
	m_hangSellPrice = getCurrentPrice(dayInfo, m_hangSellObserveTime);
	m_directSellPrice = getDirectSellPrice(dayInfo, m_directSellObserveTime);
	m_hangBuyPrice = getCurrentPrice(dayInfo, m_hangBuyObserveTime) - 1; // 根据描述-1
	m_directBuyPrice = getDirectBuyPrice(dayInfo, m_directBuyObserveTime);
	m_giveUpHangSellPrice = getCurrentPrice(dayInfo, m_giveUpHangSellObserveTime);
	m_giveUpDirectSellPrice = getDirectSellPrice(dayInfo, m_giveUpDirectSellObserveTime);
	m_aPrice = getCurrentPrice(dayInfo, m_aObserveTime);
	m_bPrice = getCurrentPrice(dayInfo, m_bObserveTime);

	// 第七步：重置当天买入标志
	m_canTrade = true;
	m_isUpdateBuyParam = false;
	m_isUpdateSellParam = false;

	// 第八步：处理首次买入逻辑，如果是第一天则买入并设置标志
	if (!m_hasFirstBuy)
	{
		if (m_import.empty())
		{
			updateOperate();
			// 收盘买入
			m_spFund->buyAll(stock, m_closePrice, m_date, ObserveTime::COUNT);
			updateBuyParam(m_closePrice, ObserveTime::COUNT, true);
		}
		m_hasFirstBuy = true;
		return true;
	}

	// 第九步：检查持仓，如果无持仓则记录错误并返回false
	std::shared_ptr<Position> spPosition = m_spFund->getPosition(stock);
	if (spPosition == nullptr)
	{
		RCSend("未找到持仓信息: hasPosition false");
		return false;
	}

	// 第十步：开盘检查（割肉或反追）
	openCheck(stock);

	// 第十一步：上午从0930到挂卖时间监控割肉或反追
	// 注意：不使用cancanTrade变量缓存，因为m_isCurrentDayBuy会在执行过程中改变
	timeCheck(ObserveTime::TIME0930, m_hangSellObserveTime, stock, dayInfo);

	// 第十二步：执行挂卖到直卖（真实或模拟交易）
	ObserveTime operateEndTime = m_hangSellObserveTime;
	if (m_import.m_operate == GIVE_UP || m_import.m_operate == OPERATE)
	{
		operateEndTime = hangToDirectSell(stock, dayInfo);
	}

	// 第十三步：上午从卖出时间到B点监控割肉或反追
	timeCheck(operateEndTime, m_bObserveTime, stock, dayInfo);

	// 第十四步：B点出来后更新操作类型
	updateOperate();

	// 第十五步：从B点到挂买时间监控割肉或反追
	timeCheck(m_bObserveTime, m_hangBuyObserveTime, stock, dayInfo);

	// 第十六步：根据操作模式执行买入或放弃卖出
	if (m_import.m_operate == HOLD || m_import.m_operate == OPERATE)
	{
		operateEndTime = hangToDirectBuy(stock, dayInfo);
	}
	else // GIVE_UP模式
	{
		operateEndTime = hangToDirectSell(stock, dayInfo);
	}

	// 第十七步：下午从操作结束时间到收盘监控割肉或反追
	timeCheck(operateEndTime, ObserveTime::COUNT, stock, dayInfo);

	return true;
}

std::string WaveStrategy::describeParam(const std::vector<int32_t>& params)
{
	// 第一步：初始化描述字符串
	std::string describe;

	// 第二步：遍历参数，转换为字符串描述（时间点用Util转换，其他直接to_string）
	for (size_t index = 0; index < params.size(); ++index)
	{
		if (index <= 5)
		{
			describe += (Util::observeTimeToWatchString((ObserveTime)params[index]) + ", ");
		}
		else
		{
			describe += std::to_string(params[index]) + ", ";
		}
	}

	// 第三步：移除末尾多余逗号和空格
	if (!describe.empty())
	{
		describe.pop_back();
		describe.pop_back();
	}

	// 第四步：返回描述字符串
	return describe;
}

bool WaveStrategy::isStrategyParamValid() const
{
	// 第一步：检查参数数量是否为11，如果不是则记录错误返回false
	if (m_strategyParam.size() != 11)
	{
		RCSend("错误: 策略%s参数数量不是11", m_modeName.c_str());
		return false;
	}

	// 第二步：检查市场和资金接口是否设置，如果未设置则记录错误返回false
	if (!m_spMarket || !m_spFund)
	{
		RCSend("错误: 策略%s市场数据或资金账户未设置", m_modeName.c_str());
		return false;
	}

	// 第三步：检查股票列表是否为空，如果为空则记录错误返回false
	if (m_vecStock.empty())
	{
		RCSend("错误: 策略%s没有股票", m_modeName.c_str());
		return false;
	}

	// 第四步：获取第一只股票，检查是否在市场数据中存在，如果不存在则记录错误返回false
	const std::string& stock = m_vecStock[0];
	if (!m_spMarket->hasStock(stock))
	{
		RCSend("错误: 策略%s股票%s不在市场数据中", m_modeName.c_str(), stock.c_str());
		return false;
	}

	// 第五步：所有检查通过，返回true
	return true;
}

void WaveStrategy::updateOperate()
{
	// 第一步：获取持仓和放弃阈值参数
	int32_t holdCent = m_strategyParam[6];
	int32_t giveUpCent = m_strategyParam[7];
	bool isNormal = (m_strategyParam[10] == 1);

	// 第二步：计算价格差
	int32_t tPrice = m_bPrice - m_aPrice;

	// 第三步：根据价格差和isNormal更新操作模式
	if (tPrice >= holdCent)
	{
		m_import.m_operate = isNormal ? HOLD : GIVE_UP;
	}
	else if (tPrice <= giveUpCent)
	{
		m_import.m_operate = isNormal ? GIVE_UP : HOLD;
	}
	else
	{
		m_import.m_operate = OPERATE;
	}

	m_isUpdateBuyParam = false;
	m_isUpdateSellParam = false;
}

bool WaveStrategy::openCheck(const std::string& stock)
{
	// 第二步：如果满仓，检查是否触发割肉
	if (m_import.m_isFull)
	{
		// 使用开盘价判断是否触发割肉
		if (m_openPrice <= cutPrice())
		{
			// 第三步：执行割肉卖出，使用开盘价交易（因为开盘时可能跳空，只有开盘价可交易）
			m_spFund->sellAllForT(stock, m_openPrice, m_date, ObserveTime::TIME0930);
			updateSellParam(m_openPrice, ObserveTime::TIME0930, true);
			return true;
		}
	}
	// 第四步：如果空仓，检查是否触发反追
	else
	{
		if (m_openPrice >= chasePrice())
		{
			// 第五步：执行反追买入
			m_spFund->buyAll(stock, m_openPrice, m_date, ObserveTime::TIME0930);
			updateBuyParam(m_openPrice, ObserveTime::TIME0930, true);
			return true;
		}
	}

	// 第六步：未触发任何操作，返回false
	return false;
}

bool WaveStrategy::timeCheck(ObserveTime beginTime, ObserveTime endTime,
							 const std::string& stock, const std::vector<int32_t>& dayInfo)
{
	if (!m_canTrade)
	{
		return false;
	}
	// 第一步：初始化当前检查时间点
	ObserveTime current = beginTime;

	// 第二步：逐10分钟段循环检查
	while ((int32_t)current < (int32_t)endTime)
	{
		// 第三步：计算下一个时间点
		ObserveTime next = (ObserveTime)((int32_t)current + 1);

		// 第四步：获取从整体开始时间到下一个时间点的最高价和最低价（关键：累积检查）
		int32_t maxPrice = getMaxPrice(dayInfo, beginTime, next);
		int32_t minPrice = getMinPrice(dayInfo, beginTime, next);

		// 第五步：计算割肉价和反追价

		// 第六步：判断是否触发割肉（满仓时）
		if (minPrice <= cutPrice() && m_import.m_isFull)
		{
			updateSellParam(cutPrice(), next, true);
			m_spFund->sellAllForT(stock, cutPrice(), m_date, next);
			
			// 第七步：割肉后检查是否需要反追
			// 需要检查下一个时间点的当前价是否达到反追价
			int32_t nextPrice = getCurrentPrice(dayInfo, next);
			if (nextPrice >= chasePrice() && maxPrice >= chasePrice())
			{
				updateBuyParam(chasePrice(), next, true);
				m_spFund->buyAll(stock, chasePrice(), m_date, next);
				return true;
			}
		}
		// 第八步：判断是否触发反追（空仓时）
		else if (maxPrice >= chasePrice() && !m_import.m_isFull)
		{
			updateBuyParam(chasePrice(), next, true);
			m_spFund->buyAll(stock, chasePrice(), m_date, next);
			return true;
		}

		// 第九步：更新当前时间，继续下一段检查
		current = next;
	}

	// 第十步：整个区间未触发，返回false
	return false;
}

ObserveTime WaveStrategy::hangToDirectSell(const std::string& stock, const std::vector<int32_t>& dayInfo)
{
	if (!m_canTrade)
	{
		return ObserveTime::COUNT;
	}
	// 第三步：如果挂卖时间等于直卖时间，直接执行直卖
	if (m_hangSellObserveTime == m_directSellObserveTime)
	{
		// 如果是满仓
		if (m_import.m_isFull)
		{
			updateSellParam(m_directSellPrice, m_directSellObserveTime, true);
			m_spFund->sellAllForT(stock, m_directSellPrice, m_date, m_directSellObserveTime);
		}
		// 空仓
		else
		{
			// 当前价已经到了反追价
			if (m_directSellPrice >= chasePrice())
			{
				updateBuyParam(chasePrice(), m_directSellObserveTime, true);
				m_spFund->buyAll(stock, chasePrice(), m_date, m_directSellObserveTime);
			}
			else
			{
				updateSellParam(m_directSellPrice, m_directSellObserveTime, false);
			}
			//updateSellParam(m_directSellPrice, m_directSellObserveTime, false);
		}
		return m_directSellObserveTime;
	}

	ObserveTime trade = ObserveTime::COUNT;

	// 第五步：从挂卖时间开始逐段检查
	ObserveTime current = m_hangSellObserveTime;

	while ((int32_t)current < (int32_t)m_directSellObserveTime)
	{
		// 第六步：计算下一个时间点
		ObserveTime next = (ObserveTime)((int32_t)current + 1);

		// 检查并卖出
		ObserveTime check = checkSell(stock, dayInfo, current);
		if (check != ObserveTime::COUNT)
		{
			trade = check;
		}

		// 第十六步：未触发任何操作，继续检查下一段
		current = next;
	}

	// 第十七步：未在挂卖期间卖出，之前出现了反追
	if (m_import.m_operate == HOLD)
	{
		return trade;
	}
	// OPERATE或GIVE_UP
	else
	{
		// 当前是满仓
		if (m_import.m_isFull)
		{
			updateSellParam(m_directSellPrice, m_directSellObserveTime, true);
			m_spFund->sellAllForT(stock, m_directSellPrice, m_date, m_directSellObserveTime);
		}
		// 当前是空仓
		else
		{
			// 说明之前更新过，用之前的时间
			if (m_isUpdateSellParam)
			{
				return trade;
			}
			// 当前价已经到了反追价
			if (m_directSellPrice >= chasePrice())
			{
				updateBuyParam(chasePrice(), m_directSellObserveTime, true);
				m_spFund->buyAll(stock, chasePrice(), m_date, m_directSellObserveTime);
			}
			else
			{
				updateSellParam(m_directSellPrice, m_directSellObserveTime, false);
			}
			//updateSellParam(m_directSellPrice, m_directSellObserveTime, false);
		}
		return m_directSellObserveTime;
	}
}

ObserveTime WaveStrategy::checkSell(const std::string& stock, const std::vector<int32_t>& dayInfo, ObserveTime current)
{
	if (!m_canTrade)
	{
		return ObserveTime::COUNT;
	}

	// 第五步：计算下一个时间点
	ObserveTime next = (ObserveTime)((int32_t)current + 1);

	// 第七步：获取从挂卖时间到next的最高价和最低价（累积检查）
	int32_t minPrice = getMinPrice(dayInfo, m_hangSellObserveTime, next);
	int32_t maxPrice = getMaxPrice(dayInfo, m_hangSellObserveTime, next);
	int32_t currentPrice = getCurrentPrice(dayInfo, current);
	int32_t nextPrice = getCurrentPrice(dayInfo, next);

	// 第八步：判断是否触发挂卖或割肉
	bool hitSell = (maxPrice >= m_hangSellPrice);
	bool hitCut = (minPrice <= cutPrice());

	// 说明在上一轮出现了反追操作
	if (m_import.m_operate == HOLD)
	{
		RCSend("checkSell HOLD");
		return ObserveTime::COUNT;
	}

	// OPERATE或者GIVE_UP，今天还可以交易
	if (m_import.m_isFull)
	{
		// 挂卖割肉同时触发
		if (hitCut && hitSell)
		{
			// 先割肉，后反追
			if (nextPrice > currentPrice)
			{
				updateSellParam(cutPrice(), next, true);
				m_spFund->sellAllForT(stock, cutPrice(), m_date, next);
				// 第十一步：挂卖后检查是否需要反追
				if (nextPrice >= chasePrice() && maxPrice >= chasePrice())
				{
					updateBuyParam(chasePrice(), next, true);
					m_spFund->buyAll(stock, chasePrice(), m_date, next);
				}
			}
			else
			{
				updateSellParam(m_hangSellPrice, next, true);
				m_spFund->sellAllForT(stock, m_hangSellPrice, m_date, next);
			}
			return next;
		}
		// 第十四步：仅触发挂卖
		else if (hitSell)
		{
			updateSellParam(m_hangSellPrice, next, true);
			m_spFund->sellAllForT(stock, m_hangSellPrice, m_date, next);
			if (maxPrice >= chasePrice())
			{
				updateBuyParam(chasePrice(), next, true);
				m_spFund->buyAll(stock, chasePrice(), m_date, next);
			}
			return next;
		}
		// 仅触发了割肉
		else if (hitCut)
		{
			// 先割肉，后反追
			if (nextPrice > currentPrice)
			{
				updateSellParam(cutPrice(), next, true);
				m_spFund->sellAllForT(stock, cutPrice(), m_date, next);
				// 第十一步：挂卖后检查是否需要反追
				if (nextPrice >= chasePrice() && maxPrice >= chasePrice())
				{
					updateBuyParam(chasePrice(), next, true);
					m_spFund->buyAll(stock, chasePrice(), m_date, next);
				}
			}
			else
			{
				updateSellParam(cutPrice(), next, true);
				m_spFund->sellAllForT(stock, cutPrice(), m_date, next);
			}
			return next;
		}
		return ObserveTime::COUNT;
	}
	// 目前是空仓
	else
	{
		// 只有到达反追线才买入
		if (maxPrice >= chasePrice())
		{
			updateBuyParam(chasePrice(), next, true);
			m_spFund->buyAll(stock, chasePrice(), m_date, next);
			return next;
		}
		else
		{
			// 说明在上一个循环里更新过
			if (m_isUpdateSellParam)
			{
				return ObserveTime::COUNT;
			}
			updateSellParam(m_hangSellPrice, next, false);
			return next;
		}
	}
}

ObserveTime WaveStrategy::hangToDirectBuy(const std::string& stock, const std::vector<int32_t>& dayInfo)
{
	// 第一步：定义反追价变量，增加可读性
	if (!m_canTrade)
	{
		return ObserveTime::COUNT;
	}
	
	// 第二步：如果挂买时间等于直买时间，直接执行直买
	if (m_hangBuyObserveTime == m_directBuyObserveTime)
	{
		// 如果当前是满仓
		if (m_import.m_isFull)
		{
			// 当前价已经到了割肉价
			if (m_directBuyPrice <= cutPrice())
			{
				updateSellParam(cutPrice(), m_directBuyObserveTime, true);
				m_spFund->sellAllForT(stock, cutPrice(), m_date, m_directBuyObserveTime);
			}
			else
			{
				updateBuyParam(m_directBuyPrice, m_directBuyObserveTime, false);
			}
			//updateBuyParam(m_directBuyPrice, m_directBuyObserveTime, false);
		}
		// 当前是空仓
		else
		{
			updateBuyParam(m_directBuyPrice, m_directBuyObserveTime, true);
			m_spFund->buyAll(stock, m_directBuyPrice, m_date, m_directBuyObserveTime);
		}
		return m_directBuyObserveTime;
	}

	ObserveTime trade = ObserveTime::COUNT;

	// 第四步：从挂买时间开始逐段检查
	ObserveTime current = m_hangBuyObserveTime;

	while ((int32_t)current < (int32_t)m_directBuyObserveTime)
	{
		// 第五步：计算下一个时间点
		ObserveTime next = (ObserveTime)((int32_t)current + 1);

		// 检查并买入
		ObserveTime check = checkBuy(stock, dayInfo, current);
		if (check != ObserveTime::COUNT)
		{
			trade = check;
		}

		// 第十一步：未触发任何操作，继续检查下一段
		current = next;
	}

	// 第十二步：未在挂买期间买入，之前出现了割肉
	if (m_import.m_operate == GIVE_UP)
	{
		//目前是满仓，不可能出现
		if (m_import.m_isFull)
		{
			RCSend("hangToDirectBuy GIVE_UP");
			return trade;
		}
		// 目前是空仓
		else
		{
			// 只有直买价触发反追条件才会重新购买
			if (m_directBuyPrice >= chasePrice())
			{
				updateBuyParam(m_directBuyPrice, m_directBuyObserveTime, true);
				m_spFund->buyAll(stock, m_directBuyPrice, m_date, m_directBuyObserveTime);
				return m_directBuyObserveTime;
			}
			// 直买价达不到新的反追价则还是按照原本的交易时间返回
			return trade;
		}
	}
	// OPERATE或者HOLD
	else
	{
		// 目前是满仓
		if (m_import.m_isFull)
		{
			// 已经在之前更新过了，要以之前的为准，不会在这里继续更新
			if (m_isUpdateBuyParam)
			{
				return trade;
			}
			// 当前价已经到了割肉价
			if (m_directBuyPrice <= cutPrice())
			{
				updateSellParam(cutPrice(), m_directBuyObserveTime, true);
				m_spFund->sellAllForT(stock, cutPrice(), m_date, m_directBuyObserveTime);
			}
			else
			{
				// 之前没有更新过，说明挂买价没达到
				updateBuyParam(m_directBuyPrice, m_directBuyObserveTime, false);
			}
			//updateBuyParam(m_directBuyPrice, m_directBuyObserveTime, false);
		}
		// 目前是空仓
		else
		{
			updateBuyParam(m_directBuyPrice, m_directBuyObserveTime, true);
			m_spFund->buyAll(stock, m_directBuyPrice, m_date, m_directBuyObserveTime);
		}
		return m_directBuyObserveTime;
	}
}

ObserveTime WaveStrategy::checkBuy(const std::string& stock, const std::vector<int32_t>& dayInfo, ObserveTime current)
{
	if (!m_canTrade)
	{
		return ObserveTime::COUNT;
	}

	// 第五步：计算下一个时间点
	ObserveTime next = (ObserveTime)((int32_t)current + 1);

	// 第六步：获取从挂买时间到next的最低价和最高价（累积检查）
	int32_t minPrice = getMinPrice(dayInfo, m_hangBuyObserveTime, next);
	int32_t maxPrice = getMaxPrice(dayInfo, m_hangBuyObserveTime, next);
	int32_t currentPrice = getCurrentPrice(dayInfo, current);
	int32_t nextPrice = getCurrentPrice(dayInfo, next);

	// 第七步：判断是否触发挂买或反追
	bool hitBuy = (minPrice <= m_hangBuyPrice);
	bool hitChase = (maxPrice >= chasePrice());

	// 说明在上一轮出现了割肉操作
	if (m_import.m_operate == GIVE_UP)
	{
		// 除非达到反追线否则不买入
		if (hitChase)
		{
			updateBuyParam(chasePrice(), next, true);
			m_spFund->buyAll(stock, chasePrice(), m_date, next);
			return next;
		}
		return ObserveTime::COUNT;
	}

	// OPERATE或者HOLD
	if (m_import.m_isFull)
	{
		// 触发割肉
		if (minPrice <= cutPrice())
		{
			updateSellParam(cutPrice(), next, true);
			m_spFund->sellAllForT(stock, cutPrice(), m_date, next);

			// 先降再升：割肉后反追
			if (nextPrice > currentPrice)
			{
				// 需要检查下一个时间点的当前价是否达到反追价
				if (nextPrice >= chasePrice() && maxPrice >= chasePrice())
				{
					updateBuyParam(chasePrice(), next, true);
					m_spFund->buyAll(stock, chasePrice(), m_date, next);
				}
			}
			return next;
		}
		else if (hitBuy)
		{
			updateBuyParam(m_hangBuyPrice, next, false);
			return next;
		}
	}
	// 目前是空仓
	else
	{
		// 第八步：处理反追+挂买同时触发
		if (hitChase && hitBuy)
		{
			// 先降再升：优先挂买
			if (nextPrice > currentPrice)
			{
				updateBuyParam(m_hangBuyPrice, next, true);
				m_spFund->buyAll(stock, m_hangBuyPrice, m_date, next);
			}
			// 先升再降：优先反追
			else
			{
				updateBuyParam(chasePrice(), next, true);
				m_spFund->buyAll(stock, chasePrice(), m_date, next);
			}
			return next;
		}
		// 第九步：仅触发反追
		else if (hitChase)
		{
			updateBuyParam(chasePrice(), next, true);
			m_spFund->buyAll(stock, chasePrice(), m_date, next);
			return next;
		}
		// 第十步：仅触发挂买
		else if (hitBuy)
		{
			updateBuyParam(m_hangBuyPrice, next, true);
			m_spFund->buyAll(stock, m_hangBuyPrice, m_date, next);
			return next;
		}
	}
	return ObserveTime::COUNT;
}

void WaveStrategy::updateSellParam(int32_t price, ObserveTime time, bool isRealTrade)
{
	if (m_isUpdateSellParam)
	{
		return;
	}
	m_isUpdateSellParam = true;
	if (isRealTrade)
	{
		m_import.m_virtualSellPrice = price;
		m_import.m_virtualSellObserveTime = time;
		m_import.m_realSellPrice = price;
		m_import.m_isFull = false;
		m_strategyLog.push_back(CStringManager::Format("在%d %s 执行%s卖出，卖出价：%s", m_date,
			Util::observeTimeToWatchString(time), (price <= cutPrice() ? "割肉" : ""),
			(BigNumber(price) / 100.0).toPrec(2).toString().c_str()));
		if (price <= cutPrice())
		{
			++m_tradeCount[3];
		}
		else
		{
			++m_tradeCount[1];
		}
	}
	else if (price < m_import.m_virtualSellPrice)
	{
		if (m_import.m_virtualSellPrice == 0)
		{
			RCSend("警告: 虚拟卖出价格为0，可能未初始化");
		}
		m_import.m_virtualSellPrice = price;
		m_import.m_virtualSellObserveTime = time;
		m_strategyLog.push_back(CStringManager::Format("在%d %s 更新虚拟卖价，卖出价：%s", m_date,
			Util::observeTimeToWatchString(time),
			(BigNumber(price) / 100.0).toPrec(2).toString().c_str()));
	}
	if (price <= cutPrice())
	{
		m_import.m_operate = GIVE_UP;
	}
}

void WaveStrategy::updateBuyParam(int32_t price, ObserveTime time, bool isRealTrade)
{
	if (m_isUpdateBuyParam)
	{
		return;
	}
	m_isUpdateBuyParam = true;
	if (isRealTrade)
	{
		m_import.m_virtualBuyPrice = price;
		m_import.m_virtualBuyObserveTime = time;
		m_import.m_realBuyPrice = price;
		m_import.m_isFull = true;
		m_canTrade = false;
		m_strategyLog.push_back(CStringManager::Format("在%d %s 执行%s买入，买入价：%s", m_date,
			Util::observeTimeToWatchString(time), (price >= chasePrice() ? "反追" : ""),
			(BigNumber(price) / 100.0).toPrec(2).toString().c_str()));
		if (price >= chasePrice())
		{
			++m_tradeCount[2];
		}
		else
		{
			++m_tradeCount[0];
		}
	}
	else if (price > m_import.m_virtualBuyPrice)
	{
		if (m_import.m_virtualBuyPrice == 0)
		{
			RCSend("警告: 虚拟买入价格为0，可能未初始化");
		}
		m_import.m_virtualBuyPrice = price;
		m_import.m_virtualBuyObserveTime = time;
		m_strategyLog.push_back(CStringManager::Format("在%d %s 更新虚拟买价，买入价：%s", m_date,
			Util::observeTimeToWatchString(time),
			(BigNumber(price) / 100.0).toPrec(2).toString().c_str()));
	}
	if (price >= chasePrice())
	{
		m_import.m_operate = HOLD;
	}
}

int32_t WaveStrategy::chasePrice()
{
	return m_import.m_virtualSellPrice + m_chaseCent;
}

int32_t WaveStrategy::cutPrice()
{
	return m_import.m_virtualBuyPrice - m_cutCent;
}