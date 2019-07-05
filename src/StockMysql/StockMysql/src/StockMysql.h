#pragma once
#include "StockMysqlMacro.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

/** ���ݿ�Խ���
*/
class StockMysqlAPI StockMysql
{
protected:
	/** ���캯��
	*/
	StockMysql();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockMysql& instance();

	/** ��ʼ��
	*/
	void init();

public:
	/** ��ȡ���й�Ʊ����
	@return �������й�Ʊ����
	*/
	std::vector<std::string> allStock();

	/** �¿�һ�����ߵ��յı�
	@param [in] stock ��Ʊ����
	*/
	void createMarketHead(const std::string& stock);

	/** ���һ����Ʊ���뵽��Ʊ����
	@param [in] stock ��Ʊ����
	*/
	void addStock(const std::string& stock);

	/** �洢���ߵ���
	@param [in] stock ��Ʊ����
	@param [in] vecMarket ��Ʊ����
	*/
	void saveMarket(const std::string& stock, const std::vector<std::vector<std::string>>& vecMarket);

	/** ��ȡ���ߵ���
	@param [in] stock ��Ʊ����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return ���ؿ��ߵ��ձ�
	*/
	std::vector<std::vector<std::string>> readMarket(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

private:
	//���ݿ�������
	MysqlCpp m_mysql;
};