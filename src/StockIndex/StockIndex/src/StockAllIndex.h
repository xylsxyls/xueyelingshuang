#pragma once
#include "StockIndexMacro.h"
#include <memory>
#include "MysqlCpp/MysqlCppAPI.h"
#include <map>

class StockIndex;
class StockIndexAPI StockAllIndex
{
private:
	/** ���캯��
	*/
	StockAllIndex();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockAllIndex& instance();

protected:
	/** ��ʼ��
	*/
	void init();

public:
	/** ��ȡָ��
	@param [in] stock ��Ʊ����
	@return ����ָ��
	*/
	std::shared_ptr<StockIndex> index(const std::string& stock) const;

	/** ��ȡ����ָ��
	@return ��������ָ��
	*/
	std::map<std::string, std::shared_ptr<StockIndex>> allIndex() const;

	/** ָ��洢
	*/
	void save() const;

	/** ����
	@param [in] vecNeedStock ��Ҫ���صĹ�Ʊ���ϣ���մ������ȫ��
	*/
	void load(const std::vector<std::string>& vecNeedStock = std::vector<std::string>());

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<std::string, std::shared_ptr<StockIndex>> m_stockAllIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	MysqlCpp m_mysql;
};