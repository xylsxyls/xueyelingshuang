#pragma once
#include "LockFreeMapMacro.h"
#include <stddef.h>

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
class LockFreeMapAPI LockFreeMap;

/** LockFreeMap下标访问代理
@note 用于支持map[key] = value写法。赋值内部调用set；需要判断写入是否成功时请直接调用set。
*/
template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
class MapValueProxy
{
public:
    /** 构造下标代理
    @param [in] map 被代理的LockFreeMap对象，不接管生命周期
    @param [in] key 被代理的键
    */
    MapValueProxy(LockFreeMap<KeyType, ValueType, LessType, MaxLevel>* map, const KeyType& key);

    /** 拷贝构造函数
    @param [in] other 被拷贝的代理对象
    */
    MapValueProxy(const MapValueProxy& other);

    /** 析构函数，不释放被代理的Map对象
    */
    ~MapValueProxy();

    /** 通过下标代理写入值
    @param [in] value 要写入的值
    @return 返回当前代理对象
    @note 内部调用LockFreeMap::set。需要判断写入是否成功时，请直接调用set。
    */
    MapValueProxy& operator=(const ValueType& value);

    /** 通过另一个下标代理的当前值写入
    @param [in] other 另一个下标代理
    @return 返回当前代理对象
    @note other不存在对应值时写入ValueType默认值。
    */
    MapValueProxy& operator=(const MapValueProxy& other);

    /** 转换为值副本
    @return 返回当前键对应的值，不存在时返回ValueType默认值
    */
    operator ValueType() const;

    /** 查询当前键对应的值
    @param [out] value 查询成功时写入值副本
    @return 找到返回true，否则返回false
    */
    bool get(ValueType& value) const;

    /** 查询当前键对应的值，不存在时返回默认值
    @param [in] defaultValue 默认值
    @return 返回查到的值或默认值
    */
    ValueType value(const ValueType& defaultValue = ValueType()) const;

private:
    // 被代理的Map对象，不拥有生命周期
    LockFreeMap<KeyType, ValueType, LessType, MaxLevel>* m_map;
    // 被代理的键副本
    KeyType m_key;
};

#include "MapValueProxy.inl"