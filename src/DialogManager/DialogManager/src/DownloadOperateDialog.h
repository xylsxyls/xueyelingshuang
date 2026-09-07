#pragma once
#include "DialogManagerMacro.h"
#include "PopDialog.h"

class ProgressBar;
class LineEdit;
/** 下载操作弹框，展示下载进度、错误状态和手动复制下载信息入口
*/
class DialogManagerAPI DownloadOperateDialog : public PopDialog
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
	void setDownloaded(const QString& downloaded);

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

	/** 设置错误类型（支持多线程）
	@param [in] errorText 错误内容
	*/
	void setErrorType(const QString& errorText);

public slots:
	/** 改变到下载失败状态
	*/
	void onChangeErrorStatus();

	/** 从失败下载状态切换到常态
	*/
	void onChangeNormalStatus();

Q_SIGNALS:
    /** 请求更新下载地址编辑框
    @param [in] addr 下载地址
    */
	void editDownloadAddr(const QString& addr);

	/** 请求更新本地路径编辑框
	@param [in] addr 本地路径
	*/
	void editPath(const QString& addr);

	/** 请求更新转后台按钮可用状态
	@param [in] enable 是否可用
	*/
	void backEnable(bool enable);

	/** 请求切换到下载错误状态
	*/
	void changeErrorStatus();

	/** 请求切换到下载正常状态
	*/
	void changeNormalStatus();

	/** 请求更新错误提示
	@param [in] errorText 错误内容
	*/
	void errorType(const QString& errorText);

	/** 请求更新进度条数值
	@param [in] rate 百分比数值
	*/
	void rateChanged(int rate);

	/** 请求更新进度百分比文本
	@param [in] persent 百分比文本
	*/
	void persentChanged(const QString& persent);

	/** 通知下载完成
	*/
	void downloadComplete();

	/** 请求更新下载速度文本
	@param [in] speed 下载速度
	*/
	void downloadSpeed(const QString& speed);

	/** 请求更新已下载量文本
	@param [in] hasDownloaded 已下载量
	*/
	void downloaded(const QString& hasDownloaded);

	/** 请求更新时间文本
	@param [in] time 剩余时间或耗时文本
	*/
	void downloadTime(const QString& time);

	/** 通知用户选择转到后台下载
	*/
	void changeToBack();

	/** 通知用户选择重新下载
	*/
	void downloadAgain();

	/** 通知用户选择取消下载
	*/
	void cancelDownload();

	/** 通知用户选择其他下载渠道
	*/
	void useOtherDownload();

	/** 通知用户复制了下载地址
	@param [in] addr 下载地址
	*/
	void copyDownloadAddr(const QString& addr);

	/** 通知用户复制了本地路径
	@param [in] path 本地路径
	*/
	void copyPath(const QString& path);

protected:
    /** 调整下载弹框内所有控件布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private slots:
	/** 键盘确认时触发其他下载渠道按钮
	@param [in] tar 当前焦点对象
	@param [in] key 按下的按键
	*/
	void downloadAccept(QObject* tar, Qt::Key key);

	/** 处理转到后台下载按钮点击
	*/
	void onBack();

	/** 处理重试按钮点击
	*/
	void onAgain();

	/** 处理取消下载按钮点击
	*/
	void onCancelDownload();

	/** 处理其他下载渠道按钮点击
	*/
	void onUseOtherDownload();

	/** 处理复制下载地址按钮点击
	*/
	void onCopyDownloadAddr();

	/** 处理复制本地路径按钮点击
	*/
	void onCopyPath();

#ifdef _MSC_VER
private:
    /** 写入文本到Windows剪贴板
    @param [in] hWnd 所属窗口句柄
    @param [in] str 写入的ANSI文本
    */
    void setClipboardData(void* hWnd, const std::string& str);
#endif

private:
	// 下载状态提示文本
	Label* m_tip;

	// 文件名文本
	Label* m_file;

	// 下载速度文本
	Label* m_downloadSpeed;

	// 已下载量文本
	Label* m_downloaded;

	// 下载时间文本
	Label* m_downloadTime;

	// 下载进度条
	ProgressBar* m_progressBar;

	// 百分比文本
	Label* m_persent;

	// 转到后台下载按钮
	COriginalButton* m_back;

	// 重试按钮
	COriginalButton* m_again;

	// 取消下载按钮
	COriginalButton* m_cancel;

	// 下载失败后的手动下载提示
	Label* m_downloadSlow;

	// 其他下载渠道按钮
	COriginalButton* m_hand;

	// 下载地址标签
	Label* m_downloadAddr;

	// 本地路径标签
	Label* m_path;

	// 下载地址只读编辑框
	LineEdit* m_downloadAddrEdit;

	// 本地路径只读编辑框
	LineEdit* m_pathEdit;

	// 复制下载地址按钮
	COriginalButton* m_downloadButton;

	// 复制本地路径按钮
	COriginalButton* m_pathButton;

	// 错误提示文本
	Label* m_error;

	// 预留任务ID
	qint32 m_taskId;
};