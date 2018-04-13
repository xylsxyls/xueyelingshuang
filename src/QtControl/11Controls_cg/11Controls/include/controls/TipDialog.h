#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

/** ��ʾ������
*/
class TipDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ������ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] title ����
	@param [in] tip ��ʾ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	static int32_t popTipDialog(int32_t& dialogId,
								const QString& title,
								const QString& tip,
								const QString& buttonText,
								int32_t done,
								QWindow* parent = nullptr,
								int32_t timeOut = -1,
								bool isCountDownVisible = false);
private:
	TipDialog(const QString& title,
			  const QString& tip,
			  const QString& buttonText,
			  int32_t done);

private slots:
	void tipAccept(QObject* tar, Qt::Key key);

private:
	Label* m_tip;
	COriginalButton* m_accept;
};