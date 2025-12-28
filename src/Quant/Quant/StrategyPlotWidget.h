#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QTimer>
#include <QIntValidator>
#include <QSharedPointer>
#include <map>
#include <vector>
#include <memory>
#include "CustomPlotWidget.h"
#include "Strategy.h"

/** 策略结果可视化控件
*  用于显示策略回测结果的折线图，包括历史周期和未来周期的对比
*  支持大量策略的同时显示，提供交互功能
*/
class StrategyPlotWidget : public QWidget
{
	Q_OBJECT

public:
	/** 构造函数
	@param [in] parent 父控件指针
	*/
	explicit StrategyPlotWidget(QWidget *parent = nullptr);

	/** 设置策略数据
	@param [in] dataPair 策略数据对，包含历史周期和未来周期数据
	*/
	void setStrategyData(const std::pair<
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> &dataPair);

	private slots:
	/** 排名范围文本改变槽函数
	*/
	void onRankTextChanged();

	/** 更新排名显示定时器超时槽函数
	*/
	void onUpdateTimerTimeout();

	/** 年化收益率图表显示状态改变槽函数
	*/
	void onAnnualReturnVisibleChanged(int state);

	/** 做T年化收益率图表显示状态改变槽函数
	*/
	void onAnnualTReturnVisibleChanged(int state);

	/** 布局方式改变槽函数
	*/
	void onLayoutChanged(int index);

private:
	/** 初始化界面
	*/
	void initUI();

	/** 创建颜色映射
	@param [in] strategyCount 策略总数
	*/
	void createColorMap(int strategyCount);

	/** 获取下一个颜色
	@return 下一个颜色值
	*/
	QColor getNextColor();

	/** 分析策略数据，计算排名信息
	@param [in] dataPair 策略数据对
	*/
	void analyzeStrategyData(const std::pair<
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> &dataPair);

	/** 绘制所有策略数据
	*/
	void plotAllStrategies();

	/** 更新策略显示
	*/
	void updateStrategyVisibility();

	/** 更新布局
	*/
	void updateLayout();

	/** 计算X轴范围
	@param [in] dataPair 策略数据
	@param [out] xMin X轴最小值
	@param [out] xMax X轴最大值
	*/
	void calculateXRange(const std::pair<
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> &dataPair,
		int &xMin, int &xMax);

	CustomPlotWidget *m_annualReturnPlot;   // 年化收益率图表
	CustomPlotWidget *m_annualTReturnPlot;  // 做T年化收益率图表

	QVBoxLayout *m_mainLayout;              // 主布局
	QHBoxLayout *m_plotLayout;              // 图表布局
	QHBoxLayout *m_controlLayout;           // 控制面板布局

	// 控制面板控件
	QCheckBox *m_showAnnualReturnCheck;     // 显示年化收益率图表
	QCheckBox *m_showAnnualTReturnCheck;    // 显示做T年化收益率图表
	QComboBox *m_layoutCombo;               // 布局方式选择
	QLineEdit *m_rankStartEdit;             // 排名起始编辑框
	QLineEdit *m_rankEndEdit;               // 排名结束编辑框
	QLabel *m_infoLabel;                    // 信息标签

	// 定时器用于延迟更新
	QTimer *m_updateTimer;                  // 更新定时器

	// 排名相关数据
	QVector<QColor> m_colors;               // 颜色列表
	int m_currentColorIndex;                // 当前颜色索引

	// 策略数据
	std::pair<
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> m_currentData;

	// 排名信息
	QVector<int32_t> m_sortedKeys;          // 按收益排序的key列表（从高到低）
	QVector<int> m_rankStrategyCounts;      // 每个排名的策略数量
	QVector<QPair<int, int>> m_rankRanges;  // 每个排名在总策略中的索引范围

	int m_totalStrategyCount;               // 总策略数
	int m_totalRankCount;                   // 总排名数

	// X轴范围
	int m_xMin;
	int m_xMax;

	// 当前布局方式
	int m_currentLayout;                    // 0: 左右布局, 1: 上下布局

	// 当前图表显示状态
	bool m_showAnnualReturn;
	bool m_showAnnualTReturn;
};

// 延迟更新时间（毫秒）
const int UPDATE_DELAY_MS = 500;