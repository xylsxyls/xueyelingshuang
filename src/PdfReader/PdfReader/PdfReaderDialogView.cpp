#include "PdfReaderDialogView.h"
#include "QtControls/Label.h"
#include "PdfReaderControlHelper.h"
#include "QtControls/LineEdit.h"
#include "QtControls/PushButton.h"
#include "QtControls/FileDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>

PdfReaderDialogView::PdfReaderDialogView() :
m_input(nullptr), m_files(nullptr), m_cancel(nullptr), m_accept(nullptr)
{
}

bool PdfReaderDialogView::initView(const DialogParam& param)
{
    const PdfReaderDialogParam* data = dynamic_cast<const PdfReaderDialogParam*>(&param);
    if (!data) return false;
    m_param = *data;
    setObjectName(QStringLiteral("pdfReaderDialogView"));
    setProperty("dialogMode", static_cast<int>(m_param.mode));
    setStyleSheet(m_param.config.dialogStyle);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(m_param.config.dialogMargin, m_param.config.dialogMargin,
        m_param.config.dialogMargin, m_param.config.dialogMargin);
    layout->setSpacing(m_param.config.dialogSpacing);
    Label* title = new Label(this);
    title->setObjectName(QStringLiteral("dialogTitle"));
    title->setText(m_param.m_title);
    layout->addWidget(title);
    if (m_param.mode >= PdfReaderDialogParam::OpenFile)
    {
        m_files = new FileDialog(this);
        m_files->setWindowFlags(Qt::Widget);
        m_files->setNameFilter(m_param.filter);
        m_files->setAcceptMode(m_param.mode == PdfReaderDialogParam::SaveFile ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
        m_files->setFileMode(m_param.mode == PdfReaderDialogParam::Directory ? QFileDialog::Directory :
            (m_param.mode == PdfReaderDialogParam::SaveFile ? QFileDialog::AnyFile : QFileDialog::ExistingFile));
        m_files->setOption(QFileDialog::ShowDirsOnly, m_param.mode == PdfReaderDialogParam::Directory);
        if (!m_param.initial.isEmpty()) m_files->selectFile(m_param.initial);
        layout->addWidget(m_files);
        connect(m_files, &QFileDialog::accepted, this, &PdfReaderDialogView::acceptValue);
        connect(m_files, &QFileDialog::rejected, this, &PdfReaderDialogView::cancel);
        return true;
    }
    Label* message = new Label(this);
    message->setObjectName(QStringLiteral("dialogMessage"));
    message->setTextFormat(Qt::PlainText);
    message->setWordWrap(true);
    message->setText(m_param.text);
    layout->addWidget(message, 1);
    if (m_param.mode == PdfReaderDialogParam::Input)
    {
        m_input = new LineEdit(this);
        m_input->setObjectName(QStringLiteral("dialogInput"));
        m_input->setEchoMode(m_param.password ? QLineEdit::Password : QLineEdit::Normal);
        m_input->setText(m_param.initial);
        m_input->selectAll();
        layout->addWidget(m_input);
        connect(m_input, &QLineEdit::returnPressed, this, &PdfReaderDialogView::acceptValue);
    }
    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addStretch();
    m_accept = new PushButton(this);
    m_accept->setObjectName(QStringLiteral("dialogAccept"));
    m_accept->setText(m_param.config.dialogAcceptText);
    PdfReaderControlHelper::configureButton(m_accept, m_param.config);
    buttons->addWidget(m_accept);
    connect(m_accept, &QPushButton::clicked, this, &PdfReaderDialogView::acceptValue);
    if (m_param.mode != PdfReaderDialogParam::Message)
    {
        m_cancel = new PushButton(this);
        m_cancel->setObjectName(QStringLiteral("dialogCancel"));
        m_cancel->setText(m_param.config.dialogCancelText);
        PdfReaderControlHelper::configureButton(m_cancel, m_param.config);
        buttons->addWidget(m_cancel);
        connect(m_cancel, &QPushButton::clicked, this, &PdfReaderDialogView::cancel);
    }
    layout->addLayout(buttons);
    return true;
}

QSize PdfReaderDialogView::preferredSize() const
{
    return m_files ? m_param.config.fileDialogSize : m_param.config.dialogSize;
}

QWidget* PdfReaderDialogView::defaultFocusWidget() const
{
    if (m_input) return m_input;
    if (m_files) return m_files;
    return m_cancel ? m_cancel : m_accept;
}

void PdfReaderDialogView::acceptValue()
{
    if (m_input) *m_param.value = m_input->text();
    if (m_files)
    {
        const QStringList selected = m_files->selectedFiles();
        if (selected.isEmpty()) return;
        *m_param.value = selected.first();
    }
    notifyCloseRequested(ACCEPT_BUTTON, 0);
}

void PdfReaderDialogView::cancel()
{
    notifyCloseRequested(IGNORE_BUTTON, 0);
}
