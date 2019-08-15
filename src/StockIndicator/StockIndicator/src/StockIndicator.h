#pragma once
#include "StockIndicatorMacro.h"
#include <memory>
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
	/** ��redis�м���ԭʼ����
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
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	*/
	std::shared_ptr<StockWrIndicator> wr(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** ��ȡrsi��ؽӿ�
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	*/
	std::shared_ptr<StockRsiIndicator> rsi(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::vector<std::string>> m_redisIndicatorData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};