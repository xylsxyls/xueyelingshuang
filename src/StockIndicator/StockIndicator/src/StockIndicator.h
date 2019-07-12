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
};