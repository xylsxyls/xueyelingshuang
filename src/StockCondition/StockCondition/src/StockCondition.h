#pragma once
#include "StockConditionMacro.h"
#include <map>

class StockCondition;
extern "C" StockConditionAPI StockCondition* StockConditionInterface();

class Strategy;
class StockConditionAPI StockCondition
{
public:
	enum ConditionEnum
	{
		WR_RSI
	};

private:
	/** ���캯��
	*/
	StockCondition();

	/** ��������
	*/
	virtual ~StockCondition();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockCondition& instance();

public:
	/** ��ȡ���Խӿ�
	@param [in] condition ���Ի�������
	@return ���ز��Խӿ�
	*/
	virtual Strategy* getStrategy(ConditionEnum condition = WR_RSI);

	/** �ͷ����в��Խӿ�
	*/
	virtual void destroyAll();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<ConditionEnum, Strategy*> m_strategy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};