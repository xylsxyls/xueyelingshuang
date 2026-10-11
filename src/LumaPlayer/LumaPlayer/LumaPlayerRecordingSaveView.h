#pragma once
#include "DialogManager/DialogManagerAPI.h"

/** DialogManager拥有的保存选项内容，不直接执行编码或管理主窗口
*/
class LumaPlayerRecordingSaveView : public CustomViewBase
{
public:
    /** 按DialogManager参数初始化内容控件
    @param [in] param DialogManager传入的参数，借用至返回
    @return true表示内容初始化完成，false表示参数无效
    */
    virtual bool initView(const DialogParam& param);

    /** 提供弹窗内容的首选逻辑像素尺寸
    @return 内容区域逻辑像素尺寸
    */
    virtual QSize preferredSize() const;
};

/** 向DialogManager创建录音保存内容，返回对象归管理器所有
*/
class LumaPlayerRecordingSaveFactory : public CustomViewFactory
{
public:
    /** 创建与参数类型匹配的内容，由DialogManager取得所有权
    @param [in] param DialogManager传入的参数，借用至返回
    @return 新内容对象，参数不匹配时为空，由DialogManager销毁
    */
    virtual CustomViewBase* createView(const DialogParam& param);

    /** 释放工厂对象，由DialogManager在注销后调用
    @param [in] factory 由DialogManager转交销毁的工厂，不可再使用
    */
    static void destroy(CustomViewFactory* factory);
};