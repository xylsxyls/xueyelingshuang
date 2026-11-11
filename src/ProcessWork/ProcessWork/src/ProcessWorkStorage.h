#pragma once
#include "ProcessWorkMacro.h"
#include "ProcessWorkProtocol.h"
#include "ProcessWorkMemory.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include <atomic>
#include <memory>

/** 本地元数据页映射；发布之后直到通道静止关闭都不移动、不释放。
*/
class ProcessWorkPage
{
public:
    /** 初始化空映射及正文缓存。
    */
    ProcessWorkPage();

    /** 外部静止时释放本页所有资源。
    */
    ~ProcessWorkPage();

public:
    // 拥有本页的共享元数据映射
    std::unique_ptr<SharedMemory> m_memory;
    // 借用m_memory中的稳定元数据地址
    ProcessWorkSharedMetadata* m_metadata;
    // 本地正文映射所有权；发送侧仅缓存不可缩容的初始块
    std::atomic<SharedMemory*> m_data[256];
    // 仅接收线程读写的正文闲置时间。
    uint64_t m_idleSince[256];
};

/** 第二级本地页表，只有实际访问的组才分配。
*/
class ProcessWorkPageGroup
{
public:
    /** 初始化空目录。
    */
    ProcessWorkPageGroup();

    /** 外部静止时释放已安装的页。
    */
    ~ProcessWorkPageGroup();

public:
    // 发布后保持地址稳定的页对象，析构统一释放
    std::atomic<ProcessWorkPage*> m_pages[256];
};

/** 按需映射元数据页；页表采用完整对象CAS发布，不等待其他映射线程。
*/
class ProcessWorkAPI ProcessWorkStorage
{
public:
    /** 构造空目录，生命期参数此后不变。
    @param [in] pid 接收进程号
    @param [in] identity 接收进程完整身份
    @param [in] session 接收器代次
    @param [in] bytes 正文槽声明大小
    @param [in] initial 不回收的初始正文槽数
    @param [in] receiver true为唯一创建/回收方
    @param [in] config 系统内存保护参数
    */
    ProcessWorkStorage(int32_t pid, uint64_t identity, uint32_t session, int32_t bytes,
        int32_t initial, bool receiver, const ProcessWorkMemoryConfig& config);

    /** 停止本实例所有使用者后释放目录和映射。
    */
    ~ProcessWorkStorage();

    /** 取得并缓存元数据页；创建只允许接收线程。
    @param [in] index 节点或正文下标，范围0..0xffffff
    @param [in] create true允许接收方创建新页
    @return 页对象，失败返回nullptr；成功对象在本实例关闭前稳定
    */
    ProcessWorkPage* page(uint32_t index, bool create = false);

    /** 取得稳定节点元数据，不创建不存在的共享页。
    @param [in] index 节点下标
    @return 元数据或nullptr
    */
    ProcessWorkSharedNode* node(uint32_t index);

    /** 取得稳定正文槽元数据。
    @param [in] index 正文下标
    @return 元数据或nullptr
    */
    ProcessWorkSharedBlock* block(uint32_t index);

    /** 取得稳定的发送线程登记，不创建尚未发布的元数据页
    @param [in] index 1..0xffffff的登记下标
    @return 登记地址或nullptr
    */
    ProcessWorkSharedStream* stream(uint32_t index);

    /** 创建正文映射，最后发布FREE；调用者负责发布extent和active。
    @param [in] index 正文下标，必须大于0
    @param [in] now 本进程单调毫秒
    @return true成功，false未发布正文
    */
    bool create(uint32_t index, uint64_t now);

    /** 在独占正文槽期间取得正文地址。
    @param [in] index 正文下标
    @param [in] generation 已取得槽位的代次
    @param [out] temporary 发送方扩容块映射的所有权，不缓存到下一次发送
    @return 正文地址；失败返回nullptr
    */
    char* data(uint32_t index, uint32_t generation, std::unique_ptr<SharedMemory>& temporary);

    /** 将FREE扩容正文槽退役，初始块不可退役。
    @param [in] index 正文下标
    @return true已释放本地映射并撤名，false仍保持原状态
    */
    bool retire(uint32_t index);

    /** 取得接收线程私有的闲置起点。
    @param [in] index 已映射的正文下标
    @return 闲置字段地址，失败为nullptr
    */
    uint64_t* idleSince(uint32_t index);

    /** 编码v5原生资源名称，正文使用24位下标和24位代次，不截断有效字段。
    @param [in] kind M元数据、B正文、C控制区、R/W通知
    @param [in] pid 接收进程号
    @param [in] identity 完整进程身份
    @param [in] session 24位接收器代次
    @param [in] index 24位下标
    @param [in] generation 24位映射代次
    @return 最多30字节名称，macOS通知最多29字节
    */
    static std::string objectName(char kind, uint32_t pid, uint64_t identity,
        uint32_t session, uint32_t index = 0, uint32_t generation = 0);

private:
    /** 只读取已经发布的本地页目录，未映射时由慢路径负责打开。
    @param [in] index 全局槽下标
    @return 已映射页或nullptr
    */
    ProcessWorkPage* cachedPage(uint32_t index) const;

    /** 禁止复制拥有映射或正文的对象
    @param [in] other 不可复制来源
    */
    ProcessWorkStorage(const ProcessWorkStorage& other);

    /** 禁止覆盖独占资源所有权
    @param [in] other 不可复制来源
    @return 不可调用
    */
    ProcessWorkStorage& operator=(const ProcessWorkStorage& other);

private:
    // 接收进程PID
    int32_t m_pid;
    // 接收进程完整创建标识
    uint64_t m_identity;
    // 本代接收通道的24位标识
    uint32_t m_session;
    // 单个正文槽的声明字节容量
    int32_t m_bytes;
    // 不可缩容的初始正文槽数
    int32_t m_initial;
    // true为唯一创建和回收方
    bool m_receiver;
    // 创建共享页和正文时使用的系统内存保护
    ProcessWorkMemoryConfig m_config;
    // 按需CAS安装的二级页组，不预分配最大目录
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<ProcessWorkPageGroup*> m_groups[256];
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#include "ProcessWorkStorage.inl"