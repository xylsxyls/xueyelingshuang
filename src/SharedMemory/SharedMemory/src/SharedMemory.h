#pragma once
#include "SharedMemoryMacro.h"
#include <stdint.h>
#include <string>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE)
#define SHARED_MEMORY_MOBILE 1
#endif

/** 命名共享内存；同一实例的创建、映射与关闭由生命周期owner串行执行。
    Windows使用页映射，桌面POSIX默认shm_open，移动端使用共享目录mmap；均无文件锁。
*/
class SharedMemoryAPI SharedMemory
{
public:
    /** 设置命名映射目录；必须在本进程首次命名IPC前由初始化线程调用。
    Android使用应用私有共享目录，iOS使用App Group容器中的专用目录，目录须已存在。
    POSIX桌面默认空值使用shm_open，也可显式选择目录；Windows仅接受空值。
    文件仅作为mmap数据后备，不加文件锁；调用方不得删除或截短正在使用的文件。
    @param [in] directory 绝对路径，不含NUL；移动端不允许空值
    @return 配置有效为true，已启用后更换目录或权限不允许为false
    */
    static bool setProcessDirectory(const std::string& directory);

    /** 创建或打开内存；size为0只打开，不创建不存在对象。
    @param [in] name 逻辑名称；创建时空名表示匿名内存；跨桌面可移植名称不超过30字节且无斜杠/NUL
    @param [in] size 创建容量，0只打开
    @param [in] isAllowExist 是否允许加入既有对象；false碰撞明确失败
    */
    SharedMemory(const std::string& name, uint32_t size = 0, bool isAllowExist = true);
    /** 解除映射并释放句柄；POSIX新建者移除名称，既有对端映射仍然有效。 */
    ~SharedMemory();
    /** 查询是否尚未取得有效资源。
    @return true创建/打开失败
    */
    bool isFailed();
    /** 本次请求的创建容量。
    @return 只打开对象时0
    */
    uint32_t size();
    /** 实际映射容量，Windows可能按页向上取整。
    @return 无效资源返回0
    */
    uint32_t realSize();
    /** 取得逻辑名称，不暴露原生HANDLE。
    @return 构造时的名称
    */
    std::string mapName();
    /** 已映射地址，不隐式创建映射。
    @return 未映射时nullptr
    */
    void* memory();
    /** 取得可读映射。
    @return 映射失败为nullptr
    */
    void* readWithoutLock();
    /** 取得可读写映射，调用方负责共享数据同步。
    @return 映射失败为nullptr
    */
    void* writeWithoutLock();
    /** 解除本实例视图，保留对象句柄以便再次映射；重复调用安全。 */
    void close();
    /** 清零本次创建容量；只打开的对象不擅自清零。 */
    void clear();
    /** POSIX撤销本代名称，既有映射继续有效；Windows由最后一个引用自动释放。
    只允许名称生命周期owner调用；不能对可能属于新代次的固定名称盲目调用。
    @return true已移除或本来不存在；Windows/匿名对象返回false
    */
    bool unlinkName();
    /** 保留命名对象至显式unlinkName，适用于由上层CAS决定逻辑所有者的发现入口。
    只影响POSIX析构撤名；仍正常解除映射和关闭描述符。Windows不需要撤名。
    */
    void preserveName();
private:
    /** 禁止复制资源。
    @param [in] other 不可复制实例
    */
    SharedMemory(const SharedMemory& other);
    /** 禁止复制资源。
    @param [in] other 不可复制实例
    @return 不可调用
    */
    SharedMemory& operator=(const SharedMemory& other);
    /** 打开既有内存对象，不创建。
    @param [in] readOnly 是否只需读权限
    */
    void open(bool readOnly);
private:
    // 调用方逻辑名及本次请求容量
    std::string m_memoryName;
    uint32_t m_size;
    // 当前借用视图
    void* m_memoryPtr;
#ifdef _WIN32
    // 原生句柄仅在库内部存在，不进入公共函数参数
    void* m_memoryHandle;
    // Windows独立读写视图
    void* m_readMemoryPtr;
    void* m_writeMemoryPtr;
#else
    // POSIX描述符、已验证容量及名称所有权
    int32_t m_descriptor;
    uint32_t m_capacity;
    bool m_created;
    // 原生对象名称；目录后备时为相对于已持有目录描述符的数据文件名
    std::string m_nativeName;
    // 非负值表示基于宿主目录的映射；持有目录描述符，避免路径在映射期被替换。
    int32_t m_directoryDescriptor;
#endif
};