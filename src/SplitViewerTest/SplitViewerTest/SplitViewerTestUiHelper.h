#pragma once
#include "../../SplitViewer/SplitViewer/SplitViewer.h"
#include <QtCore/QTextStream>
#include <QtGui/QColor>

/** 测试输入和独立判定辅助，不保存会话状态
*/
class SplitViewerTestUiHelper
{
public:
    /** 测试专用等待实际完成；生产GUI不调用本函数
    @param [in] widget 被测主窗口或其子控件
    @return 在5秒预算内结束后台请求
    */
    static bool settle(QWidget* widget);

    /** 测试专用实际点击，在前后等待产品请求完成
    @param [in] widget 输入控件
    @param [in] button 触发按钮
    @param [in] modifiers 键盘修饰键
    @param [in] point 局部坐标
    */
    static void click(QWidget* widget, Qt::MouseButton button, Qt::KeyboardModifiers modifiers, const QPoint& point);

    /** 测试专用按键，等待已提交操作实际完成
    @param [in] widget 接收控件
    @param [in] key 按键
    @param [in] modifiers 修饰键
    */
    static void key(QWidget* widget, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /** 打开真实菜单并点击启用的目标项
    @param [in] canvas 产品画布，不能为空
    @param [in] point 画布内命中点
    @param [in] label 需匹配的菜单文案
    @return 是否点击成功
    */
    static bool chooseMenu(QWidget* canvas, const QPoint& point, const QString& label);

    /** 按独立的旧版布局基准计算舞台
    @param [in] canvas 产品画布，不能为空
    @return 桌面比例与10像素边距决定的舞台矩形
    */
    static QRectF expectedStage(QWidget* canvas);

    /** 检查真实控件渲染的指定像素
    @param [in] widget 待检查控件
    @param [in] point 控件内采样点
    @param [in] expected 独立预期颜色
    @return 采样点有效且颜色一致
    */
    static bool pixel(QWidget* widget, const QPoint& point, const QColor& expected);

    /** 投递按下、移动和释放组成的完整手势
    @param [in] widget 目标控件
    @param [in] from 起点
    @param [in] to 终点
    @param [in] modifiers 键盘修饰键
    */
    static void drag(
        QWidget* widget, const QPoint& from, const QPoint& to, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /** 投递真实滚轮事件并处理呈现
    @param [in] widget 目标控件
    @param [in] point 采样位置
    @param [in] delta 角度，一格为120
    @param [in] modifiers 键盘修饰键
    */
    static void wheel(
        QWidget* widget, const QPoint& point, int delta, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    /** 通过真实拖入和落下事件加载文件
    @param [in] widget 目标控件
    @param [in] point 落点
    @param [in] path 测试文件路径
    @return 两个事件均被接受
    */
    static bool drop(QWidget* widget, const QPoint& point, const QString& path);

    /** 点击实际工具栏动作对应的可见控件
    @param [in] window 测试产品窗口
    @param [in] label 动作标题或提示
    @return 是否找到并点击动作
    */
    static bool toolbar(SplitViewer& window, const QString& label);

    /** 读取落盘配置并交公开Core接口解析
    @param [in] path 测试配置路径
    @param [in,out] document 输出文档，失败保持原状态
    @return 是否成功恢复
    */
    static bool readDocument(const QString& path, SplitViewerCoreDocument& document);

    /** 生成手写配置的UTF16LE测试输入
    @param [in] text 独立测试文本
    @return 包含BOM的测试字节
    */
    static std::vector<uint8_t> utf16(const QString& text);
};