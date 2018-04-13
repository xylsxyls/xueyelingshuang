#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

/** ѯ�ʵ�����
*/
class AskDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ����ѯ�ʿ�
	@param [out] dialogId ����IDֵ
	@param [in] title ����
	@param [in] tip ��ʾ����
	@param [in] acceptText ���ܰ�ť����
	@param [in] acceptDone ���ܰ�ť���º�ķ���ֵ
	@param [in] ignoreText �ܾ���ť����
	@param [in] ignoreDone �ܾ���ť���º�ķ���ֵ
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	static int32_t popAskDialog(int32_t& dialogId,
								const QString& title,
								const QString& tip,
								const QString& acceptText,
								int32_t acceptDone,
								const QString& ignoreText,
								int32_t ignoreDone,
								QWindow* parent = nullptr,
								int32_t timeOut = -1,
								bool isCountDownVisible = false);
private:
	AskDialog(const QString& title,
			  const QString& tip,
			  const QString& acceptText,
			  int32_t acceptDone,
			  const QString& ignoreText,
			  int32_t ignoreDone);

public slots:
	void askAccept(QObject* tar, Qt::Key key);

private:
	Label* m_tip;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};