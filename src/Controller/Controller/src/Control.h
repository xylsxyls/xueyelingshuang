#pragma once
#include <string>
#include <mutex>
#include <atomic>
#include "ControllerMacro.h"
using namespace std;

class ControllerAPI Control{
public:
    bool ifChoke;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    string strControl;
    atomic<bool> current;
    atomic<int> num;
    string strNextControl;
    atomic<int> numNext;
    mutex mutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
    //?默认不阻塞，1为阻塞，"111"表示前3次经过需要通知解除阻塞
    static Control* CreateApi(string strControl, bool ifChoke = false);
    void Set();
};