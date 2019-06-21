#ifndef SUBACCOUNTPANEL_H
#define SUBACCOUNTPANEL_H

#include <QWidget>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include "DialogManagerMacro.h"

class CExternalTextEdit;
class CTreeViewEx;
class COriginalButton;

class DialogManagerAPI SubAccountItem : public QStandardItem
{
public:
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
    SubAccountItem(){this->setSizeHint(QSize(0,94));
                     this->setLocked(false);
                     this->setId(0);
					 this->setIsCurrent(false);}

    void setName(const QString& s){setData(s,Role_Name);}
    QString name(){return data(Role_Name).toString();}
    void setDate(const QString& s){setData(s, Role_Date);}
    QString date(){return data(Role_Date).toString();}
    void setDesc(const QString& s){setData(s, Role_Desc);}
    QString desc(){return data(Role_Desc).toString();}
    void setId(quint64 v){setData(v, Role_Id);}
    quint64 id(){return data(Role_Id).toULongLong();}
    void setLocked(bool s){setData(s, Role_Locked);}
    bool locked(){return data(Role_Locked).toBool();}
	void setIsCurrent(bool s){setData(s, Role_IsCurrent);}
	bool isCurrent(){return data(Role_IsCurrent).toBool();}
	void setLockedTime(const QString& s){setData(s,Role_LockedTime);}
	QString lockedTime(){return data(Role_LockedTime).toString();}
	void setLockedReasion(const QString& s){setData(s, Role_LockedReasion);}
	QString lockedReasion(){return data(Role_LockedReasion).toString();}
	void setLockedDayCount(quint64 v){setData(v, Role_LockedDayCount);}
	quint64 lockedDayCount(){return data(Role_LockedDayCount).toULongLong();}
	void setCanRename(bool s){setData(s, Role_CanRename);}
	bool canRename(){return data(Role_CanRename).toBool();}
};

typedef QList<SubAccountItem*> SubAccountItemList;

class SubAccountItemView: public QWidget
{
    Q_OBJECT
public:
    SubAccountItemView(QWidget* parent = NULL);
private:
    COriginalButton* mSwitchButton;
	COriginalButton* mHelpButton;
	COriginalButton* mChangeNameButton;
	bool             mBlocked;
    //QString mName;
    //QString mDate;
    //QString mDesc;
    quint64 mId;

    void resizeEvent(QResizeEvent *e);
    void layoutControls();

signals:
    void switchAccount(quint64 id);
	void helpButtonClicked();
	void renameAccount(quint64 id);

private slots:
    void onSwitchButtonClicked();
	void onChangeNameButtonClicked();

public:
    //void setName(const QString& s){mName = s;update();}
    //void setDate(const QString& s){mDate = s;update();}
    //void setDesc(const QString& s){mDesc = s;update();}
    void setId(quint64 id){mId = id;}
	void showSwitchButton(bool s);
	void setBlocked(bool s);
};

class DialogManagerAPI SubAccountItemDelegate : public QStyledItemDelegate
{
public:
    SubAccountItemDelegate(QObject* parent = NULL);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};


class DialogManagerAPI SubAccountPanel : public QWidget
{
    Q_OBJECT
public:
    SubAccountPanel(QWidget* parent = NULL);
    ~SubAccountPanel();

private:
    CTreeViewEx*        mTreeView;
    QStandardItemModel* mModel;
    COriginalButton*    mCreateSubAccountButton;
    COriginalButton*    mHelpButton;
    CExternalTextEdit*  mHelpTip;
    quint64             mCanCreateCount;

    void moveEvent(QMoveEvent *e);
    void layoutContorls();
    void resizeEvent(QResizeEvent *e);
    void paintEvent(QPaintEvent *e);
    bool eventFilter(QObject *obj, QEvent *e);

signals:
    void createSubAccount();
    void siwtchSubAccount(quint64 id);
	void renameSubAccount(quint64 id);
	void helpButtonClicked();

public:
    QList<QStandardItem*> subAccountItemLiToStandardItemLi(const SubAccountItemList& li);
    SubAccountItemList    standardItemLiToSubAccountItemLi(const QList<QStandardItem*>& li);

	CExternalTextEdit* helpTip();
	
	quint64 canCreateCount();
    void setCanCreateCount(quint64 v); //* 设置可创建的子账号个数
    void setSubAccountList(const SubAccountItemList& li); //* 设置子账号列表
	SubAccountItemList subAccountList(); //* 子账号列表
	 
	void appendSubAccount(SubAccountItem* item); //* 添加子账号
	
	SubAccountItem* getSubAccountItemById(quint64 id); //* 根据id获取子账号
	void removeSubAccountItem(quint64 id); //* 根据id删除子账号
};

#endif // SUBACCOUNTPANEL_H
