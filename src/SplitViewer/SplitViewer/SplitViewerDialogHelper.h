#pragma once
#include <QtCore/QString>
#include "DialogManager/DialogManagerAPI.h"

class QWidget;

/** 业务弹窗交由DialogManager；系统原生文件选择框按约定直接调用Qt接口 */
class SplitViewerDialogHelper
{
public:
    /** 将公共弹窗日志交给当前宿主LogManager；产品和Test共用适配
    @param [in] level 弹窗日志级别
    @param [in] message 完整消息，可空
    */
    static void forwardLog(DialogLogLevel level, const char* message);

    /** 显示同步文件选择窗口
    @param [in] parent 借用的父窗口，调用期间有效
    @param [in] save 是否选择保存路径
    @param [in] title 标题
    @param [in] initial 初始文件或目录
    @param [in] filter Qt文件过滤规则
    @return 确认的路径，取消返回空字符串
    */
    static QString browseFile(QWidget* parent, bool save, const QString& title,
        const QString& initial, const QString& filter);

    /** 显示提示或关于窗口
    @param [in] parent 借用的父窗口，调用期间有效
    @param [in] title 标题
    @param [in] message 提示内容
    */
    static void showMessage(QWidget* parent, const QString& title, const QString& message);

    /** 同步获取输入文本
    @param [in] parent 借用的父窗口，调用期间有效
    @param [in] title 标题
    @param [in] prompt 输入提示
    @param [out] accepted 是否确认
    @return 输入的文本，取消返回空字符串
    */
    static QString inputText(QWidget* parent, const QString& title, const QString& prompt, bool& accepted);
};