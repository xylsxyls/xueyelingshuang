#pragma once
#include "DialogManager/DialogManagerAPI.h"
#include <mutex>
#include <string>
#include <stdint.h>

/** 日志断言的线程安全状态，不属于应用配置
*/
struct PdfReaderTestLogState
{
    /** 初始不截获日志，首次测试才安装回调
    */
    PdfReaderTestLogState();
    // 保护任意日志线程访问的捕获状态
    std::mutex m_mutex;
    // 测试作用域是否正在截获；结束后的在途消息继续转给宿主
    bool m_enabled;
    // 最近截获消息的级别，供ERROR断言读取
    DialogLogLevel m_level;
    // 最近消息的独立副本，不保存日志系统传入的临时指针
    std::string m_message;
    // 当前作用域收到的消息数，检查错误只回报一次
    int32_t m_count;
};

/** 测试范围内截获日志，正常/异常退出均恢复宿主转发
*/
class PdfReaderTestLogGuard
{
public:
    /** 清空捕获状态并安装回调，不允许嵌套使用
    */
    PdfReaderTestLogGuard();
    /** 关闭捕获并恢复宿主日志，在途回调不借用本对象
    */
    ~PdfReaderTestLogGuard();
    /** 核对唯一ERROR记录包含完整预期文本
    @param [in] text 必须包含的消息内容
    @return 级别、次数、内容均符合返回true
    */
    bool receivedError(const std::string& text) const;

private:
    /** 取得运行期状态；首次调用在GUI安装回调前完成
    @return 进程期状态，无栈对象借用
    */
    static PdfReaderTestLogState& state();
    /** 捕获完整消息，范围外的在途回调仍转发宿主
    @param [in] level 消息级别
    @param [in] message 调用期间有效的消息
    */
    static void capture(DialogLogLevel level, const char* message);
    PdfReaderTestLogGuard(const PdfReaderTestLogGuard&) = delete;
    PdfReaderTestLogGuard& operator=(const PdfReaderTestLogGuard&) = delete;
};