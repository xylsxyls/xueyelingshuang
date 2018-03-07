#pragma once
#include "DialogShow.h"

/** �ȴ�������
*/
class WaitDialog : public DialogShow
{
	Q_OBJECT
public:
	/** �����ȴ���
	@param [out] dialogId ����IDֵ
	@param [in] title ����
	@param [in] tip ��ʾ����
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	static int32_t popWaitDialog(int32_t& dialogId,
								 const QString& title,
								 const QString& tip,
								 QWindow* parent = nullptr,
								 int32_t timeOut = -1,
								 bool isCountDownVisible = false);
private:
	WaitDialog(const QString& title, const QString& tip);

private:
	Label* m_tip;
};