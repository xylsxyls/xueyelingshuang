#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "StockStrategyMacro.h"

struct StockStrategyAPI StockHandle
{
	//�������
	bool m_isBuy;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//��Ʊ����
	std::string m_stock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//���򶳽ᣬ����ⶳ
	BigNumber m_freezeOrFree;
	//�����۸�
	BigNumber m_price;
	//���򶳽����������ⶳ�����������ٶ��ᣬ�����ٽⶳ
	BigNumber m_rate;
	//����֮ǰ�ⶳ����
	BigNumber m_freeBeforeBuy;

	/** ���캯��
	*/
	StockHandle()
	{
		m_isBuy = false;
		m_freezeOrFree = 0;
		m_price = 0;
		m_rate = 0;
		m_freeBeforeBuy = 0;
	}

	/** ���캯��
	*/
	StockHandle(bool isBuy, const std::string& stock, const BigNumber& freezeOrFree, const BigNumber& price, const BigNumber& rate, const BigNumber& freeBeforeBuy)
	{
		m_isBuy = isBuy;
		m_stock = stock;
		m_freezeOrFree = freezeOrFree;
		m_price = price;
		m_rate = rate;
		m_freeBeforeBuy = freeBeforeBuy;
	}
};