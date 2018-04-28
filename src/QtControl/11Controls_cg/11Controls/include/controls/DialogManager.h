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

    //���Ƶ�������ؽӿ�
public:
    /** �����ٶȣ�֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    @param [in] speed �ٶ�
    */
    void setDownloadSpeed(int32_t userId, const QString& speed);

    /** ��������������֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    @param [in] downloaded ��������
    */
    void setDownloaded(int32_t userId, const QString& download);

    /** ����ʱ�䣨֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    @param [in] time ʱ��
    */
    void setDownloadTime(int32_t userId, const QString& time);

    /** ���ñ�����֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    @param [in] persent �ٷֱ�
    */
    void setRate(int32_t userId, int32_t persent);

    /** ���ñ༭���ڵ����ص�ַ��֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    @param [in] addr ���ص�ַ
    */
    void setEditDownloadAddr(int32_t userId, const QString& addr);

    /** ���ñ༭���ڵı���·����֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    @param [in] path ����·��
    */
    void setEditPath(int32_t userId, const QString& path);

    /** ����ת����̨���ذ�ť�Ƿ���ã�֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    @param [in] enable �Ƿ����
    */
    void setBackEnable(int32_t userId, bool enable);

    /** �����س���ʱ��ʾ���ؿ�ĳ���״̬��֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    */
    void error(int32_t userId);

    /** ��error�л�����̬��֧�ֶ��̣߳�
    @param [in] userId �û��Զ���ID
    */
    void normal(int32_t userId);

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
    void popDialogDone(const DialogDoneSignalParam& param);

    /** ֪ͨ��رյ�ʱ�����ź�
    @param [in] dialogId ����ID
    @param [in] userId �û��Զ���ID
    @param [in] type ��������
    @param [in] result ���ڷ���ֵ
    @param [in] userParam �û��Զ������
    */
    void notifyDialogDone(const DialogDoneSignalParam& param);

    /** ��̬���رյ�ʱ�����ź�
    @param [in] dialogId ����ID
    @param [in] userId �û��Զ���ID
    @param [in] type ��������
    @param [in] result ���ڷ���ֵ
    @param [in] userParam �û��Զ������
    */
    void staticDialogDone(const DialogDoneSignalParam& param);

private:
    void init();
    
private:
    bool m_init;
};