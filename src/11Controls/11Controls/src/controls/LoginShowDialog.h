#pragma once
#include "ControlsMacro.h"
#include "NotifyDialog.h"

class QWebView;
class COriginalButton;
class Separator;
/** 显示提示框
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
    void resizeEvent(QResizeEvent* eve);
    bool check();

private slots:
	void onLinkClicked(const QUrl& url);
	void onMoreClicked();
	void onChangePasswordClicked();

private:
	Label* m_greeting;
	Label* m_horn;
	COriginalButton* m_more;
	QString m_moreLinkUrl;
	QWebView* m_tip;
    Separator* m_separator;
	Label* m_preLoginLabel;
	Label* m_preLoginText;
	Label* m_preLoginAddr;
	COriginalButton* m_changePassword;
	Label* m_preLoginTime;
};