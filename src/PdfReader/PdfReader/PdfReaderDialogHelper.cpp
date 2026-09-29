#include "PdfReaderDialogHelper.h"
#include "PdfReaderFileHelper.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QPointer>

bool PdfReaderDialogHelper::run(QWidget* parent, PdfReaderDialogParam& param)
{
    const QPointer<QWidget> owner(parent);
    param.m_hasShadow = g_config.m_dialogShadowEnabled;
    param.m_shadowSize = g_config.m_dialogShadowSize;
    param.m_titleBarHeight = param.m_titleClose ? g_config.m_titleCloseHeight : g_config.m_dialogTitleBarHeight;
    if (parent)
    {
        parent->winId();
        param.m_parent = parent->windowHandle();
    }
    DialogManager::instance().makeDialog(param);
    return (parent == nullptr || !owner.isNull()) && param.m_dialogId != 0 && param.m_result == ACCEPT_BUTTON;
}

void PdfReaderDialogHelper::message(QWidget* parent, const QString& title, const QString& text, bool about)
{
    PdfReaderDialogParam param;
    param.m_title = title; param.m_text = text;
    param.m_about = about;
    param.m_titleClose = about;
    run(parent, param);
}

bool PdfReaderDialogHelper::question(QWidget* parent, const QString& title, const QString& text)
{
    PdfReaderDialogParam param;
    param.m_mode = PdfReaderDialogQuestion;
    param.m_title = title; param.m_text = text;
    return run(parent, param);
}

bool PdfReaderDialogHelper::input(QWidget* parent, const QString& title, const QString& text, QString& value, bool password, bool titleClose)
{
    PdfReaderDialogParam param;
    param.m_mode = PdfReaderDialogInput;
    param.m_title = title; param.m_text = text;
    param.m_initial = value; param.m_password = password;
    param.m_titleClose = titleClose;
    if (!run(parent, param))
    {
        return false;
    }
    value = *param.m_value;
    return true;
}

QString PdfReaderDialogHelper::file(QWidget* parent, PdfReaderDialogMode mode, const QString& title,
    const QString& initial, const QString& filter)
{
    const QPointer<QWidget> owner(parent);
    QString path;
    if (!g_config.m_useNativeFileDialog)
    {
        PdfReaderDialogParam param;
        param.m_mode = mode; param.m_title = title; param.m_initial = initial;
        param.m_filter = filter;
        if (!run(parent, param))
        {
            return QString();
        }
        path = *param.m_value;
    }
    else
    {
        switch (mode)
        {
        case PdfReaderDialogOpenFile:
            path = QFileDialog::getOpenFileName(parent, title, initial, filter);
            break;
        case PdfReaderDialogSaveFile:
            // 后缀规范化后由业务确认最终文件，只询问一次覆盖。
            path = QFileDialog::getSaveFileName(parent, title, initial, filter, nullptr, QFileDialog::DontConfirmOverwrite);
            break;
        case PdfReaderDialogDirectory:
            path = QFileDialog::getExistingDirectory(parent, title, initial, QFileDialog::ShowDirsOnly);
            break;
        default:
            return QString();
        }
    }
    if (path.isEmpty() || (parent != nullptr && owner.isNull()))
    {
        return QString();
    }
    if (mode == PdfReaderDialogSaveFile)
    {
        path = PdfReaderFileHelper::pdfOutputPath(path);
        if (QFileInfo::exists(path) && !question(parent, g_config.m_confirmOverwriteTitle,
            g_config.m_confirmOverwritePrompt + path)) return QString();
    }
    return path;
}