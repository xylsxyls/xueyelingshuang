#pragma once
#include <QObject>
#include <stdint.h>
#include "DialogType.h"
#include "ManagerBase.h"

class PopDialogManager : public ManagerBase < PopDialogManager >
{
    Q_OBJECT
public:
    void popDialog(DialogType type, PopParamBase* param);

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
    //����������λѯ�ʿ�ʱ�����ź�
    void clickAdvert(int32_t dialogId, const QString& url);
};