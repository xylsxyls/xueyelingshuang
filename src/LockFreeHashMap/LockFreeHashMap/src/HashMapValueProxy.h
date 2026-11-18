#pragma once
#include "LockFreeHashMapMacro.h"
#include "LockFreeHashMapTypes.h"

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
class LockFreeHashMap;

/** 拥有键副本并借用表的下标代理，表必须存活到代理最后一次使用
@note 支持map[key] = value；赋值重试竞争但不等待容量，未提交时抛LockFreeHashMapAssignmentError
*/
template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
class HashMapValueProxy
{
public:
    /** 构造下标代理
    @param [in] map 借用的表；空指针可构造代理，但写入报InvalidArgument，get返回false
    @param [in] key 调用期间借用并复制的键，复制异常由调用方处理
    */
    HashMapValueProxy(LockFreeHashMap<KeyType, ValueType, HashType, EqualType>* map, const KeyType& key);

    /** 拷贝构造函数
    @param [in] other 复制键并共享表的借用关系，不延长表寿命；键复制异常向调用方传播
    */
    HashMapValueProxy(const HashMapValueProxy& other);

    /** 析构函数，不释放被代理的Map对象
    */
    ~HashMapValueProxy();

    /** 通过下标代理写入值
    @param [in] value 要写入的值
    @return 返回当前代理对象
    @note 内部调用trySet(key,value,nullptr,true,false)，未提交时抛LockFreeHashMapAssignmentError
    */
    HashMapValueProxy& operator=(const ValueType& value);

    /** 通过另一个下标代理的当前值写入
    @param [in] other 另一个下标代理
    @return 返回当前代理对象
    @note 来源NotFound时写入ValueType默认值；其他读取失败或目标未提交时抛LockFreeHashMapAssignmentError
    @note 自赋值不执行读写；默认值构造异常向调用方传播
    */
    HashMapValueProxy& operator=(const HashMapValueProxy& other);

    /** 转换为值副本
    @return 先默认构造再尝试读取；未找到保持默认值，赋值异常可能留下部分修改；构造和返回复制异常向外传播
    */
    operator ValueType() const;

    /** 查询当前键对应的值
    @param [out] value 查询成功时赋值，赋值异常可能部分修改输出
    @return 找到且赋值成功为true；表为空、键不存在或其他查询失败为false
    */
    bool get(ValueType& value) const;

    /** 复制默认值后尝试读取，继承Map::value的赋值异常边界
    @param [in] defaultValue 调用期间借用并复制，复制异常由调用方处理
    @return 读取成功的值或保留的默认副本；赋值失败可能部分修改，返回值复制异常向外传播
    */
    ValueType value(const ValueType& defaultValue = ValueType()) const;

private:
    // 被代理的Map对象，不拥有生命周期
    LockFreeHashMap<KeyType, ValueType, HashType, EqualType>* m_map;
    // 被代理的键副本
    KeyType m_key;
};

#include "HashMapValueProxy.inl"