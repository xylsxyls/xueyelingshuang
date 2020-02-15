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

	/** ��ȡstockname�е���������
	@return ����stockname�е���������
	*/
	std::vector<std::vector<std::string>> stockNameDb() const;

	/** ��ȡ��stockname�л�ȡgupiao��Ӧ������
	@return ����gupiao�Ͷ�Ӧ������
	*/
	std::map<std::string, std::string> stockNameMap() const;

	/** ���һ��gupiao���뵽gupiao����
	@param [in] stock gupiao����
	*/
	void addStock(const std::string& stock);

	/** �洢kaigaodishou
	@param [in] stock gupiao����
	@param [in] vecMarket gupiao����
	@param [in] isUpdate ����Ǹ����򲻻��ڿ�ʼǰɾ�����ű��Ҿ߱����¹���
	*/
	void saveMarket(const std::string& stock, const std::vector<std::vector<std::string>>& vecMarket, bool isUpdate = false);

	/** ɾ��hangqing���ݿ�
	*/
	void deleteMarketDb();

	/** ����hangqing���ݿ�
	*/
	void createMarketDb();

	/** ��ȡkaigaodishou
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return ����kaigaodishou��
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readMarket(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ��mysql�ж�ȡkaigaodishou
	@param [in] stock gupiao����
	@return ����kaigaodishou��
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readMarketFromMysql(const std::string& stock);

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

	/** �洢��������
	@param [in] stock gupiao����
	@param [in] calcData ��������
	*/
	void saveCalc(const std::string& stock, const std::map<IntDateTime, std::vector<std::string>>& calcData);

	/** ��ȡ��������
	@param [in] stock gupiao����
	@param [in] beginTime ��ʼʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@param [in] endTime ����ʱ�䣬Ĭ�ϲ�����ʾ����ʱ��
	@return ���ؼ������ݱ�
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readCalc(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** ��ȡzhibiao��������ֵ
	@return ����zhibiao��������ֵ
	*/
	std::map<std::string, std::vector<int32_t>> getIndicatorDataIndex() const;

	/** ���������������ֵ
	*/
	void saveCalcDataIndex() const;

	/** ��ȡ������������ֵ
	@return ���ؼ�����������ֵ
	*/
	std::map<std::string, std::vector<int32_t>> getCalcDataIndex() const;

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

	/** ����ĳһ��hangqing��redis
	@param [in] stock gupiao����
	@param [in] date ����
	@param [in] market hangiqng
	*/
	void updateDateMarketToRedis(const std::string& stock, const IntDateTime& date, const std::vector<std::string>& market);

	/** ����ĳһ���zhibiao��redis
	@param [in] date ����
	@param [in] allIndicatorData ����zhibiao����
	*/
	void updateDateIndicatorToRedis(const IntDateTime& date,
		const std::map<std::string, std::map<std::string, std::vector<std::string>>>& allIndicatorData);

	/** ����0��1����ֵ��redis�����������ݵ�redis
	@param [in] date ����
	@param [in] allStock ����Ҫ���µ�gupiao����
	*/
	void updateAllDataRedis(const IntDateTime& date, const std::vector<std::string>& allStock) const;

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

	/** ����zhibiao
	@param [in] indicatorType zhibiao����
	@param [in] fields ��ͷ��Ӣ�Ķ��ŷָ���date��ͷ
	@param [in] indicatorData ����
	@param [in] isUpdate ����Ǹ������ڲ���ǰ����ɾ�����ű����Ҿ߱����¹���
	*/
	void saveIndicator(const std::string& indicatorType,
		const std::string& fields,
		const std::map<std::string, std::vector<std::vector<std::string>>>& indicatorData,
		bool isUpdate = false);

	/** ������˵�gupiao��mysql
	@param [in] filterStock ���˵�gupiao
	*/
	void saveFilterStockToMysql(const std::map<IntDateTime, std::vector<std::string>>& filterStock);

	/** ������˵�gupiao��redis
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	*/
	void saveFilterStockToRedis(const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** ��ȡ���˵�gupiao
	@param [in] date ����
	@param [out] filterStock ���˵�gupiao
	*/
	void readFilterStockFromRedis(const IntDateTime& date, std::vector<std::string>& filterStock);

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