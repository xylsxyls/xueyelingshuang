#include "Control.h"
#include <thread>

Control* Control::CreateApi(string strControl, bool ifChoke){
    Control* result = new Control;
    result->strControl = strControl;
    result->ifChoke = ifChoke;
    result->num = 0;
    if (strControl[result->num] == '1') result->current = true;
    else if (strControl[result->num] == '0') result->current = false;
    else result->current = ifChoke;
    result->numNext = -1;
    return result;
}

void Control::Set(){
    while (current == true){
        std::chrono::milliseconds dura(1);
        std::this_thread::sleep_for(dura);
    }
    mutex.lock();
    string strNextControlTemp = strNextControl;
    mutex.unlock();
    //?如果有就优先选择插入
    if (strNextControlTemp != ""){
        numNext++;
        if (strNextControlTemp[numNext] == '1') current = true;
        else if (strNextControlTemp[numNext] == '0') current = false;
        else{
            mutex.lock();
            strNextControl = "";
            mutex.unlock();
            numNext = -1;
            goto rem;
        }
    }
    else{
        rem:
        num++;
        if ((unsigned int)num < strControl.length()){
            if (strControl[num] == '1') current = true;
            else current = false;
        }
        else current = ifChoke;
    }
}