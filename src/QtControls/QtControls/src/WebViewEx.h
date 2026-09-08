#ifndef WEBVIEWEX_H
#define WEBVIEWEX_H

#include <qglobal.h>
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

#include <QWebView>
#include <QMutex>
#include <QVariant>
#include <map>
#include "QtControlsMacro.h"

/** WebKit页面扩展，拦截链接跳转并为新窗口请求创建受控页面
*/
class QtControlsAPI WebPage : public QWebPage
{
	Q_OBJECT
public:
	/** 构造页面扩展对象
	@param [in] parent 父窗口指针
	*/
	explicit WebPage(QWidget *parent = 0);

	/** 处理页面导航请求，外部链接会交给系统浏览器打开
	@param [in] frame 发起请求的网页Frame
	@param [in] request 导航请求
	@param [in] type 导航类型
	@return 返回true表示允许WebKit继续加载，false表示已拦截
	*/
	bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);

	/** 创建新窗口承载页面
	@param [in] type 新窗口类型
	@return 返回用于新窗口的页面对象
	*/
	QWebPage * createWindow(WebWindowType type);

};

/** WebKit视图扩展，封装JS对象注入、脚本执行、调试开关和网页缓存初始化
*/
class QtControlsAPI CWebViewEx : public QWebView
{
	Q_OBJECT
public:
	/** 添加一个QObject给网页JavaScript访问，加载完成后会自动注入
	@param [in] object_name JavaScript中暴露的对象名称
	@param [in] c_object 被暴露给JavaScript的QObject对象
	*/
	Q_INVOKABLE void addJSObject(QString object_name,QObject *c_object);

	/** 在当前主Frame中执行JavaScript
	@param [in] x JavaScript脚本文本
	@return 返回脚本执行结果
	*/
	Q_INVOKABLE QVariant evaluateJavaScript(QString x);

// 	Q_INVOKABLE void setUrlWithLogin(const QString& url);
// 	Q_INVOKABLE qint64 getUserId();

protected slots:
	/** 在WebKit主线程中执行延迟的JavaScript对象注入
	*/
	void realAddJSObject();

public:
	/** 构造WebKit扩展视图
	@param [in] parent 父窗口指针
	@param [in] allowWebCache true表示允许初始化磁盘缓存，false表示关闭缓存
	*/
	CWebViewEx(QWidget *parent = nullptr, bool allowWebCache = true);

	/** 析构函数，标记销毁状态防止异步注入访问已释放对象
	*/
	~CWebViewEx();

public:
	/** 设置网页脚本是否允许关闭窗口
	@param [in] enable true表示允许JavaScript关闭窗口，false表示禁止
	*/
	void setSetting_JavaScriptCanCloseWnd(bool enable);

	/** 设置是否启用WebKit开发调试功能
	@param [in] enable true表示启用调试，false表示关闭调试
	*/
	void setSetting_EnableDebug(bool enable);

	/** 初始化WebKit磁盘缓存目录
	@param [in] cache_dir 缓存目录，空字符串表示使用默认缓存目录
	*/
	void initWebCache(const QString& cache_dir = "");

private:
	/** 处理鼠标按下事件，用于兼容WebKit控件内部焦点和点击行为
	@param [in] e Qt鼠标事件
	*/
	void mousePressEvent(QMouseEvent* e);

private:
	// 自定义WebKit页面对象
	WebPage *m_page;
	// 保护JS对象列表和销毁状态的互斥量
	QMutex m_mutex;
	// 当前视图是否正在销毁
	bool m_bDestroying;
	// 待注入到JavaScript环境的QObject对象集合
	std::map<QString,QObject*> m_js_object_list;
};

#endif

#endif // WEBVIEWEX_H