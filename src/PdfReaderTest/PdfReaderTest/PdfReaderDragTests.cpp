#include "PdfReaderDragTests.h"
#include "PdfReaderTestHelper.h"
#include "PdfReaderTestUiHelper.h"
#include "../../PdfReader/PdfReader/PdfReader.h"
#include <QApplication>
#include <QScrollBar>
#include <QWheelEvent>
#include <QElapsedTimer>
#include <QEvent>

int PdfReaderDragTests::lineY(const QImage& image)
{
    int first = -1;
    int last = -1;
    for (int y = 0; y < image.height(); ++y)
    {
        int count = 0;
        for (int x = 10; x < image.width() - 10; ++x)
        {
            const QColor color = QColor::fromRgb(image.pixel(x, y));
            if (color.blue() > 150 && color.red() > 20 && color.red() < 100 && color.green() > 60 && color.green() < 160)
            {
                ++count;
            }
        }
        if (count >= image.width() - 22)
        {
            if (first < 0) { first = y; }
            last = y;
        }
    }
    return first >= 0 && last - first < 8 ? (first + last) / 2 : -1;
}

int PdfReaderDragTests::captureLine(PdfReaderThumbnailList* list, const QString& path)
{
    QApplication::processEvents();
    const QImage image = list->viewport()->grab().toImage();
    PdfReaderTestHelper::require(image.save(path), "drag screenshot written");
    const int y = lineY(image);
    PdfReaderTestHelper::require(y >= 0, "visible blue insertion line spans viewport center");
    return y;
}

void PdfReaderDragTests::heldWheel(QWidget* widget, const QPoint& point, int delta, bool control)
{
    QWheelEvent event(QPointF(point), delta, Qt::LeftButton, control ? Qt::ControlModifier : Qt::NoModifier);
    QApplication::sendEvent(widget, &event);
    QApplication::processEvents();
}

void PdfReaderDragTests::run(int id, const QString& input, const QString& directory)
{
    PdfReader window;
    window.show();
    PdfReaderTestHelper::require(window.openFile(input), "drag fixture accepted");
    PdfReaderTestUiHelper::waitIdle(window);
    PdfReaderThumbnailList* list = window.findChild<PdfReaderThumbnailList*>();
    PdfReaderTestHelper::require(list && list->count() >= 3, "drag list has independent fixture pages");
    QWidget* viewport = list->viewport();
    if (id == 30 || id == 31)
    {
        list->setStyleSheet(QStringLiteral("QListWidget{padding:0px;border:0px;}"));
        list->setFixedHeight(320);
        PdfReaderTestUiHelper::wait(40);
        PdfReaderTestUiHelper::waitIdle(window);
    }
    if (id == 29)
    {
        const QRect second = list->visualItemRect(list->item(1));
        const QPoint source(second.center().x(), second.bottom() - 5);
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonPress, source);
        PdfReaderTestUiHelper::waitIdle(window);
        QElapsedTimer elapsed;
        elapsed.start();
        PdfReaderTestUiHelper::wait(300);
        PdfReaderTestHelper::require(elapsed.elapsed() < 500, "pre-threshold observation within timing budget");
        PdfReaderTestHelper::require(lineY(viewport->grab().toImage()) < 0, "no drag indicator before 500 milliseconds");
        PdfReaderTestUiHelper::wait(260);
        const int y = captureLine(list, directory + "/initial-second.png");
        PdfReaderTestHelper::require(qAbs(y - second.top()) <= 5, "initial line is above source even when pressed in its lower half");
        const QColor tinted = QColor::fromRgb(viewport->grab().toImage().pixel(second.center()));
        PdfReaderTestHelper::require(tinted.green() > 180 && tinted.red() < 110 && tinted.blue() > 20 && tinted.blue() < 150,
            "light blue translucent overlay retains independent green page color");
        PdfReaderTestHelper::require(g_config.m_dragHoldMs == 500, "default long press is configured as 500 milliseconds");
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonRelease, source);
        PdfReaderTestHelper::require(list->currentRow() == 1, "stationary release does not move source");
        PdfReaderTestUiHelper::color(window, 1, qRgb(0,255,0));
    }
    else if (id == 30)
    {
        const QPoint source = list->visualItemRect(list->item(1)).center();
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonPress, source);
        PdfReaderTestUiHelper::wait(g_config.m_dragHoldMs + 60);
        const QPoint first(viewport->width()/2, 0);
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseMove, first);
        PdfReaderTestHelper::require(captureLine(list, directory + "/before-first.png") <= 5, "first gap is drawn inside viewport");
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonRelease, first);
        PdfReaderTestHelper::require(list->currentRow() == 0, "release before first agrees with indicator");
        PdfReaderTestUiHelper::color(window, 0, qRgb(0,255,0));
        list->verticalScrollBar()->setValue(0);
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonPress, list->visualItemRect(list->item(0)).center());
        PdfReaderTestUiHelper::wait(g_config.m_dragHoldMs + 60);
        list->verticalScrollBar()->setValue(list->verticalScrollBar()->maximum());
        const QPoint last(viewport->width()/2, viewport->height()-1);
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseMove, last);
        PdfReaderTestHelper::require(captureLine(list, directory + "/after-last.png") >= viewport->height()-6, "last gap remains inside viewport");
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonRelease, last);
        PdfReaderTestHelper::require(list->currentRow() == 2 && list->selectedItems().size() == 1, "release after last retains sole source selection");
        PdfReaderTestUiHelper::color(window, 2, qRgb(0,255,0));
    }
    else if (id == 31)
    {
        const QPoint source = list->visualItemRect(list->item(1)).center();
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonPress, source);
        PdfReaderTestUiHelper::wait(g_config.m_dragHoldMs + 60);
        const int before = list->verticalScrollBar()->value();
        const int rowHeight = list->item(0)->data(Qt::UserRole+2).toInt();
        heldWheel(viewport, source, -120, false);
        const int down = list->verticalScrollBar()->value();
        PdfReaderTestHelper::require(down > before, "held left button allows downward wheel scrolling");
        captureLine(list, directory + "/wheel-down.png");
        heldWheel(viewport, source, 120, true);
        PdfReaderTestHelper::require(list->verticalScrollBar()->value() < down, "Ctrl held wheel also scrolls during drag");
        PdfReaderTestHelper::require(list->item(0)->data(Qt::UserRole+2).toInt() == rowHeight, "held Ctrl wheel does not rebuild or zoom the model");
        PdfReaderTestHelper::require(list->currentRow() == 1 && list->selectedItems().size() == 1, "wheel keeps source identity");
        captureLine(list, directory + "/wheel-up.png");
        const int edgeBefore = list->verticalScrollBar()->value();
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseMove, QPoint(viewport->width()/2, viewport->height()-2));
        PdfReaderTestUiHelper::wait(100);
        PdfReaderTestHelper::require(list->verticalScrollBar()->value() > edgeBefore, "edge auto scroll remains active after wheel");
        captureLine(list, directory + "/edge-scroll.png");
        PdfReaderTestUiHelper::escape(list);
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonRelease, source);
        PdfReaderTestUiHelper::color(window, 1, qRgb(0,255,0));
    }
    else
    {
        const QRect second = list->visualItemRect(list->item(1));
        const QRect third = list->visualItemRect(list->item(2));
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonPress, second.center());
        PdfReaderTestUiHelper::wait(g_config.m_dragHoldMs + 60);
        const QPoint upper(third.center().x(), third.center().y()-2);
        const QPoint lower(third.center().x(), third.center().y()+2);
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseMove, upper);
        const int above = captureLine(list, directory + "/near-above.png");
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseMove, lower);
        const int below = captureLine(list, directory + "/near-below.png");
        PdfReaderTestHelper::require(below > above + 20, "four pixel crossing updates the insertion boundary");
        viewport->update(QRect(0,0,5,5));
        PdfReaderTestHelper::require(captureLine(list, directory + "/partial-repaint.png") == below, "partial repaint does not lose indicator");
        PdfReaderTestUiHelper::escape(list);
        PdfReaderTestHelper::require(lineY(viewport->grab().toImage()) < 0, "cancel removes indicator");
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonRelease, lower);
        PdfReaderTestUiHelper::color(window, 1, qRgb(0,255,0));
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonPress, second.center());
        PdfReaderTestUiHelper::wait(g_config.m_dragHoldMs + 60);
        list->setMinimumWidth(list->width()+20);
        PdfReaderTestUiHelper::wait(40);
        PdfReaderTestUiHelper::waitIdle(window);
        PdfReaderTestHelper::require(lineY(viewport->grab().toImage()) < 0, "layout rebuild cancels obsolete gesture");
        PdfReaderTestUiHelper::mouse(viewport, QEvent::MouseButtonRelease, lower);
        PdfReaderTestUiHelper::color(window, 1, qRgb(0,255,0));
    }
    window.close();
    PdfReaderTestUiHelper::waitIdle(window);
}