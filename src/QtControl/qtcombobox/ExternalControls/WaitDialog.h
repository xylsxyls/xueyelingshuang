#pragma once
#include "DialogShow.h"

/** 等待弹出框
*/
class WaitDialog : protected DialogShow
{
	Q_OBJECT
public:
	/** 弹出等待框
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@return 关闭窗口时给的返回值
	*/
	static int32_t popWaitDialog(const QString& title,
								const QString& tip,
								QDialog* parent = nullptr,
								int32_t timeOut = -1);
private:
	WaitDialog(const QString& title, const QString& tip);

private:
	Label* m_tip;
};