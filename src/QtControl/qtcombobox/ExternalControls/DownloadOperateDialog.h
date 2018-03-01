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
											const QString& buttonText,
											int32_t done,
											QWindow* parent = nullptr);

	/** ���ñ������ú���֧�ֶ��߳�
	@param [in] persent �ٷֱ�
	*/
	void setRate(int32_t persent);

	/** �����س���ʱ��ʾ���ؿ�ĳ���״̬���ú���֧�ֶ��߳�
	*/
	void error();

public slots:
	/** �ı䵽����ʧ��״̬
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