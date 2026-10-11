#pragma once

/** 保留旧bool/指针接口时的文档错误原因，0为成功
*/
enum SplitViewerCoreResult
{
    // 最近一次文档操作成功，数值0为兼容成功码
    SPLITVIEWER_CORE_SUCCESS = 0,
    // 文档没有有效基础树，须先以有效配置初始化
    SPLITVIEWER_CORE_NOT_INIT = 1,
    // 配置、节点、索引或数值不符合当前操作的输入约束
    SPLITVIEWER_CORE_INVALID_PARAM = 2,
    // 已有有效基础树时再次调用初始化；需先uninit再重新初始化
    SPLITVIEWER_CORE_ALREADY_INIT = 3,
    // 新增图层或分割将超过实例配置的数量、节点或深度上限
    SPLITVIEWER_CORE_LIMIT_EXCEEDED = 4,
    // 目标节点未在指定树中找到，或迁移方向不存在可用目标
    SPLITVIEWER_CORE_NOT_FOUND = 5
};