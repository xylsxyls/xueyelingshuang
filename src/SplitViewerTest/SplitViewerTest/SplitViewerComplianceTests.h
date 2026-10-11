#pragma once
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QString>
#include <stdint.h>
class SplitViewerDialogSession;

/** 配置快照、目录和工厂生命周期的独立回归
*/
class SplitViewerComplianceTests
{
public:
    /** 执行一个已登记场景，不改变其他用例的预期
    @param [in] id 217—224之一，220为100轮压力
    @param [in] directory 用例专属证据目录
    @param [in] owner 当前已初始化的工厂所有者
    @return 场景全部独立断言通过
    */
    static bool run(int32_t id, const QString& directory, SplitViewerDialogSession* owner);

private:
    /** 验证配置校验、复制、生效范围和重复收尾
    @return 无跨实例配置污染，失败不改变已有文档
    */
    static bool configSnapshot();

    /** 验证登记完整及严格解析的事务性
    @return 合法选择准确，非法输入不改变原输出
    */
    static bool selection();

    /** 验证重复init以及另一个会话失败不释放当前所有者
    @param [in] owner 当前已注册工厂的所有者
    @return 原会话注册仍有效
    */
    static bool dialogSession(SplitViewerDialogSession* owner);

    /** 验证停止拒绝呈现且不影响其他独立窗口的任务
    @return 停止与实际完成均在预算内被观察
    */
    static bool taskIsolation();

    /** 验证完整快照所有权及失败不替换原输出
    @return 内容独立且映射准确
    */
    static bool sceneSnapshot();

    /** 通过真实拖入检查解码失败与在途关闭
    @param [in] id 223或224
    @param [in] directory 独立素材目录
    @return 原图片保持或关闭收敛
    */
    static bool asyncImage(int32_t id, const QString& directory);

    /** 在100个独立上下文中重复分屏、保存和恢复
    @return 每轮状态准确且隔离文档保持空白
    */
    static bool documentCycles();
};