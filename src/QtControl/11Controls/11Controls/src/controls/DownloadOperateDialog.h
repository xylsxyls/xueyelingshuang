#pragma once
#include "ControlsMacro.h"
#include "PopDialog.h"

class ProgressBar;
class LineEdit;
/** 下载框
*/
class ControlsAPI DownloadOperateDialog : public PopDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
    DownloadOperateDialog();

public:
    /** 设置文件名
    @param [in] fileName 文件名
    */
    void setFileName(const QString& fileName);

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
	void setRate(qint32 persent);

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
	void downloadError();

	/** 从error切换到常态（支持多线程）
	*/
	void downloadNormal();

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

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private slots:
	void downloadAccept(QObject* tar, Qt::Key key);
	void onBack();
	void onAgain();
	void onCancelDownload();
	void onUseOtherDownload();
	void onCopyDownloadAddr();
	void onCopyPath();

private:
    void setClipboardData(HWND hWnd, const std::string& str);

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
	qint32 m_taskId;
};