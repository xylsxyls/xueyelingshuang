#pragma once
#include "SplitViewerFileDialogParam.h"

class FileDialog;

/** 文件选择内容区，窗口显示和销毁归DialogManager，控件归Qt父子关系 */
class SplitViewerFileDialogView : public CustomViewBase
{
public:
    /** 创建空的文件选择内容区 */
    SplitViewerFileDialogView();

    /** 根据路径及过滤器配置文件控件
    @param [in] param 必须为SplitViewerFileDialogParam
    @return 参数类型正确且未重复初始化时返回true
    */
    bool initView(const DialogParam& param) override;

    /** 返回文件选择窗口默认尺寸
    @return 逻辑像素尺寸
    */
    QSize preferredSize() const override;

    /** 返回默认获得焦点的文件控件
    @return 内容区持有的控件，不转移所有权
    */
    QWidget* defaultFocusWidget() const override;

private:
    /** 检查保存覆盖确认，提交结果并请求管理器关闭 */
    void acceptSelection();

    /** 保留调用方数据并请求管理器取消窗口 */
    void cancelSelection();

private:
    // 当前选择参数和共享结果
    SplitViewerFileDialogParam m_param;
    // Qt父子关系持有的文件控件
    FileDialog* m_files;
};