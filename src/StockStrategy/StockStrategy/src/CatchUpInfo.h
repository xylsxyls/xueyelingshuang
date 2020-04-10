#pragma once
#include "StrategyInfo.h"
#include <memory>
#include <set>

struct StockStrategyAPI CatchUpInfo : public StrategyInfo
{
	/** ��Сͨ������
	@return ������Сͨ������
	*/
	virtual int32_t minPollSize();

	/** ���˸���
	@return ���ع��˸���
	*/
	virtual int32_t popSize(int32_t buySize);

	/** ѡ�����
	@return ����ѡ�����
	*/
	virtual int32_t stockNum();

	/** ��ȡ��Ҫ��zhibiao
	@return ������Ҫ��zhibiao
	*/
	virtual std::set<std::string> needIndicator();
};