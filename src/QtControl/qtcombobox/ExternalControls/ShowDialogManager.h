#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include "DialogManager.h"

class DialogBase;
class QWidget;
/** 单一实例，用于统一管理窗口创建关闭，该类的对外接口，销毁所有窗口
DialogManager::instance().removeAll();
*/
class ShowDialogManager : public DialogManager
{
	Q_OBJECT
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static ShowDialogManager& instance();

	/** 显示提示框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] done 按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showTipDialog(int32_t& dialogId,
					   int32_t userType,
					   const QString& title,
					   const QString& tip,
					   int32_t done,
					   const QString& buttonText = QString::fromStdWString(L"知道了"),
					   int32_t timeOut = -1,
					   bool isCountDownVisible = false);

	/** 显示询问框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] acceptDone 确认按钮按下后的返回值
	@param [in] ignoreDone 取消按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showAskDialog(int32_t& dialogId,
					   int32_t userType,
					   const QString& title,
					   const QString& tip,
					   int32_t acceptDone,
					   int32_t ignoreDone,
					   const QString& acceptText = QString::fromStdWString(L"同意"),
					   const QString& ignoreText = QString::fromStdWString(L"拒绝"),
					   int32_t timeOut = -1,
					   bool isCountDownVisible = false);

	/** 显示登录框
	@param [out] dialogId 窗口ID值
	@param [in] userType 用户自定义值，当窗口关闭时会发送信号，信号中含有这个值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] linkUrl 了解更多链接地址
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isUrlButtonVisible 了解更多按钮是否显示
	@param [in] isCountDownVisible 倒计时是否显示
	*/
	void showLoginDialog(int32_t& dialogId,
						 int32_t userType,
						 const QString& title,
						 const QString& tip,
						 const QString& linkUrl,
						 int32_t timeOut = -1,
						 bool isUrlButtonVisible = false,
						 bool isCountDownVisible = false);
Q_SIGNALS:
	void dialogDone(int32_t dialogId, int32_t result, int32_t userType);
};