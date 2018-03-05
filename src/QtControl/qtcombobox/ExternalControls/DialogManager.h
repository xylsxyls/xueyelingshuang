#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include <QObject>
#include <QMutex>

enum DialogButton
{
	/** 确认按钮
	*/
	ACCEPT_BUTTON = 1,

	/** 取消按钮
	*/
	IGNORE_BUTTON = 2,

	/** 全部销毁专用返回值
	*/
	DESTROY_ALL = -1
};

enum
{
	/** 询问框
	*/
	ASK_DIALOG,

	/** 输出框
	*/
	INPUT_DIALOG,

	/** 提示框
	*/
	TIP_DIALOG,

	/** 等待框
	*/
	WAIT_DIALOG,

	/** 下载框（老版）
	*/
	DOWNLOAD_DIALOG,

	/** 下载错误框（老版）
	*/
	DOWNLOAD_ERROR_DIALOG,

	/** 下载框（新版）
	*/
	DOWNLOAD_OPERATE_DIALOG
};

class DialogBase;
class QWindow;
class DownloadOperateDialog;
/** 单一实例，用于统一管理窗口创建关闭，该类的对外接口，销毁所有窗口
DialogManager::instance().removeAll();
*/
class DialogManager : public QObject
{
	Q_OBJECT
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
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
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
	int32_t popTipDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& buttonText,
						 int32_t done,
						 QWindow* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** 弹出输入框
	@param [out] dialogId 窗口ID值
	@param [in] title 标题
	@param [in] editTip 提示内容
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in,out] editText 输入框默认文字
	@param [in] maxLength 输入最大长度，-1表示不限制
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popInputDialog(int32_t& dialogId,
						   const QString& title,
						   const QString& editTip,
						   const QString& buttonText,
						   int32_t done,
						   QString& editText,
						   int32_t maxLength = -1,
						   QWindow* parent = nullptr,
						   int32_t timeOut = -1,
						   bool isCountDownVisible = false);

	/** 弹出等待框
	@param [out] dialogId 窗口ID值
	@param [in] title 标题
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popWaitDialog(int32_t& dialogId,
						  const QString& title,
						  const QString& tip,
						  QWindow* parent = nullptr,
						  int32_t timeOut = -1,
						  bool isCountDownVisible = false);

	/** 弹出询问框
	@param [out] dialogId 窗口ID值
	@param [in] tip 提示内容
	@param [in] title 标题
	@param [in] parent 父窗口指针
	@param [in] acceptText 接受按钮内容
	@param [in] ignoreText 拒绝按钮内容
	@param [in] acceptDone 接受按钮按下后的返回值
	@param [in] ignoreDone 拒绝按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popAskDialog(int32_t& dialogId,
						 const QString& tip,
						 const QString& title = QString::fromStdWString(L"11对战平台"),
						 QWindow* parent = nullptr,
						 const QString& acceptText = QString::fromStdWString(L"确定"),
						 const QString& ignoreText = QString::fromStdWString(L"取消"),
						 int32_t acceptDone = ACCEPT_BUTTON,
						 int32_t ignoreDone = IGNORE_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** 弹出询问框
	@param [out] dialogId 窗口ID值
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] title 标题
	@param [in] acceptText 接受按钮内容
	@param [in] ignoreText 拒绝按钮内容
	@param [in] acceptDone 接受按钮按下后的返回值
	@param [in] ignoreDone 拒绝按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popAskDialog(int32_t& dialogId,
						 const QString& tip,
						 QWindow* parent,
						 const QString& title = QString::fromStdWString(L"11对战平台"),
						 const QString& acceptText = QString::fromStdWString(L"确定"),
						 const QString& ignoreText = QString::fromStdWString(L"取消"),
						 int32_t acceptDone = ACCEPT_BUTTON,
						 int32_t ignoreDone = IGNORE_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** 弹出提示框
	@param [out] dialogId 窗口ID值
	@param [in] tip 提示内容
	@param [in] title 标题
	@param [in] parent 父窗口指针
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popTipDialog(int32_t& dialogId,
						 const QString& tip,
						 const QString& title = QString::fromStdWString(L"11对战平台"),
						 QWindow* parent = nullptr,
						 const QString& buttonText = QString::fromStdWString(L"确定"),
						 int32_t done = ACCEPT_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** 弹出提示框
	@param [out] dialogId 窗口ID值
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] title 标题
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popTipDialog(int32_t& dialogId,
						 const QString& tip,
						 QWindow* parent,
						 const QString& title = QString::fromStdWString(L"11对战平台"),
						 const QString& buttonText = QString::fromStdWString(L"确定"),
						 int32_t done = ACCEPT_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** 弹出输入框
	@param [out] dialogId 窗口ID值
	@param [in,out] editText 输入框默认文字
	@param [in] editTip 提示内容
	@param [in] isPassword 是否将输入框换为密码框
	@param [in] title 标题
	@param [in] parent 父窗口指针
	@param [in] maxLength 输入最大长度，-1表示不限制
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popInputDialog(int32_t& dialogId,
						   QString& editText,
						   const QString& editTip,
						   bool isPassword = false,
						   const QString& title = QString::fromStdWString(L"11对战平台"),
						   QWindow* parent = nullptr,
						   int32_t maxLength = -1,
						   const QString& buttonText = QString::fromStdWString(L"确定"),
						   int32_t done = ACCEPT_BUTTON,
						   int32_t timeOut = -1,
						   bool isCountDownVisible = false);

	/** 弹出下载框（老版）
	@param [out] dialogId 窗口ID值
	@param [in] fileName 文件名
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] title 标题
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popDownloadDialog(int32_t& dialogId,
							  const QString& fileName,
							  const QString& tip,
							  QWindow* parent = nullptr,
							  const QString& title = QString::fromStdWString(L"11对战平台"),
							  const QString& buttonText = QString::fromStdWString(L"确定"),
							  int32_t done = ACCEPT_BUTTON,
							  int32_t timeOut = -1,
							  bool isCountDownVisible = false);

	/** 设置比例（老版）
	@param [in] dialogId 窗口ID值
	@param [in] persent 百分比
	*/
	void setDownloadRate(int32_t dialogId, int32_t persent);

	/** 下载完成（老版）
	@param [in] dialogId 窗口ID值
	*/
	void downloadComplete(int32_t dialogId);

	/** 将当前下载框销毁并弹窗下载失败框（老版）
	@param [out] dialogId 窗口ID值
	*/
	void downloadError(int32_t dialogId);

	/** 弹出下载框（新版）
	@param [out] dialogId 窗口ID值
	@param [in] fileName 文件名
	@param [in] tip 提示内容
	@param [in] parent 父窗口指针
	@param [in] title 标题
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	int32_t popDownloadOperateDialog(int32_t& dialogId,
									 int32_t taskId,
									 const QString& title,
									 const QString& fileName,
									 const QString& downloadAddr = "",
									 const QString& path = "",
									 QWindow* parent = nullptr);

	/** 获取下载框指针（新版）
	@param [in] dialogId 窗口ID
	*/
	DownloadOperateDialog* downloadOperatePtr(int32_t dialogId);

	/** 获取下载框指针（新版）
	@param [in] taskId 任务ID
	*/
	DownloadOperateDialog* downloadOperateTaskPtr(int32_t taskId);

	/** 根据ID号关闭窗口（有动画效果）
	@param [in] dialogId 窗口ID号
	*/
	void closeDialog(int32_t dialogId);

	/** 关闭最后一个弹出的窗口（有动画效果）
	*/
	void closeLastDialog();

	/** 销毁所有窗口（无动画效果）
	*/
	void destroyAll();

	/** 当前弹框数量
	@return 返回弹框数量
	*/
	int32_t dialogCount();

	/** 根据窗口指针在已有窗口中获取窗口ID
	@param [in] base 窗口指针
	@return 返回窗口ID
	*/
	int32_t DialogId(DialogBase* base);

Q_SIGNALS:
	//以下6个信号是给新版下载框用的
	void changeToBack(int32_t dialogId);
	void downloadAgain(int32_t dialogId);
	void cancelDownload(int32_t dialogId);
	void useOtherDownload(int32_t dialogId);
	void copyDownloadAddr(int32_t dialogId);
	void copyPath(int32_t dialogId);

protected:
	DialogManager();

	/** 根据ID号销毁窗口
	@param [in] dialogId 窗口ID号
	*/
	void removeDialog(int32_t dialogId);

	/** 根据窗口ID获取窗口指针
	@param [in] dialogId 窗口ID号
	@return 返回窗口指针
	*/
	DialogBase* dialogPtr(int32_t dialogId);

private:
	/** 存储窗口指针
	@param [in] dialog 窗口指针
	@return 返回窗口对应ID号
	*/
	int32_t setDialog(DialogBase* dialog);

	/** 根据ID号销毁窗口
	@param [in] dialogId 窗口ID号
	*/
	void removeDialog(DialogBase* base);

	/** 获取新窗口ID
	@return 返回新窗口ID
	*/
	int32_t getId();

private:
	std::map<int32_t, DialogBase*> m_mapDialog;
	int32_t m_id;
	//该锁用于限制多线程操作下载框
	QMutex m_mutex;
};