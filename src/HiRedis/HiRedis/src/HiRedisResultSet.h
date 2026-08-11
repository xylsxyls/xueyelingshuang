#pragma once
#include <map>
#include <string>
#include <stdint.h>
#include <vector>
#include "HiRedisMacro.h"

struct redisReply;
class HiRedis;

/** Redis命令执行结果封装，负责释放底层redisReply并提供常用类型转换
*/
class HiRedisAPI HiRedisResultSet
{
public:
    /** 构造函数，创建空结果集
    */
    HiRedisResultSet();

    /** 析构函数，释放底层redisReply对象
    */
    ~HiRedisResultSet();

public:
    /** 判断结果集是否可用且没有错误
    @return 返回结果集是否持有有效回复并且没有错误信息
    */
    bool isValid() const;

    /** 判断结果是否是错误状态
    @return 返回本地错误或Redis错误回复是否存在
    */
    bool isError() const;

    /** 获取错误信息
    @return 返回本地错误或Redis错误回复文本，没有错误返回空字符串
    */
    std::string error() const;

    /** 获取底层redisReply类型
    @return 返回hiredis的回复类型，结果为空时返回0
    */
    int32_t type() const;

    /** 获取结果元素数量
    @return 数组回复返回元素数量，普通回复返回1，空回复返回0
    */
    size_t size() const;

    /** 按下标读取结果元素
    @param [in] index 元素下标，普通回复只支持0
    @return 返回元素字符串表示，失败返回空字符串
    */
    std::string at(size_t index) const;

    /** 将回复转换为字符串
    @return 返回字符串、状态、错误或整数回复的字符串表示
    */
    std::string toString() const;

    /** 将回复转换为64位整数
    @return 整数回复直接返回，数字字符串尝试转换，失败返回0
    */
    int64_t toInt64() const;

    /** 判断回复是否为空值
    @return 返回是否没有有效回复或Redis返回NIL
    */
    bool isNull() const;

    /** 将数组回复转换为字符串列表
    @return 返回数组每一项的字符串表示，非数组回复返回单元素列表
    */
    std::vector<std::string> toKeys() const;

    /** 将数组回复转换为键值表
    @return 有绑定键列表时按键列表映射，否则按Redis的key-value交替数组映射
    */
    std::map<std::string, std::string> toMap() const;

    /** 将哈希结果转换为键值表
    @return 返回哈希字段和值的映射
    */
    std::map<std::string, std::string> toHashMap() const;

    /** 将哈希表值转换为二维表
    @param [in] key 字段名插回到行中的列下标，小于0表示不插入
    @param [in] split 行内容的分隔符
    @return 返回二维表数据
    */
    std::vector<std::vector<std::string>> toTable(int32_t key = -1, const std::string& split = ",") const;

    /** 将集合结果转换为字符串列表
    @return 返回集合成员列表
    */
    std::vector<std::string> toGroup() const;

    /** 将回复转换为操作状态
    @return 状态OK或非负整数回复返回true，其余返回false
    */
    bool toStatus() const;

    /** 将回复转换为数量
    @return 数组回复返回元素数量，整数回复返回整数值，其余返回0
    */
    int32_t toCount() const;

    /** 获取底层redisReply指针
    @return 返回底层redisReply指针，本对象析构时仍会释放该指针
    */
    redisReply* toReply() const;

private:
    friend class HiRedis;

    /** 设置底层回复对象，旧回复会被释放
    @param [in] reply hiredis返回的回复对象
    */
    void setReply(redisReply* reply);

    /** 设置MGET/HMGET等命令的键名列表，用于结果转map
    @param [in] vecKey 与数组回复顺序对应的键名列表
    */
    void setKeys(const std::vector<std::string>& vecKey);

    /** 设置本地错误信息
    @param [in] error 错误文本
    */
    void setError(const std::string& error);

    /** 禁止拷贝构造，避免多个对象同时释放同一个redisReply
    @param [in] other 另一个Redis结果对象
    */
    HiRedisResultSet(const HiRedisResultSet& other);

    /** 禁止赋值，避免多个对象同时释放同一个redisReply
    @param [in] other 另一个Redis结果对象
    @return 不会被调用
    */
    HiRedisResultSet& operator=(const HiRedisResultSet& other);

protected:
    /** 检查底层回复是否存在
    @return 返回底层回复是否非空
    */
    bool check() const;

    /** 将单个redisReply转换为字符串
    @param [in] reply 要转换的Redis回复对象
    @return 返回回复的字符串表示
    */
    std::string replyToString(const redisReply* reply) const;

protected:
    // 底层hiredis回复对象，由本对象负责释放
    redisReply* m_reply;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // MGET/HMGET等结果转换时使用的键名列表
    std::vector<std::string> m_vecKey;
    // 本地记录的错误信息
    std::string m_error;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};