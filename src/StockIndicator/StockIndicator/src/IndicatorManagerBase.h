#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>
#include "StockIndicatorMacro.h"

struct IndicatorBase;

class StockIndicatorAPI IndicatorManagerBase
{
public:
	/** ���캯��
	*/
	IndicatorManagerBase();

	/** ��������
	*/
	virtual ~IndicatorManagerBase();

public:
	/** ����redisԭʼ����
	@param [in] stock gupiao����
	@param [in] vecIndicator zhibiaoԭʼ����
	@param [in] dateIndex ������������ֵ
	@param [in] indicatorIndex zhibiao��������ֵ
	*/
	void setRedisData(const std::string& stock,
		const std::shared_ptr<std::vector<std::vector<std::string>>>& vecIndicator,
		int32_t dateIndex,
		const std::vector<int32_t>& indicatorIndex);

	/** ��ԭʼ���ݼ��ص���������
	*/
	virtual void load() = 0;

	/** ��ȡָ������zhibiao
	@param [in] date ����
	@return ����ָ������zhibiao
	*/
	std::shared_ptr<IndicatorBase> day(const IntDateTime& date);

	/** ���
	*/
	void clear();

	/** �Ƿ�Ϊ��
	@return �����Ƿ�Ϊ��
	*/
	bool empty();

	/** ��ȡ����
	@return ��������
	*/
	std::string type();

protected:
	/** ��������zhibiao
	*/
	virtual void calc() = 0;

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_type;
	std::string m_stock;
	std::map<IntDateTime, std::shared_ptr<IndicatorBase>> m_indicator;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_vecRedisIndicator;
	std::vector<int32_t> m_indicatorIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_dateIndex;
};