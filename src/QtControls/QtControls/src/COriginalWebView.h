#ifndef CORIGINALWEBVIEW_H
#define CORIGINALWEBVIEW_H

#include <qglobal.h>
#if (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

#include <QWebView>
#include "QtControlsMacro.h"

class QtControlsAPI COriginalWebView : public QWebView
{
    Q_OBJECT
public:
    COriginalWebView(QWidget* parent = NULL);
    ~COriginalWebView();

private:
	bool    mIsShowProgress;
    bool    mIsLoading;
    QString mLoadingText;

    void paintEvent(QPaintEvent *e);
	void contextMenuEvent(QContextMenuEvent* e);

private slots:
    void customerOnLoadStarted();
    void customerOnLoadFinished(bool s);
    void customerOnLoadProgress(int progress);

	void setIsShowProgress(bool s);
	bool isShowProgress();

};

#endif

#endif // CORINGINALWEBVIEW_H
