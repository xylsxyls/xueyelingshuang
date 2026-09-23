#ifndef SPLITVIEWER_UI_TESTS_H
#define SPLITVIEWER_UI_TESTS_H
#include "../../SplitViewer/SplitViewer/SplitViewer.h"
#include <QtCore/QStringList>

/** 只替换系统文件对话框的返回值，产品事件、模型和绘制均使用真实代码。 */
class SplitViewerTestWindow : public SplitViewer
{
public:
    QString nextFile;
    QStringList errors;
    int browseCount;
    SplitViewerTestWindow();
protected:
    QString browseFile(bool save, const QString& title, const QString& initial, const QString& filter);
    void reportError(const QString& message);
};
int SplitViewerRunUiTests(const QString& reportDirectory, int selectedCase = 0);
QStringList SplitViewerAuditCaseNames();
#endif