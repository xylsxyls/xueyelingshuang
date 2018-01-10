//#include "stdafx.h"
#include "WebViewEx.h"
#include <QWebSettings>
#include <QtNetwork/QNetworkDiskCache>
#include <QDesktopServices>
#include <QWebFrame>
#include <QDir>

WebPage::WebPage(QWidget *parent):
QWebPage(parent)
{
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type) 
{
	if (frame == nullptr)
	{
		QDesktopServices::openUrl(request.url());
		return true;
	}

	if( type == QWebPage::NavigationTypeLinkClicked ) 
	{
		QString url = request.url().toString().trimmed();
		if (url.startsWith(QString("qfyygame:///"), Qt::CaseInsensitive))
		{
			QDesktopServices::openUrl(url);
			return true;
		}
	}
		
	 return QWebPage::acceptNavigationRequest(frame,request,type);
}

QWebPage * WebPage::createWindow(WebWindowType type) {
	return QWebPage::createWindow(type);
}

CWebViewEx::CWebViewEx(QWidget *parent, bool allowWebCache)
	: QWebView(parent)
	, m_bDestroying(false)
	, m_mutex(QMutex::Recursive)
	, m_page(new WebPage(this))
{

	QWebSettings* settings = QWebView::settings();
	if (settings != NULL)
	{
		// 防止qml里面的网页和这里有冲突，loadfinish 时发生断言
		// 在函数 IconDatabase::setClient{ ...  ASSERT(!m_syncThreadRunning); ... }
 		settings->setIconDatabasePath(""); 
		settings->setAttribute(QWebSettings::PluginsEnabled,true);
		settings->setAttribute(QWebSettings::JavascriptEnabled, true);
		settings->setAttribute(QWebSettings::JavascriptCanCloseWindows, false);
		
		// html5 local storage feature，持续存储，与 cookie 有区别
		settings->setAttribute(QWebSettings::LocalStorageEnabled, true); // 代替了 LocalStorageDatabaseEnabled
#ifdef _DEBUG
		// 允许开发者模式
		settings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
#else
		settings->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);
		QWebView::setContextMenuPolicy(Qt::NoContextMenu);// 屏蔽右键菜单
#endif

		//因为当网页中使用了 canvas 元素的3d功能时会导致崩溃，所以禁止Web OpenGL功能
		settings->setAttribute(QWebSettings::WebGLEnabled, false);

		// 允许 cookie，缺省就是这样的，可以不调用
		settings->setThirdPartyCookiePolicy(QWebSettings::AlwaysAllowThirdPartyCookies);
	}
	if( allowWebCache )
	{
//		initWebCache();
	}

	setPage(m_page);
	if(page() && page()->mainFrame())
	{
		connect(page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(realAddJSObject()));
	}

	//QApplication::addLibraryPath("../Dlls/plugins");
	//setStyleSheet("background:transparent;");
}

CWebViewEx::~CWebViewEx()
{
	m_bDestroying = false;
}

// 是否允许脚本关闭窗口
void CWebViewEx::setSetting_JavaScriptCanCloseWnd(bool enable)
{
	QWebSettings* settings = QWebView::settings();
	if (settings != NULL)
	{
		settings->setAttribute(QWebSettings::JavascriptCanCloseWindows, enable);
	}
}

// 是否允许调试模式（开发者模式）
void CWebViewEx::setSetting_EnableDebug(bool enable)
{
	QWebSettings* settings = QWebView::settings();
	if (settings != NULL)
	{
		settings->setAttribute(QWebSettings::DeveloperExtrasEnabled, enable);
	}
}


void CWebViewEx::initWebCache(const QString& cache_dir)
{
	{ // 确保只调用一次
		static bool s_bFirstCall = true;
		if( s_bFirstCall )
			s_bFirstCall = false;
		else
			return;
	}

	QString webCacheDir;
	if( !cache_dir.isEmpty() && QDir(cache_dir).exists())
	{
		webCacheDir = cache_dir;
	}
	else
	{
		//webCacheDir = QCoreApplication::applicationDirPath() + QLatin1String("/Runtime/WebCache");
		webCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
			 + QLatin1String("/Runtime/WebCache");
	}

	if (!webCacheDir.isEmpty())
	{
		QWebPage* webPage =  QWebView::page(); //new QWebPage(this);

		QNetworkDiskCache*      diskCache     = new QNetworkDiskCache(this);
		QNetworkAccessManager*  accessManager = new QNetworkAccessManager(this);

		diskCache->setCacheDirectory(webCacheDir);
		accessManager->setCache(diskCache);

		webPage->setNetworkAccessManager(accessManager);

		//widget->setPage(webPage);
	}
}




void CWebViewEx::addJSObject(QString object_name,QObject *c_object)
{
	// Add pAnalyzer to JavaScript Frame as member "imageAnalyzer".
	m_mutex.lock();
	m_js_object_list.insert(std::make_pair(object_name,c_object));
	m_mutex.unlock();
	//widget->page()->mainFrame()->addToJavaScriptWindowObject(object_name, c_object);
}

void CWebViewEx::realAddJSObject()
{
	if( m_bDestroying ) return;


	QWebFrame* pFrame;
	if( page() && page()->mainFrame())
	{
		pFrame = page()->mainFrame();
	}
	if( !pFrame )
	{
		return;
	}

	m_mutex.lock();
	std::map<QString,QObject*>::iterator x = m_js_object_list.begin();
	for (;x!=m_js_object_list.end();x++)
	{
		pFrame->addToJavaScriptWindowObject(x->first, x->second);
	}
#ifdef _DEBUG
//	pFrame->addToJavaScriptWindowObject("DebugMode_7F", new QObject());
#endif
	m_mutex.unlock();
}

QVariant CWebViewEx::evaluateJavaScript(QString x)
{
	QVariant v(false);
	if( page() && page()->mainFrame())
	{
		return page()->mainFrame()->evaluateJavaScript(x);
	}
	return v;
}