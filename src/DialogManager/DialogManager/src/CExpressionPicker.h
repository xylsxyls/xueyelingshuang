#ifndef CEXPRESSIONPICKER_H
#define CEXPRESSIONPICKER_H

#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QMovie>

/** 表情选择控件，负责按组展示表情图标并在鼠标悬停时显示预览
*/
class CExpressionPicker : public QTableView
{
    Q_OBJECT
public:
    /** 表情项在模型中的数据角色
    */
    enum ExpressionRole
    {
        ExpressionRole_GroupId = Qt::UserRole + 1,
        ExpressionRole_Id      = Qt::UserRole + 2,
        ExpressionRole_Desc    = Qt::UserRole + 3,
        ExpressionRole_Shortcut= Qt::UserRole + 4,
        ExpressionRole_Tooltip = Qt::UserRole + 5,
        ExpressionRole_FileName= Qt::UserRole + 6,
    };

    /** 表情项数据，保存分组、文件和提示文本等界面展示信息
    */
    struct Expression
    {
        /** 构造函数，初始化表情项文本字段
        */
        Expression()
        {

        }

        // 表情分组ID
        QString groupid;

        // 表情ID
        QString id;

        // 表情描述
        QString desc;

        // 表情图片文件路径
        QString fileName;

        // 表情快捷文本
        QString shortcut;

        // 表情悬浮提示
        QString tooltip;
    };

    /** 表情项列表
    */
    typedef QList<Expression> ExpressionList;

    /** 构造函数
    @param [in] parent 父控件
    */
    CExpressionPicker(QWidget* parent = nullptr);

    /** 析构函数
    */
    ~CExpressionPicker();

private:
    // 表情表格数据模型
    QStandardItemModel* m_model;

    // 鼠标悬停时显示的大图预览
    QLabel* m_preview;

    // 预览GIF动画对象
    QMovie* m_movie;

    // 当前展示的表情列表
    ExpressionList m_expressionList;

    // 是否显示悬浮预览
    bool m_showPreview;

    // 是否绘制表情图标边框
    bool m_showIconRect;

    // 最大列数
    int m_maxColumnCount;

    // 最大行数
    int m_maxRowCount;

    /** 鼠标离开控件时隐藏预览
    @param [in] eve 离开事件
    */
    void leaveEvent(QEvent* eve);

    /** 鼠标移动时更新悬浮预览
    @param [in] eve 鼠标事件
    */
    void mouseMoveEvent(QMouseEvent* eve);

    /** 处理预览控件事件
    @param [in] obj 触发事件的对象
    @param [in] eve 事件对象
    @return 返回true表示事件已被过滤，否则返回false
    */
    bool eventFilter(QObject* obj, QEvent* eve);

private slots:
    /** 处理表情单元格点击
    @param [in] index 被点击的模型索引
    */
    void onClicked(const QModelIndex& index);

signals:
    /** 表情被点击时发出
    @param [in] expression 被点击的表情数据
    */
    void expressionClicked(const Expression& expression);

public:
    /** 设置是否显示悬浮预览
    @param [in] show true表示显示悬浮预览
    */
    void setShowPreView(bool show);

    /** 判断是否显示悬浮预览
    @return 返回true表示显示悬浮预览
    */
    bool isShowPreView();

    /** 设置是否绘制表情图标边框
    @param [in] show true表示绘制图标边框
    */
    void setShowIconRect(bool show);

    /** 判断是否绘制表情图标边框
    @return 返回true表示绘制图标边框
    */
    bool isShowIconRect();

    /** 设置表情列表并刷新表格模型
    @param [in] li 表情列表
    */
    void setExpressionList(const ExpressionList& li);

    /** 获取当前表情列表
    @return 返回当前表情列表
    */
    ExpressionList expressionList();

    /** 设置最大列数
    @param [in] count 最大列数
    */
    void setMaxColumnCount(int count);

    /** 设置最大行数
    @param [in] count 最大行数
    */
    void setMaxRowCount(int count);

    /** 获取最大列数
    @return 返回最大列数
    */
    int maxColumnCount();

    /** 获取最大行数
    @return 返回最大行数
    */
    int maxRowCount();
};

Q_DECLARE_METATYPE(CExpressionPicker::Expression)
Q_DECLARE_METATYPE(CExpressionPicker::ExpressionList)

#endif // CEXPRESSIONPICKER_H