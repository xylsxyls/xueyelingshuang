#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

class ProgressBar;
class LineEdit;
/** ���ؿ�
*/
class ControlsAPI DownloadOperateDialog : public PopDialog
{
	Q_OBJECT
public:
    DownloadOperateDialog();

public:
    /** �����ļ���
    @param [in] fileName �ļ���
    */
    void setFileName(const QString& fileName);

	/** �����ٶȣ�֧�ֶ��̣߳�
	@param [in] speed �ٶ�
	*/
	void setDownloadSpeed(const QString& speed);

	/** ��������������֧�ֶ��̣߳�
	@param [in] downloaded ��������
	*/
	void setDownloaded(const QString& download);

	/** ����ʱ�䣨֧�ֶ��̣߳�
	@param [in] time ʱ��
	*/
	void setDownloadTime(const QString& time);

	/** ���ñ�����֧�ֶ��̣߳�
	@param [in] persent �ٷֱ�
	*/
	void setRate(int32_t persent);

	/** ���ñ༭���ڵ����ص�ַ��֧�ֶ��̣߳�
	@param [in] addr ���ص�ַ
	*/
	void setEditDownloadAddr(const QString& addr);

	/** ���ñ༭���ڵı���·����֧�ֶ��̣߳�
	@param [in] path ����·��
	*/
	void setEditPath(const QString& path);

	/** ����ת����̨���ذ�ť�Ƿ���ã�֧�ֶ��̣߳�
	@param [in] enable �Ƿ����
	*/
	void setBackEnable(bool enable);

	/** �����س���ʱ��ʾ���ؿ�ĳ���״̬��֧�ֶ��̣߳�
	*/
	void error();

	/** ��error�л�����̬��֧�ֶ��̣߳�
	*/
	void normal();

	/** ��������IDֵ
	@param [in] taskId ����IDֵ
	*/
	void setTaskId(int32_t taskId);

	/** ����IDֵ
	@return ��������IDֵ
	*/
	int32_t getTaskId();

public slots:
	/** �ı䵽����ʧ��״̬
	*/
	void onChangeErrorStatus();

	/** ��ʧ������״̬�л�����̬
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