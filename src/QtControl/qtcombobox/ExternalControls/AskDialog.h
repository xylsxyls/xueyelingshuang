#pragma once
#include "DialogShow.h"

/** 询问弹出框
*/
class AskDialog : protected DialogShow
{
	Q_OBJECT
public:
	/** 弹出询问框
	@param [out] dialogId 窗口ID值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] acceptText 接受按钮内容
	@param [in] acceptDone 接受按钮按下后的返回值
	@param [in] ignoreText 拒绝按钮内容
	@param [in] ignoreDone 拒绝按钮按下后的返回值
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@return 关闭窗口时给的返回值
	*/
	static int32_t popAskDialog(int32_t& dialogId,
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
	AskDialog(const QString& title,
			  const QString& tip,
			  const QString& acceptText,
			  int32_t acceptDone,
			  const QString& ignoreText,
			  int32_t ignoreDone);

private:
	Label* m_tip;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};