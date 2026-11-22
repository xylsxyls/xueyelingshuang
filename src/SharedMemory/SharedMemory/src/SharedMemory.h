#pragma once
#include "SharedMemoryMacro.h"
#ifndef _WIN32
#include <atomic>
#endif
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
    /** 设置命名映射目录；必须在本进程首次命名IPC前由初始化线程调用
    Android使用应用私有共享目录，iOS使用App Group容器中的专用目录；目录须已存在且归当前有效用户所有
    POSIX桌面默认空值使用shm_open，也可显式选择目录；Windows仅接受空值
    配置成功后引用该目录本身。目录改名或原路径被重新创建时，后续映射仍使用原目录
    文件仅作为mmap数据后备，不加文件锁；调用方不得删除或截短正在使用的文件
    @param [in] directory 绝对路径，不含NUL；移动端不允许空值
    @return 目录配置被接受时为true，创建或打开对象的具体访问权限在使用时检查
    */
    static bool setProcessDirectory(const std::string& directory);

    /** 创建或打开内存；size为0只打开，不创建不存在对象
    @param [in] name 逻辑名称；非零容量时空名表示匿名；建议用 1 至 30 字节 ASCII 字母和数字，POSIX 排除斜杠、单点及双点，所有平台排除 NUL
    @param [in] size 请求的容量（字节），非零时创建或加入；0 只打开
    @param [in] isAllowExist 非零容量请求是否允许加入既有对象；false 时重名失败，true 时需要读写权限且后备足以容纳 size 字节。size 为 0 时忽略此参数
    */
    SharedMemory(const std::string& name, uint32_t size = 0, bool isAllowExist = true);

    /** 解除本实例的映射并关闭句柄或描述符
    POSIX 创建者默认撤销名称，已调用 preserveName 的实例保留名称，其他进程的既有映射继续有效
    */
    ~SharedMemory();

    /** 查询是否尚未取得有效资源
    @return 尚未取得有效后备时为 true。只打开的实例在首次成功打开前为 true，POSIX 匿名实例取得容量后为 false
    */
    bool isFailed();

    /** 本次请求的创建容量
    @return 构造时传入的字节数，只打开对象时为 0
    */
    uint32_t size();

    /** 实际后备容量（字节）。Windows 含同一视图中不同保护区域；Windows 和 macOS 默认命名后备可按页取整
    Windows 没有现成视图时临时映射整个对象，查询不会改变 memory()；POSIX 取得后备后复用已核实的容量
    @return 无法查询或容量超出 uint32_t 时返回 0
    */
    uint32_t realSize();

    /** 取得逻辑名称，不暴露原生HANDLE
    @return 构造时传入的名称副本，匿名对象返回空字符串
    */
    std::string mapName();

    /** 最近一次成功取得的视图；获取另一种权限失败时仍保留，close 后清空
    @return 未映射时nullptr
    */
    void* memory();

    /** 取得可读映射；已有后备仅授予读取权限时仍可打开
    Windows 优先使用只读视图。仅授予 FILE_MAP_WRITE 时使用操作系统允许的读写视图
    @return 借用的读地址，close 或析构后失效；失败返回 nullptr。写入须先调用 writeWithoutLock
    */
    void* readWithoutLock();

    /** 取得可读写映射，调用方负责共享数据同步
    @return 借用的读写地址，close 或析构后失效；获取失败返回 nullptr
    */
    void* writeWithoutLock();

    /** 解除本实例的视图，可重复调用，命名对象保留句柄或描述符以便再次映射
    Windows 匿名对象保留数据，POSIX 匿名映射解除后释放数据，再映射得到全零页
    */
    void close();

    /** 将可写视图中构造时请求的字节范围清零
    只打开的实例或无法取得可写视图时直接返回，Windows 请求范围含未提交或不可写页时保留整段内容
    */
    void clear();

    /** POSIX撤销本代名称，既有映射继续有效；Windows由最后一个引用自动释放
    只允许名称生命周期owner调用。非零容量创建或加入失败的实例不能撤名
    只打开的实例可按名清理已确认的遗留对象，不能对可能属于新代次的固定名称盲目调用
    @return true 已移除或本来不存在；同一实例重复成功调用不影响后建同名对象。Windows/匿名对象返回 false
    */
    bool unlinkName();

    /** 保留命名对象至显式unlinkName，适用于由上层CAS决定逻辑所有者的发现入口
    只影响POSIX析构撤名；仍正常解除映射和关闭描述符。Windows不需要撤名
    */
    void preserveName();

private:
    /** 禁止复制资源
    @param [in] other 不可复制实例
    */
    SharedMemory(const SharedMemory& other);

    /** 禁止复制资源
    @param [in] other 不可复制实例
    @return 不可调用
    */
    SharedMemory& operator=(const SharedMemory& other);

    /** 打开既有内存对象，不创建
    @param [in] readOnly 是否只需读权限
    */
    void open(bool readOnly);

#ifdef _WIN32
    /** 打开命名映射，优先保留独立只读视图的能力
    @param [in] readOnly 是否只请求读取现有数据
    @return 调用者负责关闭的句柄，权限不足或对象不存在时返回空
    */
    void* openMappingHandle(bool readOnly) const;

    /** 从现有句柄映射可读数据，优先取得只读视图
    @return 须由调用者解除的视图，无法映射时返回空
    */
    void* mapReadableView() const;

    /** 累计同一视图中连续的区域，达到所需长度即结束
    @param [in] view 从偏移零开始映射的视图基址
    @param [in] required 至少需要核对的字节数
    @param [in] writableOnly 只累计已提交且可写的连续区域
    @return 通过核对的区域总字节数，最后一个区域可以超过所需长度
    */
    static uint64_t windowsViewBytes(void* view, uint64_t required, bool writableOnly);
#else
    /** 释放进程配置持有的目录引用，切换后备或退出时调用
    */
    static void closeProcessDirectory();

#if !defined(__EMSCRIPTEN__)
    /** 打开或排他创建命名后备；文件类型由调用者在取得描述符后验证
    @param [in] flags 读写权限及创建标志
    @return 调用者负责关闭的描述符，失败时返回 -1 并保留 errno
    */
    int openSharedObject(int flags) const;

    /** 为本实例排他创建的命名后备准备存储并设置请求长度
    @return 后备长度已设置时为 true，预留或设置长度失败时由构造函数回收资源
    */
    bool initializeStorage() const;

    /** 查询已打开后备的可表示容量，目录后备还须为普通文件
    @return 可映射的字节数，空对象、类型错误、查询失败或溢出时为 0
    */
    uint32_t sharedObjectCapacity() const;
#endif

    /** 复用或建立与当前描述符权限相符的视图
    @param [in] protection mmap 的 PROT_READ 或 PROT_READ | PROT_WRITE
    @return 借用的视图地址，close 后失效；无有效容量或映射失败时返回 nullptr
    */
    void* mapPosixView(int protection);
#endif

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    // 调用方指定的逻辑名，空值表示匿名
    std::string m_memoryName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
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
    // 全进程持有的后备目录，-1 表示尚未选择目录
    static int m_processDirectoryDescriptor;
    // 首次有效命名请求后封存目录配置
    static std::atomic<bool> m_processDirectorySealed;
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
    // 非负值为本实例持有的后备目录描述符，目录改名后仍指向原目录
    int32_t m_directoryDescriptor;
#endif
};