#pragma once
#include "StockChargeMacro.h"
#include "BigNumber/BigNumberAPI.h"

/** ��Ʊ����ʱ˰�ռ�����
*/
class StockChargeAPI StockCharge
{
protected:
	/** ���캯��
	*/
	StockCharge();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockCharge& instance();

public:
	/** ��ʼ��Ӷ��Ĭ����2.5�����5Ԫ
	@param [in] rate Ӷ�����
	@param [in] minimum ���Ӷ��
	*/
	void init(const BigNumber& rate, const BigNumber& minimum);

	/** ��ȡ����ʱ����Ҫ����˰
	@param [in] stock ��Ʊ����
	@param [in] price ��Ʊ�۸�
	@param [in] position ������λ
	@return ��������ʱ����Ҫ����˰
	*/
	BigNumber buyFee(const std::string& stock, const BigNumber& price, const BigNumber& position);

	/** ��ȡ����ʱ����Ҫ����˰
	@param [in] stock ��Ʊ����
	@param [in] price ��Ʊ�۸�
	@param [in] position ������λ
	@return ��������ʱ����Ҫ����˰
	*/
	BigNumber sellFee(const std::string& stock, const BigNumber& price, const BigNumber& position);

protected:
	/** ��ȡ����ʱ��Ҫ��ӡ��˰���������뵽��
	@param [in] price ��Ʊ�۸�
	@param [in] position ������λ
	@return ������Ҫ����ӡ��˰
	*/
	BigNumber stampDuty(const BigNumber& price, const BigNumber& position);

	/** �����ѣ����й�Ʊ˫����ȡ���������뵽��
	@param [in] price ��Ʊ�۸�
	@param [in] position ������λ
	@return ������Ҫ���Ĺ�����
	*/
	BigNumber transferFee(const BigNumber& price, const BigNumber& position);

	/** Ӷ��˫����ȡ���������뵽��
	@param [in] price ��Ʊ�۸�
	@param [in] position ������λ
	@return ������Ҫ����Ӷ��
	*/
	BigNumber commission(const BigNumber& price, const BigNumber& position);

protected:
	//Ӷ�����
	BigNumber m_rate;
	//���Ӷ��
	BigNumber m_minimum;
};