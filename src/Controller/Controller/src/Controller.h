#pragma once
#include "Control.h"
#include "ControllerMacro.h"

class ControllerAPI Controller{
public:
    //生成
    Control* CreateControl(string strControl, bool ifChoke = false);
    void SetChoke(Control* control);
    void SetUnChoke(Control* control);
    //接下来的几个
    void SetTempControl(Control* control, string nextControl);
};