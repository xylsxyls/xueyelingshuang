#ifndef GAMERESULTLISTPANELMAXDELEGATE_H
#define GAMERESULTLISTPANELMAXDELEGATE_H

#include <QStyledItemDelegate>

class GameResultListPanelMaxDelegate:public QStyledItemDelegate
{
public:
    GameResultListPanelMaxDelegate(QObject* parent = NULL);
    ~GameResultListPanelMaxDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintBkg(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintHoverFrame(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintRPGCamp(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintRPGSlot(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintLadderCamp(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintLadderSlot(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
};

#endif // GAMERESULTLISTPANELMAXDELEGATE_H
