#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <memory>

class StockDay;
class StockMysql;
struct StockData;
/** hangqing����
*/
class StockMarketAPI StockMarket
{
public:
	/** ���캯��
	*/
	StockMarket();

	/** ��������
	@param [in] stockMarket ������
	*/
	StockMarket(const StockMarket& stockMarket);

	/** ��ֵ
	@param [in] stockMarket ������
	@return ���ر���
	*/
	StockMarket operator=(const StockMarket& stockMarket);

public:
	/** �����ݿ��м�������
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼ����ʱ�䣬����ͷ
	@param [in] endTime ��������ʱ�䣬����β
	*/
	void loadFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** ��mysql�м�������
	@param [in] stock gupiao����
	*/
	void loadFromMysql(const std::string& stock);

	/** ����gupiao��ʷhangqing�������ݿ�����ת��Ϊ�������ݣ�����ǿ������򲻻����ԭ������
	*/
	void load();

	/** ��ȡgupiao����
	@return ����gupiao����
	*/
	std::string stock() const;

	/** ��ȡgupiao����
	@return ����gupiao����
	*/
	std::string name() const;

	/** hangqing�Ƿ�Ϊ��
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
	@return �����Ƿ����óɹ������ɹ���ʾ�����ڲ�����
	*/
	bool setDate(const IntDateTime& date);

	/** ����������Ч���ڣ�������첻���������ǰһ����Ч����
	@param [in] date ����
	@return �����Ƿ����óɹ������ɹ���ʾ������С�ڿ�ʼ����
	*/
	bool setLastDate(const IntDateTime& date);

	/** ��ȡ�ڴ��еļ������
	@param [in] date1 jiaoyiri����1��1С
	@param [in] date2 jiaoyiri����2��2��
	@return �����ڴ��еļ������������ͷβ
	*/
	int32_t getMemoryDays(const IntDateTime& date1, const IntDateTime& date2);

	/** һ���ж�����
	@return ����һ���ж�����
	*/
	int32_t days();

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
	//���غ�����ݣ������๲��
	std::shared_ptr<StockData> m_stockData;
	//����
	IntDateTime m_date;
	//���ݿ��м��ص������ݣ�һ����ʼ������֮���ȫ����գ�������ݲ��ᱻ����
	std::shared_ptr<std::vector<std::vector<std::string>>> m_market;
	//�Ƿ��������һ���hangqing��������ݲ��ᱻ����
	bool m_isLoadPreDate;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};