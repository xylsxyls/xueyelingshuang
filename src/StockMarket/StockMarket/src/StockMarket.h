#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <memory>

class StockDay;
class StockMysql;
/** hangqing����
*/
class StockMarketAPI StockMarket
{
public:
	/** ���캯��
	*/
	StockMarket();

public:
	/** ����gupiao��ʷhangqing
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼ����ʱ�䣬����ͷ
	@param [in] endTime ��������ʱ�䣬����β
	*/
	void load(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** ��ȡgupiao����
	@return ����gupiao����
	*/
	std::string stock() const;

	/** ��ȡgupiao����
	@return ����gupiao����
	*/
	std::string name() const;

	/** �Ƿ�Ϊ��
	*/
	bool empty() const;

	/** ���
	*/
	void clear();

	/** ��ȡ��ǰ��������ָ����������
	@return ���ص�ǰ��������ָ����������
	*/
	std::shared_ptr<StockDay> day() const;

	/** ��ȡ��ǰ��������
	@return ���ص�ǰ��������
	*/
	IntDateTime date() const;

	/** ������������
	@param [in] date ����
	*/
	void setDate(const IntDateTime& date);

	/** ������صĵ�һ��
	*/
	void setFirstDate();

	/** ������ص����һ��
	*/
	void setEndDate();

	/** ������һ��
	@return �����Ƿ����
	*/
	bool previous();

	/** ������һ��
	@return �����Ƿ����
	*/
	bool next();

public:
	//���º�������Ҫload��ֱ�Ӵ����ݿ��ж�ȡ

	/** ����gupiao����
	@param [in] stock gupiao����
	*/
	void setStock(const std::string& stock);

	/** ��ȡ���ص�ĳһ��hangqing
	@param [in] date ����
	@return ���ؼ��ص�ĳһ��hangqing
	*/
	std::shared_ptr<StockDay> stockDay(const IntDateTime& date) const;

	/** ��ǰjiaoyiri�Ƿ����
	@param [in] date ����
	@return ���ص�ǰjiaoyiri�Ƿ����
	*/
	bool dateExist(const IntDateTime& date) const;

	/** ��ȡ��һ��jiaoyiri
	@return ���ص�һ��jiaoyiri
	*/
	IntDateTime beginDate();

	/** ��ȡ���һ��jiaoyiri
	@return �������һ��jiaoyiri
	*/
	IntDateTime endDate();

	/** ��ȡ��һ��jiaoyiri����
	@param [in] date ����
	@return �����Ƿ��ȡ�ɹ�
	*/
	IntDateTime getDatePre(const IntDateTime& date) const;

	/** ��ȡ��һ��jiaoyiri����
	@param [in] date ����
	@param [out] nextDate ��һ��jiaoyiri����
	@return �����Ƿ��ȡ�ɹ�
	*/
	IntDateTime getDateNext(const IntDateTime& date) const;

	/** ��ȡ��������֮���jiaoyiri����
	@param [in] date1 jiaoyiri����1��1С
	@param [in] date2 jiaoyiri����2��2��
	@return ���ؼ������������ͷβ
	*/
	BigNumber getDays(const IntDateTime& date1, const IntDateTime& date2);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//gupiao����
	std::string m_stock;
	//gupiao��
	std::string m_name;
	//����
	IntDateTime m_date;
	//���ڣ����ߵ��գ�������ӦHistroyö��
	std::map<IntDateTime, std::shared_ptr<StockDay>> m_history;
	//���ݿ�
	std::shared_ptr<StockMysql> m_spStockMysql;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};