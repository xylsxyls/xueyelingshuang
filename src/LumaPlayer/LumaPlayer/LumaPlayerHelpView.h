#pragma once
#include "DialogManager/DialogManagerAPI.h"
#include "LumaPlayerLogicModels.h"
#include <QPoint>
#include <QPointer>

class QWindow;
class QLabel;
class QPushButton;
class LumaPlayerLogicController;

/** 帮助内容参数，控制器仅在播放器仍存活的模态会话期间借用
*/
struct LumaPlayerHelpParam : public CustomDialogParam
{
public:
    // 对应播放器的逻辑控制器
    LumaPlayerLogicController* m_logic;

public:
    /** 构造帮助类型参数
    @param [in] logic 模态期间存活的控制器
    */
    explicit LumaPlayerHelpParam(LumaPlayerLogicController* logic);
};

/** 帮助内容视图，QWidget由DialogManager外壳管理，不使用shared_ptr
*/
class LumaPlayerHelpView : public CustomViewBase
{
public:
    /** 构造安全默认值，控件在initView创建
    @param [in] logic 借用播放器逻辑控制器
    */
    explicit LumaPlayerHelpView(LumaPlayerLogicController* logic);

    /** 创建标题、说明及两个同路径关闭按钮
    @param [in] param 帮助窗口参数
    @return true表示内容区已初始化
    */
    virtual bool initView(const DialogParam& param);

    /** 获取配置中的弹窗尺寸
    @return 期望尺寸
    */
    virtual QSize preferredSize() const;

    /** 返回确认按钮作为默认焦点
    @return 控件借用指针
    */
    virtual QWidget* defaultFocusWidget() const;

protected:
    /** 首次显示时相对播放器居中，标题拖动移动实际模态外壳
    @param [in] watched 模态外壳、标题栏或标题文字
    @param [in] event Qt事件，借用到返回
    @return true表示已处理标题拖动
    */
    virtual bool eventFilter(QObject* watched, QEvent* event);

private:
    /** 两个按钮均只提交关闭Task，真实关闭在队列结果到达后执行
    */
    void requestClose();

    /** 接收对应会话的关闭结果，避免旧结果关闭新窗口
    @param [in] result 逻辑线程结果
    */
    void onLogicResult(const LumaPlayerLogicResult& result);

private:
    // 播放器窗口弱引用，Qt销毁时自动清空
    QPointer<QWindow> m_playerWindow;
    // 初次显示已定位，标题拖动后不重新居中
    bool m_centered;
    // 标题栏，由Qt父子关系管理
    QWidget* m_titleBar;
    // 标题文字，由Qt父子关系管理
    QLabel* m_titleLabel;
    // 标题左键拖动状态
    bool m_titleDragging;
    // 按下时鼠标全局位置
    QPoint m_titlePressGlobal;
    // 按下时模态窗口位置
    QPoint m_titlePressWindow;
    // 模态期间借用，随播放器关闭前收敛
    LumaPlayerLogicController* m_logic;
    // 确认按钮，Qt父子所有权
    QPushButton* m_confirm;
    // 帮助会话标识
    uint64_t m_token;
    // 防止重复创建内容
    bool m_initialized;
};