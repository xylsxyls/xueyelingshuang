#include "StrategyPlotWidget.h"
#include <QDebug>
#include <algorithm>
#include <QIntValidator>

// 辅助函数：将策略参数转换为字符串
static QString paramsToString(const std::vector<int32_t>& params)
{
	QStringList paramList;
	for (int32_t param : params)
	{
		paramList.append(QString::number(param));
	}
	return paramList.join(QStringLiteral(","));
}

// 辅助函数：生成策略名称
static QString generateStrategyName(int rank, int indexInRank, const std::vector<int32_t>& params)
{
	QString paramStr = paramsToString(params);
	if (paramStr.length() > 40)
	{
		paramStr = paramStr.left(37) + QStringLiteral("...");
	}
	return QStringLiteral("策略排名%1-第%2个 (参数: %3)").arg(rank).arg(indexInRank + 1).arg(paramStr);
}

StrategyPlotWidget::StrategyPlotWidget(QWidget *parent)
	: QWidget(parent)
	, m_currentColorIndex(0)
	, m_totalStrategyCount(0)
	, m_totalRankCount(0)
	, m_xMin(-3)
	, m_xMax(3)
	, m_currentLayout(0)
	, m_showAnnualReturn(true)
	, m_showAnnualTReturn(true)
{
	initUI();
}

void StrategyPlotWidget::initUI()
{
	// 设置窗口属性
	setWindowTitle(QStringLiteral("策略回测结果可视化"));
	setMinimumSize(1400, 800);

	// 创建主布局
	m_mainLayout = new QVBoxLayout(this);

	// 创建标题
	QLabel *titleLabel = new QLabel(QStringLiteral("策略回测结果 - 历史与未来周期对比"), this);
	titleLabel->setAlignment(Qt::AlignCenter);
	QFont titleFont = titleLabel->font();
	titleFont.setPointSize(16);
	titleFont.setBold(true);
	titleLabel->setFont(titleFont);
	m_mainLayout->addWidget(titleLabel);

	// 创建控制面板
	m_controlLayout = new QHBoxLayout();

	// 图表显示复选框
	m_showAnnualReturnCheck = new QCheckBox(QStringLiteral("显示年化收益率"), this);
	m_showAnnualReturnCheck->setChecked(true);
	connect(m_showAnnualReturnCheck, SIGNAL(stateChanged(int)), this, SLOT(onAnnualReturnVisibleChanged(int)));

	m_showAnnualTReturnCheck = new QCheckBox(QStringLiteral("显示做T年化收益率"), this);
	m_showAnnualTReturnCheck->setChecked(true);
	connect(m_showAnnualTReturnCheck, SIGNAL(stateChanged(int)), this, SLOT(onAnnualTReturnVisibleChanged(int)));

	// 布局方式选择
	QLabel *layoutLabel = new QLabel(QStringLiteral("布局方式:"), this);
	m_layoutCombo = new QComboBox(this);
	m_layoutCombo->addItem(QStringLiteral("左右布局"));
	m_layoutCombo->addItem(QStringLiteral("上下布局"));
	connect(m_layoutCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onLayoutChanged(int)));

	// 排名范围输入
	QLabel *rankLabel = new QLabel(QStringLiteral("显示策略排名:"), this);
	m_rankStartEdit = new QLineEdit(this);
	m_rankStartEdit->setPlaceholderText(QStringLiteral("起始排名"));
	m_rankStartEdit->setFixedWidth(80);
	m_rankStartEdit->setValidator(new QIntValidator(1, 99999, this));

	QLabel *toLabel = new QLabel(QStringLiteral("到"), this);

	m_rankEndEdit = new QLineEdit(this);
	m_rankEndEdit->setPlaceholderText(QStringLiteral("结束排名"));
	m_rankEndEdit->setFixedWidth(80);
	m_rankEndEdit->setValidator(new QIntValidator(1, 99999, this));

	// 连接文本改变信号
	connect(m_rankStartEdit, SIGNAL(textChanged(QString)), this, SLOT(onRankTextChanged()));
	connect(m_rankEndEdit, SIGNAL(textChanged(QString)), this, SLOT(onRankTextChanged()));

	// 信息标签
	m_infoLabel = new QLabel(QStringLiteral(""), this);
	m_infoLabel->setStyleSheet(QStringLiteral("color: #666; font-style: italic;"));

	// 添加到控制面板
	m_controlLayout->addWidget(m_showAnnualReturnCheck);
	m_controlLayout->addWidget(m_showAnnualTReturnCheck);
	m_controlLayout->addWidget(layoutLabel);
	m_controlLayout->addWidget(m_layoutCombo);
	m_controlLayout->addSpacing(20);
	m_controlLayout->addWidget(rankLabel);
	m_controlLayout->addWidget(m_rankStartEdit);
	m_controlLayout->addWidget(toLabel);
	m_controlLayout->addWidget(m_rankEndEdit);
	m_controlLayout->addWidget(m_infoLabel);
	m_controlLayout->addStretch();

	m_mainLayout->addLayout(m_controlLayout);

	// 创建图表布局
	m_plotLayout = new QHBoxLayout();

	// 年化收益率图表
	QFrame *frame1 = new QFrame(this);
	frame1->setFrameStyle(QFrame::Panel | QFrame::Raised);
	frame1->setStyleSheet(QStringLiteral("QFrame { background-color: #f8f8f8; }"));

	QVBoxLayout *layout1 = new QVBoxLayout(frame1);
	m_annualReturnPlot = new CustomPlotWidget(frame1);
	m_annualReturnPlot->setTitle(QStringLiteral("年化收益率趋势 (单位: %)"));
	m_annualReturnPlot->setAxisLabels(QStringLiteral("周期（负数为历史，正数为未来）"), QStringLiteral("年化收益率"));
	layout1->addWidget(m_annualReturnPlot);

	// 做T年化收益率图表
	QFrame *frame2 = new QFrame(this);
	frame2->setFrameStyle(QFrame::Panel | QFrame::Raised);
	frame2->setStyleSheet(QStringLiteral("QFrame { background-color: #f8f8f8; }"));

	QVBoxLayout *layout2 = new QVBoxLayout(frame2);
	m_annualTReturnPlot = new CustomPlotWidget(frame2);
	m_annualTReturnPlot->setTitle(QStringLiteral("做T年化收益率趋势 (单位: %)"));
	m_annualTReturnPlot->setAxisLabels(QStringLiteral("周期（负数为历史，正数为未来）"), QStringLiteral("做T年化收益率"));
	layout2->addWidget(m_annualTReturnPlot);

	// 初始为左右布局
	m_plotLayout->addWidget(frame1);
	m_plotLayout->addWidget(frame2);

	m_mainLayout->addLayout(m_plotLayout);

	// 添加说明文字
	QLabel *helpLabel = new QLabel(QStringLiteral("使用说明：鼠标滚轮缩放Y轴，左键拖拽平移Y轴视图，鼠标悬停在数据点上查看详情。Y轴在X=0的位置，左侧为历史周期，右侧为未来周期。点击图表右上角的'复原'按钮可以重置Y轴位置。"), this);
	helpLabel->setAlignment(Qt::AlignCenter);
	helpLabel->setStyleSheet(QStringLiteral("color: #666; font-size: 10pt; font-style: italic;"));
	m_mainLayout->addWidget(helpLabel);

	// 创建更新定时器
	m_updateTimer = new QTimer(this);
	m_updateTimer->setSingleShot(true);
	connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(onUpdateTimerTimeout()));

	// 设置布局
	setLayout(m_mainLayout);
}

void StrategyPlotWidget::createColorMap(int strategyCount)
{
	m_colors.clear();
	m_currentColorIndex = 0;

	// 生成青色到蓝色的颜色（R分量为0，G和B分量变化）
	for (int i = 0; i < strategyCount; i++)
	{
		// 使用HSV颜色空间，固定R=0，G和B从50到255
		int green = 50 + (i * 205) / qMax(strategyCount - 1, 1);
		int blue = 255 - (i * 205) / qMax(strategyCount - 1, 1);
		m_colors.append(QColor(0, green, blue));
	}

	// 如果策略数量太多，补充一些随机青色/蓝色
	if (strategyCount > 1000)
	{
		for (int i = 1000; i < strategyCount; i++)
		{
			m_colors.append(QColor(0, qrand() % 206 + 50, qrand() % 206 + 50));
		}
	}
}

QColor StrategyPlotWidget::getNextColor()
{
	if (m_colors.isEmpty() || m_currentColorIndex >= m_colors.size())
	{
		// 如果颜色列表用完，返回随机青色/蓝色
		return QColor(0, qrand() % 206 + 50, qrand() % 206 + 50);
	}

	QColor color = m_colors[m_currentColorIndex];
	m_currentColorIndex = (m_currentColorIndex + 1) % m_colors.size();
	return color;
}

void StrategyPlotWidget::calculateXRange(const std::pair <
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> > &dataPair,
	int &xMin, int &xMax)
{
	xMin = 0;
	xMax = 0;

	// 检查历史周期数据
	if (!dataPair.first.empty())
	{
		const auto &firstGroup = dataPair.first.begin()->second;
		if (!firstGroup.empty() && !firstGroup[0].empty())
		{
			// 历史周期数量（包括当前周期0）
			int historyCount = static_cast<int>(firstGroup[0].size());
			xMin = -(historyCount - 1); // 例如：有3个历史周期，则xMin = -2 (0, -1, -2)
		}
	}

	// 检查未来周期数据
	if (!dataPair.second.empty())
	{
		const auto &secondGroup = dataPair.second.begin()->second;
		if (!secondGroup.empty() && !secondGroup[0].empty())
		{
			// 未来周期数量（包括当前周期0）
			int futureCount = static_cast<int>(secondGroup[0].size());
			xMax = futureCount - 1; // 例如：有3个未来周期，则xMax = 2 (0, 1, 2)
		}
	}

	// 确保至少有一定范围
	if (xMin == 0 && xMax == 0)
	{
		xMin = -3;
		xMax = 3;
	}
}

void StrategyPlotWidget::analyzeStrategyData(const std::pair <
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> > &dataPair)
{
	// 清空现有数据
	m_sortedKeys.clear();
	m_rankStrategyCounts.clear();
	m_rankRanges.clear();
	m_totalStrategyCount = 0;
	m_totalRankCount = 0;

	// 提取所有key并按收益从高到低排序（map已经是按key排序的，但我们需要从高到低）
	for (auto it = dataPair.first.rbegin(); it != dataPair.first.rend(); ++it)
	{
		int32_t key = it->first;
		int strategyCount = static_cast<int>(it->second.size());

		m_sortedKeys.append(key);
		m_rankStrategyCounts.append(strategyCount);
		m_totalStrategyCount += strategyCount;
		m_totalRankCount++;
	}

	// 计算每个排名在总策略中的索引范围
	int currentIndex = 0;
	for (int i = 0; i < m_rankStrategyCounts.size(); i++)
	{
		int startIndex = currentIndex;
		int endIndex = currentIndex + m_rankStrategyCounts[i] - 1;
		m_rankRanges.append(QPair<int, int>(startIndex, endIndex));
		currentIndex = endIndex + 1;
	}

	// 计算X轴范围
	calculateXRange(dataPair, m_xMin, m_xMax);

	// 设置排名编辑框的默认值
	if (m_rankStartEdit->text().isEmpty())
	{
		m_rankStartEdit->setText(QStringLiteral("1"));
	}
	if (m_rankEndEdit->text().isEmpty())
	{
		m_rankEndEdit->setText(QString::number(m_totalRankCount));
	}

	// 更新信息标签
	m_infoLabel->setText(QStringLiteral("共发现 %1 个策略，%2 个排名，X轴范围: %3 到 %4")
		.arg(m_totalStrategyCount)
		.arg(m_totalRankCount)
		.arg(m_xMin)
		.arg(m_xMax));
}

void StrategyPlotWidget::plotAllStrategies()
{
	// 清空图表数据
	m_annualReturnPlot->clear();
	m_annualTReturnPlot->clear();

	// 设置X轴范围
	m_annualReturnPlot->setXRange(m_xMin, m_xMax);
	m_annualTReturnPlot->setXRange(m_xMin, m_xMax);

	// 创建颜色映射
	createColorMap(m_totalStrategyCount);
	m_currentColorIndex = 0;

	// 按排名顺序绘制所有策略
	int globalStrategyIndex = 0;

	for (int rankIndex = 0; rankIndex < m_totalRankCount; rankIndex++)
	{
		int32_t key = m_sortedKeys[rankIndex];

		// 获取该排名对应的策略数据（从pair.first中获取历史周期数据）
		auto itHistory = m_currentData.first.find(key);
		if (itHistory == m_currentData.first.end())
		{
			continue;
		}

		const auto &strategyGroups = itHistory->second;
		int strategyCountInRank = strategyGroups.size();

		for (int strategyIndexInRank = 0; strategyIndexInRank < strategyCountInRank; strategyIndexInRank++)
		{
			const auto &historyStrategyGroup = strategyGroups[strategyIndexInRank];
			if (historyStrategyGroup.empty())
			{
				continue;
			}

			auto firstResult = historyStrategyGroup[0];
			if (!firstResult)
			{
				continue;
			}

			// 生成策略名称
			QString strategyName = generateStrategyName(rankIndex + 1, strategyIndexInRank, firstResult->params);

			// 获取未来周期数据
			std::vector<std::shared_ptr<StrategyResult>> futureStrategyGroup;
			auto itFuture = m_currentData.second.find(key);
			if (itFuture != m_currentData.second.end() &&
				strategyIndexInRank < itFuture->second.size())
			{
				futureStrategyGroup = itFuture->second[strategyIndexInRank];
			}

			// 1. 年化收益率图表
			QVector<QPointF> annualReturnPoints;

			// 添加历史周期数据（从远到近：-2, -1, 0）
			for (int periodIdx = static_cast<int>(historyStrategyGroup.size()) - 1; periodIdx >= 0; periodIdx--)
			{
				auto result = historyStrategyGroup[periodIdx];
				if (!result)
				{
					continue;
				}

				// 历史周期：-2, -1, 0...
				double xValue = -static_cast<double>(periodIdx);
				double yValue = result->annualReturn.toDouble();

				annualReturnPoints.append(QPointF(xValue, yValue));
			}

			// 添加未来周期数据（从当前周期0开始：0, 1, 2...）
			if (!futureStrategyGroup.empty())
			{
				for (size_t periodIdx = 0; periodIdx < futureStrategyGroup.size(); ++periodIdx)
				{
					auto result = futureStrategyGroup[periodIdx];
					if (!result)
					{
						continue;
					}

					// 未来周期：0, 1, 2...
					double xValue = static_cast<double>(periodIdx);
					double yValue = result->annualReturn.toDouble();

					// 确保X=0的点只添加一次
					if (periodIdx > 0 || historyStrategyGroup.empty())
					{
						annualReturnPoints.append(QPointF(xValue, yValue));
					}
				}
			}

			// 添加到年化收益率图表
			m_annualReturnPlot->addSeries(strategyName, annualReturnPoints, getNextColor());

			// 2. 做T年化收益率图表
			QVector<QPointF> annualTReturnPoints;

			// 添加历史周期数据（从远到近：-2, -1, 0）
			for (int periodIdx = static_cast<int>(historyStrategyGroup.size()) - 1; periodIdx >= 0; periodIdx--)
			{
				auto result = historyStrategyGroup[periodIdx];
				if (!result)
				{
					continue;
				}

				// 历史周期：-2, -1, 0...
				double xValue = -static_cast<double>(periodIdx);
				double yValue = result->annualTReturn.toDouble();

				annualTReturnPoints.append(QPointF(xValue, yValue));
			}

			// 添加未来周期数据（从当前周期0开始：0, 1, 2...）
			if (!futureStrategyGroup.empty())
			{
				for (size_t periodIdx = 0; periodIdx < futureStrategyGroup.size(); ++periodIdx)
				{
					auto result = futureStrategyGroup[periodIdx];
					if (!result)
					{
						continue;
					}

					// 未来周期：0, 1, 2...
					double xValue = static_cast<double>(periodIdx);
					double yValue = result->annualTReturn.toDouble();

					// 确保X=0的点只添加一次
					if (periodIdx > 0 || historyStrategyGroup.empty())
					{
						annualTReturnPoints.append(QPointF(xValue, yValue));
					}
				}
			}

			// 添加到做T年化收益率图表
			m_annualTReturnPlot->addSeries(strategyName, annualTReturnPoints, getNextColor());

			globalStrategyIndex++;
		}
	}

	// 自动调整Y轴范围
	m_annualReturnPlot->autoRange();
	m_annualTReturnPlot->autoRange();
}

void StrategyPlotWidget::setStrategyData(const std::pair <
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> > &dataPair)
{
	// 保存数据
	m_currentData = dataPair;

	// 分析策略数据，计算排名信息
	analyzeStrategyData(dataPair);

	// 绘制所有策略
	plotAllStrategies();

	// 更新策略显示
	updateStrategyVisibility();
}

void StrategyPlotWidget::updateStrategyVisibility()
{
	// 获取排名范围
	int startRank = 1;
	int endRank = m_totalRankCount;

	if (!m_rankStartEdit->text().isEmpty())
	{
		startRank = m_rankStartEdit->text().toInt();
	}

	if (!m_rankEndEdit->text().isEmpty())
	{
		endRank = m_rankEndEdit->text().toInt();
	}

	// 确保范围有效
	if (startRank < 1)
	{
		startRank = 1;
	}
	if (endRank > m_totalRankCount)
	{
		endRank = m_totalRankCount;
	}
	if (startRank > endRank)
	{
		int temp = startRank;
		startRank = endRank;
		endRank = temp;
	}

	// 计算可见策略的总数
	int visibleStrategyCount = 0;
	for (int rank = startRank; rank <= endRank; rank++)
	{
		if (rank - 1 < m_rankStrategyCounts.size())
		{
			visibleStrategyCount += m_rankStrategyCounts[rank - 1];
		}
	}

	// 更新信息标签
	m_infoLabel->setText(QStringLiteral("共发现 %1 个策略，%2 个排名，显示排名 %3-%4 (%5 个策略)")
		.arg(m_totalStrategyCount)
		.arg(m_totalRankCount)
		.arg(startRank)
		.arg(endRank)
		.arg(visibleStrategyCount));

	// 更新图表中的策略可见性
	int currentStrategyIndex = 0;

	for (int rankIndex = 0; rankIndex < m_totalRankCount; rankIndex++)
	{
		bool isVisible = (rankIndex + 1 >= startRank && rankIndex + 1 <= endRank);
		int strategyCountInRank = m_rankStrategyCounts[rankIndex];

		for (int i = 0; i < strategyCountInRank; i++)
		{
			if (m_showAnnualReturn)
			{
				m_annualReturnPlot->setSeriesVisible(currentStrategyIndex, isVisible);
			}

			if (m_showAnnualTReturn)
			{
				m_annualTReturnPlot->setSeriesVisible(currentStrategyIndex, isVisible);
			}

			currentStrategyIndex++;
		}
	}
}

void StrategyPlotWidget::updateLayout()
{
	// 清空当前布局
	QLayoutItem *item;
	while ((item = m_plotLayout->takeAt(0)) != nullptr)
	{
		if (item->widget())
		{
			item->widget()->setParent(nullptr);
		}
		delete item;
	}

	// 获取图表控件
	QWidget *annualReturnWidget = m_annualReturnPlot->parentWidget();
	QWidget *annualTReturnWidget = m_annualTReturnPlot->parentWidget();

	if (m_currentLayout == 0) // 左右布局
	{
		if (m_showAnnualReturn && m_showAnnualTReturn)
		{
			m_plotLayout->addWidget(annualReturnWidget);
			m_plotLayout->addWidget(annualTReturnWidget);
			annualReturnWidget->show();
			annualTReturnWidget->show();
		}
		else if (m_showAnnualReturn)
		{
			m_plotLayout->addWidget(annualReturnWidget);
			annualReturnWidget->show();
			annualTReturnWidget->hide();
		}
		else if (m_showAnnualTReturn)
		{
			m_plotLayout->addWidget(annualTReturnWidget);
			annualReturnWidget->hide();
			annualTReturnWidget->show();
		}
	}
	else // 上下布局
	{
		if (m_showAnnualReturn && m_showAnnualTReturn)
		{
			QVBoxLayout *verticalLayout = new QVBoxLayout();
			verticalLayout->addWidget(annualReturnWidget);
			verticalLayout->addWidget(annualTReturnWidget);
			m_plotLayout->addLayout(verticalLayout);
			annualReturnWidget->show();
			annualTReturnWidget->show();
		}
		else if (m_showAnnualReturn)
		{
			m_plotLayout->addWidget(annualReturnWidget);
			annualReturnWidget->show();
			annualTReturnWidget->hide();
		}
		else if (m_showAnnualTReturn)
		{
			m_plotLayout->addWidget(annualTReturnWidget);
			annualReturnWidget->hide();
			annualTReturnWidget->show();
		}
	}
}

void StrategyPlotWidget::onRankTextChanged()
{
	// 重启定时器，延迟500毫秒后更新
	m_updateTimer->stop();
	m_updateTimer->start(UPDATE_DELAY_MS);
}

void StrategyPlotWidget::onUpdateTimerTimeout()
{
	updateStrategyVisibility();
}

void StrategyPlotWidget::onAnnualReturnVisibleChanged(int state)
{
	m_showAnnualReturn = (state == Qt::Checked);
	updateLayout();
}

void StrategyPlotWidget::onAnnualTReturnVisibleChanged(int state)
{
	m_showAnnualTReturn = (state == Qt::Checked);
	updateLayout();
}

void StrategyPlotWidget::onLayoutChanged(int index)
{
	m_currentLayout = index;
	updateLayout();
}