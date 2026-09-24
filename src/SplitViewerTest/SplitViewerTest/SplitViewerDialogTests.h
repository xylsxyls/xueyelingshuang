#pragma once
#include <QtCore/QString>

/** 公共控件和真实托管弹窗回归，不替换产品文件选择结果 */
class SplitViewerDialogTests
{
public:
    /** 执行一个独立瞬时场景
    @param [in] id 稳定用例编号172至183
    @param [in] directory 本批可写素材与证据目录
    @return 真实控件结果符合独立预期且资源回收成功时返回true
    */
    static bool runCase(int id, const QString& directory);
};