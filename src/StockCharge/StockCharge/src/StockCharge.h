#pragma once
#include "StockChargeMacro.h"
#include "BigNumber/BigNumberAPI.h"

/** 股票交易时税收计算类
*/
class StockChargeAPI StockCharge
{
protected:
	/** 构造函数
	*/
	StockCharge();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockCharge& instance();

public:
	/** 初始化佣金，默认万2.5，最低5元
	@param [in] rate 佣金比例
	@param [in] minimum 最低佣金
	*/
	void init(const BigNumber& rate, const BigNumber& minimum);

	/** 获取买入时候需要交的税
	@param [in] stock 股票代码
	@param [in] price 股票价格
	@param [in] position 卖出仓位
	@return 返回买入时候需要交的税
	*/
	BigNumber buyFee(const std::string& stock, const BigNumber& price, const BigNumber& position);

	/** 获取卖出时候需要交的税
	@param [in] stock 股票代码
	@param [in] price 股票价格
	@param [in] position 卖出仓位
	@return 返回卖出时候需要交的税
	*/
	BigNumber sellFee(const std::string& stock, const BigNumber& price, const BigNumber& position);

protected:
	/** 获取卖出时需要的印花税，四舍五入到分
	@param [in] price 股票价格
	@param [in] position 卖出仓位
	@return 返回需要交的印花税
	*/
	BigNumber stampDuty(const BigNumber& price, const BigNumber& position);

	/** 过户费，沪市股票双向收取，四舍五入到分
	@param [in] price 股票价格
	@param [in] position 卖出仓位
	@return 返回需要交的过户费
	*/
	BigNumber transferFee(const BigNumber& price, const BigNumber& position);

	/** 佣金，双向收取，四舍五入到分
	@param [in] price 股票价格
	@param [in] position 卖出仓位
	@return 返回需要交的佣金
	*/
	BigNumber commission(const BigNumber& price, const BigNumber& position);

protected:
	//佣金比例
	BigNumber m_rate;
	//最低佣金
	BigNumber m_minimum;
};