#pragma once
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <QtCore/QTextStream>
#include <QtGui/QColor>
#ifdef Q_OS_WIN
#include <windows.h>

/** 测试输入和独立判定辅助，不保存会话状态
*/
class SplitViewerTestVideoHelper
{
public:
    /** 生成带四象限和帧编号的独立AVI素材
    @param [in] path 本批次输出路径
    @return 完整文件写入成功
    */
    static bool writeVideoFixture(const QString& path);

    /** 截取实际桌面中的视频分屏
    @param [in] pane 屏幕坐标矩形
    @return 桌面像素，失败为空图像
    */
    static QImage screenPane(const QRect& pane);

    /** 按独立宽高比采样四象限颜色
    @param [in] screen 实际桌面图像
    @return 四个象限均符合预期
    */
    static bool videoPixelsFill(const QImage& screen);

    /** 从独立生成的编号条读取实际视频帧
    @param [in] screen 实际分屏截图
    @return 帧编号，画面不符返回-1
    */
    static int videoFrameNumber(const QImage& screen);

    /** 在有界观察期内检查不同的实际帧编号
    @param [in] pane 屏幕上的分屏矩形
    @param [in] observationMilliseconds 观察上限，毫秒
    @param [in] trace 可选诊断输出，仅本次调用借用
    @return 至少观察到两个不同有效帧
    */
    static bool videoIsAdvancing(const QRect& pane, int observationMilliseconds = 800, QTextStream* trace = nullptr);
};
#endif