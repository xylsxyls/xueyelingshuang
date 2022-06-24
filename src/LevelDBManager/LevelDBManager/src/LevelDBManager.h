#pragma once
#include <string>
#include <map>
#include <mutex>
#include <functional>

namespace leveldb
{
	class DB;
}

/** LevelDB管理类
*/
class LevelDBManager
{
protected:
	/** 构造函数
	*/
	LevelDBManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static LevelDBManager& instance();

	/** 拷贝构造函数
	@param [in] other 拷贝类
	*/
	LevelDBManager(const LevelDBManager& other) = delete;

	/** 赋值构造函数
	@param [in] other 赋值类
	*/
    LevelDBManager& operator=(const LevelDBManager& other) = delete;

public:
	/** 打开DB文件，线程安全
	@param [in] path 文件路径，文件不存在会自动创建，文件所在路径必须存在否则创建失败
	@param [in] fileId 文件ID，会根据文件ID决定打开哪个文件，当文件ID为0时，不会查找map
	*/
	void openDBFile(const std::string& path, int32_t fileId = 0);

	/** 设置配置，线程安全
	@param [in] key 配置关键字
	@param [in] value 配置值
	@param [in] fileId 文件ID
	*/
	void setConfig(const std::string& key, const std::string& value, int32_t fileId = 0);

	/** 读取配置，线程安全
	@param [in] key 配置关键字
	@param [out] value 配置值
	@param [in] fileId 文件ID
	@return 返回是否找到该配置项
	*/
	bool getConfig(const std::string& key, std::string& value, int32_t fileId = 0);

	/** 查找匹配配置集，和读取配置不同的是，这里的key部分匹配，查找到匹配的就会循环进入回调函数，当循环结束时findConfig函数退出，线程安全
	@param [in] key 配置关键字，部分匹配
	@param [in] findConfigFun 左侧为key，右侧为value
	@param [in] fileId 文件ID
	*/
	void findConfig(const std::string& key, const std::function<void(const std::string&, const std::string&)>& findConfigFun, int32_t fileId = 0);

	/** 删除配置，线程安全
	@param [in] key 配置关键字
	@param [in] fileId 文件ID
	*/
	void deleteConfig(const std::string& key, int32_t fileId = 0);

	/** 关闭DB文件，线程安全
	@param [in] fileId 文件ID
	*/
	void closeDBFile(int32_t fileId = 0);

protected:
	/** 查找文件指针
	@param [in] fileId 文件ID
	@return 返回文件指针
	*/
	leveldb::DB* findDBFile(int32_t fileId);

private:
	//默认文件指针，文件ID为0
	leveldb::DB* m_defaultDBFile;
	//除文件ID为0之外的文件指针
	std::map<int32_t, leveldb::DB*> m_dbFile;
	//用于打开关闭文件的多线程同步锁
	std::mutex m_mutex;
};