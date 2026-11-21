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
    配置成功后引用该目录本身。目录改名或原路径被重新创建时，后续映射仍使用原目录。
    文件仅作为mmap数据后备，不加文件锁；调用方不得删除或截短正在使用的文件。
    @param [in] directory 绝对路径，不含NUL；移动端不允许空值
    @return 配置有效为true，已启用后更换目录或权限不允许为false
    */
    static bool setProcessDirectory(const std::string& directory);

    /** 创建或打开内存；size为0只打开，不创建不存在对象。
    @param [in] name 逻辑名称；创建时空名表示匿名内存；跨桌面可移植名称不超过30字节且无斜杠/NUL
    @param [in] size 创建容量，0只打开
    @param [in] isAllowExist 是否允许加入既有对象；false 碰撞明确失败，true 加入时须获读写权限并能映射请求容量
    */
    SharedMemory(const std::string& name, uint32_t size = 0, bool isAllowExist = true);

    /** 解除映射并释放句柄；POSIX新建者移除名称，既有对端映射仍然有效。 */
    ~SharedMemory();

    /** 查询是否尚未取得有效资源。
    @return 尚无有效句柄时为true；只打开的实例可能要在首次映射后才成功
    */
    bool isFailed();

    /** 本次请求的创建容量。
    @return 只打开对象时0
    */
    uint32_t size();

    /** 实际映射容量。Windows 按页取整，并包含同一视图中不同保护属性的区域。
    @return 无效或容量无法用 uint32_t 表示时返回 0
    */
    uint32_t realSize();

    /** 取得逻辑名称，不暴露原生HANDLE。
    @return 构造时的名称
    */
    std::string mapName();

    /** 最近一次成功取得的视图；获取另一种权限失败时仍保留，close 后清空。
    @return 未映射时nullptr
    */
    void* memory();

    /** 取得可读映射；已有后备仅授予读取权限时仍可打开。
    Windows 优先使用只读视图。仅授予 FILE_MAP_WRITE 时使用操作系统允许的读写视图。
    @return 映射失败为nullptr
    */
    void* readWithoutLock();

    /** 取得可读写映射，调用方负责共享数据同步。
    @return 映射失败为nullptr
    */
    void* writeWithoutLock();

    /** 解除本实例视图；重复调用安全。命名对象保留句柄以便再次映射。
    Windows 匿名对象保留数据；POSIX 匿名映射解除即释放数据，再映射将得到全零页。 */
    void close();

    /** 清零本次请求范围。只打开的实例直接返回。Windows 请求范围包含未提交页或不可写页时整段保留。 */
    void clear();

    /** POSIX撤销本代名称，既有映射继续有效；Windows由最后一个引用自动释放。
    只允许名称生命周期owner调用。非零容量创建或加入失败的实例不能撤名。
    只打开的实例可按名清理已确认的遗留对象，不能对可能属于新代次的固定名称盲目调用。
    @return true 已移除或本来不存在；同一实例重复成功调用不影响后建同名对象。Windows/匿名对象返回 false
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
    // 调用方指定的逻辑名，空值表示匿名
    std::string m_memoryName;
    // 构造时请求的字节数，只打开对象时为0
    uint32_t m_size;
    // 最近成功请求的映射视图，close后失效
    void* m_memoryPtr;
#ifdef _WIN32
    // 原生句柄仅在库内部存在，不进入公共函数参数
    void* m_memoryHandle;
    // Windows 独立读视图，仅授予 FILE_MAP_WRITE 时该视图也可写
    void* m_readMemoryPtr;
    // Windows独立读写视图
    void* m_writeMemoryPtr;
#else
    // POSIX映射后备描述符，未取得时为-1
    int32_t m_descriptor;
    // 后备描述符仅有读权限，禁止从该实例取得可写视图
    bool m_readOnly;
    // 已验证的映射字节数
    uint32_t m_capacity;
    // 本实例是否负责在销毁时撤销名称
    bool m_created;
    // 本实例是否已成功撤名，防止重复操作影响后来复用名称的映射
    bool m_nameUnlinked;
    // 原生对象名称；目录后备时为相对于已持有目录描述符的数据文件名
    std::string m_nativeName;
    // 非负值表示基于宿主目录的映射；持有目录描述符，避免路径在映射期被替换。
    int32_t m_directoryDescriptor;
#endif
};