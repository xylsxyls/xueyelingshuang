#pragma once
#include <QString>
#include <QEvent>
#include <QPoint>
#include <QImage>

class QWidget;
class QLabel;
class PdfReader;
class QFileDialog;

/** 经真实 Qt 事件和窗口进行交互测试，不复制产品交互逻辑
*/
class PdfReaderTestUiHelper
{
public:
    /** 获取管理器当前弹窗中的文件选择复合控件；没有则返回空。 */
    static QFileDialog* fileDialog();
    /** 获取当前 PdfReader 内容区；没有则返回空。 */
    static QWidget* dialogView();
    /** 点击当前弹窗确认或取消按钮。
    @param [in] accepted true 表示确认，false 表示取消。
    @return 找到并点击按钮时返回 true。
    */
    static bool answerDialog(bool accepted);
    /** 用 Qt 事件循环等待真实长按/滚动定时器；不引入额外运行库
    @param [in] milliseconds 等待时长，毫秒；期间处理 Qt 事件
    */
    static void wait(int milliseconds);

    /** 通过控件实际键盘事件取消拖动
    @param [in] target 同步接收按键事件的控件，不得为空
    */
    static void escape(QWidget* target);

    /** 发送包含按钮状态的鼠标输入，经 QWidget 实际事件处理
    @param [in] target 同步接收事件的控件，不得为空
    @param [in] type 按下、移动或释放事件
    @param [in] point 控件坐标
    */
    static void mouse(QWidget* target, QEvent::Type type, const QPoint& point);

    /** 发送 Ctrl 滚轮，分别验证缩略图和正文的独立缩放
    @param [in] target 同步接收滚轮事件的控件，不得为空
    @param [in] delta Qt 滚轮角度增量
    */
    static void wheel(QWidget* target, int delta);

    /** 获取第 index 页的正文控件，缺页会使测试失败
    @param [in] window 借用被测窗口
    @param [in] index 零起始页号
    @return 借用的正文控件，生命周期随窗口；缺页抛出异常
    */
    static QLabel* page(PdfReader& window, int index);

    /** 检查重排后真实渲染像素，不仅检查 UI 行号
    @param [in] window 借用被测窗口
    @param [in] index 零起始页号
    @param [in] expected 期望 RGB 颜色
    */
    static void color(PdfReader& window, int index, QRgb expected);

    /** 弹窗自动响应使用用户同一接受入口，超时/未出现必须失败
    @param [in] window 借用被测窗口
    @param [in] slot 触发的产品槽名
    @param [in] path 文件对话框输入路径
    @param [in] yes 确认弹窗选择 Yes 或 No
    */
    static void dialogAction(PdfReader& window, const char* slot, const QString& path, bool yes = true);
};
