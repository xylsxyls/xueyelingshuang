#include "COriginalWebView.h"

#include <qglobal.h>
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

#include <QPainter>
#include <QContextMenuEvent>
#include <QMenu>

COriginalWebView::COriginalWebView(QWidget *parent)
    : QWebView(parent)
    , m_isLoading(true)
	, m_isShowProgress(false)
{
    connect(this, &COriginalWebView::loadStarted , this, &COriginalWebView::customerOnLoadStarted);
    connect(this, &COriginalWebView::loadFinished, this, &COriginalWebView::customerOnLoadFinished);
    connect(this, &COriginalWebView::loadProgress, this, &COriginalWebView::customerOnLoadProgress);
}

COriginalWebView::~COriginalWebView()
{

}

void COriginalWebView::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
	if(!this->isShowProgress())
	{
		QWebView::paintEvent(e);
		return ;
	}
    if(m_isLoading)
    {
        painter.save();
        painter.fillRect(this->rect(),QColor(40,45,60));

        QFont loadingTextFont;
        loadingTextFont.setPixelSize(this->width()/10);
        painter.setFont(loadingTextFont);
        painter.setPen(Qt::gray);

        QFontMetrics metrics(loadingTextFont);
        QString tLoadingText = metrics.elidedText(m_loadingText, Qt::ElideRight, this->width());

        painter.drawText(this->rect(), Qt::AlignCenter, tLoadingText);

        painter.restore();
    }
    else
    {
        QWebView::paintEvent(e);
    }
}

void COriginalWebView::contextMenuEvent(QContextMenuEvent* e)
{
	QMenu menu;
	QAction* ac = menu.addAction(QStringLiteral("重新加载"));
	if(ac == menu.exec(e->globalPos()))
	{
		this->reload();
	}
}

void COriginalWebView::customerOnLoadStarted()
{
    m_loadingText  = "Loading";
    m_isLoading = true;

    this->update();
}

void COriginalWebView::customerOnLoadFinished(bool s)
{
    m_isLoading = false;
    if(s)
    {
        m_loadingText  = "Loading finished.";

    }
    else
    {
        m_loadingText  = "Net Error.";
    }


    this->update();
}

void COriginalWebView::customerOnLoadProgress(int progress)
{
    if(m_isLoading)
    {
        m_loadingText  = QString("Loading %1%...").arg(progress);
    }

    this->update();
}

void COriginalWebView::setIsShowProgress(bool s)
{
	m_isShowProgress = s;
}

bool COriginalWebView::isShowProgress()
{
	return m_isShowProgress;
}


#endif