#pragma once
#include "StrategyInfo.h"
#include <memory>

struct StockStrategyAPI T0Info : public StrategyInfo
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

	/** ��Ҫ��gupiaochi
	@return ������Ҫ��gupiaochi
	*/
	virtual StockLoadInfo needStockLoadInfo();

	/** ��ȡ��Ҫ��ǰ��jiaoyiri����
	@return ������Ҫ��ǰ��jiaoyiri����
	*/
	virtual int32_t needMoveDay();
};