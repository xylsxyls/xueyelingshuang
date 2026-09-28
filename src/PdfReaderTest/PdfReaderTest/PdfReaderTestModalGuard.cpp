#include "PdfReaderTestModalGuard.h"
#include <QApplication>

PdfReaderTestModalGuard::PdfReaderTestModalGuard() : m_timedOut(false)
{
    QObject::connect(&m_timer, &QTimer::timeout, [this]() {
        QWidget* modal = QApplication::activeModalWidget();
        if (!modal)
        {
            modal = QApplication::activePopupWidget();
        }
        if (modal != m_modal)
        {
            m_modal = modal;
            m_elapsed.start();
        }
        else if (modal && m_elapsed.isValid() && m_elapsed.elapsed() > 5000)
        {
            m_timedOut = true;
            modal->close();
        }
    });
    m_timer.start(25);
}