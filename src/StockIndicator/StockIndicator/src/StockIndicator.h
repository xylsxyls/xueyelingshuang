#pragma once
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockMarket/StockMarketAPI.h"

class StockWrIndicator;
class StockRsiIndicator;
class StockSarIndicator;
class StockBollIndicator;
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

	/** �洢sar
	*/
	void saveSar();

	/** �洢boll
	*/
	void saveBoll();

	/** ����ĳһ���wr
	@param [in] date ����
	@param [out] indicatorData ����
	@param [in] allStock ����gupiao
	@param [in] vecThreadId �����߳�id
	*/
	void dateIndicator(const IntDateTime& date,
		std::map<std::string, std::vector<std::vector<std::string>>>& wrIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>& rsiIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>& sarIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>& bollIndicatorData,
		const std::vector<std::string>& allStock,
		bool loadFromMysql,
		const std::vector<uint32_t>& vecThreadId);

	/** �洢ĳһ���zhibiao
	@param [in] date ����
	*/
	void saveDateIndicator(const IntDateTime& date);

	/** �洢avg
	@param [in] stock gupiao����
	@param [in] avgData junxian����
	*/
	void saveAvg(const std::string& stock, const std::map<IntDateTime, std::shared_ptr<StockAvg>>& avgData);

	/** ����ĳһ���zhibiao��redis
	@param [in] date ����
	@param [in] useLast ����ϴ���ֵ�����ϴε�
	*/
	void updateDateIndicatorToRedis(const IntDateTime& date, bool useLast);

	/** ��ȡwr��ؽӿ�
	@return ����wr��ؽӿ�
	*/
	std::shared_ptr<StockWrIndicator> wr();

	/** ��ȡrsi��ؽӿ�
	@return ����rsi��ؽӿ�
	*/
	std::shared_ptr<StockRsiIndicator> rsi();

	/** ��ȡsar��ؽӿ�
	@return ����sar��ؽӿ�
	*/
	std::shared_ptr<StockSarIndicator> sar();

	/** ��ȡboll��ؽӿ�
	@return ����boll��ؽӿ�
	*/
	std::shared_ptr<StockBollIndicator> boll();

	/** ��ȡavg��ؽӿ�
	@return ����avg��ؽӿ�
	*/
	std::shared_ptr<StockAvgIndicator> avg();

protected:
	std::vector<std::vector<std::string>> split(const std::vector<std::string>& allStock, int32_t num = 1000);

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

	std::map<std::string, std::vector<std::vector<std::string>>> m_wrIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>> m_rsiIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>> m_sarIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>> m_bollIndicatorData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};