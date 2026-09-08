#pragma once
#include "DialogManagerMacro.h"
#include "NotifyDialog.h"
#include <qglobal.h>

#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))

class QWebView;
class PushButton;
class Separator;
/** 登录提示通知框，WebKit可用时展示富文本提示、上次登录信息和跳转入口
*/
class LoginShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
	LoginShowDialog();

public:
    /** 设置提示
    @param [in] tip 提示内容
    */
    void setTip(const QString& tip);

    /** 设置招呼内容
    @param [in] greeting 招呼内容
    */
    void setGreeting(const QString& greeting);

    /** 设置更多按钮相关信息
    @param [in] buttonText 按钮文字内容
    @param [in] linkUrl 更多按钮链接地址
    @param [in] isUrlButtonVisible 设置更多按钮是否显示
    */
    void setMoreButton(const QString& buttonText, const QString& linkUrl, bool isUrlButtonVisible);

	/** 设置上次登录地址
	@param [in] preLoginAddr 上次登录地址
	*/
	void setPreLoginAddr(const QString& preLoginAddr);

	/** 设置上次登录时间
	@param [in] preLoginTime 上次登录时间
	*/
	void setPreLoginTime(const QString& preLoginTime);

protected:
    /** 调整登录提示、更多按钮和上次登录信息布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private slots:
	/** 处理提示内容中的链接点击
	@param [in] url 被点击的链接
	*/
	void onLinkClicked(const QUrl& url);

	/** 处理更多按钮点击，打开已配置的更多链接
	*/
	void onMoreClicked();

	/** 处理修改密码按钮点击
	*/
	void onChangePasswordClicked();

private:
	// 问候语文本
	Label* m_greeting;

	// 提示图标
	Label* m_horn;

	// 更多按钮
	PushButton* m_more;

	// 更多按钮跳转链接
	QString m_moreLinkUrl;

	// 富文本提示内容
	QWebView* m_tip;

    // 内容分割线
    Separator* m_separator;

	// 上次登录标签
	Label* m_preLoginLabel;

	// 上次登录说明文本
	Label* m_preLoginText;

	// 上次登录地址
	Label* m_preLoginAddr;

	// 修改密码按钮
	PushButton* m_changePassword;

	// 上次登录时间
	Label* m_preLoginTime;
};

#endif