#pragma once
/** 分屏树节点的结构类型；分割节点独占两个子节点，叶子保存内容状态
*/
enum SplitViewerCoreNodeKind
{
    // 没有子树的内容区域，可为空、图片或嵌入窗口
    SPLITVIEWER_CORE_NODE_LEAF = 0,
    // 保存分割方向和比例的内部节点，内容位于两个子区域
    SPLITVIEWER_CORE_NODE_SPLIT = 1
};

/** 按分割线方向划分父区域；水平形成上下区域，垂直形成左右区域
*/
enum SplitViewerCoreSplitDirection
{
    // 水平分割线形成上下区域，第一子区域在上方
    SPLITVIEWER_CORE_SPLIT_HORIZONTAL = 0,
    // 垂直分割线形成左右区域，第一子区域在左方
    SPLITVIEWER_CORE_SPLIT_VERTICAL = 1
};

/** 叶子持久化的内容类型；图片资源和原生窗口由桌面层持有
*/
enum SplitViewerCoreContentKind
{
    // 无内容的叶子；界面显示灰色背景和加载加号
    SPLITVIEWER_CORE_CONTENT_EMPTY = 0,
    // 图片路径及变换状态，解码后的图片由界面资源缓存持有
    SPLITVIEWER_CORE_CONTENT_IMAGE = 1,
    // 外部拖入的原生窗口；不承诺通过配置恢复该窗口
    SPLITVIEWER_CORE_CONTENT_EMBEDDED = 2,
    // 联动视频路径；恢复配置时重新启动播放器并嵌入对应叶子
    SPLITVIEWER_CORE_CONTENT_VIDEO = 3
};

/** 同一图层内的内容迁移方向；目标子树按第一子节点优先选取叶子
*/
enum SplitViewerCoreMoveDirection
{
    // 向左侧相邻子树迁移，存在多片区域时优先其上方叶子
    SPLITVIEWER_CORE_MOVE_LEFT = 0,
    // 向右侧相邻子树迁移，存在多片区域时优先其上方叶子
    SPLITVIEWER_CORE_MOVE_RIGHT = 1,
    // 向上侧相邻子树迁移，存在多片区域时优先其左方叶子
    SPLITVIEWER_CORE_MOVE_UP = 2,
    // 向下侧相邻子树迁移，存在多片区域时优先其左方叶子
    SPLITVIEWER_CORE_MOVE_DOWN = 3
};