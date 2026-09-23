#include "SplitViewerDialogHelper.h"
#include "SplitViewerFileDialogParam.h"
#include <QtWidgets/QWidget>

QString SplitViewerDialogHelper::browseFile(QWidget* parent, bool save, const QString& title,
    const QString& initial, const QString& filter)
{
    SplitViewerFileDialogParam param;
    param.m_parent = parent != nullptr ? parent->window()->windowHandle() : nullptr;
    param.m_title = title;
    param.save = save;
    param.initial = initial;
    param.filter = filter;
    DialogManager::instance().makeDialog(param);
    return param.m_result == ACCEPT_BUTTON ? *param.selectedPath : QString();
}

void SplitViewerDialogHelper::showMessage(QWidget* parent, const QString& title, const QString& message)
{
    TipDialogParam param;
    param.m_parent = parent != nullptr ? parent->window()->windowHandle() : nullptr;
    param.m_title = title;
    param.m_tip = message;
    DialogManager::instance().makeDialog(param);
}

QString SplitViewerDialogHelper::inputText(QWidget* parent, const QString& title, const QString& prompt, bool& accepted)
{
    InputDialogParam param;
    param.m_parent = parent != nullptr ? parent->window()->windowHandle() : nullptr;
    param.m_title = title;
    param.m_editTip = prompt;
    DialogManager::instance().makeDialog(param);
    accepted = param.m_result == ACCEPT_BUTTON;
    return accepted ? param.m_editText : QString();
}