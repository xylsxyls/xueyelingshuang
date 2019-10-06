#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"

/** ����
*/
class Strategy
{
public:
	/** ���ر�Ҫ����
	*/
	virtual void load() = 0;

	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date, BigNumber& price, BigNumber& percent) = 0;

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date, BigNumber& price, BigNumber& percent) = 0;
};