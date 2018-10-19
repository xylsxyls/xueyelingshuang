#pragma once
#include "ConfigManagerMacro.h"
#include <string>
#include <stdint.h>
#include <memory>

/********************************************************************************
@宏定义使用说明
1.key值和section值必须使用枚举值，section值唯一，key值在section值下唯一，枚举值定义文件在common文件夹下
2.初始化函数
INIT_CONFIG(dbPath);
如果没有调用该函数就执行存储则在exe同级目录下自动创建一个和exe同名的db文件
3.存储全局，使用宏GLOBAL_CONFIG
GLOBAL_CONFIG[key值] = value;
GLOBAL_CONFIG[section值][key值] = value;
其中value包括int32_t,uint64_t,double,std::string
二进制字符串也可以，例如
std::string str = "1234567";
str[3] = 0;
GLOBAL_CONFIG[section值][key值] = str;
重复存储会覆盖
4.存储用户相关，使用宏USER_CONFIG
首先要设置用户的userId
SET_CONFIG_USERID(userId值);
执行完这句话，之后的用户相关存储都在该userId值下
USER_CONFIG[key值] = value;
USER_CONFIG[section值][key值] = value;
用法同上
userId值可以中途改变，但改变之后的存储都和改变之后的userId值相关
例如
SET_CONFIG_USERID(1000001);
USER_CONFIG[key值] = value1;
SET_CONFIG_USERID(1000002);
USER_CONFIG[key值] = value2;
如果想拿到value1的值必须重新设回1000001才行
5.获取存储内容
可以转换为4种类型，int32_t,uint64_t,double,std::string
double result = GLOBAL_CONFIG[section值][key值].toDouble();
double result = USER_CONFIG[section值][key值].toDouble();
result为配置文件中的值
如果不记得存入的类型，可以先进行查看
std::string type = USER_CONFIG[section值][key值].type();
int32_t对应TYPE_INT32
uint64_t对应TYPE_UINT64
double对应TYPE_DOUBLE
std::string对应TYPE_BLOB
6.删除配置
DELETE_GLOBAL_CONFIG(key值);
DELETE_GLOBAL_CONFIG(section值, key值);
DELETE_USER_CONFIG(key值);
DELETE_USER_CONFIG(section值, key值);
注意在删除用户相关配置时userId是正确的
7.注意
所有操作必须连续，不允许这样写
GLOBAL_CONFIG[key值];
GLOBAL_CONFIG[key值] = value;
这样写是允许的
Fun(GLOBAL_CONFIG[key值].toInt32());
8.性能
目前一秒插入430条配置数据，查询几乎不耗时间
如果在这个基础上想提高速度可以在执行存储前加上CONFIG_BEGIN，执行多条存储语句后执行CONFIG_END
例如
CONFIG_BEGIN;
for(int32_t index = 0; index < 10000; ++index)
{
	GLOBAL_CONFIG[index] = index;
}
CONFIG_END;
9.存储长度
目前存储长度为SAVE_LENGTH，(4G - 1)大小，uint32_t上限
10.多线程多进程
支持多线程多进程同时使用，支持存入同一个数据库文件
11.用法实例
存储用户登录账号列表
GLOBAL_CONFIG[登录账号列表配置枚举值][复选框中账号位置的序列号] = 二进制字符串(登录账号\0登录密码\0用户头像路径);
存储和删除当前账号最近在玩游戏名
SET_CONFIG_USERID(启动时用户的ID值);
USER_CONFIG[最近在玩游戏名配置枚举值] = 最近在玩的游戏名;
DELETE_USER_CONFIG(最近在玩游戏名配置枚举值);
********************************************************************************/

#define TYPE_BLOB "blob"
#define TYPE_INT32 "int32_t"
#define TYPE_UINT64 "uint64_t"
#define TYPE_DOUBLE "double"

#define SAVE_LENGTH 4294967295

#define GET_MACRO(_1, _2, NAME,...) NAME

#define DELETE_GLOBAL_CONFIG_FUN_1(key) ConfigManager::instance().deleteConfig(key)
#define DELETE_GLOBAL_CONFIG_FUN_2(section, key) ConfigManager::instance().deleteConfig(section, key)
#define DELETE_GLOBAL_CONFIG(...) GET_MACRO(__VA_ARGS__, DELETE_GLOBAL_CONFIG_FUN_2, DELETE_GLOBAL_CONFIG_FUN_1)(__VA_ARGS__)

#define DELETE_USER_CONFIG_FUN_1(key) (*(ConfigManager::instance().getUserConfigManager())).deleteConfig(key)
#define DELETE_USER_CONFIG_FUN_2(section, key) (*(ConfigManager::instance().getUserConfigManager())).deleteConfig(section, key)
#define DELETE_USER_CONFIG(...) GET_MACRO(__VA_ARGS__, DELETE_USER_CONFIG_FUN_2, DELETE_USER_CONFIG_FUN_1)(__VA_ARGS__)

#define INIT_CONFIG(dbPath) ConfigManager::instance().init(dbPath)
#define GLOBAL_CONFIG ConfigManager::instance()
#define USER_CONFIG (*(ConfigManager::instance().getUserConfigManager()))
#define SET_CONFIG_USERID(userId) ConfigManager::instance().setUserId(userId)

//暂时不可用
//#define CONFIG_BEGIN ConfigManager::instance().transaction()
//#define CONFIG_END ConfigManager::instance().commit()

class SQLite;
class UserConfigManager;
class SQLiteResultSet;
class ProcessReadWriteMutex;

/** 配置管理类
*/
class ConfigManagerAPI ConfigManager
{
protected:
	/** 构造函数
	*/
	ConfigManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static ConfigManager& instance();

public:
	/** 存入section或key
	@param [in] sectionOrKey section或key值
	@return 返回管理类
	*/
	ConfigManager& operator[] (int32_t sectionOrKey);

	/** 存入int32_t值
	@param [in] value int32_t值
	@return 返回管理类
	*/
	ConfigManager operator = (int32_t value);

	/** 存入uint64_t值
	@param [in] value uint64_t值
	@return 返回管理类
	*/
	ConfigManager operator = (uint64_t value);

	/** 存入double值
	@param [in] value double值
	@return 返回管理类
	*/
	ConfigManager operator = (double value);

	/** 存入字符串或二进制字符串
	@param [in] value 字符串或二进制字符串
	@return 返回管理类
	*/
	ConfigManager operator = (const std::string& value);

	/** 获取存储类型
	@return 返回存储类型
	*/
	std::string type();

	/** 转换为int32_t类型
	@return 返回int32_t类型
	*/
	int32_t toInt32();

	/** 转换为uint64_t类型
	@return 返回uint64_t类型
	*/
	uint64_t toUint64();

	/** 转换为double类型
	@return 返回double类型
	*/
	double toDouble();

	/** 转换为std::string类型，包含普通字符串和二进制字符串
	@return 返回std::string类型
	*/
	std::string toString();

public:
	/** 初始化配置文件路径，如果不调用该函数则自动在运行目录下和执行文件同名的db文件
	@param [in] configPath 配置文件路径
	*/
	void init(const std::string& configPath = "");

	/** 构建数据库
	*/
	void initSQLite();

	/** 设置用户ID
	@param [in] userId 用户ID
	*/
	void setUserId(uint64_t userId);

	/** 获取用户相关配置管理类
	@return 返回用户相关配置管理类
	*/
	UserConfigManager* getUserConfigManager();

	/** 删除配置
	@param [in] key key值
	*/
	void deleteConfig(int32_t key);

	/** 删除配置
	@param [in] key key值
	@param [in] section 节点值
	*/
	void deleteConfig(int32_t section, int32_t key);

	/** 开启事务
	*/
	void transaction();

	/** 提交
	*/
	void commit();

protected:
	/** 添加配置（二进制字符串）
	@param [in] key key值
	@param [in] value 二进制字符串
	@param [in] section 节点值
	*/
	void addConfig(int32_t key, const std::string& value, int32_t section = 0);

	/** 获取配置（二进制字符串）
	@param [in] key key值
	@param [out] value 二进制字符串
	@param [in] section 节点值
	*/
	void getConfig(int32_t key, std::string& value, int32_t section = 0);

	/** 添加配置（整形）
	@param [in] key key值
	@param [in] value 整形
	@param [in] section 节点值
	*/
	void addConfig(int32_t key, int32_t value, int32_t section = 0);

	/** 添加配置（整形）
	@param [in] key key值
	@param [out] value 整形
	@param [in] section 节点值
	*/
	void getConfig(int32_t key, int32_t& value, int32_t section = 0);

	/** 添加配置（无符号长整型）
	@param [in] key key值
	@param [in] value 无符号长整型
	@param [in] section 节点值
	*/
	void addConfig(int32_t key, uint64_t value, int32_t section = 0);

	/** 添加配置（无符号长整型）
	@param [in] key key值
	@param [out] value 无符号长整型
	@param [in] section 节点值
	*/
	void getConfig(int32_t key, uint64_t& value, int32_t section = 0);

	/** 添加配置（双精度浮点型）
	@param [in] key key值
	@param [in] value 双精度浮点型
	@param [in] section 节点值
	*/
	void addConfig(int32_t key, double value, int32_t section = 0);

	/** 添加配置（双精度浮点型）
	@param [in] key key值
	@param [out] value 双精度浮点型
	@param [in] section 节点值
	*/
	void getConfig(int32_t key, double& value, int32_t section = 0);

	/** 获取配置类型
	@param [in] key key值
	@param [in] section 节点值
	@return 返回配置类型
	*/
	std::string getConfigType(int32_t key, int32_t section = 0);

protected:
	void addConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section);
	bool hasConfigBase(int32_t key, int32_t section);
	bool updateConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section);
	bool insertConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section);
	std::string getConfigBase(int32_t key, int32_t section);
	void createTableIfNotExist(const std::string& tableName);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<SQLite> m_spConfig;
	std::shared_ptr<ProcessReadWriteMutex> m_spProcessMutex;
	std::string m_tableName;
	std::string m_databasePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_key;
	int32_t m_section;
};