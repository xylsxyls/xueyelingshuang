#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <memory>

class StockDay;
class StockMysql;
struct StockData;
/** hangqing管理
*/
class StockMarketAPI StockMarket
{
public:
	/** 构造函数
	*/
	StockMarket();

	/** 拷贝构造
	@param [in] stockMarket 拷贝类
	*/
	StockMarket(const StockMarket& stockMarket);

	/** 赋值
	@param [in] stockMarket 拷贝类
	@return 返回本类
	*/
	StockMarket operator=(const StockMarket& stockMarket);

public:
	/** 从数据库中加载数据
	@param [in] stock gupiao代码
	@param [in] beginTime 开始加载时间，包括头
	@param [in] endTime 结束加载时间，包括尾
	*/
	void loadFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** 从mysql中加载数据
	@param [in] stock gupiao代码
	*/
	void loadFromMysql(const std::string& stock);

	/** 加载gupiao历史hangqing，将数据库数据转换为可用数据，如果是空数据则不会清空原有数据
	*/
	void load();

	/** 获取gupiao代码
	@return 返回gupiao代码
	*/
	std::string stock() const;

	/** 获取gupiao名称
	@return 返回gupiao名称
	*/
	std::string name() const;

	/** hangqing是否为空
	*/
	bool empty() const;

	/** 清空
	*/
	void clear();

	/** 获取当前内置日期指定的天数据
	@return 返回当前内置日期指定的天数据
	*/
	std::shared_ptr<StockDay> day() const;

	/** 获取当前内置日期
	@return 返回当前内置日期
	*/
	IntDateTime date() const;

	/** 设入内置日期
	@param [in] date 日期
	@return 返回是否设置成功，不成功表示该日期不存在
	*/
	bool setDate(const IntDateTime& date);

	/** 设入最新有效日期，如果当天不存在则进入前一个有效日期
	@param [in] date 日期
	@return 返回是否设置成功，不成功表示该日期小于开始日期
	*/
	bool setLastDate(const IntDateTime& date);

	/** 获取内存中的间隔天数
	@param [in] date1 jiaoyiri日期1，1小
	@param [in] date2 jiaoyiri日期2，2大
	@return 返回内存中的间隔天数，包括头尾
	*/
	int32_t getMemoryDays(const IntDateTime& date1, const IntDateTime& date2);

	/** 一共有多少天
	@return 返回一共有多少天
	*/
	int32_t days();

	/** 设入加载的第一天
	*/
	void setFirstDate();

	/** 设入加载的最后一天
	*/
	void setEndDate();

	/** 进入上一天
	@return 返回是否存在
	*/
	bool previous();

	/** 进入下一天
	@return 返回是否存在
	*/
	bool next();

public:
	//以下函数不需要load，直接从数据库中读取

	/** 设置gupiao代码
	@param [in] stock gupiao代码
	*/
	void setStock(const std::string& stock);

	/** 获取加载的某一天hangqing
	@param [in] date 日期
	@return 返回加载的某一天hangqing
	*/
	std::shared_ptr<StockDay> stockDay(const IntDateTime& date) const;

	/** 当前jiaoyiri是否存在
	@param [in] date 日期
	@return 返回当前jiaoyiri是否存在
	*/
	bool dateExist(const IntDateTime& date) const;

	/** 获取第一个jiaoyiri
	@return 返回第一个jiaoyiri
	*/
	IntDateTime beginDate();

	/** 获取最后一个jiaoyiri
	@return 返回最后一个jiaoyiri
	*/
	IntDateTime endDate();

	/** 获取上一个jiaoyiri日期
	@param [in] date 日期
	@return 返回是否获取成功
	*/
	IntDateTime getDatePre(const IntDateTime& date) const;

	/** 获取下一个jiaoyiri日期
	@param [in] date 日期
	@param [out] nextDate 下一个jiaoyiri日期
	@return 返回是否获取成功
	*/
	IntDateTime getDateNext(const IntDateTime& date) const;

	/** 获取两个日期之间的jiaoyiri天数
	@param [in] date1 jiaoyiri日期1，1小
	@param [in] date2 jiaoyiri日期2，2大
	@return 返回间隔天数，包括头尾
	*/
	BigNumber getDays(const IntDateTime& date1, const IntDateTime& date2);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//加载后的数据，拷贝类共享
	std::shared_ptr<StockData> m_stockData;
	//日期
	IntDateTime m_date;
	//数据库中加载到的数据，一旦初始化结束之后会全部清空，这个数据不会被拷贝
	std::shared_ptr<std::vector<std::vector<std::string>>> m_market;
	//是否加载了上一天的hangqing，这个数据不会被拷贝
	bool m_isLoadPreDate;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};