#pragma once
#include "DialogShow.h"

class QWebView;
/** 显示提示框
*/
class LoginShowDialog : protected DialogShow
{
	Q_OBJECT
public:
	/** 显示登录框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] title 标题
	@param [in] greeting 招呼语
	@param [in] urlButtonText 更多按钮文字内容
	@param [in] linkUrl 更多按钮跳转链接
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isUrlButtonVisible 更多按钮是否显示
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	static void showLoginDialog(int32_t& dialogId,
								int32_t userType,
								const QString& title,
								const QString& greeting,
								const QString& tip,
								const QString& urlButtonText,
								const QString& linkUrl,
								int32_t timeOut = -1,
								bool isUrlButtonVisible = false,
								bool isCountDownVisible = false);
private:
	LoginShowDialog(int32_t userType,
					const QString& title,
					const QString& greeting,
					const QString& tip,
					const QString& urlButtonText,
					const QString& linkUrl,
					bool isUrlButtonVisible = false);

private slots:
	void onLinkClicked(const QUrl& url);
	void onMoreClicked();

private:
	Label* m_greeting;
	Label* m_horn;
	COriginalButton* m_more;
	QString m_moreLinkUrl;
	QWebView* m_tip;
};