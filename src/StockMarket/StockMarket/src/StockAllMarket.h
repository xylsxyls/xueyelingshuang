#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "MysqlCpp/MysqlCppAPI.h"

class StockMarket;
/** �������
*/
class StockMarketAPI StockAllMarket
{
private:
	/** ���캯��
	*/
	StockAllMarket();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockAllMarket& instance();

protected:
	/** ��ʼ��
	*/
	void init();

public:
	/** ���ع�Ʊ��ʷ����
	@param [in] ��Ҫ���صĹ�Ʊ���ϣ���ձ�ʾ����ȫ��
	*/
	void load(const std::vector<std::string>& vecNeedStock = std::vector<std::string>());

	/** �洢��Ʊ��ʷ����
	@param [in] folder ��Ź�Ʊ������ļ���
	*/
	void save(const std::string& folder);

	/** ��ȡ��������
	@return ������������
	*/
	std::map<std::string, std::shared_ptr<StockMarket>> allHistory();

	/** ��ȡ���й�Ʊ
	@return �������й�Ʊ
	*/
	std::vector<std::string> allStock();

	/** ��ȡ������Ʊ����
	@param [in] stock ��Ʊ����
	@return ���ص�����Ʊ����
	*/
	std::shared_ptr<StockMarket> history(const std::string& stock);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//��������
	std::map<std::string, std::shared_ptr<StockMarket>> m_allHistory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	MysqlCpp m_mysql;
};