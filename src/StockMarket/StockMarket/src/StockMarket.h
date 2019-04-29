#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include <vector>
#include "BigNumber/BigNumberAPI.h"

/** �������
*/
class StockMarketAPI StockMarket
{
public:
	enum Histroy
	{
		//����
		OPEN,
		//���
		HIGH,
		//���
		LOW,
		//����
		CLOSE
	};
public:
	/** ���ع�Ʊ��ʷ����
	@param [in] stock ��Ʊ����
	@return �����Ƿ���سɹ�
	*/
	bool load(const std::string& stock);

	/** ��ʱ�������ڵ㣬������ӵ�������
	@param [in] date ����
	@param [in] data ����
	*/
	void add(const IntDateTime& date, const std::vector<BigNumber>& data);

	/** ��ȡ��Ʊ����
	@return ���ع�Ʊ����
	*/
	std::string stock() const;

	/** ��ȡ��Ʊ����
	@return ���ع�Ʊ����
	*/
	std::string name() const;

	/** ����
	@param [in] date ����
	@return ���ؿ��̣�ʧ�ܷ���-1
	*/
	BigNumber open(const IntDateTime& date) const;

	/** ���
	@param [in] date ����
	@return ������ߣ�ʧ�ܷ���-1
	*/
	BigNumber high(const IntDateTime& date) const;

	/** ���
	@param [in] date ����
	@return ������ͣ�ʧ�ܷ���-1
	*/
	BigNumber low(const IntDateTime& date) const;

	/** ����
	@param [in] date ����
	@return �������̣�ʧ�ܷ���-1
	*/
	BigNumber close(const IntDateTime& date) const;

	/** ��һ�������տ���
	@param [in] date ����
	@return ������һ�������գ�ʧ�ܷ���-1
	*/
	BigNumber preOpen(const IntDateTime& date) const;

	/** ��һ�����������
	@param [in] date ����
	@return ������һ����������ߣ�ʧ�ܷ���-1
	*/
	BigNumber preHigh(const IntDateTime& date) const;

	/** ��һ�����������
	@param [in] date ����
	@return ������һ�������������ͣ�ʧ�ܷ���-1
	*/
	BigNumber preLow(const IntDateTime& date) const;

	/** ��һ������������
	@param [in] date ����
	@return ������һ�����������̣�ʧ�ܷ���-1
	*/
	BigNumber preClose(const IntDateTime& date) const;

	/** ��һ�������տ���
	@param [in] date ����
	@return ������һ�������տ��̣�ʧ�ܷ���-1
	*/
	BigNumber nextOpen(const IntDateTime& date) const;

	/** ��һ�����������
	@param [in] date ����
	@return ������һ�������տ�����ߣ�ʧ�ܷ���-1
	*/
	BigNumber nextHigh(const IntDateTime& date) const;

	/** ��һ�����������
	@param [in] date ����
	@return ������һ����������ͣ�ʧ�ܷ���-1
	*/
	BigNumber nextLow(const IntDateTime& date) const;

	/** ��һ������������
	@param [in] date ����
	@return ������һ�����������̣�ʧ�ܷ���-1
	*/
	BigNumber nextClose(const IntDateTime& date) const;

	/** ��ǰ�������Ƿ����
	@param [in] date ����
	@return ���ص�ǰ�������Ƿ����
	*/
	bool dateExist(const IntDateTime& date) const;

	/** ��ȡ��һ������������
	@param [in] date ����
	@param [out] preDate ��һ������������
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool getDatePre(const IntDateTime& date, IntDateTime& preDate) const;

	/** ��ȡ��һ������������
	@param [in] date ����
	@param [out] nextDate ��һ������������
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool getDateNext(const IntDateTime& date, IntDateTime& nextDate) const;

	/** ��ȡ��һ�������յ���������
	@param [in] date ��ǰ����������
	@param [out] preDate ��һ������������
	@param [out] preData ��һ������������
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool getMarketPre(const IntDateTime& date, IntDateTime& preDate, std::vector<BigNumber>& preData) const;

	/** ��ȡ��һ�������յ���������
	@param [in] date ��ǰ����������
	@param [out] nextDate ��һ������������
	@param [out] nextData ��һ������������
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool getMarketNext(const IntDateTime& date, IntDateTime& nextDate, std::vector<BigNumber>& nextData) const;

	/** ��ȡ�����������յ��������ݣ�������ǰ������
	@param [in] date ��ǰ����������
	@param [in] days ������������һ��
	@param [out] preDaysData ���������������ں�����
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool getMarketPre(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& preDaysData) const;

	/** ��ȡ�󼸸������յ��������ݣ���������ǰ������
	@param [in] date ��ǰ����������
	@param [in] days ������������һ��
	@param [out] nextDaysData �󼸸����������ں�����
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool getMarketNext(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& nextDaysData) const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//��Ʊ����
	std::string m_stock;
	//��Ʊ��
	std::string m_name;
	//���ڣ����ߵ��գ�������ӦHistroyö��
	std::map<IntDateTime, std::vector<BigNumber>> m_histroy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};