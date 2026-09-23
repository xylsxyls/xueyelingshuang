#include "PdfReaderDialogHelper.h"
#include "PdfReaderFileHelper.h"
#include <QFileInfo>

bool PdfReaderDialogHelper::run(QWidget* parent, PdfReaderDialogParam& param)
{
    if (parent)
    {
        parent->winId();
        param.m_parent = parent->windowHandle();
    }
    DialogManager::instance().makeDialog(param);
    return param.m_dialogId != 0 && param.m_result == ACCEPT_BUTTON;
}

void PdfReaderDialogHelper::message(QWidget* parent, const QString& title, const QString& text, const Config& config)
{
    PdfReaderDialogParam param;
    param.m_title = title; param.text = text; param.config = config;
    run(parent, param);
}

bool PdfReaderDialogHelper::question(QWidget* parent, const QString& title, const QString& text, const Config& config)
{
    PdfReaderDialogParam param;
    param.mode = PdfReaderDialogParam::Question;
    param.m_title = title; param.text = text; param.config = config;
    return run(parent, param);
}

bool PdfReaderDialogHelper::input(QWidget* parent, const QString& title, const QString& text, QString& value, bool password, const Config& config)
{
    PdfReaderDialogParam param;
    param.mode = PdfReaderDialogParam::Input;
    param.m_title = title; param.text = text; param.config = config;
    param.initial = value; param.password = password;
    if (!run(parent, param)) return false;
    value = *param.value;
    return true;
}

QString PdfReaderDialogHelper::file(QWidget* parent, PdfReaderDialogParam::Mode mode, const QString& title,
    const QString& initial, const QString& filter, const Config& config)
{
    PdfReaderDialogParam param;
    param.mode = mode; param.m_title = title; param.initial = initial;
    param.filter = filter; param.config = config;
    if (!run(parent, param)) return QString();
    QString path = *param.value;
    if (mode == PdfReaderDialogParam::SaveFile)
    {
        path = PdfReaderFileHelper::pdfOutputPath(path);
        if (QFileInfo::exists(path) && !question(parent, config.confirmOverwriteTitle,
            config.confirmOverwritePrompt + path, config)) return QString();
    }
    return path;
}
