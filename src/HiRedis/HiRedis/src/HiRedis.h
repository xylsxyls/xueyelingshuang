#pragma once
#include "HiRedisConfig.h"
#include "HiRedisCommand.h"
#include "HiRedisMacro.h"
#include "HiRedisResultSet.h"
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

struct redisContext;

/** Redis客户端封装，所有命令通过本类进入hiredis，业务层不直接依赖底层库
*/
class HiRedisAPI HiRedis
{
public:
    /** 构造函数，创建未连接的Redis客户端
    */
    HiRedis();

    /** 析构函数，不做反初始化，调用方需要在退出前主动调用uninit
    */
    ~HiRedis();

public:
    /** 使用IP和端口初始化Redis连接
    @param [in] ip Redis服务器IP或域名
    @param [in] port Redis监听端口
    @return 返回是否连接成功
    */
    bool init(const std::string& ip, int32_t port = 6379);

    /** 使用完整配置初始化Redis连接
    @param [in] config Redis连接配置
    @return 返回是否连接、认证和选择数据库全部成功
    */
    bool init(const HiRedisConfig& config);

    /** 反初始化Redis连接，关闭当前底层连接
    */
    void uninit();

    /** 使用IP和端口连接Redis，兼容旧接口，内部调用init
    @param [in] ip Redis服务器IP或域名
    @param [in] port Redis监听端口
    @return 返回是否连接成功
    */
    bool connect(const std::string& ip, int32_t port = 6379);

    /** 使用完整配置连接Redis，兼容旧接口，内部调用init
    @param [in] config Redis连接配置
    @return 返回是否连接、认证和选择数据库全部成功
    */
    bool connect(const HiRedisConfig& config);

    /** 关闭当前Redis连接，兼容旧接口，内部调用uninit
    */
    void close();

    /** 使用最近一次配置重新连接Redis
    @return 返回是否重连成功
    */
    bool reconnect();

    /** 发送PING检测连接状态
    @return 返回Redis是否正常响应
    */
    bool ping();

    /** 判断本地连接对象是否处于已连接状态
    @return 返回连接对象是否存在且底层没有错误
    */
    bool isConnected();

    /** 获取最近一次错误信息
    @return 返回错误文本，没有错误返回空字符串
    */
    std::string lastError();

    /** 执行Redis命令文本，内部解析后按argv方式发送
    @param [in] command Redis命令文本，包含空格的参数需要使用引号或反斜杠转义，二进制参数请使用argv接口
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> command(const std::string& command);

    /** 使用argv方式执行Redis命令，推荐业务代码优先使用此接口
    @param [in] args Redis命令及参数，第一个元素是命令名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> command(const std::vector<std::string>& args);

    /** 执行Redis命令封装对象
    @param [in] command Redis命令封装对象
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> command(const HiRedisCommand& command);

    /** 批量发送Redis管线命令
    @param [in] commands 要按顺序发送的命令列表
    @return 返回与命令顺序对应的结果列表
    */
    std::vector<std::shared_ptr<HiRedisResultSet>> pipeline(const std::vector<HiRedisCommand>& commands);

    /** 在Redis事务中执行一组命令
    @param [in] commands MULTI和EXEC之间执行的命令列表
    @return 返回EXEC结果，失败时返回对应错误结果
    */
    std::shared_ptr<HiRedisResultSet> execTransaction(const std::vector<HiRedisCommand>& commands);

    /** 切换Redis数据库
    @param [in] index 数据库下标
    @return 返回是否切换成功
    */
    bool selectDbIndex(int32_t index);

    /** 切换Redis数据库
    @param [in] index 数据库下标
    @return 返回是否切换成功
    */
    bool selectDb(int32_t index);

    /** 使用密码认证当前连接
    @param [in] password Redis认证密码
    @return 返回是否认证成功
    */
    bool auth(const std::string& password);

    /** 设置字符串键值
    @param [in] key Redis键名
    @param [in] value Redis字符串值
    @return 返回是否设置成功
    */
    bool setKeyValue(const std::string& key, const std::string& value);

    /** 设置字符串键值
    @param [in] key Redis键名
    @param [in] value Redis字符串值
    @return 返回是否设置成功
    */
    bool set(const std::string& key, const std::string& value);

    /** 设置带过期时间的字符串键值
    @param [in] key Redis键名
    @param [in] value Redis字符串值
    @param [in] ttlSeconds 过期时间，单位秒
    @return 返回是否设置成功
    */
    bool setEx(const std::string& key, const std::string& value, int32_t ttlSeconds);

    /** 仅在键不存在时设置字符串键值
    @param [in] key Redis键名
    @param [in] value Redis字符串值
    @param [in] ttlSeconds 过期时间，单位秒，小于等于0表示不设置过期
    @return 返回是否设置成功
    */
    bool setNx(const std::string& key, const std::string& value, int32_t ttlSeconds = 0);

    /** 读取字符串键值
    @param [in] key Redis键名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> getValue(const std::string& key);

    /** 读取字符串键值
    @param [in] key Redis键名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> get(const std::string& key);

    /** 批量读取字符串键值
    @param [in] key Redis键名列表
    @return 返回命令结果对象，结果可按键名转换为map
    */
    std::shared_ptr<HiRedisResultSet> getValues(const std::vector<std::string>& key);

    /** 批量读取字符串键值
    @param [in] keys Redis键名列表
    @return 返回命令结果对象，结果可按键名转换为map
    */
    std::shared_ptr<HiRedisResultSet> mGet(const std::vector<std::string>& keys);

    /** 对整数键执行自增
    @param [in] key Redis键名
    @param [in] value 自增值
    @return 返回自增后的值，失败返回0
    */
    int64_t incrBy(const std::string& key, int64_t value);

    /** 对整数键执行自减
    @param [in] key Redis键名
    @param [in] value 自减值
    @return 返回自减后的值，失败返回0
    */
    int64_t decrBy(const std::string& key, int64_t value);

    /** 设置键的过期时间
    @param [in] key Redis键名
    @param [in] ttlSeconds 过期时间，单位秒
    @return 返回是否成功设置过期时间
    */
    bool expire(const std::string& key, int32_t ttlSeconds);

    /** 获取键的剩余过期时间
    @param [in] key Redis键名
    @return 返回TTL秒数，Redis不存在或无过期按原始命令语义返回负数
    */
    int64_t ttl(const std::string& key);

    /** 批量设置普通字符串键值
    @param [in] map 键值映射
    @return 返回是否设置成功
    */
    bool setMap(const std::map<std::string, std::string>& map);

    /** 设置Redis哈希表多个字段
    @param [in] mapName 哈希表键名
    @param [in] hashMap 字段和值映射
    @return 返回是否设置成功
    */
    bool setHashMap(const std::string& mapName, const std::map<std::string, std::string>& hashMap);

    /** 设置Redis哈希表单个字段
    @param [in] mapName 哈希表键名
    @param [in] field 字段名
    @param [in] value 字段值
    @return 返回是否设置成功
    */
    bool hSet(const std::string& mapName, const std::string& field, const std::string& value);

    /** 设置Redis哈希表多个字段
    @param [in] mapName 哈希表键名
    @param [in] hashMap 字段和值映射
    @return 返回是否设置成功
    */
    bool hmSet(const std::string& mapName, const std::map<std::string, std::string>& hashMap);

    /** 读取Redis哈希表单个字段
    @param [in] mapName 哈希表键名
    @param [in] field 字段名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> hGet(const std::string& mapName, const std::string& field);

    /** 读取Redis哈希表字段值
    @param [in] mapName 哈希表键名
    @param [in] field 字段列表，空列表表示读取全部字段
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> getHashValues(const std::string& mapName, const std::vector<std::string>& field = std::vector<std::string>());

    /** 读取Redis哈希表全部字段和值
    @param [in] mapName 哈希表键名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> hGetAll(const std::string& mapName);

    /** 批量读取Redis哈希表字段值
    @param [in] mapName 哈希表键名
    @param [in] field 字段列表
    @return 返回命令结果对象，结果可按字段名转换为map
    */
    std::shared_ptr<HiRedisResultSet> hmGet(const std::string& mapName, const std::vector<std::string>& field);

    /** 获取Redis哈希表字段名列表
    @param [in] tableName 哈希表键名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> getHashKeys(const std::string& tableName);

    /** 获取Redis哈希表字段名列表
    @param [in] tableName 哈希表键名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> hKeys(const std::string& tableName);

    /** 删除整个Redis哈希表
    @param [in] tableName 哈希表键名
    @return 返回是否删除成功
    */
    bool deleteHashMap(const std::string& tableName);

    /** 删除Redis哈希表单个字段
    @param [in] tableName 哈希表键名
    @param [in] field 字段名
    @return 返回是否删除成功
    */
    bool deleteHashMapField(const std::string& tableName, const std::string& field);

    /** 删除Redis哈希表多个字段
    @param [in] tableName 哈希表键名
    @param [in] field 字段列表
    @return 返回是否删除成功
    */
    bool deleteHashMapFields(const std::string& tableName, const std::vector<std::string>& field);

    /** 删除Redis哈希表多个字段
    @param [in] tableName 哈希表键名
    @param [in] field 字段列表
    @return 返回是否删除成功
    */
    bool hDel(const std::string& tableName, const std::vector<std::string>& field);

    /** 将二维表保存为Redis哈希表
    @param [in] tableName 哈希表键名
    @param [in] table 二维表数据
    @param [in] key 作为哈希字段名的列下标，小于0表示使用行号
    @param [in] split 保存行内容时使用的分隔符
    @return 返回是否保存成功
    */
    bool setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key = -1, const std::string& split = ",");

    /** 从Redis哈希表读取二维表数据
    @param [in] tableName 哈希表键名
    @param [in] field 字段列表，空列表表示读取全部字段
    @return 返回命令结果对象，可通过toTable转换为二维表
    */
    std::shared_ptr<HiRedisResultSet> getTableValues(const std::string& tableName, const std::vector<std::string>& field = std::vector<std::string>());

    /** 向Redis集合添加成员
    @param [in] groupName 集合键名
    @param [in] element 成员值
    @return 返回是否新增了成员
    */
    bool setGroup(const std::string& groupName, const std::string& element);

    /** 向Redis集合批量添加成员
    @param [in] groupName 集合键名
    @param [in] group 成员列表
    @return 返回是否执行成功
    */
    bool setGroups(const std::string& groupName, const std::vector<std::string>& group);

    /** 读取Redis集合全部成员
    @param [in] groupName 集合键名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> getGroup(const std::string& groupName);

    /** 判断集合成员是否存在
    @param [in] groupName 集合键名
    @param [in] element 成员值
    @return 返回成员是否存在
    */
    bool groupElementExist(const std::string& groupName, const std::string& element);

    /** 获取集合成员数量
    @param [in] groupName 集合键名
    @return 返回集合成员数量，失败返回0
    */
    int64_t getGroupCount(const std::string& groupName);

    /** 删除集合单个成员
    @param [in] groupName 集合键名
    @param [in] element 成员值
    @return 返回是否删除成功
    */
    bool deleteGroupElement(const std::string& groupName, const std::string& element);

    /** 删除集合多个成员
    @param [in] groupName 集合键名
    @param [in] element 成员列表
    @return 返回是否删除成功
    */
    bool deleteGroupElements(const std::string& groupName, const std::vector<std::string>& element);

    /** 向Redis有序集合添加成员
    @param [in] groupName 有序集合键名
    @param [in] score 分数
    @param [in] element 成员值
    @return 返回是否执行成功
    */
    bool setOrderGroup(const std::string& groupName, int32_t score, const std::string& element);

    /** 向Redis有序集合批量添加成员
    @param [in] groupName 有序集合键名
    @param [in] orderGroup 分数和成员值列表
    @return 返回是否执行成功
    */
    bool setOrderGroups(const std::string& groupName, const std::vector<std::pair<int32_t, std::string>>& orderGroup);

    /** 读取有序集合全部成员
    @param [in] groupName 有序集合键名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> getOrderGroup(const std::string& groupName);

    /** 判断有序集合成员是否存在
    @param [in] groupName 有序集合键名
    @param [in] element 成员值
    @return 返回成员是否存在
    */
    bool orderGroupElementExist(const std::string& groupName, const std::string& element);

    /** 获取有序集合成员排名
    @param [in] groupName 有序集合键名
    @param [in] element 成员值
    @return 返回成员排名，不存在或失败返回-1
    */
    int64_t getOrderGroupElementIndex(const std::string& groupName, const std::string& element);

    /** 获取有序集合成员分数
    @param [in] groupName 有序集合键名
    @param [in] element 成员值
    @return 返回成员分数，不存在或失败返回-1
    */
    int64_t getOrderGroupElementScore(const std::string& groupName, const std::string& element);

    /** 获取有序集合成员数量
    @param [in] groupName 有序集合键名
    @return 返回成员数量，失败返回0
    */
    int64_t getOrderGroupCount(const std::string& groupName);

    /** 按排名范围读取有序集合成员
    @param [in] groupName 有序集合键名
    @param [in] min 起始排名
    @param [in] max 结束排名
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> getOrderGroupByIndex(const std::string& groupName, int32_t min, int32_t max);

    /** 按分数范围读取有序集合成员
    @param [in] groupName 有序集合键名
    @param [in] min 最小分数
    @param [in] max 最大分数
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> getOrderGroupByScore(const std::string& groupName, int32_t min, int32_t max);

    /** 按分数范围统计有序集合成员数量
    @param [in] groupName 有序集合键名
    @param [in] min 最小分数
    @param [in] max 最大分数
    @return 返回范围内成员数量，失败返回0
    */
    int64_t getOrderGroupCountByScore(const std::string& groupName, int32_t min, int32_t max);

    /** 删除有序集合单个成员
    @param [in] groupName 有序集合键名
    @param [in] element 成员值
    @return 返回是否删除成功
    */
    bool deleteOrderGroupElement(const std::string& groupName, const std::string& element);

    /** 删除有序集合多个成员
    @param [in] groupName 有序集合键名
    @param [in] element 成员列表
    @return 返回是否删除成功
    */
    bool deleteOrderGroupElements(const std::string& groupName, const std::vector<std::string>& element);

    /** 按排名范围删除有序集合成员
    @param [in] groupName 有序集合键名
    @param [in] min 起始排名
    @param [in] max 结束排名
    @return 返回删除的成员数量
    */
    int64_t deleteOrderGroupElementsByIndex(const std::string& groupName, int32_t min, int32_t max);

    /** 按分数范围删除有序集合成员
    @param [in] groupName 有序集合键名
    @param [in] min 最小分数
    @param [in] max 最大分数
    @return 返回删除的成员数量
    */
    int64_t deleteOrderGroupElementsByScore(const std::string& groupName, int32_t min, int32_t max);

    /** 判断键是否存在
    @param [in] key Redis键名
    @return 返回键是否存在
    */
    bool keyExist(const std::string& key);

    /** 判断哈希表是否存在
    @param [in] mapName 哈希表键名
    @return 返回键是否存在
    */
    bool hashMapExist(const std::string& mapName);

    /** 判断二维表哈希是否存在
    @param [in] tableName 表键名
    @return 返回键是否存在
    */
    bool tableExist(const std::string& tableName);

    /** 判断集合是否存在
    @param [in] key 集合键名
    @return 返回键是否存在
    */
    bool groupExist(const std::string& key);

    /** 判断有序集合是否存在
    @param [in] key 有序集合键名
    @return 返回键是否存在
    */
    bool orderGroupExist(const std::string& key);

    /** 删除单个键
    @param [in] key Redis键名
    @return 返回是否删除了该键
    */
    bool deleteKey(const std::string& key);

    /** 删除多个键
    @param [in] key Redis键名列表
    @return 返回是否全部键都被删除
    */
    bool deleteKeys(const std::vector<std::string>& key);

    /** 删除多个键
    @param [in] keys Redis键名列表
    @return 返回实际删除的键数量
    */
    int64_t del(const std::vector<std::string>& keys);

protected:
    /** 不加锁执行argv命令，调用者必须已持有m_mutex
    @param [in] args Redis命令及参数
    @return 返回命令结果对象
    */
    std::shared_ptr<HiRedisResultSet> commandArgvNoLock(const std::vector<std::string>& args);

    /** 不加锁追加管线命令，调用者必须已持有m_mutex
    @param [in] args Redis命令及参数
    @return 返回是否追加成功
    */
    bool appendCommandArgvNoLock(const std::vector<std::string>& args);

    /** 设置最近一次错误信息
    @param [in] error 错误文本
    */
    void setLastError(const std::string& error);

    /** 不加锁关闭连接，调用者需保证线程安全
    */
    void closeNoLock();

private:
    /** 拷贝构造函数禁用，连接对象不能被复制
    @param [in] other 另一个Redis连接对象
    */
    HiRedis(const HiRedis& other);

    /** 赋值函数禁用，连接对象不能被复制
    @param [in] other 另一个Redis连接对象
    @return 不会被调用
    */
    HiRedis& operator=(const HiRedis& other);

private:
    // 底层hiredis连接上下文
    redisContext* m_redis;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 保护连接、配置和错误信息的互斥锁
    std::mutex m_mutex;
    // 最近一次Redis操作的错误信息
    std::string m_lastError;
    // 最近一次连接使用的配置，用于重连和运行期更新
    HiRedisConfig m_config;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前对象是否已经通过init完成初始化
    bool m_isInit;
};