#ifndef CTREEVIEWEX_H
#define CTREEVIEWEX_H

#include <QTreeView>
#include <QScrollBar>

class CTreeViewEx: public QTreeView
{
    Q_OBJECT
public:
    CTreeViewEx(QWidget* parent = NULL);
    ~CTreeViewEx();

private:
    int mHoveredRow;
    int mHoveredColumn;

protected:
    virtual void mouseMoveEvent(QMouseEvent* e);
	virtual void leaveEvent(QEvent *e);
    virtual bool eventFilter(QObject *obj, QEvent *e);

public:
    virtual void setHeader(QHeaderView *header);

    int hoveredRow() const;
    int hoveredColumn() const;
};

#endif // CTREEVIEWEX_H
