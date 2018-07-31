#pragma once
#include "PopDialog.h"

class CWebViewEx;
/** 包含广告位的询问弹出框
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
    void resizeEvent(QResizeEvent* eve);
    bool check();

public slots:
	void onLinkClicked(const QUrl& url);
	void onLoadFinished(bool finished);

private:
	Label* m_tip;
	CWebViewEx* m_advert;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};