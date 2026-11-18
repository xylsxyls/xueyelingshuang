#pragma once
#include "LockFreeMapMacro.h"
#include "LockFreeMapContract.h"
#include <stddef.h>

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
class LockFreeMap;

/** LockFreeMap下标访问代理
@note 支持map[key] = value。写入失败抛LockFreeMapError；需要返回码时使用lockFreeMapSet
*/
template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
class MapValueProxy
{
public:
    /** 构造下标代理
    @param [in] map 借用的Map，须活到代理最后一次使用；nullptr时get失败，实际写入抛LockFreeMapError(INVALID)
    @param [in] key 复制保存的键；复制异常可向调用者传播
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
    @note 未提交时抛LockFreeMapError，不再忽略失败
    */
    MapValueProxy& operator=(const ValueType& value);

    /** 通过另一个下标代理的当前值写入
    @param [in] other 另一个下标代理
    @return 返回当前代理对象
    @note other不存在对应值时写入ValueType默认值
    */
    MapValueProxy& operator=(const MapValueProxy& other);

    /** 转换为值副本
    @return 返回当前键对应的值，不存在时返回ValueType默认值
    */
    operator ValueType() const;

    /** 查询当前键对应的值
    @param [out] value 查询成功时写入值副本
    @return 找到且复制成功返回true，资源或用户异常等失败返回false；输出可能部分修改
    */
    bool get(ValueType& value) const;

    /** 查询当前键对应的值，不存在时返回默认值
    @param [in] defaultValue 默认值
    @return 先复制默认值再调用get；失败可能留下部分赋值，复制或按值返回也可能抛异常
    */
    ValueType value(const ValueType& defaultValue = ValueType()) const;

private:
    // 被代理的Map对象，不拥有生命周期
    LockFreeMap<KeyType, ValueType, LessType, MaxLevel>* m_map;
    // 代理独立拥有的键副本
    KeyType m_key;
};

#include "MapValueProxy.inl"