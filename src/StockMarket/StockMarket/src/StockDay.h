#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockMarketMacro.h"

class StockMarketAPI StockDay
{
public:
	/** ����
	@param [in] stock gupiao����
	@param [in] date ����
	@param [in] open kaipan
	@param [in] high ���
	@param [in] low ���
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

	/** ��ȡgupiao����
	@return ����gupiao����
	*/
	std::string stock();

	/** ��ȡ����
	@return ��������
	*/
	IntDateTime date();

	/** kaipan
	@return ����kaipan��ʧ�ܷ���-1
	*/
	BigNumber open() const;

	/** ���
	@return ������ߣ�ʧ�ܷ���-1
	*/
	BigNumber high() const;

	/** ���
	@return ������ͣ�ʧ�ܷ���-1
	*/
	BigNumber low() const;

	/** shoupan
	@return ����shoupan��ʧ�ܷ���-1
	*/
	BigNumber close() const;

	/** zuoshou
	@return ����zuoshou��ʧ�ܷ���-1
	*/
	BigNumber preClose() const;

	/** ��yingxianֵ
	@return ������yingxianֵ
	*/
	BigNumber upValue() const;

	/** ��yingxianֵ
	@return ������yingxianֵ
	*/
	BigNumber downValue() const;

	/** ʵ��ֵ���ܲ�Ϊ��
	@return ����ʵ��ֵ
	*/
	BigNumber entityValue() const;

	/** zhangdie
	@return ����zhangdie
	*/
	BigNumber riseFallValue() const;

	/** zhangdie�����ѳ���100�������ٷֺ�
	@return ����zhangdie��
	*/
	BigNumber chgValue() const;

	/** ���zhangdie�����ѳ���100�������ٷֺ�
	@return �������zhangdie��
	*/
	BigNumber highChgValue() const;

	/** ���zhangdie�����ѳ���100�������ٷֺ�
	@return �������zhangdie��
	*/
	BigNumber lowChgValue() const;

	/** zhen�����ѳ���100�������ٷֺţ��ܲ�Ϊ��
	@return ����zhen��
	*/
	BigNumber amplitudeValue() const;

	/** �Ƿ���zhangting
	@return �����Ƿ���zhangting
	*/
	bool isLimitUp() const;

	/** �Ƿ���dieting
	@return �����Ƿ���dieting
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