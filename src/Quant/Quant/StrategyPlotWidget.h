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
#include "QuantStrategyManager.h"

/** 颜色条控件，用于显示策略排名的颜色分布
*  支持鼠标悬停选择策略，与折线图联动
*/
class ColorBar : public QWidget
{
	Q_OBJECT

public:
	/** 构造函数
	*  @param [in] parent 父控件指针
	*/
	explicit ColorBar(QWidget* parent = nullptr);

	/** 设置颜色条数据
	*  @param [in] rankCounts 每个排名的策略数量（排名从高到低）
	*  @param [in] rankColors 每个排名的颜色（与rankCounts对应）
	*  @param [in] totalStrategies 总策略数
	*/
	void setColorData(const QVector<int>& rankCounts,
		const QVector<QColor>& rankColors,
		int totalStrategies);

	/** 设置悬停策略索引
	*  @param [in] strategyIndex 策略索引（在m_strategyDetails中的索引）
	*/
	void setHoveredStrategy(int strategyIndex);

	/** 清空颜色条数据
	*/
	void clear();

signals:
	/** 鼠标悬停时发出的信号，包含策略索引
	*  @param [in] strategyIndex 策略索引（在m_strategyDetails中的索引）
	*/
	void hoverStrategyChanged(int strategyIndex);

protected:
	/** 绘制事件处理函数
	*  @param [in] event 绘制事件
	*/
	void paintEvent(QPaintEvent* event) override;

	/** 鼠标移动事件处理函数
	*  @param [in] event 鼠标事件
	*/
	void mouseMoveEvent(QMouseEvent* event) override;

	/** 鼠标进入事件处理函数
	*  @param [in] event 鼠标事件
	*/
	void enterEvent(QEvent* event) override;

	/** 鼠标离开事件处理函数
	*  @param [in] event 鼠标事件
	*/
	void leaveEvent(QEvent* event) override;

private:
	/** 根据鼠标位置计算策略索引
	*  @param [in] mousePos 鼠标位置
	*  @return 策略索引（在总策略中的全局索引，从高到低排序），如果位置无效返回-1
	*/
	int calculateStrategyIndex(const QPoint& mousePos);

	QVector<int> m_rankCounts;              // 每个排名的策略数量（排名从高到低）
	QVector<QColor> m_rankColors;           // 每个排名的颜色（与rankCounts对应）
	int m_totalStrategies;                  // 总策略数
	int m_hoveredStrategyIndex;             // 当前悬停的策略索引（全局索引，从高到低）

	// 每个排名在颜色条上的起始和结束位置（像素）
	QVector<double> m_rankStartPositions;
	QVector<double> m_rankEndPositions;

	// 每个策略在颜色条上的理论位置（用于计算鼠标悬停）
	QVector<double> m_strategyPositions;

	int m_lastWidth;                        // 上次绘制的宽度
};

/** 策略结果可视化控件
*  用于显示策略回测结果的折线图，包括历史周期和未来周期的对比
*  支持大量策略的同时显示，提供交互功能
*/
class StrategyPlotWidget : public QWidget
{
	Q_OBJECT

public:
	/** 构造函数
	*  @param [in] parent 父控件指针
	*/
	explicit StrategyPlotWidget(QWidget* parent = nullptr);

	/** 设置策略数据
	*  @param [in] dataPair 策略数据对，包含历史周期和未来周期数据
	*/
	void setStrategyData(const std::pair<
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>>& dataPair);

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

	/** 年化收益率图表悬停信息改变槽函数
	*  @param [in] seriesIndex 系列索引
	*  @param [in] info 悬停信息
	*/
	void onAnnualReturnHoverInfoChanged(int seriesIndex, const QString& info);

	/** 做T年化收益率图表悬停信息改变槽函数
	*  @param [in] seriesIndex 系列索引
	*  @param [in] info 悬停信息
	*/
	void onAnnualTReturnHoverInfoChanged(int seriesIndex, const QString& info);

	/** 颜色条悬停策略改变槽函数
	*  @param [in] strategyIndex 策略索引
	*/
	void onColorBarHoverStrategyChanged(int strategyIndex);

private:
	/** 初始化界面
	*/
	void initUI();

	/** 分析策略数据，计算排名信息
	*  @param [in] dataPair 策略数据对
	*/
	void analyzeStrategyData(const std::pair<
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>>& dataPair);

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
	*  @param [in] historyData 历史周期数据
	*  @param [in] futureData 未来周期数据
	*  @param [out] xMin X轴最小值
	*  @param [out] xMax X轴最大值
	*/
	void calculateXRange(
		const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& historyData,
		const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& futureData,
		int& xMin, int& xMax);

	/** 根据排名获取颜色
	*  @param [in] rank 排名（从1开始，1为最高收益）
	*  @param [in] totalRanks 总排名数
	*  @return 对应的颜色
	*/
	QColor getColorForRank(int rank, int totalRanks);

	/** 生成多行悬停信息文本
	*  @param [in] seriesIndex 系列索引
	*  @return 多行悬停信息文本
	*/
	QString generateMultiLineHoverInfo(int seriesIndex) const;

	/** 格式化收益率数值
	*  @param [in] value 收益率值（小数形式，如0.1234表示12.34%）
	*  @return 格式化后的字符串
	*/
	QString formatRate(double value) const;

	/** 格式化周期标签
	*  @param [in] cycle 周期值（负数表示历史周期，0表示当前周期，正数表示未来周期）
	*  @return 格式化后的周期标签
	*/
	QString formatCycleLabel(int cycle) const;

	/** 计算QLabel需要的行数
	*  @return 需要的行数
	*/
	int calculateRequiredLines() const;

	/** 更新颜色条数据（根据当前显示的排名范围）
	*/
	void updateColorBar();

	/** 更新颜色条宽度（根据主窗口宽度调整）
	*/
	void updateColorBarWidth();

	/** 窗口大小改变事件处理函数
	*  @param [in] event 大小改变事件
	*/
	void resizeEvent(QResizeEvent* event) override;

	/** 显示事件处理函数
	*  @param [in] event 显示事件
	*/
	void showEvent(QShowEvent* event) override;

	CustomPlotWidget* m_annualReturnPlot;   // 年化收益率图表
	CustomPlotWidget* m_annualTReturnPlot;  // 做T年化收益率图表
	ColorBar* m_colorBar;                   // 颜色条控件

	QVBoxLayout* m_mainLayout;              // 主布局
	QHBoxLayout* m_plotLayout;              // 图表布局
	QHBoxLayout* m_controlLayout;           // 控制面板布局

	// 控制面板控件
	QCheckBox* m_showAnnualReturnCheck;     // 显示年化收益率图表
	QCheckBox* m_showAnnualTReturnCheck;    // 显示做T年化收益率图表
	QComboBox* m_layoutCombo;               // 布局方式选择
	QLineEdit* m_rankStartEdit;             // 排名起始编辑框
	QLineEdit* m_rankEndEdit;               // 排名结束编辑框
	QLabel* m_infoLabel;                    // 信息标签

	// 提示信息显示区域
	QLabel* m_hoverInfoLabel;               // 悬停信息显示标签

	// 定时器用于延迟更新
	QTimer* m_updateTimer;                  // 更新定时器

	// 策略数据
	std::pair<
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> m_currentData;

	// 排名信息
	QVector<int32_t> m_sortedKeys;          // 按收益排序的key列表（从低到高）
	QVector<int> m_rankStrategyCounts;      // 每个排名的策略数量（从低收益到高收益）
	QVector<QPair<int, int>> m_rankRanges;  // 每个排名在总策略中的索引范围（从低收益到高收益）

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

	// 存储策略详细信息，用于悬停显示
	struct StrategyDetailInfo
	{
		int rank;                                   // 排名（1为最高收益）
		int indexInRank;                            // 在该排名中的索引（0为第一个）
		std::shared_ptr<StrategyResult> result;     // 当前周期结果
		QColor color;                               // 策略颜色
		std::vector<std::shared_ptr<StrategyResult>> historyResults;  // 历史周期结果，从当前周期(0)到历史最远周期
		std::vector<std::shared_ptr<StrategyResult>> futureResults;   // 未来周期结果，从当前周期(0)到未来最远周期
	};
	QVector<StrategyDetailInfo> m_strategyDetails; // 所有策略的详细信息（按绘制顺序，从低收益到高收益）

	// 颜色条需要的数据（从高收益到低收益）
	QVector<int> m_colorBarRankCounts;             // 每个排名的策略数量（从高到低）
	QVector<QColor> m_colorBarRankColors;          // 每个排名的颜色（从高到低）

	// 映射：从颜色条索引（高到低）到策略详情索引（低到高）
	QVector<int> m_colorBarToDetailIndexMap;

	// 布局边距
	int m_leftPlotMargin;                   // 左侧图表边距
	int m_rightPlotMargin;                  // 右侧图表边距
	int m_plotSpacing;                      // 图表间距
	int m_colorBarHeight;                   // 颜色条高度
};

// 延迟更新时间（毫秒）
const int UPDATE_DELAY_MS = 300;

// 显示的正周期个数（不包括当前周期0）
const int DISPLAY_POSITIVE_CYCLES = 1;

// 显示的负周期个数（不包括当前周期0）
const int DISPLAY_NEGATIVE_CYCLES = 2;

// 颜色条高度（像素）
const int COLOR_BAR_HEIGHT = 40;

// QLabel固定高度（根据行数计算）
const int LABEL_FIXED_HEIGHT = 150;  // 预留给5行文字加上边框的高度