#pragma once
#include "NetTestProcess.h"
#include <string>
#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>

// Windows生命周期观察器，只观察夹具作业内的候选进程，不替代生产窗口实现
class NetGatewayWindowsProbe
{
public:
    /** 只读捕获当前测试进程句柄类型和名称，辅助定位计数变化，不改变资源断言
    @return 逐句柄诊断文本；系统不支持或读取失败时返回错误说明
    */
    static std::string processHandles();

    /** 在资源计量前让本测试主线程实际经过一次Windows地址等待慢路径
    @return API存在且按期限退出时返回true；不支持或系统错误返回false
    */
    static bool warmThreadWait();

    /** 创建本测试的不可见锚点，避免空桌面枚举状态被误判
    */
    NetGatewayWindowsProbe();

    /** 销毁自有锚点，不向用户桌面的其他窗口发送消息
    */
    ~NetGatewayWindowsProbe();

    /** 验证映像的PE可选头确实选择GUI子系统
    @param [in] executable 被测映像路径
    @return PE头完整且为GUI返回true，损坏或读取失败返回false
    */
    static bool guiImage(const std::string& executable);

    /** 查找自有进程的唯一隐藏生命周期窗口，并拒绝任意可见顶层窗口
    @param [in] owner 拥有该进程的测试作业
    @param [in] executable 必须匹配的映像
    @param [in] pid 要观察的存活进程
    @return 唯一隐藏窗口；归属或可见性不符时抛出测试异常
    */
    HWND window(const NetTestProcess& owner, const std::string& executable, uint32_t pid);

    /** 发送真实会话消息，核对窗口仍属于原进程，五秒不处理视为失败
    @param [in] window 本观察器查到的窗口
    @param [in] pid 预期窗口所属进程
    @param [in] message Windows消息编号
    @param [in] value 第一消息参数
    @return 被测窗口实际返回值；发送失败抛出测试异常
    */
    static uintptr_t message(HWND window, uint32_t pid, uint32_t message, uintptr_t value = 0);

    /** 只读白盒检查ProcessWork当前名称目录，结构使用当前头文件，不能写成功状态
    @param [in] name IPC逻辑名称
    @return 无目录为0，完整有效目录为PID；损坏或无法读取抛出测试异常
    */
    static uint32_t receiver(const std::string& name);

private:
    /** EnumWindows回调，仅记录原生字段，不在Win32回调边界抛异常
    @param [in] window 当前枚举窗口
    @param [in] context 本观察器
    @return TRUE继续枚举
    */
    static BOOL CALLBACK Enumerate(HWND window, LPARAM context);

private:
    // 夹具自己的不可见窗口
    HWND m_anchor;
    // 本次枚举的预期PID
    uint32_t m_pid;
    // 唯一匹配的生产生命周期窗口
    HWND m_window;
    // 匹配个数，必须恰好为一
    uint32_t m_count;
    // 目标进程出现可见窗口或查询失败
    bool m_invalid;
};

// 只备份及恢复本产品的当前用户登录项；测试失败也不能残留测试启动路径
class NetTestStartupEntry
{
public:
    /** 打开Run键并备份NetClientManager原值和类型；其他值不读取或修改
    @param [in] backupFile 本批次独占的原始值证据路径
    */
    explicit NetTestStartupEntry(const std::string& backupFile);

    /** 兜底恢复原值，显式restore的结果仍需由用例检查
    */
    ~NetTestStartupEntry();

    /** 验证生产程序写入的REG_SZ内容
    @param [in] value 预期完整启动命令
    @return 类型、长度、终止符和内容完全匹配返回true
    */
    bool matches(const std::wstring& value) const;

    /** 验证生产程序确实移除该值，权限失败不能当作不存在
    @return 查询明确返回ERROR_FILE_NOT_FOUND时为true
    */
    bool absent() const;

    /** 恢复原值及类型；原来不存在则删除测试值，可重复调用
    @return 恢复成功返回true
    */
    bool restore();

private:
    /** 禁止复制注册表恢复责任
    @param [in] other 不可复制对象
    */
    NetTestStartupEntry(const NetTestStartupEntry& other);

    /** 禁止重复拥有同一恢复责任
    @param [in] other 不可赋值对象
    @return 不提供实现
    */
    NetTestStartupEntry& operator=(const NetTestStartupEntry& other);

    // 仅当前用户Run键
    HKEY m_key;
    // 原值是否存在
    bool m_present;
    // 原值类型，不默认其为字符串
    DWORD m_type;
    // 原值原始字节
    std::vector<uint8_t> m_bytes;
    // 是否仍需恢复
    bool m_restore;
};
#endif