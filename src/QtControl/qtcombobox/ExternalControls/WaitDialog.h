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
	@param [in] taskId �û������IDֵ
	@param [in] title ����
	@param [in] tip ��ʾ����
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	static int32_t popWaitDialog(int32_t& dialogId,
								 int32_t taskId,
								 const QString& title,
								 const QString& tip,
								 QWindow* parent = nullptr,
								 int32_t timeOut = -1,
								 bool isCountDownVisible = false);

	/** ��������IDֵ
	@param [in] taskId ����IDֵ
	*/
	void setTaskId(int32_t taskId);

	/** ����IDֵ
	@return ��������IDֵ
	*/
	int32_t getTaskId();

private:
	WaitDialog(const QString& title, const QString& tip);

private:
	Label* m_tip;
	int32_t m_taskId;
};