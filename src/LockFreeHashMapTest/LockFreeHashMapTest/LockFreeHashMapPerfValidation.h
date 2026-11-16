#pragma once
#include "LockFreeHashMapPerfModels.h"
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <utility>

/** 自检载荷，计数复制操作以验证集合校验不会隐式复制实际值
*/
struct LockFreeHashMapPerfValidationValue
{
public:
    // 独立业务标识，用于排序和比较
    int32_t m_value;
    // 自检作用域内存活的复制计数器，不拥有计数器
    size_t* m_copies;
public:
    /** 保存标识及复制计数器
    @param [in] value 业务标识
    @param [in,out] copies 覆盖全部实例寿命的计数器
    */
    LockFreeHashMapPerfValidationValue(int32_t value, size_t& copies);

    /** 复制标识并记录复制发生
    @param [in] other 来源载荷，计数器仍存活
    */
    LockFreeHashMapPerfValidationValue(const LockFreeHashMapPerfValidationValue& other);

    /** 赋值标识并记录复制发生
    @param [in] other 来源载荷，计数器仍存活
    @return 当前载荷引用
    */
    LockFreeHashMapPerfValidationValue& operator=(const LockFreeHashMapPerfValidationValue& other);

    /** 按业务标识排序，不复制载荷
    @param [in] other 右侧载荷
    @return 本标识较小为true
    */
    bool operator<(const LockFreeHashMapPerfValidationValue& other) const;

    /** 比较业务标识，不把计数器地址当作预期
    @param [in] other 右侧载荷
    @return 标识相同为true
    */
    bool operator==(const LockFreeHashMapPerfValidationValue& other) const;
};

/** 性能夹具的独立集合校验，调用发生在计时区间之外
*/
class LockFreeHashMapPerfValidation
{
public:
    /** 在计时前排序独立预期并准备等长指针缓冲，不借用被测表
    @param [in,out] expected 完整预期多重集；成功后按测试严格弱序排列
    @param [out] scratch 每轮校验复用的指针数组，内容不拥有结果对象
    @return 准备完成为true，分配或比较异常返回false
    */
    template <typename T>
    static bool prepare(std::vector<T>& expected, std::vector<const T*>& scratch);

    /** 不复制测试载荷或申请临时数组，逐项核对完整多重集
    @param [in] actual 本次实际结果，调用期间保持存活且不被修改
    @param [in] expected prepare成功后未改变的有序预期
    @param [in,out] scratch prepare创建的等长缓冲；退出时解除全部借用
    @return 数量及每个元素的出现次数均一致为true
    */
    template <typename T>
    static bool prepared(const std::vector<T>& actual, const std::vector<T>& expected, std::vector<const T*>& scratch);

    /** 核对完整键多重集，拒绝缺失、重复及额外键
    @param [in] actual 实际导出结果，不依赖顺序
    @param [in] expected 预先生成的键，前 count 个为命中集合
    @param [in] count 逻辑键数量
    @return 全集相等为 true
    */
    template <typename K>
    static bool keys(const std::vector<K>& actual, const std::vector<K>& expected, size_t count);

    /** 核对完整值多重集，包括每个值出现的次数
    @param [in] actual 实际导出值
    @param [in] candidates 每个键两个候选，初始值位于偶数下标
    @param [in] count 逻辑键数量
    @return 全集相等为 true
    */
    template <typename V>
    static bool values(const std::vector<V>& actual, const std::vector<V>& candidates, size_t count);

    /** 核对全部键值对应关系和唯一性
    @param [in] actual 实际导出或遍历得到的键值对
    @param [in] keys 预先生成的命中键及缺失键
    @param [in] candidates 每个键两个候选，初始值位于偶数下标
    @param [in] count 逻辑键数量
    @return 全集及对应关系均相等为 true
    */
    template <typename K, typename V>
    static bool pairs(const std::vector<std::pair<K, V>>& actual, const std::vector<K>& keys,
        const std::vector<V>& candidates, size_t count);

    /** 对正确输入及故意损坏的集合执行设施自检
    @return 正例接受且全部负例被拒绝为 true
    */
    static bool selftest();
private:
    /** 核对预分配校验的乱序、重复、错配、大值及共享引用边界
    @return 正例接受、负例拒绝且借用完全解除为true
    */
    static bool preparedSelftest();

    /** 给测试值提供严格弱序，只用于计时外集合比较
    @param [in] left 左值
    @param [in] right 右值
    @return 左值排在右值之前为 true
    */
    template <typename V>
    static bool less(const V& left, const V& right);

    /** 完整比较大值的所有字段，不仅检查首字
    @param [in] left 左值
    @param [in] right 右值
    @return 按字段字典序排序结果
    */
    static bool less(const LockFreeHashMapPerfLargeValue& left, const LockFreeHashMapPerfLargeValue& right);

    /** 比较键值对的全部内容，沿用大值的完整字段比较
    @param [in] left 左侧键值对
    @param [in] right 右侧键值对
    @return 先按键、再按值的严格弱序
    */
    template <typename K, typename V>
    static bool less(const std::pair<K, V>& left, const std::pair<K, V>& right);
};

#include "LockFreeHashMapPerfValidation.inl"