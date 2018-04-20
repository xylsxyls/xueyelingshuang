#pragma once
#include <QObject>
#include "DialogType.h"
#include "ManagerBase.h"

class NotifyDialogManager : public ManagerBase < NotifyDialogManager >
{
    Q_OBJECT
public:
    void showDialog(DialogType type, ParamBase* param);

public:
    NotifyDialogManager(){}
};