#pragma once
#include "DialogShow.h"

/** ��ʾ��ʾ��
*/
class AskShowDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ��ʾ��ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] title ����
	@param [in] tip ��ʾ����
	@param [in] acceptText ��ť����
	@param [in] acceptDone ȷ�ϰ�ť���º��ź���Ĳ���ֵ
	@param [in] ignoreText ��ť����
	@param [in] ignoreDone ȡ����ť���º��ź���Ĳ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	static void showAskDialog(int32_t& dialogId,
							  int32_t userType,
							  const QString& title,
							  const QString& tip,
							  const QString& acceptText,
							  int32_t acceptDone,
							  const QString& ignoreText,
							  int32_t ignoreDone,
							  int32_t timeOut = -1,
							  bool isCountDownVisible = false);
private:
	AskShowDialog(int32_t userType,
				  const QString& title,
				  const QString& tip,
				  const QString& acceptText,
				  int32_t acceptDone,
				  const QString& ignoreText,
				  int32_t ignoreDone);

private:
	Label* m_tip;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};