#pragma once
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"

class StockWrIndicator;
class StockRsiIndicator;
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
	void loadFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** ���
	*/
	void clear();

	/** ��ȡwr��ؽӿ�
	@return ����wr��ؽӿ�
	*/
	std::shared_ptr<StockWrIndicator> wr();

	/** ��ȡrsi��ؽӿ�
	@return ����rsi��ؽӿ�
	*/
	std::shared_ptr<StockRsiIndicator> rsi();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<std::vector<std::vector<std::string>>> m_redisIndicatorData;
	std::string m_loadStock;
	std::map<std::string, std::vector<int32_t>> m_indicatorIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};