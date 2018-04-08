#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include "DialogManager.h"

class DialogBase;
class QWindow;
/** ��һʵ��������ͳһ�����ڴ����رգ�����Ķ���ӿڣ��������д���
DialogManager::instance().removeAll();
*/
class NotifyDialogManager : public DialogManager
{
	Q_OBJECT
	friend class DialogBase;
	friend class DialogShow;
public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static NotifyDialogManager& instance();

	/** ��ʾ��ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] tip ��ʾ����
	@param [in] title ����
	@param [in] done ��ť���º��ź���Ĳ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	void showTipDialog(int32_t& dialogId,
					   int32_t userType,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11ƽ̨"),
					   int32_t done = ACCEPT_BUTTON,
					   const QString& buttonText = QString::fromStdWString(L"֪����"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

	/** ��ʾѯ�ʿ�
	@param [out] dialogId ����IDֵ
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] tip ��ʾ����
	@param [in] title ����
	@param [in] acceptDone ȷ�ϰ�ť���º��ź���Ĳ���ֵ
	@param [in] ignoreDone ȡ����ť���º��ź���Ĳ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	void showAskDialog(int32_t& dialogId,
					   int32_t userType,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11ƽ̨"),
					   int32_t acceptDone = ACCEPT_BUTTON,
					   int32_t ignoreDone = IGNORE_BUTTON,
					   const QString& acceptText = QString::fromStdWString(L"ͬ��"),
					   const QString& ignoreText = QString::fromStdWString(L"�ܾ�"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

	/** ��ʾ��¼��
	@param [out] dialogId ����IDֵ
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] tip ��ʾ����
	@param [in] linkUrl �˽�������ӵ�ַ
	@param [in] title ����
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isUrlButtonVisible �˽���ఴť�Ƿ���ʾ
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	void showLoginDialog(int32_t& dialogId,
						 int32_t userType,
						 const QString& tip,
						 const QString& linkUrl,
						 const QString& title = QString::fromStdWString(L"11ƽ̨"),
						 int32_t timeOut = 30,
						 bool isUrlButtonVisible = false,
						 bool isCountDownVisible = false);

	/** ��ʾ��ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] tip ��ʾ����
	@param [in] title ����
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] done ��ť���º��ź���Ĳ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	void showTipDialog(int32_t& dialogId,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11ƽ̨"),
					   int32_t userType = 0,
					   int32_t done = ACCEPT_BUTTON,
					   const QString& buttonText = QString::fromStdWString(L"֪����"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

	/** ��ʾѯ�ʿ�
	@param [out] dialogId ����IDֵ
	@param [in] tip ��ʾ����
	@param [in] title ����
	@param [in] userType �û��Զ���ֵ�������ڹر�ʱ�ᷢ���źţ��ź��к������ֵ
	@param [in] acceptDone ȷ�ϰ�ť���º��ź���Ĳ���ֵ
	@param [in] ignoreDone ȡ����ť���º��ź���Ĳ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ����ʱ�Ƿ���ʾ
	*/
	void showAskDialog(int32_t& dialogId,
					   const QString& tip,
					   const QString& title = QString::fromStdWString(L"11ƽ̨"),
					   int32_t userType = 0,
					   int32_t acceptDone = ACCEPT_BUTTON,
					   int32_t ignoreDone = IGNORE_BUTTON,
					   const QString& acceptText = QString::fromStdWString(L"ͬ��"),
					   const QString& ignoreText = QString::fromStdWString(L"�ܾ�"),
					   int32_t timeOut = 30,
					   bool isCountDownVisible = false);

Q_SIGNALS:
	/** ��֪ͨ�򴰿ڹر�ʱ�����źţ��ڹرն���ִ��֮ǰ�����ź�
	@param [in] dialogId ����ID
	@param [in] result ���ڹر�ʱ�����ķ���ֵ
	@param [in] userType �û��Զ���ֵ
	*/
	void dialogDone(int32_t dialogId, int32_t result, int32_t userType);

public slots:
	void onDialogDone(int32_t dialogId, int32_t result, int32_t userType);

private:
	int32_t popAskDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& acceptText,
						 int32_t acceptDone,
						 const QString& ignoreText,
						 int32_t ignoreDone,
						 QWindow* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	int32_t popTipDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& buttonText,
						 int32_t done,
						 QWindow* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	int32_t popInputDialog(int32_t& dialogId,
						   const QString& title,
						   const QString& editTip,
						   const QString& buttonText,
						   int32_t done,
						   QString& editText,
						   QWindow* parent = nullptr,
						   int32_t timeOut = -1,
						   bool isCountDownVisible = false);

	int32_t popWaitDialog(int32_t& dialogId,
						  const QString& title,
						  const QString& tip,
						  QWindow* parent = nullptr,
						  int32_t timeOut = -1,
						  bool isCountDownVisible = false);
};