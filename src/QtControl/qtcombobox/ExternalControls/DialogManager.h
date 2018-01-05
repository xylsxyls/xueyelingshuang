#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include <QObject>

class DialogBase;
class QWidget;
/** 单一实例，用于统一管理窗口创建关闭，该类的对外接口，销毁所有窗口
DialogManager::instance().removeAll();
*/
class DialogManager : public QObject
{
	friend class DialogBase;
	friend class DialogShow;
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static DialogManager& instance();

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
	int32_t popAskDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& acceptText,
						 int32_t acceptDone,
						 const QString& ignoreText,
						 int32_t ignoreDone,
						 QWidget* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

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
	int32_t popTipDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& buttonText,
						 int32_t done,
						 QWidget* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** 弹出输入框
	@param [out] dialogId 窗口ID值
	@param [in] title 标题
	@param [in] editTip 提示内容
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in,out] editText 输入框默认文字
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@return 关闭窗口时给的返回值
	*/
	int32_t popInputDialog(int32_t& dialogId,
						   const QString& title,
						   const QString& editTip,
						   const QString& buttonText,
						   int32_t done,
						   QString& editText,
						   QWidget* parent = nullptr,
						   int32_t timeOut = -1,
						   bool isCountDownVisible = false);

	/** 弹出等待框
	@param [out] dialogId 窗口ID值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@return 关闭窗口时给的返回值
	*/
	int32_t popWaitDialog(int32_t& dialogId,
						  const QString& title,
						  const QString& tip,
						  QWidget* parent = nullptr,
						  int32_t timeOut = -1,
						  bool isCountDownVisible = false);

	/** 根据ID号销毁窗口
	@param [in] dialogId 窗口ID号
	*/
	void destroyDialog(int32_t dialogId);

	/** 销毁最后一个弹出的窗口
	*/
	void destroyLastDialog();

	/** 销毁所有窗口
	*/
	void destroyAll();

	/** 根据窗口指针在已有窗口中获取窗口ID
	*/
	int32_t DialogId(DialogBase* base);

protected:
	DialogManager();

private:
	/** 存储窗口指针
	@param [in] dialog 窗口指针
	@return 返回窗口对应ID号
	*/
	int32_t setDialog(DialogBase* dialog);

	/** 根据ID号销毁窗口
	@param [in] dialogId 窗口ID号
	*/
	void removeDialog(int32_t dialogId);

	/** 根据ID号销毁窗口
	@param [in] dialogId 窗口ID号
	*/
	void removeDialog(DialogBase* base);

	int32_t getId();

private:
	std::map<int32_t, DialogBase*> m_mapDialog;
	int32_t m_id;
};