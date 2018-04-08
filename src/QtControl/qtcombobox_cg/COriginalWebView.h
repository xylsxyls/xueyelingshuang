#ifndef CORIGINALWEBVIEW_H
#define CORIGINALWEBVIEW_H

#include <QWebView>

class COriginalWebView : public QWebView
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

#endif // CORINGINALWEBVIEW_H
