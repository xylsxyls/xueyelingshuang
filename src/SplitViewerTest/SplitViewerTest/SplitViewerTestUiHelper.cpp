#include "SplitViewerTestUiHelper.h"
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QDesktopWidget>
#include <QtTest/QtTest>

bool SplitViewerTestUiHelper::chooseMenu(QWidget* canvas, const QPoint& point, const QString& label)
{
    if (!settle(canvas))
    {
        return false;
    }
    bool clicked = false;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout,
        /** 在菜单模态循环中选择匹配的可用项，找不到时关闭本次菜单
        */
        [&]()
        {
            QMenu* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if (!menu)
            {
                return;
            }
            foreach (QAction* action, menu->actions())
            {
                if (action->text().contains(label) && action->isEnabled())
                {
                    timer.stop();
                    QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, menu->actionGeometry(action).center());
                    clicked = true;
                    return;
                }
            }
            timer.stop();
            menu->close();
        });
    timer.start(10);
    QContextMenuEvent event(QContextMenuEvent::Mouse, point, canvas->mapToGlobal(point));
    QApplication::sendEvent(canvas, &event);
    timer.stop();
    QApplication::processEvents();
    return settle(canvas) && clicked;
}

QRectF SplitViewerTestUiHelper::expectedStage(QWidget* canvas)
{
    const QRect screen = QApplication::desktop()->screenGeometry(canvas);
    const double aspect = static_cast<double>(screen.width()) / static_cast<double>(screen.height());
    QSizeF size(canvas->width() - 20, canvas->height() - 20);
    if (size.width() / size.height() > aspect)
    {
        size.setWidth(size.height() * aspect);
    }
    else
    {
        size.setHeight(size.width() / aspect);
    }
    return QRectF(QPointF((canvas->width() - size.width()) / 2, (canvas->height() - size.height()) / 2), size);
}

bool SplitViewerTestUiHelper::pixel(QWidget* widget, const QPoint& point, const QColor& expected)
{
    if (!settle(widget))
    {
        return false;
    }
    QApplication::processEvents();
    const QImage image = widget->grab().toImage();
    return image.rect().contains(point) && QColor(image.pixel(point)) == expected;
}

void SplitViewerTestUiHelper::drag(
    QWidget* widget, const QPoint& from, const QPoint& to, Qt::KeyboardModifiers modifiers)
{
    if (!settle(widget))
    {
        return;
    }
    QTest::mousePress(widget, Qt::LeftButton, modifiers, from);
    QMouseEvent move(QEvent::MouseMove, to, widget->mapToGlobal(to), Qt::NoButton, Qt::LeftButton, modifiers);
    QApplication::sendEvent(widget, &move);
    QTest::mouseRelease(widget, Qt::LeftButton, modifiers, to);
    QApplication::processEvents();
}

void SplitViewerTestUiHelper::wheel(QWidget* widget, const QPoint& point, int delta, Qt::KeyboardModifiers modifiers)
{
    if (!settle(widget))
    {
        return;
    }
    QWheelEvent event(
        point, widget->mapToGlobal(point), QPoint(), QPoint(0, delta), delta, Qt::Vertical, Qt::NoButton, modifiers);
    QApplication::sendEvent(widget, &event);
    QApplication::processEvents();
}

bool SplitViewerTestUiHelper::drop(QWidget* widget, const QPoint& point, const QString& path)
{
    if (!settle(widget))
    {
        return false;
    }
    QMimeData mime;
    mime.setUrls(QList<QUrl>() << QUrl::fromLocalFile(path));
    QDragEnterEvent enter(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &enter);
    QDropEvent drop(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(widget, &drop);
    QApplication::processEvents();
    return settle(widget) && enter.isAccepted() && drop.isAccepted();
}

bool SplitViewerTestUiHelper::toolbar(SplitViewer& window, const QString& label)
{
    if (!settle(&window))
    {
        return false;
    }
    QToolBar* bar = window.findChild<QToolBar*>();
    if (!bar)
    {
        return false;
    }
    foreach (QAction* action, bar->actions())
    {
        if (action->text() == label || action->toolTip() == label ||
            (label.contains(QStringLiteral("关于")) && action->objectName() == QStringLiteral("aboutAction")))
        {
            QWidget* button = bar->widgetForAction(action);
            if (!button)
            {
                return false;
            }
            QTest::mouseClick(button, Qt::LeftButton);
            QApplication::processEvents();
            return settle(&window);
        }
    }
    return false;
}

bool SplitViewerTestUiHelper::readDocument(const QString& path, SplitViewerCoreDocument& document)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QByteArray bytes = file.readAll();
    std::vector<uint8_t> package(bytes.begin(), bytes.end()), config;
    return SplitViewerCoreExtractEmbeddedConfig(package, config) && SplitViewerCoreDeserializeProfile(config, document);
}

std::vector<uint8_t> SplitViewerTestUiHelper::utf16(const QString& text)
{
    std::vector<uint8_t> bytes;
    bytes.push_back(255);
    bytes.push_back(254);
    for (int i = 0; i < text.size(); ++i)
    {
        const ushort c = text.at(i).unicode();
        bytes.push_back(c & 255);
        bytes.push_back(c >> 8);
    }
    return bytes;
}

bool SplitViewerTestUiHelper::settle(QWidget* widget)
{
    SplitViewer* window = widget ? qobject_cast<SplitViewer*>(widget->window()) : nullptr;
    QElapsedTimer deadline;
    deadline.start();
    while (window && window->processing() && deadline.elapsed() < 5000)
    {
        QTest::qWait(10);
    }
    QApplication::processEvents();
    return !window || !window->processing();
}

void SplitViewerTestUiHelper::click(
    QWidget* widget, Qt::MouseButton button, Qt::KeyboardModifiers modifiers, const QPoint& point)
{
    if (settle(widget))
    {
        QTest::mouseClick(widget, button, modifiers, point);
        settle(widget);
    }
}

void SplitViewerTestUiHelper::key(QWidget* widget, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    if (settle(widget))
    {
        QTest::keyClick(widget, key, modifiers);
        settle(widget);
    }
}