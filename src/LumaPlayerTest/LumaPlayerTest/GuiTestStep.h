#pragma once
#include <QString>
#include <functional>
#include <stdint.h>

/** GUI事件动作和独立可观察完成条件，单次运行，不创建工作线程 */
struct GuiTestStep
{
public:
    // 对应T编号
    int32_t m_id;
    // 预期行为
    QString m_detail;
    // 真实事件注入
    std::function<void()> m_action;
    // 仅观察，不修改产品状态
    std::function<bool()> m_ready;
    // 最大等待时间
    int32_t m_timeout;
};