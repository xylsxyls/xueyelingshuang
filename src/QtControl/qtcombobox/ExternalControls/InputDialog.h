#pragma once
#include "DialogShow.h"

/** ���뵯����
*/
class InputDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ���������
	@param [out] dialogId ����IDֵ
	@param [in] title ����
	@param [in] editTip ��ʾ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in,out] editText �����Ĭ������
	@param [in] isPassword �Ƿ������Ϊ�����
	@param [in] maxLength ������󳤶ȣ�-1��ʾ������
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	static int32_t popInputDialog(int32_t& dialogId,
								  const QString& title,
								  const QString& editTip,
								  const QString& buttonText,
								  int32_t done,
								  QString& editText,
								  bool isPassword = false,
								  int32_t maxLength = -1,
								  QWindow* parent = nullptr,
								  int32_t timeOut = -1,
								  bool isCountDownVisible = false);
private:
	InputDialog(const QString& title,
				const QString& editTip,
				const QString& buttonText,
				int32_t done,
				QString& editText,
				bool isPassword = false,
				int32_t maxLength = -1);

	void done(int result);

private slots:
	void inputAccept();

private:
	Label* m_editTip;
	COriginalButton* m_accept;
	LineEdit* m_edit;
	QString& m_editText;
};