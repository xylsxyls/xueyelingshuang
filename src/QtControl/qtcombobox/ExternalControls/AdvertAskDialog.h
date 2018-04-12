#pragma once
#include "DialogShow.h"

class CWebViewEx;
/** 包含广告位的询问弹出框
*/
class AdvertAskDialog : public DialogShow
{
	Q_OBJECT
public:
	/** 弹出询问框
	@param [out] dialogId 窗口ID值
	@param [in] advertUrl 广告位URL
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] acceptText 接受按钮内容
	@param [in] acceptDone 接受按钮按下后的返回值
	@param [in] ignoreText 拒绝按钮内容
	@param [in] ignoreDone 拒绝按钮按下后的返回值
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	static int32_t popAdvertAskDialog(int32_t& dialogId,
									  const QString& advertUrl,
									  const QString& title,
									  const QString& tip,
									  const QString& acceptText,
									  int32_t acceptDone,
									  const QString& ignoreText,
									  int32_t ignoreDone,
									  QWindow* parent = nullptr,
									  int32_t timeOut = -1,
									  bool isCountDownVisible = false);
private:
	AdvertAskDialog(const QString& advertUrl,
					const QString& title,
					const QString& tip,
					const QString& acceptText,
					int32_t acceptDone,
					const QString& ignoreText,
					int32_t ignoreDone);

Q_SIGNALS:
	void advertClicked(AdvertAskDialog* dialogPtr, const QString& url);

public slots:
	void advertAskAccept(QObject* tar, Qt::Key key);
	void onLinkClicked(const QUrl& url);

private:
	Label* m_tip;
	CWebViewEx* m_advert;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};