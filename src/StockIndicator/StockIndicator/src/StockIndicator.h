#pragma once
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"

class StockWrIndicator;
class StockRsiIndicator;
class StockAvgIndicator;
struct StockAvg;
/** zhibiao������
*/
class StockIndicatorAPI StockIndicator
{
protected:
	/** ���캯��
	*/
	StockIndicator();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockIndicator& instance();

public:
	/** ��redis�м���ԭʼ���ݣ����������Ҫ����ͨ������zhibiao�ӿڻ�ȡʹ��
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	*/
	void loadIndicatorFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** ��redis�м���ԭʼ���ݣ����������Ҫ����ͨ������zhibiao�ӿڻ�ȡʹ��
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	*/
	void loadCalcFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** ���
	*/
	void clear();

	/** �洢wr
	*/
	void saveWr();

	/** �洢rsi
	*/
	void saveRsi();

	/** �洢ĳһ���wr
	@param [in] date ����
	*/
	void saveDateWr(const IntDateTime& date);

	/** �洢ĳһ���rsi
	@param [in] date ����
	*/
	void saveDateRsi(const IntDateTime& date);

	/** �洢avg
	@param [in] stock gupiao����
	@param [in] avgData junxian����
	*/
	void saveAvg(const std::string& stock, const std::map<IntDateTime, std::shared_ptr<StockAvg>>& avgData);

	/** ��ȡwr��ؽӿ�
	@return ����wr��ؽӿ�
	*/
	std::shared_ptr<StockWrIndicator> wr();

	/** ��ȡrsi��ؽӿ�
	@return ����rsi��ؽӿ�
	*/
	std::shared_ptr<StockRsiIndicator> rsi();

	/** ��ȡavg��ؽӿ�
	@return ����avg��ؽӿ�
	*/
	std::shared_ptr<StockAvgIndicator> avg();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_loadStock;
	std::map<std::string, std::vector<int32_t>> m_indicatorIndex;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_redisIndicatorData;
	std::map<std::string, std::vector<int32_t>> m_calcIndex;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_redisCalcData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};