#ifndef CEXPRESSIONPICKER_H
#define CEXPRESSIONPICKER_H

#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QMovie>
#include "ControlsMacro.h"

class ControlsAPI CExpressionPicker : public QTableView
{
    Q_OBJECT
public:
    enum ExpressionRole
    {
        ExpressionRole_GroupId = Qt::UserRole + 1,
        ExpressionRole_Id      = Qt::UserRole + 2,
        ExpressionRole_Desc    = Qt::UserRole + 3,
        ExpressionRole_Shortcut= Qt::UserRole + 4,
        ExpressionRole_Tooltip = Qt::UserRole + 5,
        ExpressionRole_FileName= Qt::UserRole + 6,
    };

    struct Expression
    {
        QString groupid;
        QString id;
        QString desc;
        QString fileName;
        QString shortcut;
        QString tooltip;
    };
    typedef QList<Expression> ExpressionList;

    CExpressionPicker(QWidget* parent = NULL);
    ~CExpressionPicker();


private:
    QStandardItemModel* mModel;
    QLabel            * mPreView;
    QMovie            * mMovie;
    ExpressionList      mExpressionList;
    bool                mShowPreView;
    bool                mShowIconRect;
    int                 mMaxColumnCount;
    int                 mMaxRowCount;


    void leaveEvent(QEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    bool eventFilter(QObject *obj, QEvent *e);

private slots:
    void onClicked(const QModelIndex& index);

signals:
    void expressionClicked(const Expression& expression);

public:
    void setShowPreView(bool s);
    bool isShowPreView();
    void setShowIconRect(bool s);
    bool isShowIconRect();

    void setExpressionList(const ExpressionList& li);
    ExpressionList expressionList();

    void setMaxColumnCount(int n);
    void setMaxRowCount   (int n);
    int  maxColumnCount   ();
    int  maxRowCount      ();


};

Q_DECLARE_METATYPE(CExpressionPicker::Expression)
Q_DECLARE_METATYPE(CExpressionPicker::ExpressionList)

#endif // CEXPRESSIONPICKER_H
