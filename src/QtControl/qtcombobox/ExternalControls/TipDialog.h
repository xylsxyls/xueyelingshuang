#pragma once
#include "DialogShow.h"

/** 提示弹出框
*/
class TipDialog : protected DialogShow
{
	Q_OBJECT
public:
	/** 弹出提示框
	@param [out] dialogId 窗口ID值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@return 关闭窗口时给的返回值
	*/
	static int32_t popTipDialog(int32_t& dialogId,
								const QString& title,
								const QString& tip,
								const QString& buttonText,
								int32_t done,
								QDialog* parent = nullptr,
								int32_t timeOut = -1);
private:
	TipDialog(const QString& title,
			  const QString& tip,
			  const QString& buttonText,
			  int32_t done);

private:
	Label* m_tip;
	COriginalButton* m_accept;
};