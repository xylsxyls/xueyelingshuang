#ifndef GAMERESULTLISTPANELEXDELEGATE_H
#define GAMERESULTLISTPANELEXDELEGATE_H

#include <QStyledItemDelegate>

class GameResultListPanelExDelegate: public QStyledItemDelegate
{
public:
    GameResultListPanelExDelegate(QObject* parent = NULL);
    ~GameResultListPanelExDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintBkg(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintHoverFrame(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintRPGSlot(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // GAMERESULTLISTPANELEXDELEGATE_H
