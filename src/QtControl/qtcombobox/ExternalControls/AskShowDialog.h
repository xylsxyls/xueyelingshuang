#pragma once
#include "DialogShow.h"

/** 显示提示框
*/
class AskShowDialog : protected DialogShow
{
	Q_OBJECT
public:
	/** 显示提示框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] acceptText 按钮内容
	@param [in] acceptDone 确认按钮按下后信号里的参数值
	@param [in] ignoreText 按钮内容
	@param [in] ignoreDone 取消按钮按下后信号里的参数值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	static void showAskDialog(int32_t& dialogId,
							  int32_t userType,
							  const QString& title,
							  const QString& tip,
							  const QString& acceptText,
							  int32_t acceptDone,
							  const QString& ignoreText,
							  int32_t ignoreDone,
							  int32_t timeOut = -1,
							  bool isCountDownVisible = false);
private:
	AskShowDialog(int32_t userType,
				  const QString& title,
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