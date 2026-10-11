#pragma once
#include <QtCore/QString>
class QTextStream;

/** 定向移动与视频联动回归，预期由需求推导
*/
class SplitViewerMoveVideoTests
{
public:
    /** 执行一个稳定编号用例
    @param [in] id 用例ID
    @param [in] directory 本批素材及证据目录
    @param [in,out] log 本批报告流
    @return 是否满足独立预期
    */
    static bool run(int id, const QString& directory, QTextStream& log);

private:
    /** 四方向及嵌套固定目标
    */
    static bool directionalTargets();

    /** 非当前树、同一节点及空内容不能迁移
    */
    static bool invalidMoves();

    /** 内容完整迁移，源为空且树保持不变
    */
    static bool contentMove();

    /** 视频字段独立检查及图片兼容
    */
    static bool videoProfile();
};