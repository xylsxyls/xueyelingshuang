#pragma once

#include "DialogManager/DialogManagerAPI.h"

/** 创建专用关于窗口；成功返回的窗口所有权交DialogManager */
class SplitViewerAboutDialogFactory : public CustomDialogFactory
{
public:
    /** 创建包含自定义内容的关于窗口
    @param [in] param 必须为SplitViewerAboutDialogParam
    @return 新窗口，类型错误或创建失败时返回nullptr
    */
    CustomDialog* createDialog(const DialogParam& param) override;

    /** 在创建模块中释放已注册工厂
    @param [in] factory 管理器移交的工厂，允许nullptr
    */
    static void destroy(CustomDialogFactory* factory);
};