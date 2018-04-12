#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

class ProgressBar;
/** 下载框
*/
class ControlsAPI DownloadOperateDialog : public DialogShow
{
	Q_OBJECT
public:
	/** 弹出提示框
	@param [out] dialogId 窗口ID值
	@param [in] taskId 任务ID值
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
											int32_t taskId,
											const QString& title,
											const QString& fileName,
											const QString& downloadSpeed,
											const QString& hasDownloaded,
											const QString& downloadTime,
											int32_t rate,
											bool backEnable,
											const QString& downloadAddr,
											const QString& path,
											QWindow* parent = nullptr);

	/** 设置速度（支持多线程）
	@param [in] speed 速度
	*/
	void setDownloadSpeed(const QString& speed);

	/** 设置已下载量（支持多线程）
	@param [in] downloaded 已下载量
	*/
	void setDownloaded(const QString& download);

	/** 设置时间（支持多线程）
	@param [in] time 时间
	*/
	void setDownloadTime(const QString& time);

	/** 设置比例（支持多线程）
	@param [in] persent 百分比
	*/
	void setRate(int32_t persent);

	/** 设置编辑框内的下载地址（支持多线程）
	@param [in] addr 下载地址
	*/
	void setEditDownloadAddr(const QString& addr);

	/** 设置编辑框内的本地路径（支持多线程）
	@param [in] path 本地路径
	*/
	void setEditPath(const QString& path);

	/** 设置转到后台下载按钮是否可用（支持多线程）
	@param [in] enable 是否可用
	*/
	void setBackEnable(bool enable);

	/** 当下载出错时显示下载框的出错状态（支持多线程）
	*/
	void error();

	/** 从error切换到常态（支持多线程）
	*/
	void normal();

	/** 设置任务ID值
	@param [in] taskId 任务ID值
	*/
	void setTaskId(int32_t taskId);

	/** 任务ID值
	@return 返回任务ID值
	*/
	int32_t getTaskId();

public slots:
	/** 改变到下载失败状态
	*/
	void onChangeErrorStatus();

	/** 从失败下载状态切换到常态
	*/
	void onChangeNormalStatus();

Q_SIGNALS:
	void editDownloadAddr(const QString& addr);
	void editPath(const QString& addr);
	void backEnable(bool enable);
	void changeErrorStatus();
	void changeNormalStatus();
	void rateChanged(int rate);
	void persentChanged(const QString& persent);
	void downloadComplete();
	void downloadSpeed(const QString& speed);
	void downloaded(const QString& hasDownloaded);
	void downloadTime(const QString& time);
	void changeToBack();
	void downloadAgain();
	void cancelDownload();
	void useOtherDownload();
	void copyDownloadAddr(const QString& addr);
	void copyPath(const QString& path);

private:
	DownloadOperateDialog(int32_t taskId,
						  const QString& title,
						  const QString& fileName,
						  const QString& downloadSpeed,
						  const QString& hasDownloaded,
						  const QString& downloadTime,
						  int32_t rate,
						  bool backEnable,
						  const QString& downloadAddr,
						  const QString& path);

private:
	void setClipboardData(HWND hWnd, const std::string& str);

private slots:
	void downloadAccept(QObject* tar, Qt::Key key);
	void onBack();
	void onAgain();
	void onCancelDownload();
	void onUseOtherDownload();
	void onCopyDownloadAddr();
	void onCopyPath();

private:
	Label* m_tip;
	Label* m_file;

	Label* m_downloadSpeed;
	Label* m_downloaded;
	Label* m_downloadTime;

	ProgressBar *m_progressBar;
	Label* m_persent;

	COriginalButton* m_back;
	COriginalButton* m_again;
	COriginalButton* m_cancel;

	Label* m_downloadSlow;
	COriginalButton* m_hand;

	Label* m_downloadAddr;
	Label* m_path;
	LineEdit* m_downloadAddrEdit;
	LineEdit* m_pathEdit;
	COriginalButton* m_downloadButton;
	COriginalButton* m_pathButton;

	Label* m_error;

	int32_t m_taskId;

private:
	static std::wstring s_fileString;
	static QString s_slowString;
	static QString s_handString;
};