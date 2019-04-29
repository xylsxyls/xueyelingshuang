#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include <vector>
#include "BigNumber/BigNumberAPI.h"

/** 行情管理
*/
class StockMarketAPI StockMarket
{
public:
	enum Histroy
	{
		//开盘
		OPEN,
		//最高
		HIGH,
		//最低
		LOW,
		//收盘
		CLOSE
	};
public:
	/** 加载股票历史行情
	@param [in] stock 股票代码
	@return 返回是否加载成功
	*/
	bool load(const std::string& stock);

	/** 临时添加行情节点，用于添加当天行情
	@param [in] date 日期
	@param [in] data 数据
	*/
	void add(const IntDateTime& date, const std::vector<BigNumber>& data);

	/** 获取股票代码
	@return 返回股票代码
	*/
	std::string stock() const;

	/** 获取股票代码
	@return 返回股票名称
	*/
	std::string name() const;

	/** 开盘
	@param [in] date 日期
	@return 返回开盘，失败返回-1
	*/
	BigNumber open(const IntDateTime& date) const;

	/** 最高
	@param [in] date 日期
	@return 返回最高，失败返回-1
	*/
	BigNumber high(const IntDateTime& date) const;

	/** 最低
	@param [in] date 日期
	@return 返回最低，失败返回-1
	*/
	BigNumber low(const IntDateTime& date) const;

	/** 收盘
	@param [in] date 日期
	@return 返回收盘，失败返回-1
	*/
	BigNumber close(const IntDateTime& date) const;

	/** 上一个交易日开盘
	@param [in] date 日期
	@return 返回上一个交易日，失败返回-1
	*/
	BigNumber preOpen(const IntDateTime& date) const;

	/** 上一个交易日最高
	@param [in] date 日期
	@return 返回上一个交易日最高，失败返回-1
	*/
	BigNumber preHigh(const IntDateTime& date) const;

	/** 上一个交易日最低
	@param [in] date 日期
	@return 返回上一个交易日最高最低，失败返回-1
	*/
	BigNumber preLow(const IntDateTime& date) const;

	/** 上一个交易日收盘
	@param [in] date 日期
	@return 返回上一个交易日收盘，失败返回-1
	*/
	BigNumber preClose(const IntDateTime& date) const;

	/** 下一个交易日开盘
	@param [in] date 日期
	@return 返回下一个交易日开盘，失败返回-1
	*/
	BigNumber nextOpen(const IntDateTime& date) const;

	/** 下一个交易日最高
	@param [in] date 日期
	@return 返回下一个交易日开盘最高，失败返回-1
	*/
	BigNumber nextHigh(const IntDateTime& date) const;

	/** 下一个交易日最低
	@param [in] date 日期
	@return 返回下一个交易日最低，失败返回-1
	*/
	BigNumber nextLow(const IntDateTime& date) const;

	/** 下一个交易日收盘
	@param [in] date 日期
	@return 返回下一个交易日收盘，失败返回-1
	*/
	BigNumber nextClose(const IntDateTime& date) const;

	/** 当前交易日是否存在
	@param [in] date 日期
	@return 返回当前交易日是否存在
	*/
	bool dateExist(const IntDateTime& date) const;

	/** 获取上一个交易日日期
	@param [in] date 日期
	@param [out] preDate 上一个交易日日期
	@return 返回是否获取成功
	*/
	bool getDatePre(const IntDateTime& date, IntDateTime& preDate) const;

	/** 获取下一个交易日日期
	@param [in] date 日期
	@param [out] nextDate 下一个交易日日期
	@return 返回是否获取成功
	*/
	bool getDateNext(const IntDateTime& date, IntDateTime& nextDate) const;

	/** 获取上一个交易日的行情数据
	@param [in] date 当前交易日日期
	@param [out] preDate 上一个交易日日期
	@param [out] preData 上一个交易日数据
	@return 返回是否获取成功
	*/
	bool getMarketPre(const IntDateTime& date, IntDateTime& preDate, std::vector<BigNumber>& preData) const;

	/** 获取下一个交易日的行情数据
	@param [in] date 当前交易日日期
	@param [out] nextDate 下一个交易日日期
	@param [out] nextData 下一个交易日数据
	@return 返回是否获取成功
	*/
	bool getMarketNext(const IntDateTime& date, IntDateTime& nextDate, std::vector<BigNumber>& nextData) const;

	/** 获取近几个交易日的行情数据，包括当前交易日
	@param [in] date 当前交易日日期
	@param [in] days 天数，可以是一天
	@param [out] preDaysData 近几个交易日日期和数据
	@return 返回是否获取成功
	*/
	bool getMarketPre(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& preDaysData) const;

	/** 获取后几个交易日的行情数据，不包括当前交易日
	@param [in] date 当前交易日日期
	@param [in] days 天数，可以是一天
	@param [out] nextDaysData 后几个交易日日期和数据
	@return 返回是否获取成功
	*/
	bool getMarketNext(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& nextDaysData) const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//股票代码
	std::string m_stock;
	//股票名
	std::string m_name;
	//日期，开高低收，索引对应Histroy枚举
	std::map<IntDateTime, std::vector<BigNumber>> m_histroy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};