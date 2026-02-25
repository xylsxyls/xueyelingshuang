#include "Display.h"
#include <algorithm>
#include <random>

Display::Display(QObject* parent)
    : QObject(parent)
{
}

void Display::setCompetitionResult(const CompetitionFinalResult& result)
{
    m_finalResult = result;
}

const std::vector<DisplayData>& Display::getDisplayData() const
{
    return m_displayData;
}

void Display::preprocessData()
{
    m_displayData.clear();
    
    // 为每个策略结果生成展示数据
    for (const auto& strategyResult : m_finalResult.rankedResults)
    {
        m_displayData.push_back(generateDisplayData(strategyResult));
    }
    
    // 发出预处理完成信号
    emit dataPreprocessed();
}

DisplayData Display::generateDisplayData(const StrategyResult& result)
{
    DisplayData data;
    data.strategyResult = result;
    
    // 生成模拟历史数据（实际应用中应该从回测过程中记录真实数据）
    int dataPoints = 100; // 100个数据点
    
    // 生成健康值历史（从100开始逐渐变化）
    BigNumber currentHealth = 100;
    data.healthHistory.push_back(currentHealth);
    
    // 生成收益历史
    BigNumber currentProfit = 0;
    data.profitHistory.push_back(currentProfit);
    
    // 生成回撤历史
    BigNumber currentDrawdown = 0;
    data.drawdownHistory.push_back(currentDrawdown);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> healthDist(0, 2);   // 健康值变化
    std::normal_distribution<> profitDist(0.5, 2); // 收益变化
    std::normal_distribution<> drawdownDist(0, 1); // 回撤变化
    
    for (int i = 1; i < dataPoints; ++i)
    {
        // 健康值变化（最终收敛到最终健康值）
        BigNumber healthChange = healthDist(gen);
        currentHealth = currentHealth + healthChange;
        // 确保健康值在合理范围内
        if (currentHealth < 0) currentHealth = 0;
        if (currentHealth > 100) currentHealth = 100;
        data.healthHistory.push_back(currentHealth);
        
        // 收益变化（模拟增长）
        BigNumber profitChange = profitDist(gen);
        currentProfit = currentProfit + profitChange;
        data.profitHistory.push_back(currentProfit);
        
        // 回撤变化（模拟波动）
        BigNumber drawdownChange = drawdownDist(gen);
        currentDrawdown = currentDrawdown + drawdownChange;
        if (currentDrawdown < 0) currentDrawdown = 0;
        data.drawdownHistory.push_back(currentDrawdown);
    }
    
    // 调整最终值以匹配实际结果
    if (dataPoints > 0)
    {
        data.healthHistory[dataPoints - 1] = result.m_healthScore;
        data.profitHistory[dataPoints - 1] = result.m_totalReturn * 100; // 转换为百分比
        data.drawdownHistory[dataPoints - 1] = result.m_maxDrawdown * 100; // 转换为百分比
    }
    
    return data;
}