#include "PdfReaderTestUiHelper.h"
#include "PdfReaderTestHelper.h"
#include "../../PdfReader/PdfReader/PdfReader.h"
#include <QEventLoop>
#include <QKeyEvent>
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QAbstractButton>
#include <QLineEdit>
#include <QElapsedTimer>
#include <QFileInfo>
#include <stdexcept>

QWidget* PdfReaderTestUiHelper::dialogView()
{
    QWidget* modal = QApplication::activeModalWidget();
    while (modal != nullptr)
    {
        if (modal->objectName() == QStringLiteral("pdfReaderDialogView")) return modal;
        QWidget* view = modal->findChild<QWidget*>(QStringLiteral("pdfReaderDialogView"));
        if (view != nullptr) return view;
        modal = modal->parentWidget();
    }
    return nullptr;
}

QFileDialog* PdfReaderTestUiHelper::fileDialog()
{
    QWidget* view = dialogView();
    return view ? view->findChild<QFileDialog*>() : nullptr;
}

bool PdfReaderTestUiHelper::answerDialog(bool accepted)
{
    QWidget* view = dialogView();
    if (!view) return false;
    QAbstractButton* button = view->findChild<QAbstractButton*>(accepted ? QStringLiteral("dialogAccept") : QStringLiteral("dialogCancel"));
    if (!button) return false;
    button->click();
    return true;
}

void PdfReaderTestUiHelper::wait(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, SLOT(quit()));
    loop.exec();
}

void PdfReaderTestUiHelper::escape(QWidget* target)
{
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(target, &press);
    QApplication::sendEvent(target, &release);
}

void PdfReaderTestUiHelper::mouse(QWidget* target, QEvent::Type type, const QPoint& point)
{
    QMouseEvent event(type, point, target->mapToGlobal(point),
        type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
        type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &event);
    QApplication::processEvents();
}

void PdfReaderTestUiHelper::wheel(QWidget* target, int delta)
{
    QWheelEvent event(QPointF(20, 20), delta, Qt::NoButton, Qt::ControlModifier);
    QApplication::sendEvent(target, &event);
    PdfReaderTestUiHelper::wait(30);
}

void PdfReaderTestUiHelper::scrollWheel(QWidget* target, int delta)
{
    QWheelEvent event(QPointF(20, 20), delta, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(target, &event);
    PdfReaderTestUiHelper::wait(30);
}

QLabel* PdfReaderTestUiHelper::page(PdfReader& window, int index)
{
    const QList<QLabel*> pages = window.findChildren<QLabel*>(QStringLiteral("pageLabel"));
    foreach (QLabel* label, pages)
    {
        if (label->property("pageIndex").toInt() == index) { return label; }
    }
    throw std::runtime_error("page widget missing");
}

void PdfReaderTestUiHelper::color(PdfReader& window, int index, QRgb expected)
{
    QScrollArea* scroll = window.findChild<QScrollArea*>(QStringLiteral("documentScroll"));
    scroll->ensureWidgetVisible(PdfReaderTestUiHelper::page(window, index));
    PdfReaderTestUiHelper::wait(50);
    const QPixmap* pixmap = PdfReaderTestUiHelper::page(window, index)->pixmap();
    PdfReaderTestHelper::require(pixmap && !pixmap->isNull(), "visible page has rendered pixels");
    const QImage image = pixmap->toImage();
    PdfReaderTestHelper::require((image.pixel(image.width() / 2, image.height() / 2) & 0xffffff) == (expected & 0xffffff), "rendered page order/color");
}

void PdfReaderTestUiHelper::dialogAction(PdfReader& window, const char* slot, const QString& path, bool yes)
{
    bool handled = false;
    bool timedOut = false;
    QElapsedTimer deadline;
    deadline.start();
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&handled, &timedOut, &deadline, path, yes]() {
        if (deadline.elapsed() > 5000)
        {
            timedOut = true;
            if (QDialog* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget())) { dialog->reject(); }
            return;
        }
        if (QFileDialog* dialog = PdfReaderTestUiHelper::fileDialog())
        {
            if (!dialog->property("regressionPrepared").toBool())
            {
                dialog->setDirectory(QFileInfo(path).absolutePath());
                dialog->selectFile(QFileInfo(path).fileName());
                dialog->setProperty("regressionPrepared", true);
                return;
            }
            if (QLineEdit* edit = dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit")))
            {
                edit->setText(QFileInfo(path).fileName());
            }
            QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
            handled = true;
        }
        else if (PdfReaderTestUiHelper::dialogView())
        {
            if (PdfReaderTestUiHelper::answerDialog(yes))
            {
                handled = true;
            }
            else
            {
                timedOut = true;
                QApplication::activeModalWidget()->close();
            }
        }
    });
    timer.start(20);
    PdfReaderTestHelper::require(QMetaObject::invokeMethod(&window, slot, Qt::DirectConnection), "product action exists");
    timer.stop();
    PdfReaderTestHelper::require(handled, "expected confirmation/file dialog shown");
    PdfReaderTestHelper::require(!timedOut, "dialog input timed out");
    PdfReaderTestUiHelper::wait(40);
}