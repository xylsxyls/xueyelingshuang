#pragma once
#include "NetInterfaceTypes.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include <atomic>
#include <utility>

template <class T> class NetObjectPool;

// 池内对象及引用计数占用同一槽；不额外申请shared_ptr控制块
template <class T>
struct NetObjectPoolItem
{
public:
    // 最后一个引用负责销毁对象并归还整个池槽
    std::atomic<uint64_t> m_references;
    // 原位构造的实际对象，其子资源应同样使用受控池
    T m_value;

public:
    /** 构造一个初始引用和实际对象，异常由池的创建入口捕获
    @param [in] args 实际对象构造参数，借用规则由T规定
    */
    template <class... Args> explicit NetObjectPoolItem(Args&&... args);
};

// 有所有权的池引用；不同引用可并发复制/释放，同一引用变量不能并发修改
// 所属池必须覆盖所有引用及release转交的裸所有权，不依赖线程退出回调
template <class T>
class NetObjectReference
{
public:
    /** 建立空引用
    */
    NetObjectReference();

    /** 借用已存活的源引用并增加池内计数
    @param [in] other 在本调用期间不被其他线程修改的源引用
    */
    NetObjectReference(const NetObjectReference& other);

    /** 转交原引用，不修改对象引用计数
    @param [in,out] other 转交后为空
    */
    NetObjectReference(NetObjectReference&& other);

    /** 归还本引用，最后一个引用才释放实际对象
    */
    ~NetObjectReference();

    /** 替换本引用，先保留源对象再释放旧对象
    @param [in] other 借用的源引用，可与this相同
    @return 当前引用
    */
    NetObjectReference& operator=(const NetObjectReference& other);

    /** 转交引用并释放原有对象
    @param [in,out] other 转交后为空，允许自赋值
    @return 当前引用
    */
    NetObjectReference& operator=(NetObjectReference&& other);

    /** 清空并归还本引用
    */
    void reset();

    /** 查询实际对象，仅在本引用仍拥有期间有效
    @return 借用对象或nullptr
    */
    T* get() const;

    /** 在有效引用上访问实际对象
    @return 借用对象指针
    */
    T* operator->() const;

    /** 在有效引用上访问对象，不做空指针解引用保护
    @return 借用对象引用
    */
    T& operator*() const;

    /** 查询是否拥有对象
    @return 非空为true
    */
    explicit operator bool() const;

    /** 取得不透明池槽；只可由所属池归还，不允许自行解引用或重复释放
    @return 借用的槽标识，空引用为nullptr
    */
    void* storage() const;

    /** 转交原始所有权，不归还池槽
    调用者必须先保存storage，最后通过原池release归还一次；普通调用优先使用reset
    @return 转交的对象，当前引用置空
    */
    T* release();

    /** 仅池工厂接管新建槽的初始引用，不增加计数
    @param [in,out] owner 生命周期覆盖本引用的池
    @param [in] storage 工厂新建且尚未发布的槽
    */
    void adopt(NetObjectPool<T>& owner, void* storage);

private:
    // 对象所属池，必须晚于所有对象归还
    NetObjectPool<T>* m_owner;
    // 已拥有的不透明槽地址；其占用防止底层块缩容回收
    void* m_storage;
};

// 复用基础库的位图池、增长和代际保护；容量只由底层池配置决定
// 对象从入队前到最后使用者退出全程占槽，消费者出队不会提前归还
template <class T>
class NetObjectPool
{
public:
    /** 初始化对象池，配置由实际基础库验证
    @param [in] config 初始槽数、最大槽数、增长策略及保护域，构造时复制
    */
    explicit NetObjectPool(const LockFreeHashMapPoolConfig& config);

    /** 查询初始化结果
    @return NI_OK或内存/配置/平台错误
    */
    int32_t status() const;

    /** 原位创建对象；只在真实池资源不足时返回容量错误
    @param [out] output 必须为空，成功转交一个引用；失败保持为空
    @param [in] args T的构造参数，借用规则由T规定
    @return NI_OK或参数、容量、内存、构造错误，不主动抛异常
    */
    template <class... Args> int32_t create(NetObjectReference<T>& output, Args&&... args);

    /** 增加已拥有槽的引用；不能用于已释放的裸标识
    @param [in] storage 存活的本池槽
    */
    void retain(void* storage);

    /** 归还一个已拥有引用；最后一个引用销毁T
    @param [in] storage 本池槽或nullptr；不能重复归还
    */
    void release(void* storage);

    /** 获取受本引用保护的对象
    @param [in] storage 本池存活槽或nullptr
    @return 借用对象
    */
    T* value(void* storage) const;

    /** 查询基础池实际物理存储，含元数据及暂存责任
    @return 弱一致池统计，不作为接纳判断
    */
    LockFreeHashMapPoolStats statistics();

private:
    /** 禁止复制池所有权
    @param [in] other 不可复制的池
    */
    NetObjectPool(const NetObjectPool& other);

    /** 禁止赋值池所有权
    @param [in] other 不可赋值的池
    @return 不可调用
    */
    NetObjectPool& operator=(const NetObjectPool& other);

    /** 映射基础池结果，不把配置或分配错误伪装为容量不足
    @param [in] result 基础池结果
    @return 网关统一错误码
    */
    static int32_t convert(LockFreeHashMapStatus result);

private:
    // 唯一物理存储及容量控制，没有附加业务条数阈值
    LockFreeHashMapPool<NetObjectPoolItem<T>> m_storage;
    // 初始化结果，初始化后只读
    int32_t m_status;
};

#include "NetObjectPool.inl"