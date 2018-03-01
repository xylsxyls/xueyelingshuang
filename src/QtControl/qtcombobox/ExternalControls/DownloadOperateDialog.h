#pragma once
#include "DialogShow.h"

class ProgressBar;
/** 下载框
*/
class DownloadOperateDialog : protected DialogShow
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
	static int32_t popDownloadOperateDialog(int32_t& dialogId,
											const QString& title,
											const QString& fileName,
											const QString& buttonText,
											int32_t done,
											QWindow* parent = nullptr);

	/** 设置比例，该函数支持多线程
	@param [in] persent 百分比
	*/
	void setRate(int32_t persent);

	/** 当下载出错时显示下载框的出错状态，该函数支持多线程
	*/
	void error();

public slots:
	/** 改变到下载失败状态
	*/
	void onChangeErrorStatus();

Q_SIGNALS:
	void changeErrorStatus();
	void rateChanged(int rate);
	void persentChanged(const QString& persent);
	void downloadComplete();

private:
	DownloadOperateDialog(const QString& title,
						  const QString& fileName,
						  const QString& buttonText,
						  int32_t done);

	~DownloadOperateDialog();

private slots:
	void downloadAccept();

private:
	Label* m_tip;
	Label* m_file;
	Label* m_persent;
	Label* m_downloadSlow;
	ProgressBar *m_progressBar;
	COriginalButton* m_hand;
	COriginalButton* m_change;
	COriginalButton* m_cancel;

private:
	static std::wstring s_fileString;
	static QString s_slowString;
	static QString s_handString;
};