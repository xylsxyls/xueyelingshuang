#pragma once
#include "DialogManager/DialogManagerAPI.h"

/** 只创建文件选择内容区，工厂注册成功后归DialogManager */
class SplitViewerFileDialogFactory : public CustomViewFactory
{
public:
    /** 按强类型参数创建空视图
    @param [in] param 文件选择参数
    @return 新视图交给DialogManager，类型不匹配返回nullptr
    */
    CustomViewBase* createView(const DialogParam& param) override;

    /** 在创建模块销毁工厂
    @param [in] factory 由本模块创建的工厂
    */
    static void destroy(CustomViewFactory* factory);
};