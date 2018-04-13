#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include "DialogManager.h"
#include "ControlsMacro.h"

class DialogBase;
class QWindow;
/** 单一实例，用于统一管理窗口创建关闭，该类的对外接口，销毁所有窗口
DialogManager::instance().removeAll();
*/
class ControlsAPI NotifyDialogManager : public DialogManager
{
	Q_OBJECT
	friend class DialogBase;
	friend class DialogShow;
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static NotifyDialogManager& instance();

	/** 显示提示框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] tip 提示内容
	@param [in] title 标题
	@param [in] done 按钮按下后信号里的参数值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showTipDialog(int32_t& dialogId,
					   int32_t userType,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11平台"),
					   int32_t done = ACCEPT_BUTTON,
					   const QString& buttonText = QString::fromStdWString(L"知道了"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

	/** 显示询问框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] tip 提示内容
	@param [in] title 标题
	@param [in] acceptDone 确认按钮按下后信号里的参数值
	@param [in] ignoreDone 取消按钮按下后信号里的参数值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showAskDialog(int32_t& dialogId,
					   int32_t userType,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11平台"),
					   int32_t acceptDone = ACCEPT_BUTTON,
					   int32_t ignoreDone = IGNORE_BUTTON,
					   const QString& acceptText = QString::fromStdWString(L"同意"),
					   const QString& ignoreText = QString::fromStdWString(L"拒绝"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

	/** 显示登录框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] tip 提示内容
	@param [in] linkUrl 了解更多链接地址
	@param [in] title 标题
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isUrlButtonVisible 了解更多按钮是否显示
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showLoginDialog(int32_t& dialogId,
						 int32_t userType,
						 const QString& tip,
						 const QString& linkUrl,
						 const QString& title = QString::fromStdWString(L"11平台"),
						 int32_t timeOut = 30,
						 bool isUrlButtonVisible = false,
						 bool isCountDownVisible = false);

	/** 显示提示框
	@param [out] dialogId 窗口ID值
	@param [in] tip 提示内容
	@param [in] title 标题
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] done 按钮按下后信号里的参数值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showTipDialog(int32_t& dialogId,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11平台"),
					   int32_t userType = 0,
					   int32_t done = ACCEPT_BUTTON,
					   const QString& buttonText = QString::fromStdWString(L"知道了"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

	/** 显示询问框
	@param [out] dialogId 窗口ID值
	@param [in] tip 提示内容
	@param [in] title 标题
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] acceptDone 确认按钮按下后信号里的参数值
	@param [in] ignoreDone 取消按钮按下后信号里的参数值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showAskDialog(int32_t& dialogId,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11平台"),
					   int32_t userType = 0,
					   int32_t acceptDone = ACCEPT_BUTTON,
					   int32_t ignoreDone = IGNORE_BUTTON,
					   const QString& acceptText = QString::fromStdWString(L"同意"),
					   const QString& ignoreText = QString::fromStdWString(L"拒绝"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

Q_SIGNALS:
	/** 当通知框窗口关闭时发送信号，在关闭动画执行之前发送信号
	@param [in] dialogId 窗口ID
	@param [in] result 窗口关闭时所给的返回值
	@param [in] userType 用户自定义值
	*/
	void dialogDone(int32_t dialogId, int32_t result, int32_t userType);

public slots:
	void onDialogDone(int32_t dialogId, int32_t result, int32_t userType);

private:
	int32_t popAskDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& acceptText,
						 int32_t acceptDone,
						 const QString& ignoreText,
						 int32_t ignoreDone,
						 QWindow* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	int32_t popTipDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& buttonText,
						 int32_t done,
						 QWindow* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	int32_t popInputDialog(int32_t& dialogId,
						   const QString& title,
						   const QString& editTip,
						   const QString& buttonText,
						   int32_t done,
						   QString& editText,
						   QWindow* parent = nullptr,
						   int32_t timeOut = -1,
						   bool isCountDownVisible = false);

	int32_t popWaitDialog(int32_t& dialogId,
						  const QString& title,
						  const QString& tip,
						  QWindow* parent = nullptr,
						  int32_t timeOut = -1,
						  bool isCountDownVisible = false);
};