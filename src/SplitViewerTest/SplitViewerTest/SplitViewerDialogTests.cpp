#include "SplitViewerDialogTests.h"
#include "../../SplitViewer/SplitViewer/SplitViewer.h"
#include "../../SplitViewer/SplitViewer/SplitViewerDialogHelper.h"
#include "DialogManager/DialogManagerAPI.h"
#include "QtControls/FileDialog.h"
#include "QtControls/Label.h"
#include "QtControls/MainWindow.h"
#include "QtControls/StatusBar.h"
#include "QtControls/ToolBar.h"
#include "QtControls/Widget.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QTextStream>
#include <QtGui/QWindow>
#include <QtWidgets/QApplication>
#include <QtWidgets/QAction>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtTest/QtTest>

bool SplitViewerDialogTests::runCase(int id, const QString& directory)
{
    SplitViewer window;
    window.resize(900, 700);
    window.show();
    window.activateWindow();
    QTest::qWait(40);
    QWidget* canvas = window.centralWidget();
    const QString fixture = QDir(directory).absoluteFilePath(QStringLiteral("真实选择 红色.png"));
    QImage input(120, 120, QImage::Format_RGB32);
    const QColor red(230, 30, 40);
    input.fill(red);
    if (!input.save(fixture))
    {
        return false;
    }
    QFile original(fixture);
    if (!original.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QByteArray bytesBefore = original.readAll();
    original.close();
    bool timedOut = false;
    bool observed = false;
    bool valid = true;
    int phase = 0;
    bool fileStateLogged = false;
    QFile evidence(QDir(directory).filePath(QStringLiteral("dialog-evidence.txt")));
    if (!evidence.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    QTextStream details(&evidence);
    details.setCodec("UTF-8");
    QTimer watchdog;
    watchdog.setSingleShot(true);
    // 看门狗只把挂起转成失败，并清理本项唯一会话中的弹窗。
    QObject::connect(&watchdog, &QTimer::timeout, [&]()
    {
        timedOut = true;
        QWidget* modal = QApplication::activeModalWidget();
        if (modal != nullptr)
        {
            modal->grab().save(QDir(directory).filePath(QStringLiteral("timeout-dialog.png")));
        }
        DestroyAllOperateParam close;
        DialogManager::instance().operateDialog(close);
    });
    watchdog.start(10000);
    QTimer confirmationDriver;
    // 独立定时器驱动嵌套确认；文件按钮的clicked尚在其模态循环内，原定时器不能重入。
    QObject::connect(&confirmationDriver, &QTimer::timeout, [&]()
    {
        if (id != 175 || (phase != 1 && phase != 3))
        {
            return;
        }
        QWidget* modal = QApplication::activeModalWidget();
        if (modal == nullptr || modal->windowTitle() != QStringLiteral("确认覆盖"))
        {
            return;
        }
        const QString label = phase == 1 ? QStringLiteral("取消") : QStringLiteral("确认");
        foreach (QPushButton* button, modal->findChildren<QPushButton*>())
        {
            if (button->isVisible() && button->text() == label)
            {
                details << "overwrite choice=" << label << "\n";
                details.flush();
                ++phase;
                QTest::mouseClick(button, Qt::LeftButton);
                return;
            }
        }
    });
    confirmationDriver.start(40);
    QTimer driver;
    // 观察实际模态窗口后点击内部控件；不写产品返回值或模型。
    QObject::connect(&driver, &QTimer::timeout, [&]()
    {
        QWidget* modal = QApplication::activeModalWidget();
        if (modal == nullptr)
        {
            return;
        }
        DialogCountOperateParam count;
        DialogManager::instance().operateDialog(count);
        valid = valid && count.m_count > 0;
        observed = true;
        QFileDialog* files = modal->findChild<QFileDialog*>(QStringLiteral("splitViewerFilePicker"));
        if (files != nullptr)
        {
            valid = valid && dynamic_cast<FileDialog*>(files) != nullptr && !files->isWindow();
            QDialogButtonBox* buttons = files->findChild<QDialogButtonBox*>();
            if (!fileStateLogged)
            {
                details << "FileDialog wrapper=" << (dynamic_cast<FileDialog*>(files) != nullptr) <<
                    " isWindow=" << files->isWindow() << " acceptMode=" << files->acceptMode() << "\n";
                if (buttons != nullptr)
                {
                    foreach (QAbstractButton* button, buttons->buttons())
                    {
                        details << "button=" << button->text() << " standard=" << buttons->standardButton(button) << "\n";
                    }
                }
                details.flush();
                fileStateLogged = true;
            }
            if (buttons == nullptr)
            {
                valid = false;
                return;
            }
            if (id == 173)
            {
                driver.stop();
                QPushButton* cancel = buttons->button(QDialogButtonBox::Cancel);
                valid = valid && cancel != nullptr;
                if (cancel != nullptr)
                {
                    QTest::mouseClick(cancel, Qt::LeftButton);
                }
            }
            else if ((id == 174 && phase == 0) || (id == 175 && (phase == 0 || phase == 2)))
            {
                files->selectFile(fixture);
                QLineEdit* fileName = files->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"));
                if (fileName != nullptr)
                {
                    fileName->setFocus();
                    fileName->selectAll();
                    QTest::keyClick(fileName, Qt::Key_Backspace);
                    fileName->insert(fixture);
                }
                QPushButton* accept = buttons->button(id == 175 ? QDialogButtonBox::Save : QDialogButtonBox::Open);
                valid = valid && accept != nullptr;
                if (accept != nullptr && accept->isEnabled())
                {
                    details << "file accept phase=" << phase << " paths=" << files->selectedFiles().join(QStringLiteral(";")) << "\n";
                    details.flush();
                    ++phase;
                    QTest::mouseClick(accept, Qt::LeftButton);
                }
            }
            return;
        }
        if (id == 175)
        {
            return;
        }
        if (id == 177 && phase == 1)
        {
            driver.stop();
            QTest::keyClick(modal, Qt::Key_Escape);
            ++phase;
            return;
        }
        if (id == 177 && phase == 0)
        {
            foreach (QLineEdit* edit, modal->findChildren<QLineEdit*>())
            {
                if (edit->isVisible())
                {
                    edit->setFocus();
                    QTest::keyClicks(edit, "0x1234");
                    break;
                }
            }
        }
        const QString label = id == 175 && phase == 1 ? QStringLiteral("取消") : QStringLiteral("确认");
        foreach (QPushButton* button, modal->findChildren<QPushButton*>())
        {
            if (button->isVisible() && button->text() == label)
            {
                ++phase;
                QTest::mouseClick(button, Qt::LeftButton);
                return;
            }
        }
    });
    driver.start(40);
    if (id == 172)
    {
        valid = dynamic_cast<MainWindow*>(&window) != nullptr && dynamic_cast<Widget*>(canvas) != nullptr &&
            dynamic_cast<ToolBar*>(window.findChild<QToolBar*>()) != nullptr &&
            dynamic_cast<StatusBar*>(window.statusBar()) != nullptr;
        foreach (QLabel* label, window.findChildren<QLabel*>())
        {
            valid = valid && dynamic_cast<Label*>(label) != nullptr;
        }
        observed = true;
        window.grab().save(QDir(directory).filePath(QStringLiteral("controls.png")));
    }
    else if (id == 173 || id == 174)
    {
        QTest::mouseClick(canvas, Qt::LeftButton, Qt::NoModifier, canvas->rect().center());
        QApplication::processEvents();
        const QColor center(canvas->grab().toImage().pixel(canvas->rect().center()));
        valid = valid && center == (id == 174 ? red : QColor(70, 70, 70));
    }
    else if (id == 175)
    {
        const QString selected = SplitViewerDialogHelper::browseFile(&window, true, QStringLiteral("覆盖回归"),
            fixture, QStringLiteral("图片 (*.png)"));
        valid = valid && phase == 4 && selected == fixture;
        valid = valid && original.open(QIODevice::ReadOnly) && original.readAll() == bytesBefore;
    }
    else if (id == 176)
    {
        QToolBar* bar = window.findChild<QToolBar*>();
        foreach (QAction* action, bar->actions())
        {
            if (action->objectName() == QStringLiteral("aboutAction"))
            {
                QTest::mouseClick(bar->widgetForAction(action), Qt::LeftButton);
                break;
            }
        }
        valid = valid && phase == 1;
    }
    else if (id == 177)
    {
        bool accepted = false;
        const QString value = SplitViewerDialogHelper::inputText(&window, QStringLiteral("输入回归"), QStringLiteral("句柄"), accepted);
        valid = valid && accepted && value == QStringLiteral("0x1234");
        const QString cancelled = SplitViewerDialogHelper::inputText(&window, QStringLiteral("取消回归"), QStringLiteral("句柄"), accepted);
        valid = valid && !accepted && cancelled.isEmpty() && phase == 2;
    }
    driver.stop();
    confirmationDriver.stop();
    watchdog.stop();
    QTest::qWait(30);
    DialogCountOperateParam remaining;
    DialogManager::instance().operateDialog(remaining);
    details << "phase=" << phase << " valid=" << valid << " observed=" << observed <<
        " timeout=" << timedOut << " remaining=" << remaining.m_count << "\n";
    canvas->grab().save(QDir(directory).filePath(QStringLiteral("dialog-canvas.png")));
    return valid && observed && !timedOut && remaining.m_count == 0;
}