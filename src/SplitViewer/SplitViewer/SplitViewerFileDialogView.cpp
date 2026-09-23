#include "SplitViewerFileDialogView.h"
#include "Config.h"
#include "QtControls/FileDialog.h"
#include "QtControls/Label.h"
#include <QtCore/QFileInfo>
#include <QtWidgets/QVBoxLayout>

SplitViewerFileDialogView::SplitViewerFileDialogView() :
m_files(nullptr)
{

}

bool SplitViewerFileDialogView::initView(const DialogParam& param)
{
    const SplitViewerFileDialogParam* data = dynamic_cast<const SplitViewerFileDialogParam*>(&param);
    if (data == nullptr || m_files != nullptr)
    {
        return false;
    }
    m_param = *data;
    setObjectName(QStringLiteral("splitViewerFileView"));
    setStyleSheet(Config::fileDialogStyle());
    QVBoxLayout* layout = new QVBoxLayout(this);
    Label* title = new Label(this);
    title->setObjectName(QStringLiteral("fileDialogTitle"));
    title->setText(m_param.m_title);
    layout->addWidget(title);
    m_files = new FileDialog(this);
    m_files->setObjectName(QStringLiteral("splitViewerFilePicker"));
    m_files->setWindowFlags(Qt::Widget);
    m_files->setAcceptMode(m_param.save ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
    m_files->setFileMode(m_param.save ? QFileDialog::AnyFile : QFileDialog::ExistingFile);
    m_files->setNameFilter(m_param.filter);
    if (!m_param.initial.isEmpty())
    {
        if (QFileInfo(m_param.initial).isDir())
        {
            m_files->setDirectory(m_param.initial);
        }
        else
        {
            m_files->selectFile(m_param.initial);
        }
    }
    layout->addWidget(m_files);
    connect(m_files, &QFileDialog::accepted, this, &SplitViewerFileDialogView::acceptSelection);
    connect(m_files, &QFileDialog::rejected, this, &SplitViewerFileDialogView::cancelSelection);
    return true;
}

QSize SplitViewerFileDialogView::preferredSize() const
{
    return Config::fileDialogSize();
}

QWidget* SplitViewerFileDialogView::defaultFocusWidget() const
{
    return m_files;
}

void SplitViewerFileDialogView::acceptSelection()
{
    const QStringList selected = m_files->selectedFiles();
    if (selected.isEmpty())
    {
        return;
    }
    if (m_param.save && QFileInfo::exists(selected.first()))
    {
        AskDialogParam confirm;
        confirm.m_title = QStringLiteral("确认覆盖");
        confirm.m_tip = QStringLiteral("文件已经存在，是否覆盖？\n") + selected.first();
        confirm.m_parent = window()->windowHandle();
        DialogManager::instance().makeDialog(confirm);
        if (confirm.m_result != ACCEPT_BUTTON)
        {
            // QFileDialog已发出accepted，取消覆盖时仍需重新显示内容以便改名或取消。
            m_files->show();
            return;
        }
    }
    *m_param.selectedPath = selected.first();
    notifyCloseRequested(ACCEPT_BUTTON, 0);
}

void SplitViewerFileDialogView::cancelSelection()
{
    notifyCloseRequested(IGNORE_BUTTON, 0);
}