#pragma once
#include <QObject>
#include <stdint.h>
#include "DialogType.h"
#include "../core/ManagerBase.h"

/** �����Ĺ�����
*/
class PopDialogManager :
    public QObject,
    public ManagerBase < PopDialogManager >
{
    Q_OBJECT
public:
    /** ��������
    @param [in] type ��������
    @param [in] param ��������
    */
    void popDialog(DialogType type, ParamBase* param);

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
    //�����رյ�ʱ�����ź�
    void popDialogDone(const DialogDoneSignalParam& param);

private slots:
    void onClosedSignal(int result);
    void onChangeToBack();
    void onDownloadAgain();
    void onCancelDownload();
    void onUseOtherDownload();
    void onCopyDownloadAddr(const QString& addr);
    void onCopyPath(const QString& path);

private:
    int32_t userId();
};