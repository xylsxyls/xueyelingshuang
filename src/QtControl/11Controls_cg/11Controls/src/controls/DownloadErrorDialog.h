#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

/** 下载错误框
*/
class DownloadErrorDialog : public DialogShow
{
	Q_OBJECT
public:
	/** 弹出提示框
	@param [out] dialogId 窗口ID值
	@param [in] fileName 文件名
	@param [in] title 标题
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	static int32_t popDownloadErrorDialog(int32_t& dialogId,
										  const QString& fileName,
										  const QString& title,
										  const QString& buttonText,
										  int32_t done,
										  QWindow* parent = nullptr,
										  int32_t timeOut = -1,
										  bool isCountDownVisible = false);

private:
	DownloadErrorDialog(const QString& fileName,
						const QString& title,
						const QString& buttonText,
						int32_t done);

private:
	Label* m_tip;
	Label* m_file;
	Label* m_error;
	COriginalButton* m_accept;

private:
	static std::wstring s_fileString;
	static QString s_tipString;
};