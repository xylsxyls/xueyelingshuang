#pragma once
#include "DialogShow.h"

class CWebViewEx;
/** �������λ��ѯ�ʵ�����
*/
class AdvertAskDialog : public DialogShow
{
	Q_OBJECT
public:
	/** ����ѯ�ʿ�
	@param [out] dialogId ����IDֵ
	@param [in] advertUrl ���λURL
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
	static int32_t popAdvertAskDialog(int32_t& dialogId,
									  const QString& advertUrl,
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
	AdvertAskDialog(const QString& advertUrl,
					const QString& title,
					const QString& tip,
					const QString& acceptText,
					int32_t acceptDone,
					const QString& ignoreText,
					int32_t ignoreDone);

Q_SIGNALS:
	void advertClicked(AdvertAskDialog* dialogPtr, const QString& url);

public slots:
	void advertAskAccept(QObject* tar, Qt::Key key);
	void onLinkClicked(const QUrl& url);

private:
	Label* m_tip;
	CWebViewEx* m_advert;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};