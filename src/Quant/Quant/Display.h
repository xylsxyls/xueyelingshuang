#pragma once
#include "CompetitionManager.h"
#include <vector>
#include <memory>
#include <QObject>

// 前向声明QT类
class QWidget;
class QScrollArea;
class QVBoxLayout;

/** 展示数据预处理结构体
*/
struct DisplayData
{
    StrategyResult strategyResult;    // 策略结果
    std::vector<BigNumber> healthHistory; // 健康值历史变化
    std::vector<BigNumber> profitHistory; // 收益历史变化
    std::vector<BigNumber> drawdownHistory; // 回撤历史变化
};

/** 展示类
* 负责预处理竞赛结果数据，为界面展示做准备
*/
class Display : public QObject
{
    Q_OBJECT

public:
    /** 构造函数
    @param [in] parent 父对象
    */
    explicit Display(QObject* parent = nullptr);

    /** 设置竞赛结果
    @param [in] result 竞赛最终结果
    */
    void setCompetitionResult(const CompetitionFinalResult& result);

    /** 获取展示数据
    @return 返回所有策略的展示数据
    */
    const std::vector<DisplayData>& getDisplayData() const;

    /** 预处理展示数据
    */
    void preprocessData();

signals:
    /** 数据预处理完成信号
    */
    void dataPreprocessed();

private:
    /** 生成模拟历史数据（实际应该从回测过程中记录）
    @param [in] result 策略结果
    @return 返回展示数据
    */
    DisplayData generateDisplayData(const StrategyResult& result);

private:
    CompetitionFinalResult m_finalResult;
    std::vector<DisplayData> m_displayData;
};