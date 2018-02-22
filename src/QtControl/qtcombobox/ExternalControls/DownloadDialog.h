#pragma once
#include "DialogShow.h"

class ProgressBar;
/** 提示弹出框
*/
class DownloadDialog : protected DialogShow
{
	Q_OBJECT
public:
	/** 弹出提示框
	@param [out] dialogId 窗口ID值
	@param [in] title 标题
	@param [in] fileName 文件名
	@param [in] tip 提示内容
	@param [in] buttonText 按钮内容
	@param [in] done 按钮按下后的返回值
	@param [in] parent 父窗口指针
	@param [in] timeOut 超时自动关闭，单位秒
	@param [in] isCountDownVisible 超时自动关闭提示是否可见
	@return 关闭窗口时给的返回值
	*/
	static int32_t popDownloadDialog(int32_t& dialogId,
									 const QString& title,
									 const QString& fileName,
									 const QString& tip,
									 const QString& buttonText,
									 int32_t done,
									 QWindow* parent = nullptr,
									 int32_t timeOut = -1,
									 bool isCountDownVisible = false);
private:
	DownloadDialog(const QString& title,
				   const QString& fileName,
				   const QString& tip,
				   const QString& buttonText,
				   int32_t done);

	void setRate(int32_t persent);

private slots:
	void downloadAccept();

private:
	Label* m_tip;
	Label* m_file;
	Label* m_persent;
	Label* m_downloadSlow;
	ProgressBar *progressBar;
	COriginalButton* m_hand;

private:
	static std::wstring s_fileString;
	static QString s_slowString;
	static QString s_handString;
};