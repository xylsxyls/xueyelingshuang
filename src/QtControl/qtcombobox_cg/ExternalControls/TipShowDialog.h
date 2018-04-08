#pragma once
#include "DialogShow.h"

/** ��ʾ��ʾ��
*/
class TipShowDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ��ʾ��ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] title ����
	@param [in] tip ��ʾ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º��ź���Ĳ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	static void showTipDialog(int32_t& dialogId,
							  int32_t userType,
							  const QString& title,
							  const QString& tip,
							  const QString& buttonText,
							  int32_t done,
							  int32_t timeOut = -1,
							  bool isCountDownVisible = false);

private:
	TipShowDialog(int32_t userType,
				  const QString& title,
				  const QString& tip,
				  const QString& buttonText,
				  int32_t done);

private:
	Label* m_tip;
	COriginalButton* m_accept;
};