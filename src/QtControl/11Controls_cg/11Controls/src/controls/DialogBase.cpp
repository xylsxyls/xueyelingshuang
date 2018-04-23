#include "DialogBase.h"
#include "Label.h"
#include "COriginalButton.h"
#include <QSplitter>
#include "LineEdit.h"
#include <QWindow>
#include "CPasswordInputBox.h"
#include "DialogManager.h"
#include "NotifyDialogManager.h"
#include "CGeneralStyle.h"
#include <Windows.h>
#include <QApplication>
#include <QKeyEvent>
#include <fstream>

DialogBase::DialogBase():
m_escAltF4Enable(true),
m_timeRest(-1),
m_timeId(-1),
m_title(nullptr)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    m_title = new Label(this);
}

void DialogBase::setNativeWindow(bool hasHandle)
{
    setAttribute(Qt::WA_NativeWindow, hasHandle);
}

void DialogBase::setExistFocus(bool focus)
{
    setAttribute(Qt::WA_ShowWithoutActivating, !focus);
}

void DialogBase::setTimeRest(int32_t timeOut)
{
    m_timeRest = timeOut;
    if (m_timeId != -1)
    {
        killTimer(m_timeId);
    }
}

void DialogBase::setEscAltF4Enable(bool enable)
{
    m_escAltF4Enable = enable;
}

int32_t DialogBase::exec()
{
    setWindowModality((transientWindow() != nullptr) ? Qt::WindowModal : Qt::ApplicationModal);
    return QDialog::exec();
}

void DialogBase::setChangeSizeEnable(bool enable)
{
    if (enable == false)
    {
        setFixedSize(width(), height());
    }
}

void DialogBase::setWindowTiTle(const QString& title,
                                const QColor& color,
                                int32_t fontSize,
                                Qt::Alignment align,
                                int32_t origin,
                                const QString& fontName)
{
    setWindowTitle(title);
    m_title->setText(title);
    m_title->setTextColor(color);
    m_title->setFontSize(fontSize);
    m_title->setAlignment(align);
    m_title->setTextOrigin(origin);
    m_title->setFontFace(fontName);
}

void DialogBase::showEvent(QShowEvent* eve)
{
    if (m_timeRest == 0)
    {
        eve->ignore();
        return;
    }
	else if (m_timeRest > 0)
	{
		m_timeId = startTimer(1000);
		emit timeRest(m_timeRest);
	}

	QDialog::showEvent(eve);
	raise();
}

void DialogBase::timerEvent(QTimerEvent* eve)
{
	--m_timeRest;
	emit timeRest(m_timeRest);
	if (m_timeRest == 0)
	{
		reject();
	}
	QDialog::timerEvent(eve);
}

void DialogBase::keyPressEvent(QKeyEvent* eve)
{
    if (((m_escAltF4Enable == false) && (eve != nullptr) && (eve->key() == Qt::Key_Escape)))
    {
        eve->ignore();
        return;
    }
    QDialog::keyPressEvent(eve);
}

void DialogBase::closeEvent(QCloseEvent* eve)
{
    if (m_escAltF4Enable == false)
    {
        eve->ignore();
        return;
    }
    QDialog::closeEvent(eve);
}

bool DialogBase::eventFilter(QObject* tar, QEvent* eve)
{
    if (tar == nullptr || eve == nullptr)
    {
        static ofstream file("eventFilter.txt");
        file << "tar = " << tar << "eve = " << eve << "\r\n";
        if (tar != nullptr)
        {
            file << "objectName = " << tar->objectName().toStdString() << "\r\n";
        }
        return true;
    }

    bool res = COriginalDialog::eventFilter(tar, eve);
    if (eve->type() == QEvent::KeyPress)
    {
        auto keyEvent = (QKeyEvent*)eve;
        if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
        {
            emit keyboardAccept(tar, (Qt::Key)keyEvent->key());
        }
    }

    return res;
}

void DialogBase::resizeEvent(QResizeEvent* eve)
{
    COriginalDialog::resizeEvent(eve);
    int32_t titleHeight = customerTitleBarHeight();
    m_title->setGeometry(0, 0, width(), titleHeight);
}

bool DialogBase::check()
{
    return m_title != nullptr;
}
