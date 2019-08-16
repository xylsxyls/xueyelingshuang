#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>

class StockWr;

/** wr���zhibiao
*/
class StockIndicatorAPI StockWrIndicator
{
public:
	/** ���캯��
	*/
	StockWrIndicator();

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
	void load();

	/** ��ȡָ������zhibiao
	@param [in] date ����
	@return ����ָ������zhibiao
	*/
	std::shared_ptr<StockWr> day(const IntDateTime& date);

	/** ���
	*/
	void clear();

	/** �Ƿ�Ϊ��
	@return �����Ƿ�Ϊ��
	*/
	bool empty();

protected:
	/** ��������zhibiao
	*/
	void calc();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_stock;
	std::map<IntDateTime, std::shared_ptr<StockWr>> m_indicator;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_vecRedisIndicator;
	int32_t m_dateIndex;
	std::vector<int32_t> m_indicatorIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};