#pragma once
#include "Widget.h"
#include "DialogShadowConfig.h"
#include <QImage>

class QPaintEvent;

/** 弹窗外侧的透明阴影层，生命周期由父弹窗管理，仅在GUI线程使用
窗口不接收输入，不参与正文布局；原生轮廓拖动期间只显示目标位置边线
*/
class DialogShadow : public Widget
{
public:
    /** 创建由目标弹窗拥有的透明工具窗口
    @param [in] dialog 阴影所属弹窗，不可为空，不由本对象释放
    */
    explicit DialogShadow(QWidget* dialog);

    /** 移除事件监听，Qt父子所有权负责销毁窗口资源
    */
    virtual ~DialogShadow();

    /** 更新阴影参数并同步窗口，非正值禁用，正值限制到16级
    @param [in] size 阴影扩散级别，2对应8逻辑像素的主模糊标准差
    */
    void setShadowSize(qint32 size);

    /** 同步显示状态、位置、尺寸和缓存，保持主体客户区不变
    */
    void synchronize();

    /** 开始轮廓拖动，保留原位置直到用户释放或取消
    @param [in] globalPos 鼠标在Qt全局逻辑坐标中的位置
    */
    void beginOutlineMove(const QPoint& globalPos);

    /** 随真实鼠标移动更新唯一目标轮廓
    @param [in] globalPos 鼠标在Qt全局逻辑坐标中的位置
    */
    void moveOutline(const QPoint& globalPos);

    /** 结束轮廓拖动并释放本次捕获；重复结束无副作用
    @param [in] accepted 为true提交目标位置，为false保持原位置
    */
    void finishOutlineMove(bool accepted);

    /** 查询是否正在使用独立轮廓拖动
    @return 轮廓拖动中返回true
    */
    bool outlineMoveActive() const;

protected:
    /** 监听主体的显示、隐藏、几何和层级变化
    @param [in] watched 被监听的对象
    @param [in] event Qt事件
    @return 不拦截主体事件，始终返回false
    */
    bool eventFilter(QObject* watched, QEvent* event);

    /** 绘制透明投影或移动目标边线，不绘制主体底板
    @param [in] event Qt绘制事件
    */
    void paintEvent(QPaintEvent* event);

private:
    /** 根据主体尺寸和DPI更新缓存，相同输入不重复计算
    @param [in] bodySize 可见主体的逻辑像素尺寸
    @param [in] ratio 当前窗口设备像素比例
    */
    void updateImage(const QSize& bodySize, qreal ratio);

    /** 积分一维高斯核，计算矩形区间对当前采样点的覆盖率
    @param [in] sample 采样坐标
    @param [in] start 区间起点
    @param [in] end 区间终点
    @param [in] sigma 模糊标准差，必须大于0
    @return 0到1之间的覆盖率
    */
    qreal coverage(qreal sample, qreal start, qreal end, qreal sigma) const;

private:
    // 此阴影实例的只读默认配置
    const DialogShadowConfig m_config;
    // 借用父弹窗，父对象销毁时本对象同步销毁
    QWidget* m_dialog;
    // 0禁用，正值为已限幅的扩散级别
    qint32 m_size;
    // 缓存使用的扩散级别
    qint32 m_imageSize;
    // 当前缓存主体尺寸
    QSize m_bodySize;
    // 当前缓存的设备像素比例
    qreal m_ratio;
    // 阴影外延，主体四边保留相同逻辑留白
    qint32 m_margin;
    // 透明投影缓存，中心始终透明
    QImage m_image;
    // 是否处于仅轮廓的原生移动循环
    bool m_outlineMove;
    // 是否已有超过拖动阈值的目标轮廓
    bool m_targetVisible;
    // 轮廓拖动起点，Qt全局逻辑坐标
    QPoint m_dragStart;
    // 拖动前的实际主体矩形
    QRect m_dragOrigin;
    // 当前鼠标指定的目标主体矩形
    QRect m_dragTarget;
};