#ifndef LOCKEDACCOUNTPANEL_H
#define LOCKEDACCOUNTPANEL_H

#include <QWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

class CTreeViewEx;

class LockedAccountItem:public QStandardItem
{
public:
    enum LockedAccountItemRole
    {
        Role_StartDate    = Qt::UserRole + 1,
        Role_Reasion      = Qt::UserRole + 2,
        Role_DayCount     = Qt::UserRole + 3,
    };
    LockedAccountItem(){setSizeHint(QSize(0,30));}

    void setStartDate(const QString& s){setData(s, Role_StartDate);}
    QString startDate(){return data(Role_StartDate).toString();}
    void setReasion(const QString& s){setData(s, Role_Reasion);}
    QString reasion(){return data(Role_Reasion).toString();}
    void setDayCount(quint64 v){setData(v, Role_DayCount);}
    quint64 dayCount(){return data(Role_DayCount).toULongLong();}
};

typedef QList<LockedAccountItem*> LockedAccountItemList;

class LockedAccountItemDelegate: public QStyledItemDelegate
{
public:
    LockedAccountItemDelegate(QObject* parent = NULL);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class LockedAccountPanel: public QWidget
{
public:
    LockedAccountPanel(QWidget* parent = NULL);

private:
    CTreeViewEx* mTreeView;
    QStandardItemModel* mModel;
    QString mName;

    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);
    void layoutControls();

public:
    void setName(const QString& s);
    QString name();

    void setLockedAccountItemList(const LockedAccountItemList& li);
	void appendLockedAccountItem(LockedAccountItem* item);
	void clearLockedAccountItem();
};

#endif // LOCKEDACCOUNTPANEL_H
