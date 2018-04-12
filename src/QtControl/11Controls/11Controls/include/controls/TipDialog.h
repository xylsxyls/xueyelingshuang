#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

/** 提示弹出框
*/
class TipDialog : public DialogShow
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
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	static int32_t popTipDialog(int32_t& dialogId,
								const QString& title,
								const QString& tip,
								const QString& buttonText,
								int32_t done,
								QWindow* parent = nullptr,
								int32_t timeOut = -1,
								bool isCountDownVisible = false);
private:
	TipDialog(const QString& title,
			  const QString& tip,
			  const QString& buttonText,
			  int32_t done);

private slots:
	void tipAccept(QObject* tar, Qt::Key key);

private:
	Label* m_tip;
	COriginalButton* m_accept;
};