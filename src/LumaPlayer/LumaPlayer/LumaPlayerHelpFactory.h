#pragma once
#include "DialogManager/DialogManagerAPI.h"

/** 无状态帮助工厂，DialogManager拥有，销毁回调在应用模块执行
*/
class LumaPlayerHelpFactory : public CustomViewFactory
{
public:
    /** 根据本次参数创建帮助内容
    @param [in] param LumaPlayerHelpParam派生参数
    @return 新视图，交由DialogManager接管
    */
    virtual CustomViewBase* createView(const DialogParam& param);

    /** 从应用模块销毁工厂，避免跨CRT delete
    @param [in] factory 注册时创建的工厂
    */
    static void destroy(CustomViewFactory* factory);
};