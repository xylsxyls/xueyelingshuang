#include "PdfReaderDialogTests.h"
#include "PdfReaderTestHelper.h"
#include "PdfReaderTestUiHelper.h"
#include "../../PdfReader/PdfReader/PdfReader.h"
#include "../../PdfReader/PdfReader/PdfReaderDialogHelper.h"
#include "QtControls/PushButton.h"
#include "QtControls/LineEdit.h"
#include "QtControls/DialogBase.h"
#include "QtControls/FileDialog.h"
#include "QtControls/Widget.h"
#include "QtControls/ScrollBar.h"
#include "QtControls/GroupBox.h"
#include "QtControls/StatusBar.h"
#include <QApplication>
#include <QDialog>
#include <QAbstractButton>
#include <QTimer>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFileDialog>
#include <QImage>
#include <QKeyEvent>

void PdfReaderDialogTests::run(const QString& input, const QString& directory)
{
    Config testConfig;
    testConfig.useNativeFileDialog = false;
    PdfReader window(nullptr, testConfig);
    window.show();
    PdfReaderTestHelper::require(window.openFile(input), "dialog fixture open");
    PdfReaderTestUiHelper::wait(30);
    PdfReaderTestHelper::require(dynamic_cast<MainWindow*>(&window) &&
        dynamic_cast<ListWidget*>(window.findChild<QListWidget*>()) &&
        dynamic_cast<ScrollArea*>(window.findChild<QScrollArea*>()) &&
        dynamic_cast<ToolBar*>(window.findChild<QToolBar*>()) &&
        dynamic_cast<Splitter*>(window.centralWidget()), "product uses QtControls containers");
    PdfReaderTestHelper::require(dynamic_cast<ScrollBar*>(window.findChild<QListWidget*>()->verticalScrollBar()), "thumbnail scrollbar wrapper");
    PdfReaderTestHelper::require(dynamic_cast<Label*>(PdfReaderTestUiHelper::page(window, 0)), "page uses QtControls Label");
    Splitter styled;
    styled.setBackgroundColor(QColor(11, 22, 33), QColor(44, 55, 66), QColor(77, 88, 99));
    styled.setBorderWidth(3);
    styled.repaint();
    const QString style = styled.styleSheet();
    styled.show();
    PdfReaderTestUiHelper::wait(10);
    PdfReaderTestHelper::require(style.contains("background-color") && style.contains(":hover") &&
        style.contains(":disabled") && style.contains("3px") && style == styled.styleSheet(),
        "new wrapper preserves normal/hover/disabled QSS across show");
    styled.hide();
    PdfReaderTestHelper::require(window.styleSheet().contains("QListWidget::item:selected") &&
        window.styleSheet().contains("QScrollArea") &&
        window.styleSheet().contains("QMenu{background:#ffffff"),
        "MainWindow preserves supplied descendant QSS and opaque context menu style");

    QObject connectionScope;
    int doneCount = 0;
    bool tracked = false, passwordHidden = false, defaultCancel = false, timedOut = false, fileWrapped = false;
    bool windowOptions = false, helpLayout = false, aboutCloseButton = false;
    QObject::connect(&DialogManager::instance(), &DialogManager::dialogSignal, &connectionScope,
        [&doneCount](const DialogSignalPtr& signal) {
            if (signal && signal->signalType() == CUSTOM_DIALOG_DONE_SIGNAL) ++doneCount;
        });
    int action = 0;
    QElapsedTimer deadline;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        QWidget* modal = QApplication::activeModalWidget();
        if (!modal) return;
        if (deadline.elapsed() > 4000)
        {
            timedOut = true;
            modal->close();
            return;
        }
        DialogCountOperateParam count;
        DialogManager::instance().operateDialog(count);
        tracked = tracked || count.m_count == 1;
        if (action == 4)
        {
            QDialog* dialog = qobject_cast<QDialog*>(modal);
            if (!dialog) return;
            DialogBase* base = dynamic_cast<DialogBase*>(dialog);
            windowOptions = base && base->windowShadowEnabled() && base->windowShadowSize() == 2 &&
                base->customerTitleBarHeight() == 40 && dialog->windowTitle() == QStringLiteral("PDF阅读器");
            QLabel* message = dialog->findChild<QLabel*>(QStringLiteral("dialogMessage"));
            helpLayout = message && message->text().startsWith(QStringLiteral("1.0版本\n1.")) &&
                (message->alignment() & Qt::AlignLeft);
            QAbstractButton* close = dialog->findChild<QAbstractButton*>(QStringLiteral("dialogCloseButton"));
            aboutCloseButton = close != nullptr && close->isVisible() && close->text().isEmpty() && !close->icon().isNull();
            const QImage dialogImage = modal->grab().toImage();
            const QColor bodyPixel = QColor::fromRgba(dialogImage.pixel(dialogImage.width() / 2, dialogImage.height() / 2));
            const QColor edgePixel = QColor::fromRgba(dialogImage.pixel(dialogImage.width() - 1, dialogImage.height() - 1));
            const QColor titlePixel = QColor::fromRgba(dialogImage.pixel(5, 5));
            windowOptions = windowOptions && edgePixel.alpha() == 255 && edgePixel != bodyPixel && titlePixel.alpha() == 255;
            dialogImage.save(directory + "/help-dialog.png");
            if (close != nullptr)
            {
                close->click();
            }
            return;
        }
        if (action == 3)
        {
            FileDialog* files = nullptr;
            foreach (QWidget* widget, QApplication::allWidgets())
            {
                FileDialog* candidate = dynamic_cast<FileDialog*>(widget);
                if (candidate != nullptr && candidate->isVisible())
                {
                    files = candidate;
                    break;
                }
            }
            fileWrapped = files != nullptr;
            if (files != nullptr)
            {
                files->reject();
            }
            else if (QFileDialog* native = qobject_cast<QFileDialog*>(modal))
            {
                native->reject();
            }
            return;
        }
        QWidget* view = PdfReaderTestUiHelper::dialogView();
        if (!view) return;
        if (action == 0)
        {
            LineEdit* edit = dynamic_cast<LineEdit*>(view->findChild<QLineEdit*>(QStringLiteral("dialogInput")));
            passwordHidden = edit && edit->echoMode() == QLineEdit::Password;
            if (edit) edit->setText(QStringLiteral("secret"));
            passwordHidden = passwordHidden && edit->toolTip().isEmpty();
            modal->grab().save(directory + "/password-dialog.png");
            PdfReaderTestUiHelper::answerDialog(true);
        }
        else if (action == 1)
        {
            PdfReaderTestUiHelper::escape(modal);
        }
        else if (action == 2)
        {
            QWidget* focus = QApplication::focusWidget();
            defaultCancel = focus && focus->objectName() == QStringLiteral("dialogCancel");
            modal->grab().save(directory + "/confirmation-dialog.png");
            PdfReaderTestUiHelper::answerDialog(false);
        }
    });
    timer.start(20);
    QString value;
    deadline.start();
    PdfReaderTestHelper::require(PdfReaderDialogHelper::input(&window, "Password", "Encrypted PDF", value, true), "managed password accept");
    PdfReaderTestHelper::require(passwordHidden && value == "secret", "password masked and value returned");
    action = 1; deadline.restart();
    PdfReaderTestHelper::require(!PdfReaderDialogHelper::input(&window, "Cancel", "Input", value), "Escape cancels input");
    PdfReaderTestHelper::require(value == "secret", "cancel preserves caller value");
    action = 2; deadline.restart();
    PdfReaderTestHelper::require(!PdfReaderDialogHelper::question(&window, "Overwrite", input), "overwrite canceled");
    PdfReaderTestHelper::require(defaultCancel, "destructive confirmation defaults to cancel");
    action = 3; deadline.restart();
    PdfReaderTestHelper::require(PdfReaderDialogHelper::file(&window, PdfReaderDialogParam::OpenFile,
        "Open", input, "PDF (*.pdf)", testConfig).isEmpty(), "file picker cancellation");
    action = 4; deadline.restart();
    QMetaObject::invokeMethod(&window, "showHelp", Qt::DirectConnection);
    timer.stop();
    DialogCountOperateParam count;
    DialogManager::instance().operateDialog(count);
    PdfReaderTestHelper::require(!timedOut, "managed dialogs complete without timeout");
    PdfReaderTestHelper::require(tracked, "DialogManager tracks the active dialog");
    PdfReaderTestHelper::require(fileWrapped, "file picker uses the QtControls fallback in regression mode");
    PdfReaderTestHelper::require(windowOptions, "help dialog applies title, shadow, and title bar options");
    PdfReaderTestHelper::require(helpLayout, "help dialog uses the requested version and left-aligned feature text");
    PdfReaderTestHelper::require(aboutCloseButton, "about dialog has a title-bar close button");
    PdfReaderTestHelper::require(doneCount == 5 && count.m_count == 0,
        "managed dialogs leave no retained windows");
    PdfReaderTestHelper::require(window.isEnabled() && window.openFile(input), "parent works after modal dialogs");
}