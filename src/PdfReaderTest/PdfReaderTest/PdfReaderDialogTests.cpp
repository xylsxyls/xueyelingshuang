#include "PdfReaderDialogTests.h"
#include "PdfReaderTestHelper.h"
#include "PdfReaderTestUiHelper.h"
#include "../../PdfReader/PdfReader/PdfReader.h"
#include "../../PdfReader/PdfReader/PdfReaderDialogHelper.h"
#include "QtControls/PushButton.h"
#include "QtControls/LineEdit.h"
#include "QtControls/FileDialog.h"
#include "QtControls/Widget.h"
#include "QtControls/ScrollBar.h"
#include "QtControls/GroupBox.h"
#include "QtControls/StatusBar.h"
#include <QApplication>
#include <QTimer>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QKeyEvent>

void PdfReaderDialogTests::run(const QString& input, const QString& directory)
{
    PdfReader window;
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
    PdfReaderTestHelper::require(window.styleSheet().contains("emptyDocumentLabel"), "MainWindow preserves supplied descendant QSS");

    QObject connectionScope;
    int doneCount = 0;
    bool tracked = false, passwordHidden = false, defaultCancel = false, timedOut = false, fileWrapped = false;
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
        QWidget* view = PdfReaderTestUiHelper::dialogView();
        if (!view) return;
        DialogCountOperateParam count;
        DialogManager::instance().operateDialog(count);
        tracked = tracked || count.m_count == 1;
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
        else if (action == 3)
        {
            FileDialog* files = dynamic_cast<FileDialog*>(PdfReaderTestUiHelper::fileDialog());
            fileWrapped = files != nullptr;
            if (files) files->reject();
        }
        else
        {
            modal->grab().save(directory + "/help-dialog.png");
            PdfReaderTestUiHelper::answerDialog(true);
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
        "Open", input, "PDF (*.pdf)").isEmpty(), "file picker cancellation");
    action = 4; deadline.restart();
    QMetaObject::invokeMethod(&window, "showHelp", Qt::DirectConnection);
    timer.stop();
    DialogCountOperateParam count;
    DialogManager::instance().operateDialog(count);
    PdfReaderTestHelper::require(!timedOut && tracked && fileWrapped && doneCount == 5 && count.m_count == 0,
        "all five dialogs managed, close signaled, no retained windows");
    PdfReaderTestHelper::require(window.isEnabled() && window.openFile(input), "parent works after modal dialogs");
}
