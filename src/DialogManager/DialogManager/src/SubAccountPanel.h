#ifndef SUBACCOUNTPANEL_H
#define SUBACCOUNTPANEL_H

#include <QWidget>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include "DialogManagerMacro.h"

class CExternalTextEdit;
class CTreeViewEx;
class PushButton;

/** 子账号列表项，使用QStandardItem角色保存子账号名称、登录信息、锁定状态等数据
*/
class DialogManagerAPI SubAccountItem : public QStandardItem
{
public:
    /** 子账号列表项的数据角色
    */
    enum SubAccountItemRole
    {
		Role_Name            = Qt::UserRole + 1,
		Role_Date            = Qt::UserRole + 2,
		Role_Desc            = Qt::UserRole + 3,
		Role_Id              = Qt::UserRole + 4,
		Role_Locked          = Qt::UserRole + 5,
		Role_IsCurrent       = Qt::UserRole + 6,
		Role_LockedTime      = Qt::UserRole + 7,
		Role_LockedReasion   = Qt::UserRole + 8,
		Role_LockedDayCount  = Qt::UserRole + 9,
		Role_CanRename       = Qt::UserRole + 10,
    };

    /** 构造函数
    */
    SubAccountItem();

    /** 设置子账号名称
    @param [in] name 子账号名称
    */
    void setName(const QString& name);

    /** 获取子账号名称
    @return 返回子账号名称
    */
    QString name();

    /** 设置登录日期文本
    @param [in] date 登录日期文本
    */
    void setDate(const QString& date);

    /** 获取登录日期文本
    @return 返回登录日期文本
    */
    QString date();

    /** 设置描述文本
    @param [in] desc 描述文本
    */
    void setDesc(const QString& desc);

    /** 获取描述文本
    @return 返回描述文本
    */
    QString desc();

    /** 设置子账号ID
    @param [in] id 子账号ID
    */
    void setId(quint64 id);

    /** 获取子账号ID
    @return 返回子账号ID
    */
    quint64 id();

    /** 设置是否被锁定
    @param [in] locked true表示被锁定
    */
    void setLocked(bool locked);

    /** 判断是否被锁定
    @return 返回true表示被锁定
    */
    bool locked();

	/** 设置是否是当前子账号
	@param [in] current true表示当前子账号
	*/
	void setIsCurrent(bool current);

	/** 判断是否是当前子账号
	@return 返回true表示当前子账号
	*/
	bool isCurrent();

	/** 设置锁定时间
	@param [in] lockedTime 锁定时间
	*/
	void setLockedTime(const QString& lockedTime);

	/** 获取锁定时间
	@return 返回锁定时间
	*/
	QString lockedTime();

	/** 设置锁定原因
	@param [in] lockedReasion 锁定原因，保留历史接口拼写
	*/
	void setLockedReasion(const QString& lockedReasion);

	/** 获取锁定原因
	@return 返回锁定原因
	*/
	QString lockedReasion();

	/** 设置锁定天数
	@param [in] lockedDayCount 锁定天数
	*/
	void setLockedDayCount(quint64 lockedDayCount);

	/** 获取锁定天数
	@return 返回锁定天数
	*/
	quint64 lockedDayCount();

	/** 设置是否允许改名
	@param [in] canRename true表示允许改名
	*/
	void setCanRename(bool canRename);

	/** 判断是否允许改名
	@return 返回true表示允许改名
	*/
	bool canRename();
};

/** 子账号列表项指针列表
*/
typedef QList<SubAccountItem*> SubAccountItemList;

/** 子账号列表项内嵌按钮面板，负责切换、帮助和改名入口
*/
class SubAccountItemView : public QWidget
{
    Q_OBJECT
public:
    /** 构造函数
    @param [in] parent 父控件
    */
    SubAccountItemView(QWidget* parent = nullptr);
private:
    // 切换子账号按钮
    PushButton* m_switchButton;

	// 锁定帮助按钮
	PushButton* m_helpButton;

	// 修改子账号名称按钮
	PushButton* m_changeNameButton;

	// 当前子账号是否处于锁定状态
	bool m_blocked;

    // 当前列表项对应的子账号ID
    quint64 m_id;

    /** 处理列表项面板尺寸变化
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 调整列表项内按钮布局
    */
    void layoutControls();

signals:
    /** 请求切换子账号
    @param [in] id 子账号ID
    */
    void switchAccount(quint64 id);

	/** 请求显示锁定帮助
	*/
	void helpButtonClicked();

	/** 请求修改子账号名称
	@param [in] id 子账号ID
	*/
	void renameAccount(quint64 id);

private slots:
    /** 处理切换子账号按钮点击
    */
    void onSwitchButtonClicked();

	/** 处理修改子账号名称按钮点击
	*/
	void onChangeNameButtonClicked();

public:
    /** 设置子账号ID
    @param [in] id 子账号ID
    */
    void setId(quint64 id);

	/** 设置切换按钮是否显示
	@param [in] show true表示显示
	*/
	void showSwitchButton(bool show);

	/** 设置锁定状态
	@param [in] blocked true表示锁定
	*/
	void setBlocked(bool blocked);
};

/** 子账号列表项绘制和编辑委托
*/
class DialogManagerAPI SubAccountItemDelegate : public QStyledItemDelegate
{
public:
    /** 构造函数
    @param [in] parent 父对象
    */
    SubAccountItemDelegate(QObject* parent = nullptr);

    /** 绘制子账号列表项内容
    @param [in] painter 绘图对象
    @param [in] option 绘制选项
    @param [in] index 模型索引
    */
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    /** 创建列表项内嵌按钮面板
    @param [in] parent 父控件
    @param [in] option 绘制选项
    @param [in] index 模型索引
    @return 返回创建的编辑控件
    */
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    /** 将模型数据同步到列表项内嵌按钮面板
    @param [in] editor 编辑控件
    @param [in] index 模型索引
    */
    void setEditorData(QWidget* editor, const QModelIndex& index) const;

    /** 更新列表项内嵌按钮面板位置
    @param [in] editor 编辑控件
    @param [in] option 绘制选项
    @param [in] index 模型索引
    */
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};


/** 子账号管理面板，负责展示子账号列表、创建入口和创建规则说明
*/
class DialogManagerAPI SubAccountPanel : public QWidget
{
    Q_OBJECT
public:
    /** 构造函数
    @param [in] parent 父控件
    */
    SubAccountPanel(QWidget* parent = nullptr);

    /** 析构函数
    */
    ~SubAccountPanel();

private:
    // 子账号列表控件
    CTreeViewEx* m_treeView;

    // 子账号列表模型
    QStandardItemModel* m_model;

    // 创建子账号按钮
    PushButton* m_createSubAccountButton;

    // 创建规则帮助按钮
    PushButton* m_helpButton;

    // 创建规则浮层提示
    CExternalTextEdit* m_helpTip;

    // 当前可创建的子账号数量
    quint64 m_canCreateCount;

    /** 处理面板移动，更新帮助浮层位置
    @param [in] eve 移动事件
    */
    void moveEvent(QMoveEvent* eve);

    /** 调整面板内部控件布局
    */
    void layoutControls();

    /** 处理面板尺寸变化
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 绘制面板背景和说明区
    @param [in] eve 绘制事件
    */
    void paintEvent(QPaintEvent* eve);

    /** 处理帮助按钮和面板鼠标事件
    @param [in] obj 触发事件的对象
    @param [in] eve 事件对象
    @return 返回true表示事件已被过滤，否则返回false
    */
    bool eventFilter(QObject* obj, QEvent* eve);

signals:
    /** 请求创建子账号
    */
    void createSubAccount();

    /** 请求切换子账号
    @param [in] id 子账号ID
    */
    void siwtchSubAccount(quint64 id);

	/** 请求修改子账号名称
	@param [in] id 子账号ID
	*/
	void renameSubAccount(quint64 id);

	/** 请求显示帮助说明
	*/
	void helpButtonClicked();

public:
    /** 将子账号项列表转换为标准项列表
    @param [in] li 子账号项列表
    @return 返回标准项列表
    */
    QList<QStandardItem*> subAccountItemLiToStandardItemLi(const SubAccountItemList& li);

    /** 将标准项列表转换为子账号项列表
    @param [in] li 标准项列表
    @return 返回子账号项列表
    */
    SubAccountItemList    standardItemLiToSubAccountItemLi(const QList<QStandardItem*>& li);

	/** 获取帮助提示浮层
	@return 返回帮助提示浮层指针
	*/
	CExternalTextEdit* helpTip();

	/** 获取可创建的子账号数量
	@return 返回可创建的子账号数量
	*/
	quint64 canCreateCount();

    /** 设置可创建的子账号数量
    @param [in] count 可创建的子账号数量
    */
    void setCanCreateCount(quint64 count);

    /** 设置子账号列表
    @param [in] li 子账号列表
    */
    void setSubAccountList(const SubAccountItemList& li);

	/** 获取子账号列表
	@return 返回子账号列表
	*/
	SubAccountItemList subAccountList();

	/** 添加子账号
	@param [in] item 子账号项指针，添加后由模型管理生命周期
	*/
	void appendSubAccount(SubAccountItem* item);

	/** 根据ID获取子账号
	@param [in] id 子账号ID
	@return 返回子账号项指针，未找到时返回nullptr
	*/
	SubAccountItem* getSubAccountItemById(quint64 id);

	/** 根据ID删除子账号
	@param [in] id 子账号ID
	*/
	void removeSubAccountItem(quint64 id);
};

#endif // SUBACCOUNTPANEL_H