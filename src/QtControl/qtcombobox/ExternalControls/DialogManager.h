#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include <QObject>
#include <QMutex>

enum DialogButton
{
	/** ȷ�ϰ�ť
	*/
	ACCEPT_BUTTON = 1,

	/** ȡ����ť
	*/
	IGNORE_BUTTON = 2,

	/** ȫ������ר�÷���ֵ
	*/
	DESTROY_ALL = -1
};

enum
{
	/** ѯ�ʿ�
	*/
	ASK_DIALOG,

	/** �����
	*/
	INPUT_DIALOG,

	/** ��ʾ��
	*/
	TIP_DIALOG,

	/** �ȴ���
	*/
	WAIT_DIALOG,

	/** ���ؿ��ϰ棩
	*/
	DOWNLOAD_DIALOG,

	/** ���ش�����ϰ棩
	*/
	DOWNLOAD_ERROR_DIALOG,

	/** ���ؿ��°棩
	*/
	DOWNLOAD_OPERATE_DIALOG
};

class DialogBase;
class QWindow;
class DownloadOperateDialog;
class AccountManagerDialog;
/** ��һʵ��������ͳһ�����ڴ����رգ�����Ķ���ӿڣ��������д���
DialogManager::instance().removeAll();
*/
class DialogManager : public QObject
{
	Q_OBJECT
	friend class DialogBase;
	friend class DialogShow;
public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static DialogManager& instance();
	
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
	int32_t popTipDialog(int32_t& dialogId,
						 const QString& title,
						 const QString& tip,
						 const QString& buttonText,
						 int32_t done,
						 QWindow* parent = nullptr,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** ���������
	@param [out] dialogId ����IDֵ
	@param [in] title ����
	@param [in] editTip ��ʾ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in,out] editText �����Ĭ������
	@param [in] maxLength ������󳤶ȣ�-1��ʾ������
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popInputDialog(int32_t& dialogId,
						   const QString& title,
						   const QString& editTip,
						   const QString& buttonText,
						   int32_t done,
						   QString& editText,
						   int32_t maxLength = -1,
						   QWindow* parent = nullptr,
						   int32_t timeOut = -1,
						   bool isCountDownVisible = false);

	/** �����ȴ���
	@param [out] dialogId ����IDֵ
	@param [in] title ����
	@param [in] tip ��ʾ����
	@param [in] parent ������ָ��
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popWaitDialog(int32_t& dialogId,
						  const QString& title,
						  const QString& tip,
						  QWindow* parent = nullptr,
						  int32_t timeOut = -1,
						  bool isCountDownVisible = false);

	/** ����ѯ�ʿ�
	@param [out] dialogId ����IDֵ
	@param [in] tip ��ʾ����
	@param [in] title ����
	@param [in] parent ������ָ��
	@param [in] acceptText ���ܰ�ť����
	@param [in] ignoreText �ܾ���ť����
	@param [in] acceptDone ���ܰ�ť���º�ķ���ֵ
	@param [in] ignoreDone �ܾ���ť���º�ķ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popAskDialog(int32_t& dialogId,
						 const QString& tip,
						 const QString& title = QString::fromStdWString(L"11��սƽ̨"),
						 QWindow* parent = nullptr,
						 const QString& acceptText = QString::fromStdWString(L"ȷ��"),
						 const QString& ignoreText = QString::fromStdWString(L"ȡ��"),
						 int32_t acceptDone = ACCEPT_BUTTON,
						 int32_t ignoreDone = IGNORE_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** ����ѯ�ʿ�
	@param [out] dialogId ����IDֵ
	@param [in] tip ��ʾ����
	@param [in] parent ������ָ��
	@param [in] title ����
	@param [in] acceptText ���ܰ�ť����
	@param [in] ignoreText �ܾ���ť����
	@param [in] acceptDone ���ܰ�ť���º�ķ���ֵ
	@param [in] ignoreDone �ܾ���ť���º�ķ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popAskDialog(int32_t& dialogId,
						 const QString& tip,
						 QWindow* parent,
						 const QString& title = QString::fromStdWString(L"11��սƽ̨"),
						 const QString& acceptText = QString::fromStdWString(L"ȷ��"),
						 const QString& ignoreText = QString::fromStdWString(L"ȡ��"),
						 int32_t acceptDone = ACCEPT_BUTTON,
						 int32_t ignoreDone = IGNORE_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** ������ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] tip ��ʾ����
	@param [in] title ����
	@param [in] parent ������ָ��
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popTipDialog(int32_t& dialogId,
						 const QString& tip,
						 const QString& title = QString::fromStdWString(L"11��սƽ̨"),
						 QWindow* parent = nullptr,
						 const QString& buttonText = QString::fromStdWString(L"ȷ��"),
						 int32_t done = ACCEPT_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** ������ʾ��
	@param [out] dialogId ����IDֵ
	@param [in] tip ��ʾ����
	@param [in] parent ������ָ��
	@param [in] title ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popTipDialog(int32_t& dialogId,
						 const QString& tip,
						 QWindow* parent,
						 const QString& title = QString::fromStdWString(L"11��սƽ̨"),
						 const QString& buttonText = QString::fromStdWString(L"ȷ��"),
						 int32_t done = ACCEPT_BUTTON,
						 int32_t timeOut = -1,
						 bool isCountDownVisible = false);

	/** ���������
	@param [out] dialogId ����IDֵ
	@param [in,out] editText �����Ĭ������
	@param [in] editTip ��ʾ����
	@param [in] isPassword �Ƿ������Ϊ�����
	@param [in] title ����
	@param [in] parent ������ָ��
	@param [in] maxLength ������󳤶ȣ�-1��ʾ������
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popInputDialog(int32_t& dialogId,
						   QString& editText,
						   const QString& editTip,
						   bool isPassword = false,
						   const QString& title = QString::fromStdWString(L"11��սƽ̨"),
						   QWindow* parent = nullptr,
						   int32_t maxLength = -1,
						   const QString& buttonText = QString::fromStdWString(L"ȷ��"),
						   int32_t done = ACCEPT_BUTTON,
						   int32_t timeOut = -1,
						   bool isCountDownVisible = false);

	/** �������ؿ��ϰ棩
	@param [out] dialogId ����IDֵ
	@param [in] fileName �ļ���
	@param [in] tip ��ʾ����
	@param [in] parent ������ָ��
	@param [in] title ����
	@param [in] buttonText ��ť����
	@param [in] done ��ť���º�ķ���ֵ
	@param [in] timeOut ��ʱ�Զ��رգ���λ��
	@param [in] isCountDownVisible ��ʱ�Զ��ر���ʾ�Ƿ�ɼ�
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popDownloadDialog(int32_t& dialogId,
							  const QString& fileName,
							  const QString& tip,
							  QWindow* parent = nullptr,
							  const QString& title = QString::fromStdWString(L"11��սƽ̨"),
							  const QString& buttonText = QString::fromStdWString(L"ȷ��"),
							  int32_t done = ACCEPT_BUTTON,
							  int32_t timeOut = -1,
							  bool isCountDownVisible = false);

	/** �����˺Ź���ҳ��
	*/
	void popAccountManagerDialog(QWindow* parent = nullptr);

	/** ��ȡ�����˺Ź���ҳ��Ĵ���ָ��
	@return ���ص����˺Ź���ҳ��Ĵ���ָ��
	*/
	AccountManagerDialog* accountMannagerDialogPtr();

	/** �����˺Ź���ҳ�洰��
	*/
	void destroyAccountManagerDialog();

	/** ���ñ������ϰ棩
	@param [in] dialogId ����IDֵ
	@param [in] persent �ٷֱ�
	*/
	void setDownloadRate(int32_t dialogId, int32_t persent);

	/** ������ɣ��ϰ棩
	@param [in] dialogId ����IDֵ
	*/
	void downloadComplete(int32_t dialogId);

	/** ����ǰ���ؿ����ٲ���������ʧ�ܿ��ϰ棩
	@param [out] dialogId ����IDֵ
	*/
	void downloadError(int32_t dialogId);

	/** �������ؿ��°棩
	@param [out] dialogId ����IDֵ
	@param [in] taskId ����IDֵ
	@param [in] title ����
	@param [in] fileName �ļ���
	@param [in] downloadAddr ���ص�ַ
	@param [in] path ���ص�ַ
	@param [in] parent ������ָ��
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t popDownloadOperateDialog(int32_t& dialogId,
									 QWindow* parent,
									 int32_t taskId,
									 const QString& title,
									 const QString& fileName,
									 const QString& downloadSpeed,
									 const QString& hasDownloaded,
									 const QString& downloadTime,
									 int32_t rate,
									 bool backEnable,
									 const QString& downloadAddr = "",
									 const QString& path = "");

	/** ��ȡ���ؿ�ָ�루�°棩
	@param [in] dialogId ����ID
	@return �������ؿ�ָ��
	*/
	DownloadOperateDialog* downloadOperatePtr(int32_t dialogId);

	/** ��ȡ���ؿ�ָ�루�°棩
	@param [in] taskId ����ID
	@return �������ؿ�ָ��
	*/
	DownloadOperateDialog* downloadOperateTaskPtr(int32_t taskId);

	/** ����ID�Źرմ��ڣ��ж���Ч����
	@param [in] dialogId ����ID��
	*/
	void closeDialog(int32_t dialogId);

	/** �ر����һ�������Ĵ��ڣ��ж���Ч����
	*/
	void closeLastDialog();

	/** �������д��ڣ��޶���Ч����
	*/
	void destroyAll();

	/** ��ǰ��������
	@return ���ص�������
	*/
	int32_t dialogCount();

	/** ���ݴ���ָ�������д����л�ȡ����ID
	@param [in] base ����ָ��
	@return ���ش���ID
	*/
	int32_t dialogId(DialogShow* base);

	/** �������ؿ��ָ���ҵ�����ID
	@param [in] base ����ָ��
	@return ��������ID
	*/
	int32_t taskId(DialogShow* base);

Q_SIGNALS:
	//����6���ºŸ��°����ؿ���
	//ת����̨����
	void changeToBack(qint32 taskId);
	//����
	void downloadAgain(qint32 taskId);
	//ȡ������
	void cancelDownload(qint32 taskId);
	//ʹ��������������
	void useOtherDownload(qint32 taskId);
	//���ص�ַ�ĸ��ư�ť
	void copyDownloadAddr(qint32 taskId, const QString& addr);
	//����·���ĸ��ư�ť
	void copyPath(qint32 taskId, const QString& path);

protected:
	DialogManager();

	/** ����ID�����ٴ���
	@param [in] dialogId ����ID��
	*/
	void removeDialog(int32_t dialogId);

	/** ���ݴ���ID��ȡ����ָ��
	@param [in] dialogId ����ID��
	@return ���ش���ָ��
	*/
	DialogBase* dialogPtr(int32_t dialogId);

private:
	/** �洢����ָ��
	@param [in] dialog ����ָ��
	@return ���ش��ڶ�ӦID��
	*/
	int32_t setDialog(DialogBase* dialog);

	/** ����ID�����ٴ���
	@param [in] dialogId ����ID��
	*/
	void removeDialog(DialogBase* base);

	/** ��ȡ�´���ID
	@return �����´���ID
	*/
	int32_t getId();

	bool mapIsEmpty();
	DialogBase* mapFind(int32_t dialogId);
	int32_t mapFind(DialogBase* base);
	int32_t mapFindTaskId(DialogBase* base);
	void mapErase(DialogBase* base);
	void mapErase(int32_t dialogId);
	void mapInsert(int32_t dialogId, DialogBase* base);

	void onDestroyAccountManagerDialog(QObject* obj);

private:
	std::map<int32_t, DialogBase*> m_mapDialog;
	int32_t m_id;
	//�����������ƶ��̲߳������ؿ�
	QMutex m_mutex;

	AccountManagerDialog* m_accountManagerDialog;
};