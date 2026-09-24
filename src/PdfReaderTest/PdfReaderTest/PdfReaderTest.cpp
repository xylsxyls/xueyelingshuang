#include "PdfReaderTest.h"
#include "PdfReaderRegression.h"
#include "../../PdfReader/PdfReader/PdfReaderControlHelper.h"
#include "../../PdfReader/PdfReader/PdfReaderDialogHelper.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtGui/QImage>
#include <QtWidgets/QFormLayout>
#include "QtControls/GroupBox.h"
#include <QtWidgets/QHBoxLayout>
#include "QtControls/Label.h"
#include "QtControls/LineEdit.h"
#include "QtControls/PushButton.h"
#include "QtControls/TextEdit.h"
#include <QtWidgets/QVBoxLayout>
#include "QtControls/Widget.h"

PdfReaderTest::PdfReaderTest(QWidget* parent)
    : MainWindow(parent)
    , m_handle(pdfReaderCoreCreate())
    , m_pathEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_pageCountLabel(nullptr)
    , m_logEdit(nullptr)
{
    setWindowTitle(QStringLiteral("PdfReaderTest - Core C API"));
    resize(900, 600);
    QWidget* central = new Widget(this);
    QVBoxLayout* root = new QVBoxLayout(central);
    GroupBox* documentBox = new GroupBox(central);
    documentBox->setTitle(QStringLiteral("文档测试"));
    QFormLayout* form = new QFormLayout(documentBox);
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_pathEdit = new LineEdit(documentBox);
    PushButton* browse = new PushButton(documentBox);
    browse->setText(QStringLiteral("选择"));
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(browse);
    Label* pathLabel = new Label(documentBox);
    pathLabel->setText(QStringLiteral("PDF路径"));
    form->addRow(pathLabel, pathLayout);
    m_passwordEdit = new LineEdit(documentBox);
    m_passwordEdit->setEchoMode(LineEdit::Password);
    Label* passwordLabel = new Label(documentBox);
    passwordLabel->setText(QStringLiteral("密码"));
    form->addRow(passwordLabel, m_passwordEdit);
    QHBoxLayout* buttons = new QHBoxLayout();
    PushButton* initButton = new PushButton(documentBox);
    initButton->setText(QStringLiteral("初始化Core"));
    PushButton* openButton = new PushButton(documentBox);
    openButton->setText(QStringLiteral("打开"));
    PushButton* renderButton = new PushButton(documentBox);
    renderButton->setText(QStringLiteral("渲染第一页"));
    PushButton* smokeButton = new PushButton(documentBox);
    smokeButton->setText(QStringLiteral("C API冒烟测试"));
    buttons->addWidget(initButton);
    buttons->addWidget(openButton);
    buttons->addWidget(renderButton);
    buttons->addWidget(smokeButton);
    buttons->addStretch(1);
    form->addRow(buttons);
    m_pageCountLabel = new Label(documentBox);
    m_pageCountLabel->setText(QStringLiteral("页面数：0"));
    form->addRow(m_pageCountLabel);
    root->addWidget(documentBox);
    m_logEdit = new TextEdit(central);
    m_logEdit->setReadOnly(true);
    root->addWidget(m_logEdit, 1);
    QHBoxLayout* regression = new QHBoxLayout();
    const QStringList labels = QStringList() << QStringLiteral("指定测试") << QStringLiteral("瞬时测试") << QStringLiteral("压力测试") << QStringLiteral("全量测试");
    for (int i = 0; i < labels.size(); ++i)
    {
        PushButton* button = new PushButton(central);
    button->setText(labels[i]);
        regression->addWidget(button);
        // GUI 和 CLI 使用相同 ID、调度、断言与报告。运行期间禁止另一批启动。
        connect(button, &PushButton::clicked, this, [this, central, i]() {
            if (i == 2)
            {
                PdfReaderDialogHelper::message(this, QStringLiteral("测试范围"), QStringLiteral("当前登记为 18 项瞬时回归，未登记压力用例，不启动空批次。"));
                return;
            }
            bool accepted = true;
            QString selection = i == 0 ? QStringLiteral("1-16,19,20") : QString();
            if (i == 0) accepted = PdfReaderDialogHelper::input(this, QStringLiteral("指定测试"), QStringLiteral("ID 或范围，例如 1,3-6；有效 ID 1-16,19,20"), selection);
            if (!accepted) { return; }
            central->setEnabled(false);
            const int result = PdfReaderRegression::run(selection);
            central->setEnabled(true);
            log(result == 0 ? QStringLiteral("所选回归通过；逐项结果已写入程序旁 reports。") : QStringLiteral("回归失败或选择无效；请查看 reports 中的断言。"));
        });
    }
    root->addLayout(regression);
    const Config controlConfig;
    foreach (PushButton* button, central->findChildren<PushButton*>())
        PdfReaderControlHelper::configureButton(button, controlConfig);
    setCentralWidget(central);

    connect(browse, SIGNAL(clicked()), this, SLOT(chooseFile()));
    connect(initButton, SIGNAL(clicked()), this, SLOT(initCore()));
    connect(openButton, SIGNAL(clicked()), this, SLOT(openDocument()));
    connect(renderButton, SIGNAL(clicked()), this, SLOT(renderFirstPage()));
    connect(smokeButton, SIGNAL(clicked()), this, SLOT(runApiSmoke()));
    log(QStringLiteral("测试工程启动，API句柄：%1。\n可以先初始化，再打开PDF。")
        .arg(m_handle ? QStringLiteral("已创建") : QStringLiteral("创建失败")));
}

PdfReaderTest::~PdfReaderTest()
{
    if (m_handle)
    {
        pdfReaderCoreUninit(m_handle);
        pdfReaderCoreDestroy(m_handle);
        m_handle = nullptr;
    }
}

void PdfReaderTest::log(const QString& text)
{
    if (m_logEdit) m_logEdit->append(text);
}

QString PdfReaderTest::lastError() const
{
    if (!m_handle) return QStringLiteral("句柄为空");
    const size_t size = pdfReaderCoreGetLastError(m_handle, nullptr, 0);
    if (!size) return QStringLiteral("未返回错误文本");
    QByteArray buffer(static_cast<int>(size), 0);
    pdfReaderCoreGetLastError(m_handle, buffer.data(), static_cast<size_t>(buffer.size()));
    return QString::fromUtf8(buffer.constData());
}

void PdfReaderTest::chooseFile()
{
    const QString path = PdfReaderDialogHelper::file(this, PdfReaderDialogParam::OpenFile, QStringLiteral("选择PDF"), QString(), QStringLiteral("PDF文件 (*.pdf);;所有文件 (*.*)"));
    if (!path.isEmpty()) m_pathEdit->setText(path);
}

void PdfReaderTest::initCore()
{
    if (!m_handle)
    {
        log(QStringLiteral("失败：句柄为空"));
        return;
    }
    const int32_t result = pdfReaderCoreInit(m_handle);
    log(QStringLiteral("pdfReaderCoreInit -> %1 (%2)").arg(result).arg(QString::fromLatin1(pdfReaderCoreResultDescription(result))));
}

void PdfReaderTest::openDocument()
{
    if (!m_handle) return;
    const QByteArray path = m_pathEdit->text().toUtf8();
    const QByteArray password = m_passwordEdit->text().toUtf8();
    const int32_t result = pdfReaderCoreOpen(m_handle, path.constData(), password.constData());
    log(QStringLiteral("pdfReaderCoreOpen -> %1 (%2)").arg(result).arg(QString::fromLatin1(pdfReaderCoreResultDescription(result))));
    if (result != PdfReaderCoreCResultSuccess)
    {
        log(QStringLiteral("错误：%1").arg(lastError()));
        return;
    }
    m_pageCountLabel->setText(QStringLiteral("页面数：%1").arg(pdfReaderCorePageCount(m_handle)));
}

void PdfReaderTest::renderFirstPage()
{
    if (!m_handle || pdfReaderCorePageCount(m_handle) <= 0)
    {
        log(QStringLiteral("没有可渲染的页面"));
        return;
    }
    PdfReaderCoreCPageInfo info;
    int32_t result = pdfReaderCoreGetPageInfo(m_handle, 0, &info);
    if (result != PdfReaderCoreCResultSuccess)
    {
        log(QStringLiteral("获取页面信息失败：%1").arg(lastError()));
        return;
    }
    const int width = 800;
    const int height = qMax(1, qRound(width * info.height / qMax(1.0, info.width)));
    QImage image(width, height, QImage::Format_ARGB32);
    int32_t outWidth = 0, outHeight = 0, outStride = 0;
    size_t outBytes = 0;
    result = pdfReaderCoreRenderPage(m_handle, 0, width, height,
                                     image.bits(), static_cast<size_t>(image.byteCount()),
                                     &outWidth, &outHeight, &outStride, &outBytes);
    log(QStringLiteral("pdfReaderCoreRenderPage -> %1 (%2), %3x%4 stride=%5 bytes=%6")
        .arg(result).arg(QString::fromLatin1(pdfReaderCoreResultDescription(result)))
        .arg(outWidth).arg(outHeight).arg(outStride).arg(static_cast<qulonglong>(outBytes)));
    if (result == PdfReaderCoreCResultSuccess)
    {
        const QString output = QDir::temp().absoluteFilePath(QStringLiteral("pdfreader-test-page.png"));
        image.save(output);
        log(QStringLiteral("渲染图已保存：%1").arg(output));
    }
}

void PdfReaderTest::runApiSmoke()
{
    if (!m_handle)
    {
        log(QStringLiteral("C API冒烟失败：句柄为空"));
        return;
    }
    const int32_t invalid = pdfReaderCoreOpen(m_handle, nullptr, nullptr);
    const int32_t countBefore = pdfReaderCorePageCount(m_handle);
    log(QStringLiteral("空路径参数检查 -> %1 (%2)，页面数保持 %3")
        .arg(invalid).arg(QString::fromLatin1(pdfReaderCoreResultDescription(invalid))).arg(countBefore));
    log(QStringLiteral("错误文本：%1").arg(lastError()));
}