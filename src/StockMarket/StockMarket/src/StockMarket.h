#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "MysqlCpp/MysqlCppAPI.h"

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
	@param [in] name ����
	@param [in] history ��ʷ
	*/
	void load(const std::string& stock, const std::string& name, const std::map<IntDateTime, std::vector<BigNumber>>& history);

	/** �洢��Ʊ��ʷ����
	@param [in] stock ��Ʊ����
	@param [in] mysql ���ݿ�
	@param [in] file ���ص�ԭʼ�ļ�
	*/
	static void save(const std::string& stock, const MysqlCpp& mysql, const std::string& file = "D:\\Table.txt");

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

	/** ��Ӱ��ֵ
	@param [in] date ����
	@return ������Ӱ��ֵ
	*/
	BigNumber upValue(const IntDateTime& date) const;

	/** ��Ӱ��ֵ
	@param [in] date ����
	@return ������Ӱ��ֵ
	*/
	BigNumber downValue(const IntDateTime& date) const;

	/** ʵ��ֵ
	@param [in] date ����
	@return ����ʵ��ֵ
	*/
	BigNumber entityValue(const IntDateTime& date) const;

	/** �ǵ�
	@param [in] date ����
	@return �����ǵ�
	*/
	BigNumber riseFallValue(const IntDateTime& date) const;

	/** �ǵ������ѳ���100�������ٷֺ�
	@param [in] date ����
	@return �����ǵ���
	*/
	BigNumber chgValue(const IntDateTime& date) const;

	/** �Ƿ�����ͣ
	@param [in] date ����
	@return �����Ƿ�����ͣ
	*/
	bool isLimitUp(const IntDateTime& date) const;

	/** �Ƿ��ǵ�ͣ
	@param [in] date ����
	@return �����Ƿ��ǵ�ͣ
	*/
	bool isLimitDown(const IntDateTime& date) const;

	/** ��ǰ�������Ƿ����
	@param [in] date ����
	@return ���ص�ǰ�������Ƿ����
	*/
	bool dateExist(const IntDateTime& date) const;

	/** ��ȡ��һ��������
	@return ���ص�һ��������
	*/
	IntDateTime beginDate();

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

	/** ��ȡ��������֮��Ľ���������
	@param [in] date1 ����������1��1С
	@param [in] date2 ����������2��2��
	@return ���ؼ������������ͷβ
	*/
	BigNumber getDays(const IntDateTime& date1, const IntDateTime& date2);

	/** ��ȡ��������ʷ
	@return ���ظ�������ʷ
	*/
	std::map<IntDateTime, std::vector<BigNumber>> history() const;

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
	std::map<IntDateTime, std::vector<BigNumber>> m_history;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};