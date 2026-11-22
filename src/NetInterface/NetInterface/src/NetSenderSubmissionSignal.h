#pragma once
#include "NetInterfaceMacro.h"

/** 单张同步交付票据的持久通知；由等待者和票据共同持有，不负责业务回执
*/
class NetInterfaceAPI NetSenderSubmissionSignal
{
public:
    /** 最后一个拥有者释放通知对象
    */
    virtual ~NetSenderSubmissionSignal();

    /** 发布一个可积存的通知，允许完成与登记竞态产生重复提示，不得抛异常
    */
    virtual void notify() = 0;
};