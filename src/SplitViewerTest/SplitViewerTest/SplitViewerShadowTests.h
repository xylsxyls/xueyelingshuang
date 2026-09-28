#pragma once
#include <QString>
#include <QStringList>
#include <QImage>
#include <QRect>

/** 阴影公共接口与真实桌面拖动回归，复用产品关于入口和正式报告
*/
class SplitViewerShadowTests
{
public:
    /** 执行指定阴影用例
    @param [in] id 稳定编号184至188
    @param [in] directory 本批可写证据目录
    @return 用例实际后置条件全部满足时返回true
    */
    static bool runCase(int id, const QString& directory);

    /** 在独立测试子进程发送原生输入并检查桌面像素
    @param [in] arguments 本Test内部驱动参数，含本批目标句柄、PID、目录和模式
    @return 成功为0，失败或无效参数为非0
    */
    static int runDragDriver(const QStringList& arguments);

private:
    /** 检查参数开关、独立主体几何和透明投影衰减
    @param [in] directory 证据目录
    @return 所有参数与像素断言满足时返回true
    */
    static bool checkParameters(const QString& directory);

    /** 检查三次显示隐藏、最小化、禁用及销毁回收
    @param [in] directory 证据目录
    @return 生命周期断言全部满足时返回true
    */
    static bool checkLifetime(const QString& directory);

    /** 通过产品关于按钮启动父子进程实际拖动场景
    @param [in] directory 证据目录
    @param [in] fullWindowDrag 是否启用系统整窗拖动，测试后恢复原系统值
    @param [in] cancelMove 是否通过真实Esc取消移动并验证鼠标捕获释放
    @return 驱动与窗口回收均成功时返回true
    */
    static bool checkNativeDrag(const QString& directory, bool fullWindowDrag, bool cancelMove);

    /** 在目标四边的多个独立位置查找深色轮廓
    @param [in] desktop 实际桌面图像
    @param [in] target 屏幕像素中的目标矩形
    @return 四边均连续可见时返回true
    */
    static bool hasDarkOutline(const QImage& desktop, const QRect& target);

    /** 比较影子区域与同一灰色画布上的远端参考点
    @param [in] desktop 实际桌面图像
    @param [in] body 可见主体的屏幕矩形
    @return 四周有阴影且底部更深时返回true
    */
    static bool hasSoftShadow(const QImage& desktop, const QRect& body);
};