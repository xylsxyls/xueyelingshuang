#ifndef CEXPRESSIONDIALOG_H
#define CEXPRESSIONDIALOG_H

#include <QDialog>
#include "CExpressionPicker.h"
#include "DialogManagerMacro.h"

class DialogManagerAPI CExpressionDialog : public QDialog
{
    Q_OBJECT
public:
    CExpressionDialog(QWidget* parent = NULL);
    ~CExpressionDialog();

private:
    CExpressionPicker* mExpressionPicker;
    CExpressionPicker* mExpressionGroupPicker;
    CExpressionPicker::ExpressionList mGroups;
    CExpressionPicker::ExpressionList mExpressions;

    void layoutControl();
    void resizeEvent(QResizeEvent *e);
    void paintEvent(QPaintEvent *e);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result);

signals:
    void expressionChoosed(const CExpressionPicker::Expression& exp);
    void ncActiveChanged(const bool& ncActive);

private slots:
    void selectGroup(const CExpressionPicker::Expression& group);

public :
    bool loadExpressions(const QString &emotionPath); //传入Emotions文件夹所在目录
};

#endif // CEXPRESSIONDIALOG_H
