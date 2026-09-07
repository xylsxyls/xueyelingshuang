#ifndef LOCKEDACCOUNTPANEL_H
#define LOCKEDACCOUNTPANEL_H

#include <QWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include "DialogManagerMacro.h"

class CTreeViewEx;

/** 被封禁账号列表项，使用QStandardItem角色保存封号时间、原因和天数
*/
class DialogManagerAPI LockedAccountItem : public QStandardItem
{
public:
    /** 被封禁账号列表项的数据角色
    */
    enum LockedAccountItemRole
    {
        Role_StartDate    = Qt::UserRole + 1,
        Role_Reasion      = Qt::UserRole + 2,
        Role_DayCount     = Qt::UserRole + 3,
    };

    /** 构造函数
    */
    LockedAccountItem();

    /** 设置封号开始时间
    @param [in] startDate 封号开始时间
    */
    void setStartDate(const QString& startDate);

    /** 获取封号开始时间
    @return 返回封号开始时间
    */
    QString startDate();

    /** 设置封号原因
    @param [in] reasion 封号原因，保留历史接口拼写
    */
    void setReasion(const QString& reasion);

    /** 获取封号原因
    @return 返回封号原因
    */
    QString reasion();

    /** 设置封号天数
    @param [in] dayCount 封号天数
    */
    void setDayCount(quint64 dayCount);

    /** 获取封号天数
    @return 返回封号天数
    */
    quint64 dayCount();
};

typedef QList<LockedAccountItem*> LockedAccountItemList;

/** 被封禁账号列表项绘制委托
*/
class DialogManagerAPI LockedAccountItemDelegate : public QStyledItemDelegate
{
public:
    /** 构造函数
    @param [in] parent 父对象
    */
    LockedAccountItemDelegate(QObject* parent = nullptr);

    /** 绘制列表项内容
    @param [in] painter 绘图对象
    @param [in] option 绘制选项
    @param [in] index 模型索引
    */
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

/** 被封禁账号列表面板，负责展示封号列表标题和表格
*/
class DialogManagerAPI LockedAccountPanel : public QWidget
{
public:
    /** 构造函数
    @param [in] parent 父控件
    */
    LockedAccountPanel(QWidget* parent = nullptr);

private:
    // 被封禁账号表格
    CTreeViewEx* m_treeView;

    // 被封禁账号表格模型
    QStandardItemModel* m_model;

    // 面板标题
    QString m_name;

    /** 绘制面板背景和标题
    @param [in] eve 绘制事件
    */
    void paintEvent(QPaintEvent* eve);

    /** 处理面板尺寸变化
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 调整表格控件布局
    */
    void layoutControls();

public:
    /** 设置面板标题
    @param [in] name 面板标题
    */
    void setName(const QString& name);

    /** 获取面板标题
    @return 返回面板标题
    */
    QString name();

    /** 设置被封禁账号列表
    @param [in] li 被封禁账号列表
    */
    void setLockedAccountItemList(const LockedAccountItemList& li);

	/** 添加一个被封禁账号列表项
	@param [in] item 列表项指针，添加后由模型管理生命周期
	*/
	void appendLockedAccountItem(LockedAccountItem* item);

	/** 清空被封禁账号列表
	*/
	void clearLockedAccountItem();
};

#endif // LOCKEDACCOUNTPANEL_H