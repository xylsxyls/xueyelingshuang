#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

/** ���ش����
*/
class DownloadErrorDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ������ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] fileName �ļ���
	@param [in] title ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	static qint32 popDownloadErrorDialog(qint32& dialogId,
										  const QString& fileName,
										  const QString& title,
										  const QString& buttonText,
										  qint32 done,
										  QWindow* parent = nullptr,
										  qint32 timeOut = -1,
										  bool isCountDownVisible = false);

private:
	DownloadErrorDialog(const QString& fileName,
						const QString& title,
						const QString& buttonText,
						qint32 done);

private:
	Label* m_tip;
	Label* m_file;
	Label* m_error;
	COriginalButton* m_accept;

private:
	static std::wstring s_fileString;
	static QString s_tipString;
};