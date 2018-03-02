#pragma once
#include "DialogShow.h"

class ProgressBar;
/** ���ؿ�
*/
class DownloadOperateDialog : protected DialogShow
{
	Q_OBJECT
public:
	/** ������ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] title ����
	@param [in] fileName �ļ���
	@param [in] tip ��ʾ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	static int32_t popDownloadOperateDialog(int32_t& dialogId,
											const QString& title,
											const QString& fileName,
											const QString& downloadAddr,
											const QString& path,
											QWindow* parent = nullptr);

	/** ���ñ������ú���֧�ֶ��߳�
	@param [in] persent �ٷֱ�
	*/
	void setRate(int32_t persent);

	/** �����س���ʱ��ʾ���ؿ�ĳ���״̬���ú���֧�ֶ��߳�
	*/
	void error();

	/** ��error�л�����̬���ú���֧�ֶ��߳�
	*/
	void normal();

public slots:
	/** �ı䵽����ʧ��״̬
	*/
	void onChangeErrorStatus();

	/** ��ʧ������״̬�л�����̬
	*/
	void onChangeNormalStatus();

Q_SIGNALS:
	void changeErrorStatus();
	void changeNormalStatus();
	void rateChanged(int rate);
	void persentChanged(const QString& persent);
	void downloadComplete();

private:
	DownloadOperateDialog(const QString& title,
						  const QString& fileName,
						  const QString& downloadAddr,
						  const QString& path);

	~DownloadOperateDialog();

private slots:
	void downloadAccept();

private:
	Label* m_tip;
	Label* m_file;

	Label* m_speed;
	Label* m_hasDownload;
	Label* m_nowTime;

	ProgressBar *m_progressBar;
	Label* m_persent;

	COriginalButton* m_change;
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

private:
	static std::wstring s_fileString;
	static QString s_slowString;
	static QString s_handString;
};