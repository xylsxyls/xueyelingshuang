#pragma once
#include <stdint.h>
#include <string>
#include <vector>

// 用测试侧OS替身执行原始启动策略，不启动真实网关或修改开机项
class NetGatewayLaunchTest
{
public:
    /** 运行独立用例并将失败交给统一测试入口
    */
    static void run();

private:
    /** 核对生产接口的边界和资源收尾
    @return 0通过，非0断言失败
    */
    static int check();
};

// 只在测试翻译单元内编译生产启动策略，避免替换正式CSystem符号。
class NetGatewayLaunchSubject
{
public:
    /** 调用生产路径选择实现
    @param [in] businessExecutable 固定业务程序路径
    @return 找到的网关路径，未找到为空
    */
    static std::string find(const std::string& businessExecutable);

    /** 调用生产复用和启动实现
    @param [out] pid 模拟的就绪实例、守护或新启动进程
    @return 实际生产策略的返回码
    */
    static int32_t ensure(uint32_t& pid);
};

// 启动策略的无副作用OS替身，所有状态仅由当前测试进程拥有。
class NetGatewayLaunchSystem
{
public:
    /** 查询模拟进程并记录查询顺序
    @param [in] processName 要查询的可执行文件名
    @return 对应的模拟PID集合
    */
    static std::vector<uint32_t> processPid(const std::string& processName);

    /** 检查模拟租约是否就绪
    @param [in] name 目标租约名称
    @param [in] pid 模拟进程标识
    @return 该进程是否持有已就绪租约
    */
    static bool isProcessLeaseReady(const std::string& name, uint32_t pid);

    /** 返回固定测试路径并累计查询次数
    @return 固定业务EXE路径
    */
    static std::string currentExecutablePathUtf8();

    /** 提供固定目录，其他模式返回空串
    @param [in] path 被查询路径，本替身使用固定夹具
    @param [in] flag 4表示取所在目录
    @return 固定目录或空串
    */
    static std::string GetName(const std::string& path, int32_t flag);

    /** 保留相对段以核对生产选择顺序
    @param [in] directory 起始目录
    @param [in] name 后续路径
    @return 使用斜杠连接的路径
    */
    static std::string joinPath(const std::string& directory, const std::string& name);

    /** 查询夹具中启用的两个部署位置
    @param [in] path 要查询的固定路径
    @return 测试是否指定该文件存在
    */
    static bool fileExists(const std::wstring& path);

    /** 记录启动参数，不创建真实进程
    @param [in] executable 生产策略选定的程序
    @param [in] arguments 复制记录的启动参数
    @param [out] pid 成功为99，失败为0
    @param [out] monitor 成功为900，失败为0
    @return 测试注入的系统错误码
    */
    static uint32_t startDetachedProcess(const std::string& executable, const std::vector<std::string>& arguments,
        uint32_t& pid, uint64_t& monitor);

    /** 记录监视句柄释放并清零
    @param [in,out] monitor 模拟句柄
    */
    static void closeProcessMonitor(uint64_t& monitor);
};

// 固定ASCII路径的转换替身，不替代真实UTF8转换测试。
class NetGatewayLaunchString
{
public:
    /** 将固定ASCII夹具逐字节扩展为宽字符
    @param [in] input 仅含ASCII的固定测试路径
    @return 固定路径的宽字符串副本
    */
    static std::wstring Utf8ToUnicode(const std::string& input);
};