#include "COriginalWebView.h"

#include <QPainter>
#include <QContextMenuEvent>
#include <QMenu>

COriginalWebView::COriginalWebView(QWidget *parent)
    : QWebView(parent)
    , mIsLoading(true)
	, mIsShowProgress(false)
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
    if(mIsLoading)
    {
        painter.save();
        painter.fillRect(this->rect(),QColor(40,45,60));

        QFont loadingTextFont;
        loadingTextFont.setPixelSize(this->width()/10);
        painter.setFont(loadingTextFont);
        painter.setPen(Qt::gray);

        QFontMetrics metrics(loadingTextFont);
        QString tLoadingText = metrics.elidedText(mLoadingText, Qt::ElideRight, this->width());

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
	QAction* ac = menu.addAction(QStringLiteral("ÖØÐÂ¼ÓÔØ"));
	if(ac == menu.exec(e->globalPos()))
	{
		this->reload();
	}
}

void COriginalWebView::customerOnLoadStarted()
{
    mLoadingText  = "Loading";
    mIsLoading = true;

    this->update();
}

void COriginalWebView::customerOnLoadFinished(bool s)
{
    mIsLoading = false;
    if(s)
    {
        mLoadingText  = "Loading finished.";

    }
    else
    {
        mLoadingText  = "Net Error.";
    }


    this->update();
}

void COriginalWebView::customerOnLoadProgress(int progress)
{
    if(mIsLoading)
    {
        mLoadingText  = QString("Loading %1%...").arg(progress);
    }

    this->update();
}

void COriginalWebView::setIsShowProgress(bool s)
{
	mIsShowProgress = s;
}

bool COriginalWebView::isShowProgress()
{
	return mIsShowProgress;
}

