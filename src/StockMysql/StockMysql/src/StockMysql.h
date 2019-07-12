#pragma once
#include "StockMysqlMacro.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"

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
	/** ��ȡ����gupiao����
	@return ��������gupiao����
	*/
	std::vector<std::string> allStock() const;

	/** �¿�һ��kaigaodishou�ı�
	@param [in] stock gupiao����
	*/
	void createMarketHead(const std::string& stock);

	/** ���һ��gupiao���뵽gupiao����
	@param [in] stock gupiao����
	*/
	void addStock(const std::string& stock);

	/** �洢kaigaodishou
	@param [in] stock gupiao����
	@param [in] vecMarket gupiao����
	*/
	void saveMarket(const std::string& stock, const std::vector<std::vector<std::string>>& vecMarket);

	/** ��ȡkaigaodishou
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return ����kaigaodishou��
	*/
	std::vector<std::vector<std::string>> readMarket(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ��ȡwr
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return ����wr��
	*/
	std::vector<std::vector<std::string>> readWr(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ��ȡrsi
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return ����wr��
	*/
	std::vector<std::vector<std::string>> readRsi(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ��ȡ��ǰ�����Ƿ����
	@param [in] stock gupiao����
	@param [in] date ����
	@return �����Ƿ����
	*/
	bool dateExist(const std::string& stock, const IntDateTime& date) const;

	/** ��ȡ��һ��jiaoyiri
	@param [in] stock gupiao����
	@return ���ص�һ��jiaoyiri
	*/
	IntDateTime beginDate(const std::string& stock) const;

	/** ��ȡ���һ��jiaoyiri
	@param [in] stock gupiao����
	@return �������һ��jiaoyiri
	*/
	IntDateTime endDate(const std::string& stock) const;

	/** ��ȡ��һ��jiaoyiri����
	@param [in] stock gupiao����
	@param [in] date ����
	@return ������һ��jiaoyiri����
	*/
	IntDateTime getDatePre(const std::string& stock, const IntDateTime& date) const;

	/** ��ȡ��һ��jiaoyiri����
	@param [in] stock gupiao����
	@param [in] date ����
	@return ������һ��jiaoyiri����
	*/
	IntDateTime getDateNext(const std::string& stock, const IntDateTime& date) const;

	/** ��ȡ��������֮���jiaoyiri����
	@param [in] stock gupiao����
	@param [in] date1 jiaoyiri����1��1С
	@param [in] date2 jiaoyiri����2��2��
	@return ���ؼ������������ͷβ
	*/
	BigNumber getDays(const std::string& stock, const IntDateTime& date1, const IntDateTime& date2);

private:
	//���ݿ�������
	MysqlCpp m_mysql;
};