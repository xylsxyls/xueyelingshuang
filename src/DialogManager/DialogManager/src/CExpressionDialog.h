#ifndef CEXPRESSIONDIALOG_H
#define CEXPRESSIONDIALOG_H

#include <QDialog>
#include "CExpressionPicker.h"
#include "DialogManagerMacro.h"

/** 表情选择弹窗，负责加载表情配置并组合表情分组和表情列表控件
*/
class DialogManagerAPI CExpressionDialog : public QDialog
{
    Q_OBJECT
public:
    /** 构造函数
    @param [in] parent 父控件
    */
    CExpressionDialog(QWidget* parent = nullptr);

    /** 析构函数
    */
    ~CExpressionDialog();

private:
    // 表情列表选择控件
    CExpressionPicker* m_expressionPicker;

    // 表情分组选择控件
    CExpressionPicker* m_expressionGroupPicker;

    // 已加载的表情分组列表
    CExpressionPicker::ExpressionList m_groups;

    // 已加载的表情列表
    CExpressionPicker::ExpressionList m_expressions;

    /** 调整表情列表和分组控件布局
    */
    void layoutControl();

    /** 处理窗口大小变化
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 绘制弹窗背景和边框
    @param [in] eve 绘制事件
    */
    void paintEvent(QPaintEvent* eve);

    /** 处理原生窗口激活事件
    @param [in] eventType 原生事件类型
    @param [in] message 原生事件消息
    @param [out] result 原生事件返回结果
    @return 返回true表示事件已处理，否则返回false
    */
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);

signals:
    /** 表情被选中时发出
    @param [in] exp 被选中的表情数据
    */
    void expressionChoosed(const CExpressionPicker::Expression& exp);

    /** 非客户区激活状态变化时发出
    @param [in] ncActive true表示窗口处于激活状态
    */
    void ncActiveChanged(const bool& ncActive);

private slots:
    /** 根据分组切换当前表情列表
    @param [in] group 被选中的表情分组
    */
    void selectGroup(const CExpressionPicker::Expression& group);

public :
    /** 加载表情配置
    @param [in] emotionPath Emotions文件夹所在目录
    @return 返回true表示加载成功
    */
    bool loadExpressions(const QString& emotionPath);
};

#endif // CEXPRESSIONDIALOG_H