#pragma once
#include <QtCore/QString>
#include <QtCore/QRectF>
class QWidget;
class QTextStream;
class SplitViewerTestWindow;
class SplitViewerCoreDocument;

/** 使用真实产品入口验证移动、媒体恢复及进程收尾
*/
class SplitViewerMoveUiTests
{
public:
    /** 执行一个界面用例，素材由Test生成
    @param [in] id 稳定编号207至216及225至228
    @param [in] directory 本用例独立证据目录
    @param [in] videoPath 独立AVI素材
    @param [in] playerPath 本次指定的真实播放器
    @param [in,out] log 结果及进程身份记录
    @return 所有预期是否满足
    */
    static bool run(
        int id, const QString& directory, const QString& videoPath, const QString& playerPath, QTextStream& log);

    /** 仅Test子进程：不创建窗口，写PID并等待收尾，以复现启动中取消
    @param [in] path 本次素材路径，PID记录写在其旁边
    @return 进程退出码
    */
    static int runStartupProbe(const QString& path);

    /** 仅Test子进程：窗口接受关闭并销毁，事件循环继续，独立模拟消失窗口和存活进程
    @return 本测试子进程退出码
    */
    static int runLifetimeProbe();

private:
    /** 根据桌面比例独立推导舞台区域
    @param [in] canvas 工作区
    @return 舞台内部区域
    */
    static QRectF stage(QWidget* canvas);

    /** 真实菜单选择，禁用项目或找不到返回false
    @param [in] canvas 工作区
    @param [in] point 命中位置
    @param [in] text 精确菜单文案
    @param [in] native 使用Windows实际右键输入
    @param [in,out] log 可选菜单与原生窗口诊断记录
    @return 是否实际点击已启用项目
    */
    static bool menu(
        QWidget* canvas, const QPoint& point, const QString& text, bool native = false, QTextStream* log = nullptr);

    /** 从产品保存的包提取Core状态
    @param [in,out] window 产品窗口，仅替代文件选择返回
    @param [in] path 快照路径
    @param [out] document 保存结果
    @return 是否成功保存和独立读取
    */
    static bool snapshot(SplitViewerTestWindow& window, const QString& path, SplitViewerCoreDocument& document);

    /** 查询并等待本宿主的嵌入原生窗口
    @param [in] window 宿主
    @param [in] expected 要等待的数量
    @return 是否达到指定数量
    */
    static bool waitEmbedded(SplitViewerTestWindow& window, int expected);

    /** 观察指定区域的真实桌面视频像素
    @param [in] canvas 工作区
    @param [in] rect 本地分屏区域
    @param [in] output 截图路径
    @return 四象限独立像素预期是否满足
    */
    static bool videoPixels(QWidget* canvas, const QRectF& rect, const QString& output);

    /** 对AVI中独立帧编号取样，确认移动后视频仍在播放
    @param [in] canvas 工作区
    @param [in] rect 视频分屏本地区域
    @return 是否观察到不同帧编号
    */
    static bool videoAdvances(QWidget* canvas, const QRectF& rect);

    /** 模拟播放器延后改位，静置观察宿主恢复而不触发布局
    @param [in] container 当前借用的嵌入宿主
    @param [in] windowId 本用例启动的播放器窗口身份
    @param [in,out] log 初始偏移及最终客户区诊断
    @return 是否实际改位且在一秒内恢复到客户区原点并填满
    */
    static bool displacedWindowReturns(QWidget* container, quintptr windowId, QTextStream& log);
};