#include "WaveStrategy.h"
#include <algorithm>
#include <iostream>
#include "Util.h"

// 操作类型枚举定义
enum
{
	OPERATE, ///< 操作模式：上午卖出下午买入
	HOLD,    ///< 持有模式：上午不卖，下午买入
	GIVE_UP  ///< 放弃模式：如果当前空仓：当天不买入；如果当前满仓：当天下午卖出（已经买入则第二天早晨卖出）
};

WaveStrategy::WaveStrategy() :
	m_operate(OPERATE),
	m_virtualSellPrice(0),
	m_virtualSellObserveTime(ObserveTime::COUNT),
	m_virtualBuyPrice(0),
	m_virtualBuyObserveTime(ObserveTime::COUNT),
	m_realSellPrice(0),
	m_realBuyPrice(0),
	m_isFull(false),
	m_isCurrentDayBuy(false),
	m_hasFirstBuy(false)
{
	// 初始化策略模式和名称
	m_mode = StrategyMode::WAVE;
	m_modeName = "Wave";
}

WaveStrategy::~WaveStrategy()
{
	// 析构函数，无需特殊清理
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

	// 第三步：获取当天分时数据，如果为空则记录错误并返回false
	const std::vector<int32_t>& dayInfo = m_spMarket->getStockData(stock, date);
	if (dayInfo.empty())
	{
		RCSend("当天分时数据为空: dayInfo empty, date = %u", date);
		return false;
	}

	// 第四步：解析策略参数，包括各种时间点和差价阈值
	ObserveTime hangSellObserveTime = (ObserveTime)m_strategyParam[0]; // 挂卖价时间点
	ObserveTime directSellObserveTime = (ObserveTime)m_strategyParam[1]; // 直卖价时间点
	ObserveTime hangBuyObserveTime = (ObserveTime)m_strategyParam[2]; // 挂买价时间点
	ObserveTime directBuyObserveTime = (ObserveTime)m_strategyParam[3]; // 直买价时间点
	ObserveTime giveUpHangSellObserveTime = hangBuyObserveTime; // 放弃挂卖价时间点
	ObserveTime giveUpDirectSellObserveTime = directBuyObserveTime; // 放弃直卖价时间点
	ObserveTime aObserveTime = (ObserveTime)m_strategyParam[4]; // 观察价A时间点
	ObserveTime bObserveTime = (ObserveTime)m_strategyParam[5]; // 观察价B时间点
	int32_t holdCent = m_strategyParam[6]; // 观察价B-A大于等于多少时当天直买，第二天早上不卖
	int32_t giveUpCent = m_strategyParam[7]; // 观察价B-A小于等于多少时，当天不买，如果当前持有则当天下午卖出，如果已经买入则第二天早晨卖出
	int32_t chaseCent = m_strategyParam[8]; // 当天早上卖出后反追差价
	int32_t cutCent = m_strategyParam[9]; // 割肉差价
	bool isNormal = (m_strategyParam[10] == 1); // 是否为常规走势

	// 第五步：验证时间点顺序和参数逻辑，如果无效则返回false
	if ((int32_t)directSellObserveTime < (int32_t)hangSellObserveTime ||
		(int32_t)directSellObserveTime - (int32_t)hangSellObserveTime > 3 ||
		(int32_t)directBuyObserveTime < (int32_t)hangBuyObserveTime ||
		(int32_t)directBuyObserveTime - (int32_t)hangBuyObserveTime > 3 ||
		(int32_t)hangBuyObserveTime <= ((int32_t)directSellObserveTime + 1) ||
		bObserveTime <= aObserveTime ||
		(int32_t)bObserveTime > (int32_t)hangBuyObserveTime ||
		giveUpCent >= holdCent)
	{
		return false;
	}

	// 第六步：获取各种价格指标，包括开盘价和收盘价
	int32_t openPrice = dayInfo[(int32_t)Overall::OPEN]; // 开盘价（9:25数据）
	int32_t closePrice = dayInfo[(int32_t)Overall::CLOSE]; // 收盘价
	int32_t hangSellPrice = getCurrentPrice(dayInfo, hangSellObserveTime);
	int32_t directSellPrice = getDirectSellPrice(dayInfo, directSellObserveTime);
	int32_t hangBuyPrice = getCurrentPrice(dayInfo, hangBuyObserveTime) - 1; // 根据描述-1
	int32_t directBuyPrice = getDirectBuyPrice(dayInfo, directBuyObserveTime);
	int32_t giveUpHangSellPrice = getCurrentPrice(dayInfo, giveUpHangSellObserveTime);
	int32_t giveUpDirectSellPrice = getDirectSellPrice(dayInfo, giveUpDirectSellObserveTime);
	int32_t aPrice = getCurrentPrice(dayInfo, aObserveTime);
	int32_t bPrice = getCurrentPrice(dayInfo, bObserveTime);

	// 第七步：处理首次买入逻辑，如果是第一天则买入并设置标志
	if (!m_hasFirstBuy)
	{
		updateOperate(aPrice, bPrice);
		m_spFund->buyAll(stock, closePrice, date, ObserveTime::COUNT); // 收盘买入
		updateBuyParam(closePrice, ObserveTime::COUNT, true);
		m_hasFirstBuy = true;
		return true;
	}

	// 第八步：检查持仓，如果无持仓则记录错误并返回false
	std::shared_ptr<Position> spPosition = m_spFund->getPosition(stock);
	if (spPosition == nullptr)
	{
		RCSend("未找到持仓信息: hasPosition false");
		return false;
	}

	// 第九步：重置当天买入标志
	m_isCurrentDayBuy = false;

	// 第十步：开盘检查（割肉或反追）
	openCheck(stock, date, dayInfo, chaseCent, cutCent);

	// 第十一步：上午从0930到挂卖时间监控割肉或反追
	// 注意：不使用canRealTrade变量缓存，因为m_isCurrentDayBuy会在执行过程中改变
	timeCheck(ObserveTime::TIME0930, hangSellObserveTime, stock, date, dayInfo, chaseCent, cutCent, !m_isCurrentDayBuy);

	// 第十二步：执行挂卖到直卖（真实或模拟交易）
	ObserveTime operateEndTime = hangToDirectSell(stock, date, dayInfo, chaseCent, cutCent,
		hangSellObserveTime, hangSellPrice,
		directSellObserveTime, directSellPrice, !m_isCurrentDayBuy);

	// 第十三步：上午从卖出时间到B点监控割肉或反追
	timeCheck(operateEndTime, bObserveTime,
		stock, date, dayInfo, chaseCent, cutCent, !m_isCurrentDayBuy);

	// 第十四步：B点出来后更新操作类型
	updateOperate(aPrice, bPrice);

	// 第十五步：从B点到挂买时间监控割肉或反追
	timeCheck(bObserveTime, hangBuyObserveTime,
		stock, date, dayInfo, chaseCent, cutCent, !m_isCurrentDayBuy);

	// 第十六步：根据操作模式执行买入或放弃卖出
	if (m_operate == HOLD || m_operate == OPERATE)
	{
		operateEndTime = hangToDirectBuy(stock, date, dayInfo, chaseCent, cutCent,
			hangBuyObserveTime, hangBuyPrice,
			directBuyObserveTime, directBuyPrice, !m_isCurrentDayBuy);
	}
	else // GIVE_UP模式
	{
		operateEndTime = hangToDirectSell(stock, date, dayInfo, chaseCent, cutCent,
			giveUpHangSellObserveTime, giveUpHangSellPrice,
			giveUpDirectSellObserveTime, giveUpDirectSellPrice, !m_isCurrentDayBuy);
	}

	// 第十七步：下午从操作结束时间到收盘监控割肉或反追
	timeCheck(operateEndTime, ObserveTime::COUNT, stock, date, dayInfo, chaseCent, cutCent, !m_isCurrentDayBuy);

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

void WaveStrategy::updateOperate(int32_t sellPrice, int32_t buyPrice)
{
	// 第一步：获取持仓和放弃阈值参数
	int32_t holdCent = m_strategyParam[6];
	int32_t giveUpCent = m_strategyParam[7];
	bool isNormal = (m_strategyParam[10] == 1);

	// 第二步：计算价格差
	int32_t tPrice = buyPrice - sellPrice;

	// 第三步：根据价格差和isNormal更新操作模式
	if (tPrice >= holdCent)
	{
		m_operate = isNormal ? HOLD : GIVE_UP;
	}
	else if (tPrice <= giveUpCent)
	{
		m_operate = isNormal ? GIVE_UP : HOLD;
	}
	else
	{
		m_operate = OPERATE;
	}
}

bool WaveStrategy::openCheck(const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
							 int32_t chaseCent, int32_t cutCent)
{
	// 第一步：获取开盘价
	int32_t openPrice = dayInfo[(int32_t)Overall::OPEN];

	// 第二步：如果满仓，检查是否触发割肉
	if (m_isFull)
	{
		int32_t cutPrice = m_virtualBuyPrice - cutCent;
		// 使用开盘价判断是否触发割肉
		if (openPrice <= cutPrice)
		{
			// 第三步：执行割肉卖出，使用开盘价交易（因为开盘时可能跳空，只有开盘价可交易）
			m_spFund->sellAllForT(stock, openPrice, date, ObserveTime::TIME0930);
			updateSellParam(openPrice, ObserveTime::TIME0930, true);
			m_operate = GIVE_UP;
			return true;
		}
	}
	// 第四步：如果空仓，检查是否触发反追
	else
	{
		int32_t chasePrice = m_virtualSellPrice + chaseCent;
		if (openPrice >= chasePrice)
		{
			// 第五步：执行反追买入
			m_spFund->buyAll(stock, openPrice, date, ObserveTime::TIME0930);
			updateBuyParam(openPrice, ObserveTime::TIME0930, true);
			m_operate = HOLD;
			return true;
		}
	}

	// 第六步：未触发任何操作，返回false
	return false;
}

bool WaveStrategy::timeCheck(ObserveTime beginTime, ObserveTime endTime,
							 const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
							 int32_t chaseCent, int32_t cutCent, bool realTrade)
{
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
		int32_t cutPrice = m_virtualBuyPrice - cutCent;
		int32_t chasePrice = m_virtualSellPrice + chaseCent;

		// 第六步：判断是否触发割肉（满仓时）
		if (minPrice <= cutPrice && m_isFull)
		{
			if (realTrade)
			{
				m_spFund->sellAllForT(stock, cutPrice, date, next);
				updateSellParam(cutPrice, next, realTrade);
			}
			m_operate = GIVE_UP;
			
			// 第七步：割肉后检查是否需要反追
			// 重新计算反追价（使用更新后的卖出价格）
			int32_t newChasePrice = m_virtualSellPrice + chaseCent;
			// 需要检查下一个时间点的当前价是否达到反追价
			int32_t nextPrice = getCurrentPrice(dayInfo, next);
			if (nextPrice >= newChasePrice && maxPrice >= newChasePrice)
			{
				if (realTrade)
				{
					m_spFund->buyAll(stock, newChasePrice, date, next);
					updateBuyParam(newChasePrice, next, realTrade);
				}
				m_operate = HOLD;
				return true;
			}
		}
		// 第八步：判断是否触发反追（空仓时）
		else if (maxPrice >= chasePrice && !m_isFull)
		{
			if (realTrade)
			{
				m_spFund->buyAll(stock, chasePrice, date, next);
				updateBuyParam(chasePrice, next, realTrade);
			}
			m_operate = HOLD;
			return true;
		}

		// 第九步：更新当前时间，继续下一段检查
		current = next;
	}

	// 第十步：整个区间未触发，返回false
	return false;
}

ObserveTime WaveStrategy::hangToDirectSell(const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
	int32_t chaseCent, int32_t cutCent,
	ObserveTime hangSellObserveTime, int32_t hangSellPrice,
	ObserveTime directSellObserveTime, int32_t directSellPrice,
	bool realTrade)
{
	// 第一步：如果不是满仓或者操作模式为HOLD，不执行卖出
	if (!m_isFull || m_operate == HOLD)
	{
		return hangSellObserveTime;
	}
	
	// 第二步：提前计算割肉价（基于当前虚拟买价，在卖出前不会变）
	int32_t cutPrice = m_virtualBuyPrice - cutCent;

	// 第三步：如果挂卖时间等于直卖时间，直接执行直卖
	if (hangSellObserveTime == directSellObserveTime)
	{
		if (realTrade)
		{
			m_spFund->sellAllForT(stock, directSellPrice, date, directSellObserveTime);
		}
		updateSellParam(directSellPrice, directSellObserveTime, realTrade);

		// 第四步：直卖后检查卖出价格是否低于割肉价，如果低于则改变操作模式为GIVE_UP
		if (directSellPrice <= cutPrice)
		{
			m_operate = GIVE_UP;
		}
		return directSellObserveTime;
	}

	// 第五步：从挂卖时间开始逐段检查
	ObserveTime current = hangSellObserveTime;

	while ((int32_t)current < (int32_t)directSellObserveTime)
	{
		// 第六步：计算下一个时间点
		ObserveTime next = (ObserveTime)((int32_t)current + 1);

		// 第七步：获取从挂卖时间到next的最高价和最低价（累积检查）
		int32_t maxPrice = getMaxPrice(dayInfo, hangSellObserveTime, next);
		int32_t minPrice = getMinPrice(dayInfo, hangSellObserveTime, next);

		// 第八步：判断是否触发挂卖或割肉
		bool hitSell = (maxPrice >= hangSellPrice);
		bool hitCut = (minPrice <= cutPrice);

		// 第九步：处理割肉+挂卖同时触发
		if (hitCut && hitSell)
		{
			int32_t currentPrice = getCurrentPrice(dayInfo, current);
			int32_t nextPrice = getCurrentPrice(dayInfo, next);

			if (nextPrice >= currentPrice) // 先降再升：优先割肉
			{
				if (realTrade)
				{
					m_spFund->sellAllForT(stock, cutPrice, date, next);
				}
				updateSellParam(cutPrice, next, realTrade);
				m_operate = GIVE_UP;
				
				// 第十步：割肉后检查是否需要反追
				int32_t newChasePrice = m_virtualSellPrice + chaseCent;
				// 需要检查下一个时间点的当前价是否达到反追价
				if (nextPrice >= newChasePrice && maxPrice >= newChasePrice)
				{
					if (realTrade)
					{
						m_spFund->buyAll(stock, newChasePrice, date, next); 
					}
					updateBuyParam(newChasePrice, next, realTrade);
					m_operate = HOLD;
				}
			}
			else // 先升再降：优先挂卖
			{
				if (realTrade)
				{
					m_spFund->sellAllForT(stock, hangSellPrice, date, next);
				}
				updateSellParam(hangSellPrice, next, realTrade);

				// 第十一步：挂卖后检查是否需要反追
				int32_t chasePrice = m_virtualSellPrice + chaseCent;
				if (maxPrice >= chasePrice)
				{
					if (realTrade)
					{
						m_spFund->buyAll(stock, chasePrice, date, next);
					}
					updateBuyParam(chasePrice, next, realTrade);
					m_operate = HOLD;
				}
			}
			return next;
		}
		// 第十二步：仅触发割肉
		else if (hitCut)
		{
			if (realTrade)
			{
				m_spFund->sellAllForT(stock, cutPrice, date, next);
			}
			updateSellParam(cutPrice, next, realTrade);
			m_operate = GIVE_UP;

			// 第十三步：割肉后检查是否需要反追
			int32_t newChasePrice = m_virtualSellPrice + chaseCent;
			int32_t nextPrice = getCurrentPrice(dayInfo, next);
			if (nextPrice >= newChasePrice && maxPrice >= newChasePrice)
			{
				if (realTrade)
				{
					m_spFund->buyAll(stock, newChasePrice, date, next);
				}
				updateBuyParam(newChasePrice, next, realTrade);
				m_operate = HOLD;
			}
			return next;
		}
		// 第十四步：仅触发挂卖
		else if (hitSell)
		{
			if (realTrade)
			{
				m_spFund->sellAllForT(stock, hangSellPrice, date, next);
			}
			updateSellParam(hangSellPrice, next, realTrade);
			
			// 第十五步：挂卖后检查是否需要反追
			int32_t chasePrice = m_virtualSellPrice + chaseCent;
			if (maxPrice >= chasePrice)
			{
				if (realTrade)
				{
					m_spFund->buyAll(stock, chasePrice, date, next);
					updateBuyParam(chasePrice, next, realTrade);
					m_operate = HOLD;
				}
			}
			return next;
		}

		// 第十六步：未触发任何操作，继续检查下一段
		current = next;
	}

	// 第十七步：未在挂卖期间卖出，使用直卖价强制卖出
	if (realTrade)
	{
		m_spFund->sellAllForT(stock, directSellPrice, date, directSellObserveTime);
	}
	updateSellParam(directSellPrice, directSellObserveTime, realTrade);

	// 第十八步：直卖后检查卖出价格是否低于割肉价，如果低于则改变操作模式为GIVE_UP
	if (directSellPrice <= cutPrice)
	{
		m_operate = GIVE_UP;
	}

	return directSellObserveTime;
}

ObserveTime WaveStrategy::hangToDirectBuy(const std::string& stock, uint32_t date, const std::vector<int32_t>& dayInfo,
	int32_t chaseCent, int32_t cutCent,
	ObserveTime hangBuyObserveTime, int32_t hangBuyPrice,
	ObserveTime directBuyObserveTime, int32_t directBuyPrice,
	bool realTrade)
{
	// 第一步：定义反追价变量，增加可读性
	int32_t chasePrice = m_virtualSellPrice + chaseCent;
	
	// 第二步：如果挂买时间等于直买时间，直接执行直买
	if (hangBuyObserveTime == directBuyObserveTime)
	{
		if (realTrade)
		{
			m_spFund->buyAll(stock, directBuyPrice, date, directBuyObserveTime);
		}
		updateBuyParam(directBuyPrice, directBuyObserveTime, realTrade);
		// 第三步：直买后检查是否触发反追条件
		if (directBuyPrice >= chasePrice)
		{
			m_operate = HOLD;
		}
		return directBuyObserveTime;
	}

	// 第四步：从挂买时间开始逐段检查
	ObserveTime current = hangBuyObserveTime;

	while ((int32_t)current < (int32_t)directBuyObserveTime)
	{
		// 第五步：计算下一个时间点
		ObserveTime next = (ObserveTime)((int32_t)current + 1);

		// 第六步：获取从挂买时间到next的最低价和最高价（累积检查）
		int32_t minPrice = getMinPrice(dayInfo, hangBuyObserveTime, next);
		int32_t maxPrice = getMaxPrice(dayInfo, hangBuyObserveTime, next);

		// 第七步：判断是否触发挂买或反追
		bool hitBuy = (minPrice <= hangBuyPrice);
		bool hitChase = (maxPrice >= chasePrice);

		// 第八步：处理反追+挂买同时触发
		if (hitChase && hitBuy)
		{
			int32_t currentPrice = getCurrentPrice(dayInfo, current);
			int32_t nextPrice = getCurrentPrice(dayInfo, next);

			if (nextPrice > currentPrice) // 先降再升：优先挂买
			{
				if (realTrade)
				{
					m_spFund->buyAll(stock, hangBuyPrice, date, next);
				}
				updateBuyParam(hangBuyPrice, next, realTrade);
			}
			else // 先升再降：优先反追
			{
				if (realTrade)
				{
					m_spFund->buyAll(stock, chasePrice, date, next);
				}
				updateBuyParam(chasePrice, next, realTrade);
				m_operate = HOLD;
			}
			return next;
		}
		// 第九步：仅触发反追
		else if (hitChase)
		{
			if (realTrade)
			{
				m_spFund->buyAll(stock, chasePrice, date, next);
			}
			updateBuyParam(chasePrice, next, realTrade);
			m_operate = HOLD;
			return next;
		}
		// 第十步：仅触发挂买
		else if (hitBuy)
		{
			if (realTrade)
			{
				m_spFund->buyAll(stock, hangBuyPrice, date, next);
			}
			updateBuyParam(hangBuyPrice, next, realTrade);
			return next;
		}

		// 第十一步：未触发任何操作，继续检查下一段
		current = next;
	}

	// 第十二步：未在挂买期间买入，使用直买价强制买入
	if (realTrade)
	{
		m_spFund->buyAll(stock, directBuyPrice, date, directBuyObserveTime);
	}
	updateBuyParam(directBuyPrice, directBuyObserveTime, realTrade);
	// 第十三步：直买后检查是否触发反追条件
	if (directBuyPrice >= chasePrice)
	{
		m_operate = HOLD;
	}

	return directBuyObserveTime;
}

void WaveStrategy::updateSellParam(int32_t price, ObserveTime time, bool realTrade)
{
	if (realTrade)
	{
		m_virtualSellPrice = price;
		m_virtualSellObserveTime = time;
		m_realSellPrice = price;
		m_isFull = false;
		// 注意：不重置m_isCurrentDayBuy，因为A股当天买入后不能卖出
	}
	else if (price < m_virtualSellPrice)
	{
		if (m_virtualSellPrice == 0)
		{
			RCSend("警告: 虚拟卖出价格为0，可能未初始化");
		}
		m_virtualSellPrice = price;
		m_virtualSellObserveTime = time;
	}
}

void WaveStrategy::updateBuyParam(int32_t price, ObserveTime time, bool realTrade)
{
	if (realTrade)
	{
		m_virtualBuyPrice = price;
		m_virtualBuyObserveTime = time;
		m_realBuyPrice = price;
		m_isFull = true;
		m_isCurrentDayBuy = true;
	}
	else if (price > m_virtualBuyPrice)
	{
		if (m_virtualBuyPrice == 0)
		{
			RCSend("警告: 虚拟买入价格为0，可能未初始化");
		}
		m_virtualBuyPrice = price;
		m_virtualBuyObserveTime = time;
	}
}