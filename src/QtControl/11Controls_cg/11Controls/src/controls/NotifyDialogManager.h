#pragma once
#include <QObject>
#include "DialogType.h"
#include "../core/ManagerBase.h"

/** ֪ͨ��Ĺ�����
*/
class NotifyDialogManager :
    public QObject,
    public ManagerBase < NotifyDialogManager >
{
    Q_OBJECT
public:
    void showDialog(DialogType type, ParamBase* param);

public:
    NotifyDialogManager(){}

Q_SIGNALS:
    void notifyDialogDone(int32_t dialogId, int32_t userId, DialogType type, int32_t result, int32_t userParam);

private slots:
    void onFinished(int result);
};