#pragma once
#include "DialogShow.h"

class ProgressBar;
/** 下载框
*/
class DownloadDialog : public DialogShow
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

	/** 设置比例
	@param [in] persent 百分比
	*/
	void setRate(int32_t persent);

	/** 下载完成
	*/
	void complete();

	/** 当下载出错时显示下载错误框，该函数支持多线程
	*/
	void error();

public slots:
	/** 下载失败时显示下载失败框，会先reject下载框再弹出下载失败框
	*/
	void showDownloadErrorDialog();

Q_SIGNALS:
	void showError();
	void rateChanged(int rate);
	void persentChanged(const QString& persent);
	void downloadComplete();

private:
	DownloadDialog(const QString& title,
				   const QString& fileName,
				   const QString& tip,
				   const QString& buttonText,
				   int32_t done);

	~DownloadDialog();

private slots:
	void downloadAccept(QObject* tar, Qt::Key key);

private:
	Label* m_tip;
	Label* m_file;
	Label* m_persent;
	Label* m_downloadSlow;
	ProgressBar *m_progressBar;
	COriginalButton* m_hand;

private:
	static std::wstring s_fileString;
	static QString s_slowString;
	static QString s_handString;
};