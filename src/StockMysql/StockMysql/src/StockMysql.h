#pragma once
#include "StockMysqlMacro.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <memory>
#include "HiRedis/HiRedisAPI.h"

/** ���ݿ�Խ���
*/
class StockMysqlAPI StockMysql
{
public:
	/** ���캯��
	*/
	StockMysql();

public:
	/** ����һ��ʵ��
	@return ����ʵ������Ψһ
	*/
	static std::shared_ptr<StockMysql> newCase();

	/** ��ʼ��
	*/
	void init();

public:
	/** ��ȡ����gupiao����
	@return ��������gupiao����
	*/
	std::vector<std::string> allStock() const;

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

	std::vector<std::vector<std::string>> readTest(const std::string& stock,
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

protected:
	/** �¿�һ��kaigaodishou�ı�
	@param [in] stock gupiao����
	*/
	void createMarketHead(const std::string& stock);

	/** ��redis��ȡ���ݣ�û�����mysql����ֲ��redis
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] redisDbIndex ѡ������redis���ݿ�����
	@param [in] mysqlDbName Դ�����������ݿ���
	@param [in] mysqlFields ȡ��Դ������Ҫ���ֶΣ�Ӣ�Ķ��ŷָ�
	@return ���ر�
	*/
	std::vector<std::vector<std::string>> redisFromMysql(const std::string& stock,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		int32_t redisDbIndex,
		const std::string& mysqlDbName,
		const std::string& mysqlFields) const;

private:
	//���ݿ�������
	MysqlCpp m_mysql;
	//redis������
	HiRedis m_redis;
};