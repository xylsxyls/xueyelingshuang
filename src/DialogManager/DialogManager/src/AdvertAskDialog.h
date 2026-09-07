#pragma once
#include "PopDialog.h"

#include <qglobal.h>
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

class CWebViewEx;
/** 包含广告位的询问弹出框，WebKit可用时展示广告并返回用户选择
*/
class AdvertAskDialog : public PopDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
	AdvertAskDialog();

public:
    /** 设置提示
    @param [in] tip 提示内容
    */
    void setTip(const QString& tip);

    /** 设置确定按钮文字内容和按下的返回值
    @param [in] acceptText 按钮文字内容
    @param [in] acceptDone 按下的返回值
    */
    void setAcceptButton(const QString& acceptText, DialogResult acceptDone);

    /** 设置取消按钮文字内容和按下的返回值
    @param [in] ignoreText 按钮文字内容
    @param [in] ignoreDone 按下的返回值
    */
    void setIgnoreButton(const QString& ignoreText, DialogResult ignoreDone);

    /** 初始化广告
    @param [in] advertUrl 广告地址
    */
    void initAdvertUrl(const QString& advertUrl);

Q_SIGNALS:
    /** 广告被点击发送信号
    @param [in] url 广告链接地址
    */
    void advertClicked(const QString& url);

protected:
    /** 调整提示、广告和按钮布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

public slots:
	/** 处理广告链接点击
	@param [in] url 被点击的广告链接
	*/
	void onLinkClicked(const QUrl& url);

	/** 处理广告页面加载完成状态
	@param [in] finished true表示加载成功完成
	*/
	void onLoadFinished(bool finished);

private:
	// 提示文本
	Label* m_tip;

	// 广告展示控件
	CWebViewEx* m_advert;

	// 确定按钮
	COriginalButton* m_accept;

	// 取消按钮
	COriginalButton* m_ignore;
};

#endif