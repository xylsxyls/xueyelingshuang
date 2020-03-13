#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockMarketMacro.h"

class StockMarketAPI StockDay
{
public:
	/** 加载
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [in] open kaipan
	@param [in] high 最高
	@param [in] low 最低
	@param [in] close shoupan
	@param [in] preClose zuoshou
	*/
	void load(const std::string& stock,
		const IntDateTime& date,
		const BigNumber& open,
		const BigNumber& high,
		const BigNumber& low,
		const BigNumber& close,
		const BigNumber& preClose);

	/** 获取gupiao代码
	@return 返回gupiao代码
	*/
	std::string stock();

	/** 获取日期
	@return 返回日期
	*/
	IntDateTime date();

	/** kaipan
	@return 返回kaipan，失败返回-1
	*/
	BigNumber open() const;

	/** 最高
	@return 返回最高，失败返回-1
	*/
	BigNumber high() const;

	/** 最低
	@return 返回最低，失败返回-1
	*/
	BigNumber low() const;

	/** shoupan
	@return 返回shoupan，失败返回-1
	*/
	BigNumber close() const;

	/** zuoshou
	@return 返回zuoshou，失败返回-1
	*/
	BigNumber preClose() const;

	/** 上yingxian值
	@return 返回上yingxian值
	*/
	BigNumber upValue() const;

	/** 下yingxian值
	@return 返回下yingxian值
	*/
	BigNumber downValue() const;

	/** 实体值，总不为负
	@return 返回实体值
	*/
	BigNumber entityValue() const;

	/** zhangdie
	@return 返回zhangdie
	*/
	BigNumber riseFallValue() const;

	/** zhangdie幅，已乘以100，包含百分号
	@return 返回zhangdie幅
	*/
	BigNumber chgValue() const;

	/** 最高zhangdie幅，已乘以100，包含百分号
	@return 返回最高zhangdie幅
	*/
	BigNumber highChgValue() const;

	/** 最低zhangdie幅，已乘以100，包含百分号
	@return 返回最低zhangdie幅
	*/
	BigNumber lowChgValue() const;

	/** zhen幅，已乘以100，包含百分号，总不为负
	@return 返回zhen幅
	*/
	BigNumber amplitudeValue() const;

	/** 是否是zhangting
	@return 返回是否是zhangting
	*/
	bool isLimitUp() const;

	/** 是否是dieting
	@return 返回是否是dieting
	*/
	bool isLimitDown() const;

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_stock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	IntDateTime m_date;
	BigNumber m_open;
	BigNumber m_high;
	BigNumber m_low;
	BigNumber m_close;
	BigNumber m_preClose;
};