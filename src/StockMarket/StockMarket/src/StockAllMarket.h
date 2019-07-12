#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "MysqlCpp/MysqlCppAPI.h"

class StockMarket;
/** hangqing����
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
	/** ����gupiao��ʷhangqing
	@param [in] ��Ҫ���ص�gupiao���ϣ���ձ�ʾ����ȫ��
	*/
	void load(const std::vector<std::string>& vecNeedStock = std::vector<std::string>());

	/** �洢gupiao��ʷhangqing
	@param [in] folder ���gupiaohangqing���ļ���
	*/
	void save(const std::string& folder);

	/** ��ȡ����hangqing
	@return ��������hangqing
	*/
	std::map<std::string, std::shared_ptr<StockMarket>> allHistory();

	/** ��ȡ����gupiao
	@return ��������gupiao
	*/
	std::vector<std::string> allStock();

	/** ��ȡ����gupiaohangqing
	@param [in] stock gupiao����
	@return ���ص���gupiaohangqing
	*/
	std::shared_ptr<StockMarket> history(const std::string& stock);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//����hangqing
	std::map<std::string, std::shared_ptr<StockMarket>> m_allHistory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	MysqlCpp m_mysql;
};