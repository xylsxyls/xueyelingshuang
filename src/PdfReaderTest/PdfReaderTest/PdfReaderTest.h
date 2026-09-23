#ifndef PDFREADERTEST_H
#define PDFREADERTEST_H

#include "QtControls/MainWindow.h"

#define PDFREADERCORE_USE_C_API
#include "PdfReaderCore/PdfReaderCoreAPI.h"
#undef PDFREADERCORE_USE_C_API

class LineEdit;
class TextEdit;
class Label;

class PdfReaderTest : public MainWindow
{
    Q_OBJECT
public:
    explicit PdfReaderTest(QWidget* parent = nullptr);
    ~PdfReaderTest();

private slots:
    void chooseFile();
    void initCore();
    void openDocument();
    void renderFirstPage();
    void runApiSmoke();

private:
    void log(const QString& text);
    QString lastError() const;

private:
    PdfReaderCoreHandle m_handle;
    LineEdit* m_pathEdit;
    LineEdit* m_passwordEdit;
    Label* m_pageCountLabel;
    TextEdit* m_logEdit;
};

#endif
