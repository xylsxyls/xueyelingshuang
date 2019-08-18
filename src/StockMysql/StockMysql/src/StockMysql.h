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

	/** ���ص�һʵ��
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

	/** ��mysql��ȡ����gupiao����
	@return ��������gupiao����
	*/
	std::vector<std::string> allStockFromMysql() const;

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
	std::shared_ptr<std::vector<std::vector<std::string>>> readMarket(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ����hangqing��������ֵ
	*/
	void saveMarketDataIndex() const;

	/** ��ȡhangqing��������ֵ
	@return ����hangqing��������ֵ
	*/
	std::map<std::string, std::vector<int32_t>> getMarketDataIndex() const;

	/** ��ȡzhibiao
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return ����zhibiao��
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readIndicator(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ����zhibiao��������ֵ
	*/
	void saveIndicatorDataIndex() const;

	/** ��ȡzhibiao��������ֵ
	@return ����zhibiao��������ֵ
	*/
	std::map<std::string, std::vector<int32_t>> getIndicatorDataIndex() const;

	/** ��ȡ��������
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return �����������ݱ�
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readAll(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ����������������ֵ
	*/
	void saveAllDataIndex() const;

	/** ��ȡ������������ֵ
	@return ����������������ֵ
	*/
	std::map<std::string, std::vector<int32_t>> getAllDataIndex() const;

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
	@param [in] dateBegin jiaoyiri����1��1С
	@param [in] dateEnd jiaoyiri����2��2��
	@return ���ؼ������������ͷβ
	*/
	BigNumber getDays(const std::string& stock, const IntDateTime& dateBegin, const IntDateTime& dateEnd) const;

	/** ת�����ݵ�redis
	*/
	void initRedis();

	/** ��������gupiao����
	@param [in] allStock ����gupiao����
	*/
	void saveAllStock(const std::vector<std::vector<std::string>>& allStock);

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
	std::shared_ptr<std::vector<std::vector<std::string>>> redisFromMysql(const std::string& stock,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		int32_t redisDbIndex,
		const std::vector<std::string>& mysqlDbName,
		const std::vector<std::string>& mysqlFields) const;

	std::map<std::string, std::vector<int32_t>> getIndex(const std::string& indexHashMapName) const;

private:
	//���ݿ�������
	MysqlCpp m_mysql;
	//redis������
	HiRedis m_redis;
};