#ifndef CORIGINALWEBVIEW_H
#define CORIGINALWEBVIEW_H

#include <qglobal.h>
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

#include <QWebView>
#include "QtControlsMacro.h"

/** 旧版WebKit网页控件，封装加载进度文字、右键菜单拦截和加载状态显示
*/
class QtControlsAPI COriginalWebView : public QWebView
{
    Q_OBJECT
public:
	/** 构造旧版WebKit网页控件
	@param [in] parent 父窗口指针
	*/
    COriginalWebView(QWidget* parent = nullptr);

	/** 析构函数
	*/
    ~COriginalWebView();

private:
	/** 绘制加载进度提示
	@param [in] e Qt绘制事件
	*/
    void paintEvent(QPaintEvent *e);

	/** 拦截默认网页右键菜单
	@param [in] e Qt右键菜单事件
	*/
	void contextMenuEvent(QContextMenuEvent* e);

private slots:
	/** 处理页面开始加载信号，进入加载状态
	*/
    void customerOnLoadStarted();

	/** 处理页面加载完成信号，退出加载状态
	@param [in] s true表示加载成功，false表示加载失败
	*/
    void customerOnLoadFinished(bool s);

	/** 处理页面加载进度变化
	@param [in] progress 当前加载进度百分比
	*/
    void customerOnLoadProgress(int progress);

	/** 设置是否显示加载进度
	@param [in] s true表示显示，false表示隐藏
	*/
	void setIsShowProgress(bool s);

	/** 获取加载进度显示状态
	@return 返回true表示显示加载进度，false表示隐藏
	*/
	bool isShowProgress();

private:
	// 是否显示加载进度文字
	bool    m_isShowProgress;
	// 当前页面是否处于加载中
    bool    m_isLoading;
	// 加载中展示的提示文字
    QString m_loadingText;
};

#endif

#endif // CORINGINALWEBVIEW_H
