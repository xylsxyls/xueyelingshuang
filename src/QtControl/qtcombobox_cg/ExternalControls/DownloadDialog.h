#pragma once
#include "DialogShow.h"

class ProgressBar;
/** ���ؿ�
*/
class DownloadDialog : public DialogShow
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
	static int32_t popDownloadDialog(int32_t& dialogId,
									 const QString& title,
									 const QString& fileName,
									 const QString& tip,
									 const QString& buttonText,
									 int32_t done,
									 QWindow* parent = nullptr,
									 int32_t timeOut = -1,
									 bool isCountDownVisible = false);

	/** ���ñ���
	@param [in] persent �ٷֱ�
	*/
	void setRate(int32_t persent);

	/** �������
	*/
	void complete();

	/** �����س���ʱ��ʾ���ش���򣬸ú���֧�ֶ��߳�
	*/
	void error();

public slots:
	/** ����ʧ��ʱ��ʾ����ʧ�ܿ򣬻���reject���ؿ��ٵ�������ʧ�ܿ�
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