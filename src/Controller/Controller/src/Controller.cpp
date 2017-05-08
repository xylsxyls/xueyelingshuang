#include "Controller.h"

Control* Controller::CreateControl(string strControl, bool ifChoke){
    return Control::CreateApi(strControl, ifChoke);
}

void Controller::SetChoke(Control* control){
    control->current = true;
}

void Controller::SetUnChoke(Control* control){
    control->current = false;
}

void Controller::SetTempControl(Control* control, string nextControl){
    control->mutex.lock();
    control->strNextControl = nextControl;
    control->mutex.unlock();
}