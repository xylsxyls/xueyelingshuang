#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "MysqlCpp/MysqlCppAPI.h"

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
	@param [in] name 名字
	@param [in] history 历史
	*/
	void load(const std::string& stock, const std::string& name, const std::map<IntDateTime, std::vector<BigNumber>>& history);

	/** 存储股票历史行情
	@param [in] stock 股票代码
	@param [in] mysql 数据库
	@param [in] file 下载的原始文件
	*/
	static void save(const std::string& stock, const MysqlCpp& mysql, const std::string& file = "D:\\Table.txt");

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

	/** 上影线值
	@param [in] date 日期
	@return 返回上影线值
	*/
	BigNumber upValue(const IntDateTime& date) const;

	/** 下影线值
	@param [in] date 日期
	@return 返回下影线值
	*/
	BigNumber downValue(const IntDateTime& date) const;

	/** 实体值
	@param [in] date 日期
	@return 返回实体值
	*/
	BigNumber entityValue(const IntDateTime& date) const;

	/** 涨跌
	@param [in] date 日期
	@return 返回涨跌
	*/
	BigNumber riseFallValue(const IntDateTime& date) const;

	/** 涨跌幅，已乘以100，包含百分号
	@param [in] date 日期
	@return 返回涨跌幅
	*/
	BigNumber chgValue(const IntDateTime& date) const;

	/** 是否是涨停
	@param [in] date 日期
	@return 返回是否是涨停
	*/
	bool isLimitUp(const IntDateTime& date) const;

	/** 是否是跌停
	@param [in] date 日期
	@return 返回是否是跌停
	*/
	bool isLimitDown(const IntDateTime& date) const;

	/** 当前交易日是否存在
	@param [in] date 日期
	@return 返回当前交易日是否存在
	*/
	bool dateExist(const IntDateTime& date) const;

	/** 获取第一个交易日
	@return 返回第一个交易日
	*/
	IntDateTime beginDate();

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

	/** 获取两个日期之间的交易日天数
	@param [in] date1 交易日日期1，1小
	@param [in] date2 交易日日期2，2大
	@return 返回间隔天数，包括头尾
	*/
	BigNumber getDays(const IntDateTime& date1, const IntDateTime& date2);

	/** 获取该行情历史
	@return 返回该行情历史
	*/
	std::map<IntDateTime, std::vector<BigNumber>> history() const;

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
	std::map<IntDateTime, std::vector<BigNumber>> m_history;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};