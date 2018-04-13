#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"

class QWebView;
/** ��ʾ��ʾ��
*/
class LoginShowDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ��ʾ��¼��
	@param [out] dialogId ����IDֵ
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] title ����
	@param [in] greeting �к���
	@param [in] urlButtonText ���ఴť��������
	@param [in] linkUrl ���ఴť��ת����
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isUrlButtonVisible ���ఴť�Ƿ���ʾ
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	static void showLoginDialog(int32_t& dialogId,
								int32_t userType,
								const QString& title,
								const QString& greeting,
								const QString& tip,
								const QString& urlButtonText,
								const QString& linkUrl,
								int32_t timeOut = -1,
								bool isUrlButtonVisible = false,
								bool isCountDownVisible = false);
private:
	LoginShowDialog(int32_t userType,
					const QString& title,
					const QString& greeting,
					const QString& tip,
					const QString& urlButtonText,
					const QString& linkUrl,
					bool isUrlButtonVisible = false);

private slots:
	void onLinkClicked(const QUrl& url);
	void onMoreClicked();

private:
	Label* m_greeting;
	Label* m_horn;
	COriginalButton* m_more;
	QString m_moreLinkUrl;
	QWebView* m_tip;
};