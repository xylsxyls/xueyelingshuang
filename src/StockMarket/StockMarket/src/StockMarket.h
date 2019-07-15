#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <memory>

class StockDay;
class StockMysql;
/** hangqing管理
*/
class StockMarketAPI StockMarket
{
public:
	/** 构造函数
	*/
	StockMarket();

public:
	/** 加载gupiao历史hangqing
	@param [in] stock gupiao代码
	@param [in] beginTime 开始加载时间，包括头
	@param [in] endTime 结束加载时间，包括尾
	*/
	void load(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** 获取gupiao代码
	@return 返回gupiao代码
	*/
	std::string stock() const;

	/** 获取gupiao名称
	@return 返回gupiao名称
	*/
	std::string name() const;

	/** 是否为空
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
	*/
	void setDate(const IntDateTime& date);

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
	//gupiao代码
	std::string m_stock;
	//gupiao名
	std::string m_name;
	//日期
	IntDateTime m_date;
	//日期，开高低收，索引对应Histroy枚举
	std::map<IntDateTime, std::shared_ptr<StockDay>> m_history;
	//数据库
	std::shared_ptr<StockMysql> m_spStockMysql;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};