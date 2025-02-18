#pragma once
#include "StockMysqlMacro.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <memory>
#include "HiRedis/HiRedisAPI.h"

/** 数据库对接类
*/
class StockMysqlAPI StockMysql
{
public:
	/** 构造函数
	*/
	StockMysql();

public:
	/** 生成一个实例
	@return 返回实例，不唯一
	*/
	static std::shared_ptr<StockMysql> newCase();

	/** 返回单一实例
	@return 返回单一实例
	*/
	static StockMysql& instance();

	/** 初始化
	*/
	void init();

public:
	/** 获取所有gupiao代码
	@return 返回所有gupiao代码
	*/
	std::vector<std::string> allStock() const;

	/** 从mysql获取所有gupiao代码
	@return 返回所有gupiao代码
	*/
	std::vector<std::string> allStockFromMysql() const;

	/** 获取stockname中的所有数据
	@return 返回stockname中的所有数据
	*/
	std::vector<std::vector<std::string>> stockNameDb() const;

	/** 获取从stockname中获取gupiao对应的名字
	@return 返回gupiao和对应的名字
	*/
	std::map<std::string, std::string> stockNameMap() const;

	/** 添加一个gupiao代码到gupiao集合
	@param [in] stock gupiao代码
	*/
	void addStock(const std::string& stock);

	/** 存储kaigaodishou
	@param [in] stock gupiao代码
	@param [in] vecMarket gupiao代码
	@param [in] isUpdate 如果是更新则不会在开始前删除整张表并且具备更新功能
	*/
	void saveMarket(const std::string& stock, const std::vector<std::vector<std::string>>& vecMarket, bool isUpdate = false);

	/** 删除hangqing数据库
	*/
	void deleteMarketDb();

	/** 创建hangqing数据库
	*/
	void createMarketDb();

	/** 读取kaigaodishou
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回kaigaodishou表
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readMarket(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** 从mysql中读取kaigaodishou
	@param [in] stock gupiao代码
	@return 返回kaigaodishou表
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readMarketFromMysql(const std::string& stock);

	/** 保存hangqing数据索引值
	*/
	void saveMarketDataIndex() const;

	/** 获取hangqing数据索引值
	@return 返回hangqing数据索引值
	*/
	std::map<std::string, std::vector<int32_t>> getMarketDataIndex() const;

	/** 读取zhibiao
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回zhibiao表
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readIndicator(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** 保存zhibiao数据索引值
	*/
	void saveIndicatorDataIndex() const;

	/** 存储计算数据
	@param [in] stock gupiao代码
	@param [in] calcData 计算数据
	*/
	void saveCalc(const std::string& stock, const std::map<IntDateTime, std::vector<std::string>>& calcData);

	/** 读取计算数据
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回计算数据表
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readCalc(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** 获取zhibiao数据索引值
	@return 返回zhibiao数据索引值
	*/
	std::map<std::string, std::vector<int32_t>> getIndicatorDataIndex() const;

	/** 保存计算数据索引值
	*/
	void saveCalcDataIndex() const;

	/** 获取计算数据索引值
	@return 返回计算数据索引值
	*/
	std::map<std::string, std::vector<int32_t>> getCalcDataIndex() const;

	/** 读取所有数据
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回所有数据表
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> readAll(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** 保存所有数据索引值
	*/
	void saveAllDataIndex() const;

	/** 获取所有数据索引值
	@return 返回所有数据索引值
	*/
	std::map<std::string, std::vector<int32_t>> getAllDataIndex() const;

	/** 更新某一天hangqing到redis
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [in] market hangiqng
	*/
	void updateDateMarketToRedis(const std::string& stock, const IntDateTime& date, const std::vector<std::string>& market);

	/** 更新某一天的zhibiao到redis
	@param [in] date 日期
	@param [in] allIndicatorData 所有zhibiao数据
	*/
	void updateDateIndicatorToRedis(const IntDateTime& date,
		const std::map<std::string, std::map<std::string, std::vector<std::string>>>& allIndicatorData);

	/** 根据0和1索引值的redis更新所有数据的redis
	@param [in] date 日期
	@param [in] allStock 所有要更新的gupiao集合
	*/
	void updateAllDataRedis(const IntDateTime& date, const std::vector<std::string>& allStock) const;

	/** 获取当前日期是否存在
	@param [in] stock gupiao代码
	@param [in] date 日期
	@return 返回是否存在
	*/
	bool dateExist(const std::string& stock, const IntDateTime& date) const;

	/** 获取第一个jiaoyiri
	@param [in] stock gupiao代码
	@return 返回第一个jiaoyiri
	*/
	IntDateTime beginDate(const std::string& stock) const;

	/** 获取最后一个jiaoyiri
	@param [in] stock gupiao代码
	@return 返回最后一个jiaoyiri
	*/
	IntDateTime endDate(const std::string& stock) const;

	/** 获取上一个jiaoyiri日期
	@param [in] stock gupiao代码
	@param [in] date 日期
	@return 返回上一个jiaoyiri日期
	*/
	IntDateTime getDatePre(const std::string& stock, const IntDateTime& date) const;

	/** 获取下一个jiaoyiri日期
	@param [in] stock gupiao代码
	@param [in] date 日期
	@return 返回下一个jiaoyiri日期
	*/
	IntDateTime getDateNext(const std::string& stock, const IntDateTime& date) const;

	/** 获取两个日期之间的jiaoyiri天数
	@param [in] stock gupiao代码
	@param [in] dateBegin jiaoyiri日期1，1小
	@param [in] dateEnd jiaoyiri日期2，2大
	@return 返回间隔天数，包括头尾
	*/
	BigNumber getDays(const std::string& stock, const IntDateTime& dateBegin, const IntDateTime& dateEnd) const;

	/** 转移数据到redis
	*/
	void initRedis();

	/** 保存所有gupiao代码
	@param [in] allStock 所有gupiao代码
	*/
	void saveAllStock(const std::vector<std::vector<std::string>>& allStock);

	/** 保存zhibiao
	@param [in] indicatorType zhibiao类型
	@param [in] fields 表头，英文逗号分隔，date开头
	@param [in] indicatorData 数据
	@param [in] isUpdate 如果是更新则在插入前不会删除整张表，并且具备更新功能
	*/
	void saveIndicator(const std::string& indicatorType,
		const std::string& fields,
		const std::map<std::string, std::vector<std::vector<std::string>>>& indicatorData,
		bool isUpdate = false);

	/** 保存过滤的gupiao到mysql
	@param [in] filterStock 过滤的gupiao
	*/
	void saveFilterStockToMysql(const std::map<IntDateTime, std::vector<std::vector<std::string>>>& filterStock);

	/** 保存过滤的gupiao到redis
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	void saveAllFilterStockToRedis(const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** 读取过滤的gupiao
	@param [in] date 日期
	@param [out] filterStock 过滤的gupiao
	*/
	void readRiseUpStockFromRedis(const IntDateTime& date, std::vector<std::string>& riseUpStock);

	/** 读取过滤的gupiao
	@param [in] date 日期
	@param [out] filterStock 过滤的gupiao
	*/
	void readLiftBanStockFromRedis(const IntDateTime& date, std::vector<std::string>& liftBanStock);

public:
	/** 去除部分数据
	@param [in, out] destVec 目标数据
	@param [in] srcVec 源数据
	*/
	static void filterRemove(std::vector<std::string>& destVec, const std::vector<std::string>& srcVec);

	/** 合并数据
	@param [in, out] destVec 目标数据
	@param [in] srcVec 源数据
	*/
	static void filterMerge(std::vector<std::string>& destVec, const std::vector<std::vector<std::string>>& srcVec);

protected:
	/** 新开一个kaigaodishou的表
	@param [in] stock gupiao代码
	*/
	void createMarketHead(const std::string& stock);

	/** 从redis中取数据，没有则从mysql中移植到redis
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@param [in] redisDbIndex 选择存入的redis数据库索引
	@param [in] mysqlDbName 源数据所在数据库名
	@param [in] mysqlFields 取出源数据需要的字段，英文逗号分隔
	@return 返回表
	*/
	std::shared_ptr<std::vector<std::vector<std::string>>> redisFromMysql(const std::string& stock,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		int32_t redisDbIndex,
		const std::vector<std::string>& mysqlDbName,
		const std::vector<std::string>& mysqlFields) const;

	std::map<std::string, std::vector<int32_t>> getIndex(const std::string& indexHashMapName) const;

	

private:
	//数据库连接类
	MysqlCpp m_mysql;
	//redis连接类
	HiRedis m_redis;
};