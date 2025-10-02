#pragma once
#include <QWidget>
#include "Display.h"

// 前向声明QT类
class QLabel;
class QCustomPlot;

/** 策略展示窗口部件
* 显示单个策略的详细结果和图表
*/
class StrategyWidget : public QWidget
{
    Q_OBJECT

public:
    /** 构造函数
    @param [in] displayData 展示数据
    @param [in] rank 排名
    @param [in] parent 父窗口
    */
    explicit StrategyWidget(const DisplayData& displayData, int rank, QWidget* parent = nullptr);

    /** 获取推荐尺寸
    @return 返回推荐尺寸
    */
    QSize sizeHint() const override;

protected:
    /** 绘制事件
    @param [in] event 绘制事件
    */
    void paintEvent(QPaintEvent* event) override;

private:
    /** 初始化UI
    */
    void initUI();

    /** 创建图表
    */
    void createCharts();

    /** 绘制健康值图表
    */
    void drawHealthChart();

    /** 绘制收益图表
    */
    void drawProfitChart();

    /** 绘制回撤图表
    */
    void drawDrawdownChart();

private:
    DisplayData m_displayData;
    int m_rank;
    
    // UI组件
    QLabel* m_rankLabel;
    QLabel* m_paramsLabel;
    QLabel* m_returnLabel;
    QLabel* m_annualReturnLabel;
    QLabel* m_drawdownLabel;
    QLabel* m_winRateLabel;
    QLabel* m_profitAreaLabel;
    QLabel* m_healthScoreLabel;
    
    // 图表区域
    QWidget* m_chartArea;
};