#pragma once
#include <map>
#include <stdint.h>
#include <QString>
#include <QObject>
#include <QMutex>
#include "ControlsMacro.h"
#include "DialogType.h"
#include "../core/ManagerBase.h"

/** ��һʵ��������ͳһ�����ڴ����رգ�����Ϊ����ӿ�
*/
class ControlsAPI DialogManager :
    public QObject,
    public ManagerBase < DialogManager >
{
	Q_OBJECT
public:
    /** ���캯��
    */
    DialogManager();

public:
    /** ��������
    @param [in] type ��������
    @param [in,out] param ���ڲ����ṹ��ָ�룬�ṹ���п����д�������
    */
    void createDialog(DialogType type, ParamBase* param = nullptr);

	/** ����ID�Źرմ��ڣ��޶���Ч����
	@param [in] dialogId ����ID��
	*/
	void destroyDialog(int32_t dialogId);

	/** �ر����һ�������Ĵ��ڣ��޶���Ч����
	*/
	void destroyLastDialog();

	/** �������д��ڣ��޶���Ч����
	*/
	void destroyAll();

	/** ��ǰ��������
	@return ���ص�������
	*/
	int32_t dialogCount();

Q_SIGNALS:
    /** ת����̨����
    @param [in] taskId �û��Զ���ID
    */
    void changeToBack(qint32 taskId);

    /** ����
    @param [in] taskId �û��Զ���ID
    */
    void downloadAgain(qint32 taskId);

    /** ȡ������
    @param [in] taskId �û��Զ���ID
    */
    void cancelDownload(qint32 taskId);

    /** ʹ��������������
    @param [in] taskId �û��Զ���ID
    */
    void useOtherDownload(qint32 taskId);

    /** ���ص�ַ�ĸ��ư�ť
    @param [in] taskId �û��Զ���ID
    @param [in] addr ���ص�ַ
    */
    void copyDownloadAddr(qint32 taskId, const QString& addr);

    /** ����·���ĸ��ư�ť
    @param [in] taskId �û��Զ���ID
    @param [in] path ����·��
    */
    void copyPath(qint32 taskId, const QString& path);

    /** �����رյ�ʱ�����ź�
    @param [in] dialogId ����ID
    @param [in] userId �û��Զ���ID
    @param [in] type ��������
    @param [in] result ���ڷ���ֵ
    @param [in] userParam �û��Զ������
    */
    void popDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

    /** ֪ͨ��رյ�ʱ�����ź�
    @param [in] dialogId ����ID
    @param [in] userId �û��Զ���ID
    @param [in] type ��������
    @param [in] result ���ڷ���ֵ
    @param [in] userParam �û��Զ������
    */
    void notifyDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

    /** ��̬���رյ�ʱ�����ź�
    @param [in] dialogId ����ID
    @param [in] userId �û��Զ���ID
    @param [in] type ��������
    @param [in] result ���ڷ���ֵ
    @param [in] userParam �û��Զ������
    */
    void staticDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

private:
    void init();
    
private:
    bool m_init;
};