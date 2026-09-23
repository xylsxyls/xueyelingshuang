#pragma once
#include "SplitViewerPlatform.h"

/** 平台文件关联与缩略图注册。 */
class SplitViewerFileAssociationHelper
{
public:
    /** 注册当前用户的.sv文件关联及PNG预览处理器。
    @param [in] applicationPath 应用程序完整路径
    @return 是否满足条件或操作成功。
    */
    static bool registerSvFileAssociation(const QString& applicationPath);
    /** 注册.sv缩略图处理器；未实现的平台返回false。
    @param [in] applicationPath 应用程序完整路径
    @return 是否满足条件或操作成功。
    */
    static bool registerSvThumbnailHandler(const QString& applicationPath);
};