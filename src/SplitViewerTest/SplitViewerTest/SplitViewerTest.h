#ifndef SPLITVIEWER_TEST_H
#define SPLITVIEWER_TEST_H

#include <QtWidgets/QMainWindow>

class QPlainTextEdit;
class QComboBox;

class SplitViewerTest : public QMainWindow
{
    Q_OBJECT
public:
    explicit SplitViewerTest(QWidget* parent = NULL);
    ~SplitViewerTest();
    bool allTestsPassed() const;

private slots:
    void runTests();
    void runUiTests();

private:
    void appendResult(const QString& name, bool passed, const QString& detail = QString());
    QPlainTextEdit* m_output;
    bool m_allTestsPassed;
    QComboBox* m_uiCases;
};

#endif