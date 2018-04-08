#ifndef WEBVIEWEX_H
#define WEBVIEWEX_H

#include <QWebView>

class WebPage : public QWebPage
{
	Q_OBJECT
public:
	explicit WebPage(QWidget *parent = 0);
	bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);
	QWebPage * createWindow(WebWindowType type);

};

class CWebViewEx : public QWebView
{
	Q_OBJECT
public:
	Q_INVOKABLE void addJSObject(QString object_name,QObject *c_object);

	Q_INVOKABLE QVariant evaluateJavaScript(QString x);

// 	Q_INVOKABLE void setUrlWithLogin(const QString& url);
// 	Q_INVOKABLE qint64 getUserId();

protected slots:
	void realAddJSObject();

public:
	CWebViewEx(QWidget *parent=NULL, bool allowWebCache=true);
	~CWebViewEx();

public:
	void setSetting_JavaScriptCanCloseWnd(bool enable);
	void setSetting_EnableDebug(bool enable);
	void initWebCache(const QString& cache_dir = "");
private:
	WebPage *m_page;

private:
	QMutex m_mutex;
	bool m_bDestroying;
	std::map<QString,QObject*> m_js_object_list;
};

#endif // WEBVIEWEX_H
