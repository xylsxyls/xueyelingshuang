#pragma once
#include <string>
#include <mutex>
#include <atomic>
#include "ControllerMacro.h"
using namespace std;

//障碍器，让程序通过或不通过
class ControllerAPI Obstacles{
    friend class Controller;
private:
    Obstacles(){}
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
    atomic<int> status = outside;
    mutex mutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    enum{
        outside,
        insideChoke,
        insideUnChoke
    };
private:
    //?默认不阻塞，1为阻塞，"111"表示前3次经过需要通知解除阻塞
    static Obstacles* CreateApi(string strControl, bool ifChoke = false);
public:
    void Set();
};