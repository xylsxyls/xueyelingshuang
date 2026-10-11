#pragma once
#include "GuiTestStep.h"
#include "TestResults.h"
#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QRect>
#include <QImage>
#include <vector>

class LumaPlayer;

/** 实际播放器GUI事件测试，使用事件循环等待，不阻塞GUI
*/
class GuiTestRunner : public QObject
{
public:
    /** 创建步骤但不运行
    @param [in] player 测试期间存活的播放器
    @param [in] media 规则媒体
    @param [in] directory 输出目录
    @param [in] mode gui/quick；quick用于验证启动就绪和退出
    */
    GuiTestRunner(LumaPlayer* player, const QString& media, const QString& directory, const QString& mode);

    /** 开始事件驱动测试
    */
    void start();

    /** 只有全部步骤执行并通过才算完成，事件循环退出本身不是完成证据
    @return 全部步骤实际执行且通过才返回true
    */
    bool completed() const;

private:
    /** 向桌面坐标发送一次鼠标点击，Windows使用系统输入
    @param [in] point 桌面像素坐标
    @return 输入成功提交；不代表目标操作已完成
    */
    static bool clickDesktopPoint(const QPoint& point);

    /** 通过系统键盘输入提交原生保存路径
    @param [in] path 要填入原生文件框的路径
    @return Windows输入成功提交；其他平台返回false
    */
    static bool acceptDesktopSavePath(const QString& path);

    /** 添加一步真实交互和观察条件
    @param [in] id 用例编号
    @param [in] detail 预期说明
    @param [in] action 事件操作
    @param [in] ready 只读完成条件
    @param [in] timeout 超时毫秒
    */
    void add(int32_t id, const QString& detail, const std::function<void()>& action,
        const std::function<bool()>& ready, int32_t timeout = 4000);

    /** 驱动下一步；模态嵌套事件循环也可推进
    */
    void tick();

    /** 向窗口发送鼠标移动
    @param [in] point 本地坐标
    @param [in] buttons 当前按键集合
    */
    void move(const QPoint& point, Qt::MouseButtons buttons = Qt::NoButton);

    /** 发送完整按下/松开
    @param [in] point 本地坐标
    @param [in] button 按键
    */
    void click(const QPoint& point, Qt::MouseButton button = Qt::LeftButton);

    /** 发送键盘按下和松开
    @param [in] key Qt键值
    @param [in] modifiers 修饰键
    */
    void key(int32_t key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /** 发送视频双击
    */
    void doubleClick();

    /** 通过菜单鼠标事件选择条目
    @param [in] index 条目下标，负数取消
    */
    void chooseMenu(int32_t index);

    /** 通过桌面鼠标选择指定麦克风槽位的输入设备，包含菜单命中检查
    @param [in] slot 麦克风槽位索引，0至2
    @param [in] device 枚举时的设备索引
    @param [in] caseId 报告归属的稳定用例ID
    */
    void selectMicrophoneDevice(int32_t slot, int32_t device, int32_t caseId = 63);

    /** 在时间位置右击并选菜单
    @param [in] position 100ns
    @param [in] index 菜单下标
    */
    void menuAt(int64_t position, int32_t index);

private:
    // 借用真实播放器
    LumaPlayer* m_player;
    // 测试输出
    TestResults m_results;
    // 报告目录
    QString m_directory;
    // 有序步骤
    std::vector<GuiTestStep> m_steps;
    // 当前步骤索引
    size_t m_index;
    // 当前动作已经发出
    bool m_started;
    // 失败后仅收敛关闭
    bool m_aborting;
    // 按下态前的截图
    QImage m_image;
    // 事件计时
    QElapsedTimer m_elapsed;
    // 主线程定时器
    QTimer m_timer;
    // 还原目标矩形
    QRect m_normal;
    // 异步操作前位置
    int64_t m_position;
    // 拖动时统计跟手次数
    int32_t m_dragMatches;
    // 定位期间是否观察到错误暂停；观察覆盖后续真实画面推进
    bool m_unexpectedPause;
    // 播放中AB解析期间的实际帧间隔观察
    bool m_monitorFrames;
    // GUI观察器最近读取的视频帧时间戳，100纳秒单位
    int64_t m_observedFrame;
    // GUI观察器最近观测到新帧的单调毫秒时间
    int64_t m_lastFrameMs;
    // GUI观察器记录的最大帧间间隔，毫秒
    int64_t m_maxFrameGapMs;
    // 保留默认帮助框宽度，用奇数配置覆盖实际弹窗的向上取偶路径
    int32_t m_originalHelpWidth;
};