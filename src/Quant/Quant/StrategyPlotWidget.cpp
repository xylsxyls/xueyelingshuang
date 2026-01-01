#include "StrategyPlotWidget.h"
#include <QDebug>
#include <algorithm>
#include <QIntValidator>
#include <QScrollArea>
#include <QFontMetrics>
#include <cmath>
#include <QPainter>
#include <QDebug>

ColorBar::ColorBar(QWidget* parent)
	: QWidget(parent)
	, m_totalStrategies(0)
	, m_hoveredStrategyIndex(-1)
	, m_lastWidth(0)
{
	setMouseTracking(true);
	setMinimumHeight(COLOR_BAR_HEIGHT);
	setMaximumHeight(COLOR_BAR_HEIGHT);
}

void ColorBar::setColorData(const QVector<int>& rankCounts,
	const QVector<QColor>& rankColors,
	int totalStrategies)
{
	m_rankCounts = rankCounts;
	m_rankColors = rankColors;
	m_totalStrategies = totalStrategies;
	m_lastWidth = 0;

	// 清空位置缓存
	m_rankStartPositions.clear();
	m_rankEndPositions.clear();
	m_strategyPositions.clear();

	update();
}

void ColorBar::setHoveredStrategy(int strategyIndex)
{
	if (m_hoveredStrategyIndex != strategyIndex)
	{
		m_hoveredStrategyIndex = strategyIndex;
		update();
	}
}

void ColorBar::clear()
{
	m_rankCounts.clear();
	m_rankColors.clear();
	m_totalStrategies = 0;
	m_hoveredStrategyIndex = -1;
	m_rankStartPositions.clear();
	m_rankEndPositions.clear();
	m_strategyPositions.clear();
	m_lastWidth = 0;

	update();
}

int ColorBar::calculateStrategyIndex(const QPoint& mousePos)
{
	// 如果没有数据或宽度为0，返回-1
	if (m_totalStrategies == 0 || width() == 0)
	{
		return -1;
	}

	// 获取鼠标的X坐标
	double mouseX = static_cast<double>(mousePos.x());
	double totalWidth = static_cast<double>(width());

	// 如果宽度变化或位置列表为空，重新计算位置
	if (width() != m_lastWidth || m_strategyPositions.isEmpty())
	{
		// 计算每个策略在颜色条上占用的理论宽度
		double strategyWidth = totalWidth / m_totalStrategies;

		// 计算每个策略的起始位置
		m_strategyPositions.clear();
		for (int i = 0; i < m_totalStrategies; i++)
		{
			double startPos = i * strategyWidth;
			m_strategyPositions.append(startPos);
		}

		// 计算每个排名的起始和结束位置
		m_rankStartPositions.clear();
		m_rankEndPositions.clear();

		int accumulatedStrategies = 0;
		for (int i = 0; i < m_rankCounts.size(); i++)
		{
			double startPos = accumulatedStrategies * strategyWidth;
			double endPos = (accumulatedStrategies + m_rankCounts[i]) * strategyWidth;

			m_rankStartPositions.append(startPos);
			m_rankEndPositions.append(endPos);

			accumulatedStrategies += m_rankCounts[i];
		}

		m_lastWidth = width();
	}

	// 如果策略数量远大于像素宽度（每个策略宽度小于1像素），我们采用采样策略
	// 计算每个策略占用的理论宽度
	double strategyWidth = totalWidth / m_totalStrategies;

	if (strategyWidth < 1.0)
	{
		// 每个策略宽度小于1像素，采用采样
		// 计算每个像素应该显示的策略数量
		double strategiesPerPixel = m_totalStrategies / totalWidth;

		// 根据鼠标位置计算策略索引
		int strategyIndex = static_cast<int>(mouseX * strategiesPerPixel);
		strategyIndex = qBound(0, strategyIndex, m_totalStrategies - 1);

		return strategyIndex;
	}
	else
	{
		// 每个策略宽度大于等于1像素，可以直接计算
		int strategyIndex = static_cast<int>(mouseX / strategyWidth);
		strategyIndex = qBound(0, strategyIndex, m_totalStrategies - 1);

		return strategyIndex;
	}
}

void ColorBar::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制背景
	painter.fillRect(rect(), QColor(240, 240, 240));

	// 如果没有数据，直接返回
	if (m_totalStrategies == 0 || m_rankCounts.isEmpty() || m_rankColors.isEmpty())
	{
		// 绘制无数据提示
		painter.setPen(QColor(150, 150, 150));
		painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("无策略数据"));
		return;
	}

	// 计算颜色条的绘制区域
	int barHeight = height() - 4; // 留出2像素的上下边距
	int barTop = 2;

	// 如果宽度变化或位置列表为空，重新计算位置
	double totalWidth = static_cast<double>(width());
	if (width() != m_lastWidth || m_strategyPositions.isEmpty())
	{
		// 计算每个策略在颜色条上占用的理论宽度
		double strategyWidth = totalWidth / m_totalStrategies;

		// 计算每个策略的起始位置
		m_strategyPositions.clear();
		for (int i = 0; i < m_totalStrategies; i++)
		{
			double startPos = i * strategyWidth;
			m_strategyPositions.append(startPos);
		}

		// 计算每个排名的起始和结束位置
		m_rankStartPositions.clear();
		m_rankEndPositions.clear();

		int accumulatedStrategies = 0;
		for (int i = 0; i < m_rankCounts.size(); i++)
		{
			double startPos = accumulatedStrategies * strategyWidth;
			double endPos = (accumulatedStrategies + m_rankCounts[i]) * strategyWidth;

			m_rankStartPositions.append(startPos);
			m_rankEndPositions.append(endPos);

			accumulatedStrategies += m_rankCounts[i];
		}

		m_lastWidth = width();
	}

	// 绘制颜色条（从左到右：收益从高到低）
	for (int i = 0; i < m_rankCounts.size(); i++)
	{
		double startPos = m_rankStartPositions[i];
		double endPos = m_rankEndPositions[i];

		// 确保至少绘制1像素宽
		int pixelStart = static_cast<int>(startPos);
		int pixelEnd = static_cast<int>(endPos);

		if (pixelEnd <= pixelStart)
		{
			pixelEnd = pixelStart + 1;
		}

		// 绘制该排名的颜色块
		QRect rankRect(pixelStart, barTop, pixelEnd - pixelStart, barHeight);
		painter.fillRect(rankRect, m_rankColors[i]);
	}

	// 确保整个颜色条都被填满
	if (!m_rankStartPositions.isEmpty() && !m_rankEndPositions.isEmpty())
	{
		double lastEndPos = m_rankEndPositions.last();
		if (lastEndPos < totalWidth)
		{
			int pixelStart = static_cast<int>(lastEndPos);
			int pixelEnd = width();

			if (pixelEnd > pixelStart)
			{
				QRect remainingRect(pixelStart, barTop, pixelEnd - pixelStart, barHeight);
				// 用最后一个颜色填充剩余部分
				painter.fillRect(remainingRect, m_rankColors.last());
			}
		}
	}

	// 绘制悬停指示器（蓝色竖线）
	if (m_hoveredStrategyIndex >= 0 && m_hoveredStrategyIndex < m_totalStrategies)
	{
		// 计算悬停策略在颜色条上的位置
		double strategyWidth = totalWidth / m_totalStrategies;
		double hoverX = m_hoveredStrategyIndex * strategyWidth + strategyWidth / 2;

		// 确保位置在有效范围内
		if (hoverX >= 0 && hoverX <= totalWidth)
		{
			// 绘制蓝色竖线
			QColor hoverColor(g_hoverColor[0], g_hoverColor[1], g_hoverColor[2]);
			painter.setPen(QPen(hoverColor, 2));
			painter.drawLine(static_cast<int>(hoverX), 0, static_cast<int>(hoverX), height());

			// 绘制一个矩形框突出显示当前策略位置
			int strategyPixelWidth = qMax(1, static_cast<int>(strategyWidth));
			int rectX = static_cast<int>(m_hoveredStrategyIndex * strategyWidth);
			QRect hoverRect(rectX, 0, strategyPixelWidth, height());
			painter.setPen(QPen(hoverColor, 1));
			painter.setBrush(Qt::NoBrush);
			painter.drawRect(hoverRect);
		}
	}

	// 绘制边框
	painter.setPen(QColor(200, 200, 200));
	painter.setBrush(Qt::NoBrush);
	painter.drawRect(rect());
}

void ColorBar::mouseMoveEvent(QMouseEvent* event)
{
	// 计算鼠标位置对应的策略索引
	int strategyIndex = calculateStrategyIndex(event->pos());

	if (strategyIndex != m_hoveredStrategyIndex)
	{
		m_hoveredStrategyIndex = strategyIndex;
		update();

		// 发出信号
		emit hoverStrategyChanged(strategyIndex);
	}

	QWidget::mouseMoveEvent(event);
}

void ColorBar::enterEvent(QEvent* event)
{
	Q_UNUSED(event);
	setCursor(Qt::PointingHandCursor);
}

void ColorBar::leaveEvent(QEvent* event)
{
	Q_UNUSED(event);
	setCursor(Qt::ArrowCursor);

	// 鼠标离开时清除悬停状态
	if (m_hoveredStrategyIndex != -1)
	{
		m_hoveredStrategyIndex = -1;
		update();
		emit hoverStrategyChanged(-1);
	}
}

// 辅助函数：通过QuantStrategyManager获取参数描述
static QString getParamDescription(StrategyMode mode, const std::vector<int32_t>& params)
{
	try
	{
		std::shared_ptr<Strategy> strategy = QuantStrategyManager::instance().createStrategy(mode);
		if (strategy)
		{
			std::string desc = strategy->describeParam(params);
			return QString::fromStdString(desc);
		}
	}
	catch (const std::exception& e)
	{
		qWarning() << "获取参数描述失败:" << e.what();
	}
	catch (...)
	{
		qWarning() << "获取参数描述失败: 未知错误";
	}

	// 失败时使用原始参数
	QStringList paramList;
	for (int32_t param : params)
	{
		paramList.append(QString::number(param));
	}
	return paramList.join(QStringLiteral(","));
}

StrategyPlotWidget::StrategyPlotWidget(QWidget* parent)
	: QWidget(parent)
	, m_totalStrategyCount(0)
	, m_totalRankCount(0)
	, m_xMin(-3)
	, m_xMax(3)
	, m_currentLayout(0)
	, m_showAnnualReturn(true)
	, m_showAnnualTReturn(true)
	, m_leftPlotMargin(10)
	, m_rightPlotMargin(10)
	, m_plotSpacing(10)
	, m_colorBarHeight(COLOR_BAR_HEIGHT)
{
	initUI();
}

int StrategyPlotWidget::calculateRequiredLines() const
{
	// 计算需要的行数：排名行 + 周期0行 + 正周期行 + 负周期行
	return 1 + 1 + DISPLAY_POSITIVE_CYCLES + DISPLAY_NEGATIVE_CYCLES;
}

void StrategyPlotWidget::initUI()
{
	// 设置窗口属性
	setWindowTitle(QStringLiteral("策略回测结果可视化"));
	setMinimumSize(1400, 800);

	// 创建主布局
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setSpacing(10);
	m_mainLayout->setContentsMargins(10, 10, 10, 10);

	// 创建标题
	QLabel* titleLabel = new QLabel(QStringLiteral("策略回测结果 - 历史与未来周期对比"), this);
	titleLabel->setAlignment(Qt::AlignCenter);
	QFont titleFont = titleLabel->font();
	titleFont.setPointSize(16);
	titleFont.setBold(true);
	titleLabel->setFont(titleFont);
	m_mainLayout->addWidget(titleLabel);

	// 创建控制面板
	m_controlLayout = new QHBoxLayout();
	m_controlLayout->setSpacing(15);

	// 图表显示复选框
	m_showAnnualReturnCheck = new QCheckBox(QStringLiteral("显示年化收益率"), this);
	m_showAnnualReturnCheck->setChecked(true);
	connect(m_showAnnualReturnCheck, SIGNAL(stateChanged(int)),
		this, SLOT(onAnnualReturnVisibleChanged(int)));

	m_showAnnualTReturnCheck = new QCheckBox(QStringLiteral("显示做T年化收益率"), this);
	m_showAnnualTReturnCheck->setChecked(true);
	connect(m_showAnnualTReturnCheck, SIGNAL(stateChanged(int)),
		this, SLOT(onAnnualTReturnVisibleChanged(int)));

	// 布局方式选择
	QLabel* layoutLabel = new QLabel(QStringLiteral("布局方式:"), this);
	m_layoutCombo = new QComboBox(this);
	m_layoutCombo->addItem(QStringLiteral("左右布局"));
	m_layoutCombo->addItem(QStringLiteral("上下布局"));
	connect(m_layoutCombo, SIGNAL(currentIndexChanged(int)),
		this, SLOT(onLayoutChanged(int)));

	// 排名范围输入
	QLabel* rankLabel = new QLabel(QStringLiteral("显示策略排名:"), this);
	m_rankStartEdit = new QLineEdit(this);
	m_rankStartEdit->setPlaceholderText(QStringLiteral("起始排名"));
	m_rankStartEdit->setFixedWidth(80);
	m_rankStartEdit->setValidator(new QIntValidator(1, 99999, this));

	QLabel* toLabel = new QLabel(QStringLiteral("到"), this);

	m_rankEndEdit = new QLineEdit(this);
	m_rankEndEdit->setPlaceholderText(QStringLiteral("结束排名"));
	m_rankEndEdit->setFixedWidth(80);
	m_rankEndEdit->setValidator(new QIntValidator(1, 99999, this));

	// 连接文本改变信号
	connect(m_rankStartEdit, SIGNAL(textChanged(QString)),
		this, SLOT(onRankTextChanged()));
	connect(m_rankEndEdit, SIGNAL(textChanged(QString)),
		this, SLOT(onRankTextChanged()));

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

	// 创建悬停信息显示区域（放在控制面板下方）
	m_hoverInfoLabel = new QLabel(QStringLiteral("鼠标悬停在折线上查看详细信息..."), this);
	m_hoverInfoLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_hoverInfoLabel->setWordWrap(true);  // 允许自动换行
	m_hoverInfoLabel->setStyleSheet(QStringLiteral("QLabel { background-color: #f8f8f8; border: 1px solid #ccc; border-radius: 4px; padding: 5px; font-size: 10pt; }"));

	// 设置QLabel的固定宽度和高度
	QFontMetrics fm(m_hoverInfoLabel->font());
	int charWidth = fm.width('A');
	int estimatedWidth = charWidth * 75; // 大约75个字符的宽度

	// 固定宽度为窗口宽度的70%
	int fixedWidth = 1400 * 0.7;
	m_hoverInfoLabel->setMinimumWidth(fixedWidth);
	m_hoverInfoLabel->setMaximumWidth(fixedWidth);

	// 固定高度，避免抖动
	m_hoverInfoLabel->setFixedHeight(LABEL_FIXED_HEIGHT);

	m_mainLayout->addWidget(m_hoverInfoLabel);

	// 创建颜色条
	m_colorBar = new ColorBar(this);
	m_colorBar->setMinimumHeight(m_colorBarHeight);
	m_colorBar->setMaximumHeight(m_colorBarHeight);
	connect(m_colorBar, &ColorBar::hoverStrategyChanged,
		this, &StrategyPlotWidget::onColorBarHoverStrategyChanged);
	m_mainLayout->addWidget(m_colorBar);

	// 创建图表布局
	m_plotLayout = new QHBoxLayout();
	m_plotLayout->setSpacing(m_plotSpacing);
	m_plotLayout->setContentsMargins(m_leftPlotMargin, 0, m_rightPlotMargin, 0);

	// 年化收益率图表
	QFrame* frame1 = new QFrame(this);
	frame1->setFrameStyle(QFrame::Panel | QFrame::Raised);
	frame1->setStyleSheet(QStringLiteral("QFrame { background-color: #f8f8f8; }"));

	QVBoxLayout* layout1 = new QVBoxLayout(frame1);
	layout1->setContentsMargins(1, 1, 1, 1);
	m_annualReturnPlot = new CustomPlotWidget(frame1);
	m_annualReturnPlot->setTitle(QStringLiteral("年化收益率趋势 (单位: %)"));
	m_annualReturnPlot->setAxisLabels(QStringLiteral("周期（负数为历史，正数为未来）"),
		QStringLiteral("年化收益率"));
	// 连接悬停信号
	connect(m_annualReturnPlot, &CustomPlotWidget::hoverInfoChanged,
		this, &StrategyPlotWidget::onAnnualReturnHoverInfoChanged);
	layout1->addWidget(m_annualReturnPlot);

	// 做T年化收益率图表
	QFrame* frame2 = new QFrame(this);
	frame2->setFrameStyle(QFrame::Panel | QFrame::Raised);
	frame2->setStyleSheet(QStringLiteral("QFrame { background-color: #f8f8f8; }"));

	QVBoxLayout* layout2 = new QVBoxLayout(frame2);
	layout2->setContentsMargins(1, 1, 1, 1);
	m_annualTReturnPlot = new CustomPlotWidget(frame2);
	m_annualTReturnPlot->setTitle(QStringLiteral("做T年化收益率趋势 (单位: %)"));
	m_annualTReturnPlot->setAxisLabels(QStringLiteral("周期（负数为历史，正数为未来）"),
		QStringLiteral("做T年化收益率"));
	// 连接悬停信号
	connect(m_annualTReturnPlot, &CustomPlotWidget::hoverInfoChanged,
		this, &StrategyPlotWidget::onAnnualTReturnHoverInfoChanged);
	layout2->addWidget(m_annualTReturnPlot);

	// 初始为左右布局
	m_plotLayout->addWidget(frame1);
	m_plotLayout->addWidget(frame2);

	m_mainLayout->addLayout(m_plotLayout);

	// 添加说明文字
	QLabel* helpLabel = new QLabel(QStringLiteral("使用说明：鼠标滚轮缩放Y轴，左键拖拽平移Y轴视图，鼠标悬停在数据点上查看详情。Y轴在X=0的位置，左侧为历史周期，右侧为未来周期。点击图表右下角的'复原'按钮可以将Y轴零点重新放到中间位置。"), this);
	helpLabel->setAlignment(Qt::AlignCenter);
	helpLabel->setStyleSheet(QStringLiteral("color: #666; font-size: 10pt; font-style: italic;"));
	helpLabel->setWordWrap(true);
	m_mainLayout->addWidget(helpLabel);

	// 创建更新定时器
	m_updateTimer = new QTimer(this);
	m_updateTimer->setSingleShot(true);
	connect(m_updateTimer, SIGNAL(timeout()),
		this, SLOT(onUpdateTimerTimeout()));

	// 设置布局
	setLayout(m_mainLayout);
}

void StrategyPlotWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);

	// 窗口显示后，更新布局以确保图表高度正确
	updateLayout();
}

QString StrategyPlotWidget::formatRate(double value) const
{
	// 将小数转换为百分比，保留2位小数
	double percentValue = value * 100.0;

	// 观察数据范围，确定最大位数
	// 假设收益率范围在-999.99%到999.99%之间，最大需要7个字符（包括负号、数字、小数点、百分号）
	// 格式：-123.45% 或 123.45%

	QString str;
	if (percentValue >= 0) {
		// 正数：前面加一个空格占位负号位置
		str = QStringLiteral(" %1%").arg(percentValue, 6, 'f', 2, QLatin1Char(' '));
	}
	else {
		// 负数：直接格式化，负号会占据一个位置
		str = QStringLiteral("%1%").arg(percentValue, 6, 'f', 2, QLatin1Char(' '));
	}

	// 确保总长度为7个字符（包括百分号）
	// 如果长度不足7，左侧填充空格
	if (str.length() < 7) {
		str = str.leftJustified(7, QLatin1Char(' '));
	}

	return str;
}

QString StrategyPlotWidget::formatCycleLabel(int cycle) const
{
	// 格式化周期标签，确保标签长度一致
	// 周期标签格式："周期X:"，其中X可能是负数、0、正数
	// 为了使长度一致，我们需要调整数字后面的空格

	QString label;
	if (cycle < 0) {
		// 负数周期：如"周期-1:"
		label = QStringLiteral("周期%1:").arg(cycle);
	}
	else if (cycle == 0) {
		// 周期0：如"周期0 :"（在0后面加一个空格，使长度与"周期-1:"一致）
		label = QStringLiteral("周期0 :");
	}
	else {
		// 正数周期：如"周期1 :"（在1后面加两个空格，使长度与"周期-1:"一致）
		// 计算需要添加的空格数
		int digits = QString::number(cycle).length();
		if (digits == 1) {
			label = QStringLiteral("周期%1  :").arg(cycle);  // 一位数加两个空格
		}
		else {
			label = QStringLiteral("周期%1 :").arg(cycle);   // 两位数加一个空格
		}
	}

	// 确保所有周期标签长度一致（中文字符算1个字符）
	// "周期-1:" 长度为4（中文字符）+ 2（数字和负号）+ 1（冒号）= 7
	// 我们确保所有标签长度为7个字符
	const int targetLength = 7;
	if (label.length() < targetLength) {
		// 在冒号后面添加空格
		label = label.leftJustified(targetLength, QLatin1Char(' '));
	}

	return label;
}

QString StrategyPlotWidget::generateMultiLineHoverInfo(int seriesIndex) const
{
	if (seriesIndex < 0 || seriesIndex >= m_strategyDetails.size()) {
		return QStringLiteral("鼠标悬停在折线上查看详细信息...");
	}

	const auto& detailInfo = m_strategyDetails[seriesIndex];
	if (!detailInfo.result) {
		return QStringLiteral("鼠标悬停在折线上查看详细信息...");
	}

	// 使用QuantStrategyManager获取参数描述
	QString paramStr = getParamDescription(detailInfo.result->strategyMode, detailInfo.result->params);
	// 不再截断参数字符串，让QLabel自动换行显示

	// 生成多行信息
	QStringList lines;

	// 第一行：排名和参数
	lines.append(QStringLiteral("排名: %1-第%2个 | 参数: %3")
		.arg(detailInfo.rank)
		.arg(detailInfo.indexInRank + 1)
		.arg(paramStr));

	// 第二行：周期0对应的年化收益率和做T年化收益率
	if (detailInfo.result) {
		QString annualReturnStr = formatRate(detailInfo.result->annualReturn.toDouble());
		QString annualTReturnStr = formatRate(detailInfo.result->annualTReturn.toDouble());
		lines.append(QStringLiteral("%1年化收益率 %2  做T年化收益率 %3")
			.arg(formatCycleLabel(0))
			.arg(annualReturnStr)
			.arg(annualTReturnStr));
	}

	// 显示未来周期（从周期1开始）
	for (int i = 1; i <= DISPLAY_POSITIVE_CYCLES; i++) {
		if (i < detailInfo.futureResults.size()) {
			auto result = detailInfo.futureResults[i];
			if (result) {
				QString annualReturnStr = formatRate(result->annualReturn.toDouble());
				QString annualTReturnStr = formatRate(result->annualTReturn.toDouble());
				lines.append(QStringLiteral("%1年化收益率 %2  做T年化收益率 %3")
					.arg(formatCycleLabel(i))
					.arg(annualReturnStr)
					.arg(annualTReturnStr));
			}
			else {
				// 如果没有数据，显示空值
				lines.append(QStringLiteral("%1年化收益率 ------  做T年化收益率 ------")
					.arg(formatCycleLabel(i)));
			}
		}
		else {
			// 如果周期不存在，显示空行
			lines.append(QStringLiteral("%1年化收益率 ------  做T年化收益率 ------")
				.arg(formatCycleLabel(i)));
		}
	}

	// 显示历史周期（从周期-1开始）
	for (int i = 1; i <= DISPLAY_NEGATIVE_CYCLES; i++) {
		if (i < detailInfo.historyResults.size()) {
			auto result = detailInfo.historyResults[i];
			if (result) {
				QString annualReturnStr = formatRate(result->annualReturn.toDouble());
				QString annualTReturnStr = formatRate(result->annualTReturn.toDouble());
				lines.append(QStringLiteral("%1年化收益率 %2  做T年化收益率 %3")
					.arg(formatCycleLabel(-i))
					.arg(annualReturnStr)
					.arg(annualTReturnStr));
			}
			else {
				// 如果没有数据，显示空值
				lines.append(QStringLiteral("%1年化收益率 ------  做T年化收益率 ------")
					.arg(formatCycleLabel(-i)));
			}
		}
		else {
			// 如果周期不存在，显示空行
			lines.append(QStringLiteral("%1年化收益率 ------  做T年化收益率 ------")
				.arg(formatCycleLabel(-i)));
		}
	}

	return lines.join(QStringLiteral("\n"));
}

QColor StrategyPlotWidget::getColorForRank(int rank, int totalRanks)
{
	// 颜色从高颜色渐变到低颜色
	// 排名1（收益最高）是高颜色，排名totalRanks（收益最低）是低颜色

	if (totalRanks <= 1)
	{
		// 只有一个排名时，使用中间颜色
		// 计算中间颜色：三个分量各取一半
		return QColor(g_initHighColor[0] / 2, g_initHighColor[1] / 2, g_initHighColor[2] / 2);
	}

	// 将排名映射到0-255的颜色索引
	// 注意：rank=1（最高收益）对应ratio=0（高颜色）
	// rank=totalRanks（最低收益）对应ratio=1（低颜色）
	double ratio = static_cast<double>(rank - 1) / (totalRanks - 1);
	int colorIndex = static_cast<int>(ratio * 255);
	colorIndex = qBound(0, colorIndex, 255);

	// 找到高颜色中哪个分量为255
	int highComponent = -1;
	for (int i = 0; i < 3; i++) {
		if (g_initHighColor[i] == 255) {
			highComponent = i;
			break;
		}
	}

	// 找到低颜色中哪个分量为255
	int lowComponent = -1;
	for (int i = 0; i < 3; i++) {
		if (g_initLowColor[i] == 255) {
			lowComponent = i;
			break;
		}
	}

	// 计算渐变颜色
	int color[3] = { 0, 0, 0 };

	// 高颜色分量从255渐变到0
	if (highComponent >= 0) {
		color[highComponent] = 255 - colorIndex;
	}

	// 低颜色分量从0渐变到255
	if (lowComponent >= 0) {
		color[lowComponent] = colorIndex;
	}

	// 第三个分量保持为0（根据规则，两个颜色的255分量不同，所以第三个分量始终为0）

	return QColor(color[0], color[1], color[2]);
}

void StrategyPlotWidget::calculateXRange(
	const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& historyData,
	const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& futureData,
	int& xMin, int& xMax)
{
	xMin = 0;
	xMax = 0;

	// 检查历史周期数据
	if (!historyData.empty())
	{
		const auto& firstGroup = historyData.begin()->second;
		if (!firstGroup.empty() && !firstGroup[0].empty())
		{
			// 历史周期数量（包括当前周期）
			int historyCount = static_cast<int>(firstGroup[0].size());
			if (historyCount > 0)
			{
				// 历史周期：从最远的历史周期到当前周期
				// 最远的历史周期对应的X值为-(historyCount-1)，当前周期为0
				xMin = -(historyCount - 1);
			}
		}
	}

	// 检查未来周期数据
	if (!futureData.empty())
	{
		const auto& secondGroup = futureData.begin()->second;
		if (!secondGroup.empty() && !secondGroup[0].empty())
		{
			// 未来周期数量（包括当前周期）
			int futureCount = static_cast<int>(secondGroup[0].size());
			if (futureCount > 0)
			{
				// 未来周期：当前周期为0，未来周期从1开始
				xMax = futureCount - 1;
			}
		}
	}

	// 确保至少有一定范围
	if (xMin == 0 && xMax == 0)
	{
		xMin = -3;
		xMax = 3;
	}

	// 添加一些边距
	xMin -= 1;
	xMax += 1;
}

void StrategyPlotWidget::analyzeStrategyData(const std::pair <
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> > & dataPair)
{
	// 清空现有数据
	m_sortedKeys.clear();
	m_rankStrategyCounts.clear();
	m_rankRanges.clear();
	m_strategyDetails.clear();
	m_colorBarRankCounts.clear();
	m_colorBarRankColors.clear();
	m_colorBarToDetailIndexMap.clear();
	m_totalStrategyCount = 0;
	m_totalRankCount = 0;

	// map的key是实际收益，从小到大排序（收益低的在前，收益高的在后）
	// 但排名1应该是收益最高的，排名最后是收益最低的
	// 所以我们需要反转排名：map.rbegin()是收益最高的（排名1），map.rend()是收益最低的
	// 但我们画图要从收益低的开始画（蓝色在底层），所以要从map.begin()开始遍历
	// 注意：排名1对应收益最高的（map.rbegin()），排名m_totalRankCount对应收益最低的（map.begin()）

	// 首先收集所有key，按收益从低到高排序（map默认排序）
	for (auto it = dataPair.first.begin(); it != dataPair.first.end(); ++it)
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
	calculateXRange(dataPair.first, dataPair.second, m_xMin, m_xMax);

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
	m_infoLabel->setText(QStringLiteral("共发现 %1 个策略，%2 个不同年化收益")
		.arg(m_totalStrategyCount)
		.arg(m_totalRankCount));
}

void StrategyPlotWidget::plotAllStrategies()
{
	// 清空图表数据
	m_annualReturnPlot->clear();
	m_annualTReturnPlot->clear();

	// 设置X轴范围
	m_annualReturnPlot->setXRange(m_xMin, m_xMax);
	m_annualTReturnPlot->setXRange(m_xMin, m_xMax);

	// 清空策略详细信息
	m_strategyDetails.clear();
	m_colorBarRankCounts.clear();
	m_colorBarRankColors.clear();
	m_colorBarToDetailIndexMap.clear();

	// 按排名顺序绘制所有策略（从收益低的开始绘制，在底层）
	// 注意：m_sortedKeys是按收益从低到高排列（map.begin()到map.end()）
	// rankIndex=0对应收益最低的（排名m_totalRankCount），rankIndex=m_totalRankCount-1对应收益最高的（排名1）
	for (int rankIndex = 0; rankIndex < m_totalRankCount; rankIndex++)
	{
		int32_t key = m_sortedKeys[rankIndex];
		// 实际排名：收益最高的排名1，收益最低的排名m_totalRankCount
		int actualRank = m_totalRankCount - rankIndex; // rankIndex=0是收益最低的，所以actualRank=m_totalRankCount

		// 获取该排名对应的策略数据
		auto itHistory = m_currentData.first.find(key);
		if (itHistory == m_currentData.first.end())
		{
			continue;
		}

		const auto& historyStrategyGroups = itHistory->second;
		int strategyCountInRank = static_cast<int>(historyStrategyGroups.size());

		// 获取对应的未来周期数据
		auto itFuture = m_currentData.second.find(key);
		const auto& futureStrategyGroups = (itFuture != m_currentData.second.end()) ?
			itFuture->second :
			std::vector<std::vector<std::shared_ptr<StrategyResult>>>();

		// 该排名对应的颜色（收益高的红色，收益低的绿色）
		// actualRank=1是收益最高的（红色），actualRank=m_totalRankCount是收益最低的（绿色）
		QColor rankColor = getColorForRank(actualRank, m_totalRankCount);

		// 注意：我们需要从左到右（排名第一的第一个、第二个...）绘制颜色条
		// 但画折线图时，我们要从收益低的开始画（在底层）
		// 所以这里我们按照正常的顺序（从0到strategyCountInRank-1）处理策略
		for (int strategyIndexInRank = 0; strategyIndexInRank < strategyCountInRank; strategyIndexInRank++)
		{
			// 获取历史周期数据
			std::vector<std::shared_ptr<StrategyResult>> historyGroup;
			if (strategyIndexInRank < historyStrategyGroups.size())
			{
				historyGroup = historyStrategyGroups[strategyIndexInRank];
			}

			// 获取未来周期数据
			std::vector<std::shared_ptr<StrategyResult>> futureGroup;
			if (strategyIndexInRank < futureStrategyGroups.size())
			{
				futureGroup = futureStrategyGroups[strategyIndexInRank];
			}

			if (historyGroup.empty())
			{
				continue;
			}

			// 获取当前周期结果用于生成名称
			auto currentResult = historyGroup[0]; // historyGroup[0]是当前周期
			if (!currentResult)
			{
				continue;
			}

			// 保存策略详细信息
			StrategyDetailInfo detailInfo;
			detailInfo.rank = actualRank;
			detailInfo.indexInRank = strategyIndexInRank;
			detailInfo.result = currentResult;
			detailInfo.color = rankColor;  // 保存颜色

			// 保存历史周期结果（从当前周期0到历史最远周期）
			// historyGroup[0]是当前周期，historyGroup[1]是前一个周期（-1），historyGroup[2]是前两个周期（-2）...
			detailInfo.historyResults = historyGroup;

			// 保存未来周期结果（从当前周期0到未来最远周期）
			// futureGroup[0]是当前周期，futureGroup[1]是下一个周期（1），futureGroup[2]是下两个周期（2）...
			detailInfo.futureResults = futureGroup;

			m_strategyDetails.append(detailInfo);

			// 构建年化收益率数据点
			QVector<QPointF> annualReturnPoints;
			QVector<QPointF> annualTReturnPoints;

			// 获取历史周期和未来周期的总点数
			int historyCount = static_cast<int>(historyGroup.size());
			int futureCount = static_cast<int>(futureGroup.size());

			// 首先添加历史周期数据点：从最早的历史周期到当前周期
			// historyGroup[historyCount-1]是历史最早周期，historyGroup[0]是当前周期
			// X坐标：历史最早周期应该是-(historyCount-1)，当前周期是0
			for (int i = historyCount - 1; i >= 0; i--)
			{
				auto result = historyGroup[i];
				if (!result)
				{
					continue;
				}

				// X坐标：从-(historyCount-1)到0
				// 当i=historyCount-1时，xValue=-(historyCount-1)
				// 当i=0时，xValue=0
				double xValue = -static_cast<double>(i);
				double yValueAnnual = result->annualReturn.toDouble();
				double yValueAnnualT = result->annualTReturn.toDouble();

				annualReturnPoints.append(QPointF(xValue, yValueAnnual));
				annualTReturnPoints.append(QPointF(xValue, yValueAnnualT));
			}

			// 然后添加未来周期数据点：从下一个周期到最远的未来周期
			// futureGroup[0]是当前周期（已包含），futureGroup[1]是下一个周期
			for (int i = 1; i < futureCount; i++)
			{
				auto result = futureGroup[i];
				if (!result)
				{
					continue;
				}

				// X坐标：1, 2, 3, ..., (futureCount-1)
				double xValue = static_cast<double>(i);
				double yValueAnnual = result->annualReturn.toDouble();
				double yValueAnnualT = result->annualTReturn.toDouble();

				annualReturnPoints.append(QPointF(xValue, yValueAnnual));
				annualTReturnPoints.append(QPointF(xValue, yValueAnnualT));
			}

			// 生成策略名称
			QString strategyName = QStringLiteral("策略排名%1-第%2个").arg(actualRank).arg(strategyIndexInRank + 1);

			// 添加到图表
			m_annualReturnPlot->addSeries(strategyName, annualReturnPoints, rankColor);
			m_annualTReturnPlot->addSeries(strategyName, annualTReturnPoints, rankColor);
		}
	}

	// 构建颜色条需要的数据（从高收益到低收益）
	for (int rank = 1; rank <= m_totalRankCount; rank++)
	{
		// 计算该排名在m_rankStrategyCounts中的索引
		int rankIndex = m_totalRankCount - rank;
		if (rankIndex >= 0 && rankIndex < m_rankStrategyCounts.size())
		{
			m_colorBarRankCounts.append(m_rankStrategyCounts[rankIndex]);
			QColor rankColor = getColorForRank(rank, m_totalRankCount);
			m_colorBarRankColors.append(rankColor);
		}
	}

	// 构建颜色条索引到策略详情索引的映射
	// 颜色条索引：从高收益到低收益
	// 策略详情索引：从低收益到高收益
	for (int rank = 1; rank <= m_totalRankCount; rank++)
	{
		// 计算该排名在m_rankStrategyCounts中的索引
		int rankIndex = m_totalRankCount - rank;
		if (rankIndex >= 0 && rankIndex < m_rankRanges.size())
		{
			const auto& range = m_rankRanges[rankIndex];
			int startIdx = range.first;
			int endIdx = range.second;

			// 将该排名的所有策略按顺序（从第一个到最后一个）添加到映射
			for (int i = startIdx; i <= endIdx; i++)
			{
				m_colorBarToDetailIndexMap.append(i);
			}
		}
	}

	// 自动调整Y轴范围
	m_annualReturnPlot->autoRange();
	m_annualTReturnPlot->autoRange();
}

void StrategyPlotWidget::updateColorBar()
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
	if (startRank < 1) startRank = 1;
	if (endRank > m_totalRankCount) endRank = m_totalRankCount;
	if (startRank > endRank) std::swap(startRank, endRank);

	// 计算显示的总策略数
	int totalVisibleStrategies = 0;
	QVector<int> visibleRankCounts;
	QVector<QColor> visibleRankColors;

	// 构建当前显示的颜色条数据
	for (int rank = startRank; rank <= endRank; rank++)
	{
		// 计算该排名在颜色条数据中的索引
		int colorBarIndex = rank - 1;
		if (colorBarIndex >= 0 && colorBarIndex < m_colorBarRankCounts.size())
		{
			visibleRankCounts.append(m_colorBarRankCounts[colorBarIndex]);
			visibleRankColors.append(m_colorBarRankColors[colorBarIndex]);
			totalVisibleStrategies += m_colorBarRankCounts[colorBarIndex];
		}
	}

	// 设置颜色条数据
	m_colorBar->setColorData(visibleRankCounts, visibleRankColors, totalVisibleStrategies);
}

void StrategyPlotWidget::setStrategyData(const std::pair <
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> > & dataPair)
{
	// 保存数据
	m_currentData = dataPair;

	// 分析策略数据，计算排名信息
	analyzeStrategyData(dataPair);

	// 绘制所有策略
	plotAllStrategies();

	// 更新策略显示
	updateStrategyVisibility();

	// 更新颜色条宽度
	updateColorBarWidth();
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
	if (startRank < 1) startRank = 1;
	if (endRank > m_totalRankCount) endRank = m_totalRankCount;
	if (startRank > endRank) std::swap(startRank, endRank);

	// 计算可见策略的总数
	int visibleStrategyCount = 0;
	for (int rank = startRank; rank <= endRank; rank++)
	{
		// 计算该排名在m_rankStrategyCounts中的索引
		int rankIndex = m_totalRankCount - rank;
		if (rankIndex >= 0 && rankIndex < m_rankStrategyCounts.size())
		{
			visibleStrategyCount += m_rankStrategyCounts[rankIndex];
		}
	}

	// 更新信息标签
	m_infoLabel->setText(QStringLiteral("共发现 %1 个策略，显示排名 %2-%3 (%4 个策略)")
		.arg(m_totalStrategyCount)
		.arg(startRank)
		.arg(endRank)
		.arg(visibleStrategyCount));

	// 更新图表中的策略可见性
	// 注意：m_strategyDetails中的策略索引顺序是从收益低到高
	// 我们需要设置每个策略的可见性
	for (int strategyIndex = 0; strategyIndex < m_strategyDetails.size(); strategyIndex++)
	{
		// 获取该策略的排名
		const auto& detail = m_strategyDetails[strategyIndex];
		int rank = detail.rank;

		// 判断该策略是否在可见范围内
		bool isVisible = (rank >= startRank && rank <= endRank);

		if (m_showAnnualReturn)
		{
			m_annualReturnPlot->setSeriesVisible(strategyIndex, isVisible);
		}
		else
		{
			m_annualReturnPlot->setSeriesVisible(strategyIndex, false);
		}

		if (m_showAnnualTReturn)
		{
			m_annualTReturnPlot->setSeriesVisible(strategyIndex, isVisible);
		}
		else
		{
			m_annualTReturnPlot->setSeriesVisible(strategyIndex, false);
		}
	}

	// 强制重绘
	m_annualReturnPlot->update();
	m_annualTReturnPlot->update();

	// 更新颜色条
	updateColorBar();
}

void StrategyPlotWidget::updateLayout()
{
	// 清空当前布局
	QLayoutItem* item;
	while ((item = m_plotLayout->takeAt(0)) != nullptr)
	{
		if (item->widget())
		{
			item->widget()->setParent(nullptr);
		}
		delete item;
	}

	// 获取图表控件
	QWidget* annualReturnWidget = m_annualReturnPlot->parentWidget();
	QWidget* annualTReturnWidget = m_annualTReturnPlot->parentWidget();

	// 设置图表最小高度
	int baseHeight = 400;
	int expandedHeight = static_cast<int>(baseHeight * LAYOUT_HEIGHT_FACTOR);

	if (m_currentLayout == 0) // 左右布局
	{
		// 设置图表高度为原来高度的1.5倍
		if (m_showAnnualReturn && m_showAnnualTReturn)
		{
			m_annualReturnPlot->setMinHeight(expandedHeight);
			m_annualTReturnPlot->setMinHeight(expandedHeight);

			m_plotLayout->addWidget(annualReturnWidget, 1);
			m_plotLayout->addWidget(annualTReturnWidget, 1);
			annualReturnWidget->show();
			annualTReturnWidget->show();
		}
		else if (m_showAnnualReturn)
		{
			m_annualReturnPlot->setMinHeight(expandedHeight);

			m_plotLayout->addWidget(annualReturnWidget);
			annualReturnWidget->show();
			annualTReturnWidget->hide();
		}
		else if (m_showAnnualTReturn)
		{
			m_annualTReturnPlot->setMinHeight(expandedHeight);

			m_plotLayout->addWidget(annualTReturnWidget);
			annualReturnWidget->hide();
			annualTReturnWidget->show();
		}
	}
	else // 上下布局
	{
		// 恢复图表高度为默认值
		m_annualReturnPlot->setMinHeight(baseHeight);
		m_annualTReturnPlot->setMinHeight(baseHeight);

		if (m_showAnnualReturn && m_showAnnualTReturn)
		{
			QVBoxLayout* verticalLayout = new QVBoxLayout();
			verticalLayout->setSpacing(10);
			verticalLayout->addWidget(annualReturnWidget, 1);
			verticalLayout->addWidget(annualTReturnWidget, 1);
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

	// 更新颜色条宽度
	updateColorBarWidth();

	// 更新整个窗口布局
	updateGeometry();
}

void StrategyPlotWidget::updateColorBarWidth()
{
	// 颜色条宽度只与主Widget的宽度相关，减去左右边距
	int mainLayoutLeftMargin, mainLayoutRightMargin;
	m_mainLayout->getContentsMargins(&mainLayoutLeftMargin, nullptr, &mainLayoutRightMargin, nullptr);
	int colorBarWidth = width() - mainLayoutLeftMargin - mainLayoutRightMargin;

	if (colorBarWidth > 0)
	{
		m_colorBar->setMinimumWidth(colorBarWidth);
		m_colorBar->setMaximumWidth(colorBarWidth);
	}
}

void StrategyPlotWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	// 窗口大小改变时，更新颜色条宽度
	updateColorBarWidth();
}

void StrategyPlotWidget::onRankTextChanged()
{
	// 重启定时器，延迟更新
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
	updateStrategyVisibility();
}

void StrategyPlotWidget::onAnnualTReturnVisibleChanged(int state)
{
	m_showAnnualTReturn = (state == Qt::Checked);
	updateLayout();
	updateStrategyVisibility();
}

void StrategyPlotWidget::onLayoutChanged(int index)
{
	m_currentLayout = index;
	updateLayout();
}

void StrategyPlotWidget::onAnnualReturnHoverInfoChanged(int seriesIndex, const QString& info)
{
	QString hoverText = generateMultiLineHoverInfo(seriesIndex);

	if (seriesIndex >= 0) {
		m_hoverInfoLabel->setText(hoverText);
		m_hoverInfoLabel->setStyleSheet(QStringLiteral("QLabel { background-color: #fff8f8; border: 2px solid #ffcccc; border-radius: 4px; padding: 5px; font-size: 10pt; }"));

		// 在颜色条上设置悬停策略
		// 需要将策略详情索引转换为颜色条索引
		int colorBarIndex = -1;
		for (int i = 0; i < m_colorBarToDetailIndexMap.size(); i++)
		{
			if (m_colorBarToDetailIndexMap[i] == seriesIndex)
			{
				colorBarIndex = i;
				break;
			}
		}

		if (colorBarIndex >= 0)
		{
			m_colorBar->setHoveredStrategy(colorBarIndex);
		}

		// 在另一个图表中设置相同的悬停系列
		m_annualTReturnPlot->setHoveredSeries(seriesIndex);
		m_annualReturnPlot->setHoveredSeries(seriesIndex);
	}
	else {
		m_hoverInfoLabel->setText(QStringLiteral("鼠标悬停在折线上查看详细信息..."));
		m_hoverInfoLabel->setStyleSheet(QStringLiteral("QLabel { background-color: #f8f8f8; border: 1px solid #ccc; border-radius: 4px; padding: 5px; font-size: 10pt; }"));

		// 清除颜色条悬停状态
		m_colorBar->setHoveredStrategy(-1);

		// 清除图表悬停状态
		m_annualTReturnPlot->setHoveredSeries(-1);
		m_annualReturnPlot->setHoveredSeries(-1);
	}
}

void StrategyPlotWidget::onAnnualTReturnHoverInfoChanged(int seriesIndex, const QString& info)
{
	QString hoverText = generateMultiLineHoverInfo(seriesIndex);

	if (seriesIndex >= 0) {
		m_hoverInfoLabel->setText(hoverText);
		m_hoverInfoLabel->setStyleSheet(QStringLiteral("QLabel { background-color: #f8fff8; border: 2px solid #ccffcc; border-radius: 4px; padding: 5px; font-size: 10pt; }"));

		// 在颜色条上设置悬停策略
		// 需要将策略详情索引转换为颜色条索引
		int colorBarIndex = -1;
		for (int i = 0; i < m_colorBarToDetailIndexMap.size(); i++)
		{
			if (m_colorBarToDetailIndexMap[i] == seriesIndex)
			{
				colorBarIndex = i;
				break;
			}
		}

		if (colorBarIndex >= 0)
		{
			m_colorBar->setHoveredStrategy(colorBarIndex);
		}

		// 在另一个图表中设置相同的悬停系列
		m_annualReturnPlot->setHoveredSeries(seriesIndex);
		m_annualTReturnPlot->setHoveredSeries(seriesIndex);
	}
	else {
		m_hoverInfoLabel->setText(QStringLiteral("鼠标悬停在折线上查看详细信息..."));
		m_hoverInfoLabel->setStyleSheet(QStringLiteral("QLabel { background-color: #f8f8f8; border: 1px solid #ccc; border-radius: 4px; padding: 5px; font-size: 10pt; }"));

		// 清除颜色条悬停状态
		m_colorBar->setHoveredStrategy(-1);

		// 清除图表悬停状态
		m_annualReturnPlot->setHoveredSeries(-1);
		m_annualTReturnPlot->setHoveredSeries(-1);
	}
}

void StrategyPlotWidget::onColorBarHoverStrategyChanged(int strategyIndex)
{
	if (strategyIndex >= 0 && strategyIndex < m_colorBarToDetailIndexMap.size())
	{
		// 将颜色条索引转换为策略详情索引
		int detailIndex = m_colorBarToDetailIndexMap[strategyIndex];

		if (detailIndex >= 0 && detailIndex < m_strategyDetails.size())
		{
			// 生成悬停信息
			QString hoverText = generateMultiLineHoverInfo(detailIndex);
			m_hoverInfoLabel->setText(hoverText);
			m_hoverInfoLabel->setStyleSheet(QStringLiteral("QLabel { background-color: #fff8f8; border: 2px solid #ffcccc; border-radius: 4px; padding: 5px; font-size: 10pt; }"));

			// 在两个图表中设置悬停系列
			m_annualReturnPlot->setHoveredSeries(detailIndex);
			m_annualTReturnPlot->setHoveredSeries(detailIndex);
		}
	}
	else
	{
		m_hoverInfoLabel->setText(QStringLiteral("鼠标悬停在折线上查看详细信息..."));
		m_hoverInfoLabel->setStyleSheet(QStringLiteral("QLabel { background-color: #f8f8f8; border: 1px solid #ccc; border-radius: 4px; padding: 5px; font-size: 10pt; }"));

		// 清除图表悬停状态
		m_annualReturnPlot->setHoveredSeries(-1);
		m_annualTReturnPlot->setHoveredSeries(-1);
	}
}