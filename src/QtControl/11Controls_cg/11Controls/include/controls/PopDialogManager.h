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
    void popDialog(DialogType type, ParamBase* param);

public:
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
    void popDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

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