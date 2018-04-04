#pragma once
#include "DialogShow.h"

/** 等待弹出框
*/
class WaitDialog : public DialogShow
{
	Q_OBJECT
public:
	/** 弹出等待框
	@param [out] dialogId 窗口ID值
	@param [in] taskId 用户传入的ID值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	static int32_t popWaitDialog(int32_t& dialogId,
								 int32_t taskId,
								 const QString& title,
								 const QString& tip,
								 QWindow* parent = nullptr,
								 int32_t timeOut = -1,
								 bool isCountDownVisible = false);

	/** 设置任务ID值
	@param [in] taskId 任务ID值
	*/
	void setTaskId(int32_t taskId);

	/** 任务ID值
	@return 返回任务ID值
	*/
	int32_t getTaskId();

private:
	WaitDialog(const QString& title, const QString& tip);

private:
	Label* m_tip;
	int32_t m_taskId;
};