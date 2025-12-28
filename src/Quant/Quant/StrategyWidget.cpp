#include "StrategyWidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOption>

StrategyWidget::StrategyWidget(const DisplayData& displayData, int rank, QWidget* parent)
    : QWidget(parent)
    , m_displayData(displayData)
    , m_rank(rank)
    , m_rankLabel(nullptr)
    , m_paramsLabel(nullptr)
    , m_returnLabel(nullptr)
    , m_annualReturnLabel(nullptr)
    , m_drawdownLabel(nullptr)
    , m_winRateLabel(nullptr)
    , m_profitAreaLabel(nullptr)
    , m_healthScoreLabel(nullptr)
    , m_chartArea(nullptr)
{
    initUI();
    setFixedSize(400, 300);
}

QSize StrategyWidget::sizeHint() const
{
    return QSize(400, 300);
}

void StrategyWidget::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    
    // 绘制边框
    p.setPen(QPen(Qt::gray, 1));
    p.setBrush(Qt::white);
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}

void StrategyWidget::initUI()
{
    // 设置背景色
    setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::white);
    setPalette(palette);
    
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 排名和参数区域
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    m_rankLabel = new QLabel(QString("排名: %1").arg(m_rank), this);
    m_rankLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    headerLayout->addWidget(m_rankLabel);
    
    QString paramsText = "参数: ";
    for (size_t i = 0; i < m_displayData.strategyResult.params.size(); ++i)
    {
        paramsText += QString::number(m_displayData.strategyResult.params[i]);
        if (i < m_displayData.strategyResult.params.size() - 1)
        {
            paramsText += ", ";
        }
    }
    m_paramsLabel = new QLabel(paramsText, this);
    m_paramsLabel->setStyleSheet("font-size: 12px;");
    headerLayout->addWidget(m_paramsLabel);
    
    mainLayout->addLayout(headerLayout);
    
    // 指标区域
    QGridLayout* metricsLayout = new QGridLayout();
    
    m_returnLabel = new QLabel(
        QString("总收益率: %1%").arg((double)m_displayData.strategyResult.totalReturn * 100, 0, 'f', 2), 
        this);
    m_annualReturnLabel = new QLabel(
        QString("年化收益率: %1%").arg(m_displayData.strategyResult.annualReturn.toDouble() * 100, 0, 'f', 2), 
        this);
    m_drawdownLabel = new QLabel(
        QString("最大回撤: %1%").arg(m_displayData.strategyResult.maxDrawdown.toDouble() * 100, 0, 'f', 2), 
        this);
    m_winRateLabel = new QLabel(
        QString("胜率: %1%").arg(m_displayData.strategyResult.winRate.toDouble() * 100, 0, 'f', 2), 
        this);
    m_profitAreaLabel = new QLabel(
        QString("收益面积: %1").arg(m_displayData.strategyResult.profitArea.toDouble(), 0, 'f', 2), 
        this);
    m_healthScoreLabel = new QLabel(
        QString("健康值: %1").arg(m_displayData.strategyResult.healthScore.toDouble(), 0, 'f', 1), 
        this);
    
    // 设置颜色
    if ((double)m_displayData.strategyResult.totalReturn >= 0)
    {
        m_returnLabel->setStyleSheet("color: green; font-weight: bold;");
    }
    else
    {
        m_returnLabel->setStyleSheet("color: red; font-weight: bold;");
    }
    
    if (m_displayData.strategyResult.healthScore.toDouble() >= 80)
    {
        m_healthScoreLabel->setStyleSheet("color: green; font-weight: bold;");
    }
    else if (m_displayData.strategyResult.healthScore.toDouble() >= 60)
    {
        m_healthScoreLabel->setStyleSheet("color: orange; font-weight: bold;");
    }
    else
    {
        m_healthScoreLabel->setStyleSheet("color: red; font-weight: bold;");
    }
    
    metricsLayout->addWidget(m_returnLabel, 0, 0);
    metricsLayout->addWidget(m_annualReturnLabel, 0, 1);
    metricsLayout->addWidget(m_drawdownLabel, 1, 0);
    metricsLayout->addWidget(m_winRateLabel, 1, 1);
    metricsLayout->addWidget(m_profitAreaLabel, 2, 0);
    metricsLayout->addWidget(m_healthScoreLabel, 2, 1);
    
    mainLayout->addLayout(metricsLayout);
    
    // 图表区域
    m_chartArea = new QWidget(this);
    m_chartArea->setFixedHeight(150);
    m_chartArea->setStyleSheet("background-color: #f0f0f0; border: 1px solid #ccc;");
    mainLayout->addWidget(m_chartArea);
    
    createCharts();
}

void StrategyWidget::createCharts()
{
    // 这里应该使用QCustomPlot或QChart来创建图表
    // 由于时间关系，我们先使用简单的QPainter绘制
    
    // 实际项目中应该使用专业的图表库
    drawHealthChart();
}

void StrategyWidget::drawHealthChart()
{
    // 简单的健康值折线图绘制
    // 实际项目中应该使用QCustomPlot或QChart
    
    QPainter painter(m_chartArea);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int width = m_chartArea->width();
    int height = m_chartArea->height();
    int margin = 10;
    
    // 绘制背景网格
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    for (int i = 1; i < 4; ++i)
    {
        int y = margin + i * (height - 2 * margin) / 4;
        painter.drawLine(margin, y, width - margin, y);
    }
    
    // 绘制健康值折线
    if (m_displayData.healthHistory.size() > 1)
    {
        painter.setPen(QPen(QColor(0, 100, 0), 2));
        
        for (size_t i = 1; i < m_displayData.healthHistory.size(); ++i)
        {
            double x1 = margin + (i - 1) * (width - 2 * margin) / (m_displayData.healthHistory.size() - 1);
            double y1 = height - margin - m_displayData.healthHistory[i - 1].toDouble() * (height - 2 * margin) / 100;
            
            double x2 = margin + i * (width - 2 * margin) / (m_displayData.healthHistory.size() - 1);
            double y2 = height - margin - m_displayData.healthHistory[i].toDouble() * (height - 2 * margin) / 100;
            
            painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }
    }
    
    // 绘制标题
    painter.setPen(Qt::black);
    painter.drawText(QRect(margin, 5, width - 2 * margin, 20), Qt::AlignCenter, "健康值变化曲线");
}