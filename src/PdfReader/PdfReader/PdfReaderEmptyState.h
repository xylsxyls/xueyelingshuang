#pragma once

#include "QtControls/Widget.h"
#include <QRectF>

class QMouseEvent;
class QPaintEvent;

/** Empty document control that draws the SplitViewer-style plus button. */
class PdfReaderEmptyState : public Widget
{
    Q_OBJECT
public:
    /** Create a clickable empty-state control. */
    explicit PdfReaderEmptyState(QWidget* parent = nullptr);

Q_SIGNALS:
    /** Emitted when the centered plus button is clicked. */
    void clicked();

protected:
    /** Paint the centered gray square and dark cross. */
    void paintEvent(QPaintEvent* event) override;
    /** Emit clicked only when the plus button area is pressed. */
    void mousePressEvent(QMouseEvent* event) override;

private:
    /** Calculate the plus button hit and paint rectangle. */
    QRectF plusButtonRect() const;
};