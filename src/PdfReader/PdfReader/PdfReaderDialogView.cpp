#include "PdfReaderDialogView.h"
#include "QtControls/Label.h"
#include "PdfReaderControlHelper.h"
#include "QtControls/LineEdit.h"
#include "QtControls/PushButton.h"
#include "QtControls/FileDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

PdfReaderDialogView::PdfReaderDialogView() :
m_input(nullptr), m_files(nullptr), m_cancel(nullptr), m_accept(nullptr)
{

}

bool PdfReaderDialogView::initView(const DialogParam& param)
{
    const PdfReaderDialogParam* data = dynamic_cast<const PdfReaderDialogParam*>(&param);
    if (!data)
    {
        return false;
    }
    m_param = *data;
    setObjectName(QStringLiteral("pdfReaderDialogView"));
    setProperty("dialogMode", static_cast<int>(m_param.m_mode));
    setStyleSheet(g_config.m_dialogStyle);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(g_config.m_dialogMargin, g_config.m_dialogMargin,
        g_config.m_dialogMargin, g_config.m_dialogMargin);
    layout->setSpacing(g_config.m_dialogSpacing);
    if (m_param.m_mode >= PdfReaderDialogOpenFile)
    {
        if (g_config.m_useNativeFileDialog)
        {
            return false;
        }
        m_files = new FileDialog(this);
        m_files->setWindowFlags(Qt::Widget);
        m_files->setNameFilter(m_param.m_filter);
        m_files->setAcceptMode(m_param.m_mode == PdfReaderDialogSaveFile ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
        m_files->setFileMode(m_param.m_mode == PdfReaderDialogDirectory ? QFileDialog::Directory :
            (m_param.m_mode == PdfReaderDialogSaveFile ? QFileDialog::AnyFile : QFileDialog::ExistingFile));
        m_files->setOption(QFileDialog::ShowDirsOnly, m_param.m_mode == PdfReaderDialogDirectory);
        if (!m_param.m_initial.isEmpty())
        {
            m_files->selectFile(m_param.m_initial);
        }
        layout->addWidget(m_files);
        connect(m_files, &QFileDialog::accepted, this, &PdfReaderDialogView::acceptValue);
        connect(m_files, &QFileDialog::rejected, this, &PdfReaderDialogView::cancel);
        return true;
    }
    Label* message = new Label(this);
    message->setObjectName(QStringLiteral("dialogMessage"));
    message->setTextFormat(Qt::PlainText);
    message->setWordWrap(true);
    message->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    message->setText(m_param.m_text);
    layout->addWidget(message, 1);
    if (m_param.m_mode == PdfReaderDialogInput)
    {
        m_input = new LineEdit(this);
        m_input->setObjectName(QStringLiteral("dialogInput"));
        m_input->setEchoMode(m_param.m_password ? QLineEdit::Password : QLineEdit::Normal);
        m_input->setText(m_param.m_initial);
        m_input->selectAll();
        layout->addWidget(m_input);
        connect(m_input, &QLineEdit::returnPressed, this, &PdfReaderDialogView::acceptValue);
    }
    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addStretch();
    m_accept = new PushButton(this);
    m_accept->setObjectName(QStringLiteral("dialogAccept"));
    m_accept->setText(g_config.m_dialogAcceptText);
    PdfReaderControlHelper::configureButton(m_accept);
    buttons->addWidget(m_accept);
    connect(m_accept, &QPushButton::clicked, this, &PdfReaderDialogView::acceptValue);
    if (m_param.m_mode != PdfReaderDialogMessage)
    {
        m_cancel = new PushButton(this);
        m_cancel->setObjectName(QStringLiteral("dialogCancel"));
        m_cancel->setText(g_config.m_dialogCancelText);
        PdfReaderControlHelper::configureButton(m_cancel);
        buttons->addWidget(m_cancel);
        connect(m_cancel, &QPushButton::clicked, this, &PdfReaderDialogView::cancel);
    }
    layout->addLayout(buttons);
    return true;
}

QSize PdfReaderDialogView::preferredSize() const
{
    if (m_files)
    {
        return g_config.m_fileDialogSize;
    }
    return m_param.m_about ?
        g_config.m_aboutDialogSize : g_config.m_dialogSize;
}

QWidget* PdfReaderDialogView::defaultFocusWidget() const
{
    if (m_input)
    {
        return m_input;
    }
    if (m_files)
    {
        return m_files;
    }
    return m_cancel ? m_cancel : m_accept;
}

void PdfReaderDialogView::acceptValue()
{
    if (m_input)
    {
        *m_param.m_value = m_input->text();
    }
    if (m_files)
    {
        const QStringList selected = m_files->selectedFiles();
        if (selected.isEmpty())
        {
            return;
        }
        *m_param.m_value = selected.first();
    }
    notifyCloseRequested(ACCEPT_BUTTON, 0);
}

void PdfReaderDialogView::cancel()
{
    notifyCloseRequested(IGNORE_BUTTON, 0);
}