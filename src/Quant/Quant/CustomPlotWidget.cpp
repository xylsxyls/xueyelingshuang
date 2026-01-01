#include "CustomPlotWidget.h"
#include <QPainter>
#include <QToolTip>
#include <QDebug>
#include <QFontMetrics>
#include <cmath>

// 全局颜色变量定义（初始值为红色最高，绿色最低，蓝色悬停）
unsigned char g_initHighColor[3] = { 255, 0, 0 };   // 红色
unsigned char g_initLowColor[3] = { 0, 255, 0 };    // 绿色
unsigned char g_hoverColor[3] = { 0, 0, 255 };      // 蓝色

// 全局线条粗细定义
double g_normalLineWidth = 0.5;                   // 正常线条粗细
double g_hoverLineWidth = 2.0;                    // 悬停线条粗细

// 验证颜色变量是否有效
bool validateColorVariables()
{
	// 验证高颜色：只有一个分量为255，其他为0
	int highColorCount = 0;
	for (int i = 0; i < 3; i++) {
		if (g_initHighColor[i] == 255) highColorCount++;
		else if (g_initHighColor[i] != 0) return false;
	}
	if (highColorCount != 1) return false;

	// 验证低颜色：只有一个分量为255，其他为0
	int lowColorCount = 0;
	for (int i = 0; i < 3; i++) {
		if (g_initLowColor[i] == 255) lowColorCount++;
		else if (g_initLowColor[i] != 0) return false;
	}
	if (lowColorCount != 1) return false;

	// 验证悬停颜色：只有一个分量为255，其他为0
	int hoverColorCount = 0;
	for (int i = 0; i < 3; i++) {
		if (g_hoverColor[i] == 255) hoverColorCount++;
		else if (g_hoverColor[i] != 0) return false;
	}
	if (hoverColorCount != 1) return false;

	// 验证高颜色和低颜色的255分量不是同一个
	for (int i = 0; i < 3; i++) {
		if (g_initHighColor[i] == 255 && g_initLowColor[i] == 255) return false;
	}

	return true;
}

CustomPlotWidget::CustomPlotWidget(QWidget* parent)
	: QWidget(parent)
	, m_title(QStringLiteral("图表"))
	, m_xLabel(QStringLiteral("X轴"))
	, m_yLabel(QStringLiteral("Y轴"))
	, m_xMin(-3.0)
	, m_xMax(3.0)
	, m_yMin(-0.1)
	, m_yMax(0.1)
	, m_originalYMin(-0.1)
	, m_originalYMax(0.1)
	, m_leftMargin(80)
	, m_rightMargin(60)
	, m_topMargin(50)
	, m_bottomMargin(80)
	, m_isDragging(false)
	, m_hoveredSeries(-1)
{
	// 验证颜色变量
	if (!validateColorVariables()) {
		qWarning() << "颜色变量验证失败，使用默认颜色方案";
		// 重置为默认值
		g_initHighColor[0] = 255; g_initHighColor[1] = 0; g_initHighColor[2] = 0;  // 红
		g_initLowColor[0] = 0; g_initLowColor[1] = 255; g_initLowColor[2] = 0;    // 绿
		g_hoverColor[0] = 0; g_hoverColor[1] = 0; g_hoverColor[2] = 255;          // 蓝
	}

	setMouseTracking(true);
	setMinimumSize(500, 400);

	// 创建重置按钮
	m_resetButton = new QPushButton(QStringLiteral("复原"), this);
	m_resetButton->setFixedSize(60, 30);

	// 连接按钮点击信号
	connect(m_resetButton, &QPushButton::clicked, this, &CustomPlotWidget::onResetButtonClicked);
}

void CustomPlotWidget::addSeries(const QString& name, const QVector<QPointF>& points, const QColor& color)
{
	Series series;
	series.name = name;
	series.data = points;
	series.color = color;
	series.visible = true;

	m_series.append(series);

	// 更新坐标轴范围
	autoRange();
	update();
}

void CustomPlotWidget::setAxisLabels(const QString& xLabel, const QString& yLabel)
{
	m_xLabel = xLabel;
	m_yLabel = yLabel;
	update();
}

void CustomPlotWidget::setTitle(const QString& title)
{
	m_title = title;
	update();
}

void CustomPlotWidget::clear()
{
	m_series.clear();
	m_xMin = -3.0;
	m_xMax = 3.0;
	m_yMin = -0.1;
	m_yMax = 0.1;
	m_originalYMin = -0.1;
	m_originalYMax = 0.1;
	m_hoveredSeries = -1;
	update();
}

void CustomPlotWidget::autoRange()
{
	// 如果系列为空，直接返回
	if (m_series.isEmpty())
	{
		return;
	}

	bool first = true;

	// 遍历所有系列，更新Y轴范围
	for (const auto& series : m_series)
	{
		if (!series.visible || series.data.isEmpty())
		{
			continue;
		}

		for (const auto& point : series.data)
		{
			if (first)
			{
				m_yMin = m_yMax = point.y();
				first = false;
			}
			else
			{
				if (point.y() < m_yMin)
				{
					m_yMin = point.y();
				}
				if (point.y() > m_yMax)
				{
					m_yMax = point.y();
				}
			}
		}
	}

	// 保存原始Y轴范围
	m_originalYMin = m_yMin;
	m_originalYMax = m_yMax;

	// 如果数据范围太小，设置一个最小范围
	if (fabs(m_yMax - m_yMin) < 0.01)
	{
		m_yMin -= 0.01;
		m_yMax += 0.01;
		m_originalYMin = m_yMin;
		m_originalYMax = m_yMax;
	}

	// 调整Y轴范围，使刻度为10的整数倍
	adjustYRange();

	// 添加一些边距
	double yRange = m_yMax - m_yMin;

	if (yRange < 1e-10)
	{
		yRange = 1.0;
	}

	m_yMin -= yRange * 0.05;
	m_yMax += yRange * 0.05;
	m_originalYMin = m_yMin;
	m_originalYMax = m_yMax;
}

void CustomPlotWidget::adjustYRange()
{
	// 调整Y轴范围，使最小值和最大值都是0.1的整数倍
	// 0.1的整数倍就是10%的整数倍
	double minMultiple = 0.1;
	m_yMin = floor(m_yMin / minMultiple) * minMultiple;
	m_yMax = ceil(m_yMax / minMultiple) * minMultiple;
}

void CustomPlotWidget::setXRange(int min, int max)
{
	m_xMin = static_cast<double>(min);
	m_xMax = static_cast<double>(max);
	update();
}

void CustomPlotWidget::setSeriesVisible(int seriesIndex, bool visible)
{
	if (seriesIndex >= 0 && seriesIndex < m_series.size())
	{
		m_series[seriesIndex].visible = visible;
		update();
	}
}

void CustomPlotWidget::setHoveredSeries(int seriesIndex)
{
	if (m_hoveredSeries != seriesIndex)
	{
		m_hoveredSeries = seriesIndex;
		update();
	}
}

void CustomPlotWidget::setMinHeight(int height)
{
	setMinimumHeight(height);
}

QVector<QPointF> CustomPlotWidget::seriesData(int seriesIndex) const
{
	if (seriesIndex >= 0 && seriesIndex < m_series.size())
	{
		return m_series[seriesIndex].data;
	}
	return QVector<QPointF>();
}

void CustomPlotWidget::resetYAxis()
{
	// 计算新的Y轴范围，使Y=0在中间
	double yRange = m_yMax - m_yMin;
	double center = 0.0;
	m_yMin = center - yRange / 2.0;
	m_yMax = center + yRange / 2.0;
	// 调整Y轴范围，使刻度为0.1的整数倍
	adjustYRange();
	update();
}

double CustomPlotWidget::pointToLineDistance(const QPointF& point, const QPointF& lineStart, const QPointF& lineEnd) const
{
	// 计算点到线段的最短距离
	double lineLength = QLineF(lineStart, lineEnd).length();
	if (lineLength < 1e-6)
	{
		return QLineF(point, lineStart).length();
	}

	// 计算投影参数t
	double t = ((point.x() - lineStart.x()) * (lineEnd.x() - lineStart.x()) +
		(point.y() - lineStart.y()) * (lineEnd.y() - lineStart.y())) /
		(lineLength * lineLength);
	t = qBound(0.0, t, 1.0);

	// 计算投影点
	QPointF projection(lineStart.x() + t * (lineEnd.x() - lineStart.x()),
		lineStart.y() + t * (lineEnd.y() - lineStart.y()));

	return QLineF(point, projection).length();
}

bool CustomPlotWidget::findNearestSeries(const QPoint& mousePos, int& seriesIndex) const
{
	double minDistance = 15.0;  // 像素距离阈值
	bool found = false;

	// 遍历所有系列
	for (int i = 0; i < m_series.size(); i++)
	{
		const Series& series = m_series[i];
		if (!series.visible || series.data.size() < 2)
		{
			continue;
		}

		// 检查每个线段
		for (int j = 0; j < series.data.size() - 1; j++)
		{
			QPointF p1 = dataToWidget(series.data[j]);
			QPointF p2 = dataToWidget(series.data[j + 1]);

			// 跳过无效点
			if (qIsNaN(p1.x()) || qIsNaN(p1.y()) || qIsNaN(p2.x()) || qIsNaN(p2.y()))
			{
				continue;
			}

			// 计算鼠标点到线段的距离
			double distance = pointToLineDistance(mousePos, p1, p2);
			if (distance < minDistance)
			{
				minDistance = distance;
				seriesIndex = i;
				found = true;
			}
		}
	}

	return found;
}

void CustomPlotWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制背景
	painter.fillRect(rect(), QColor(250, 250, 250));

	// 计算绘图区域
	m_plotRect = rect().adjusted(m_leftMargin, m_topMargin, -m_rightMargin, -m_bottomMargin);

	// 绘制标题
	painter.setPen(QColor(50, 50, 50));
	QFont titleFont = painter.font();
	titleFont.setPointSize(14);
	titleFont.setBold(true);
	painter.setFont(titleFont);
	QRect titleRect(0, 10, width(), 30);
	painter.drawText(titleRect, Qt::AlignCenter, m_title);

	// 绘制网格
	drawGrid(painter);

	// 绘制坐标轴
	drawAxes(painter);

	// 绘制正常折线
	drawSeries(painter);

	// 绘制悬停折线（在最上层）
	drawHoverSeries(painter);

	// 更新重置按钮位置（右下角，绘图区域外部）
	int buttonX = m_plotRect.right() - m_resetButton->width();
	int buttonY = m_plotRect.bottom() + 30; // 放在绘图区域下方，给X轴标签留空间
	m_resetButton->move(buttonX, buttonY);
}

void CustomPlotWidget::drawAxes(QPainter& painter)
{
	painter.save();

	// 设置坐标轴颜色
	QColor axisColor(100, 100, 100);
	painter.setPen(QPen(axisColor, 2));

	// 计算Y轴在X=0时的位置
	int yAxisAtZero = calculateYAxisAtZero();
	int xAxisAtZero = calculateXAxisAtZero();

	// 绘制X轴（在Y=0的位置）
	if (m_yMin <= 0 && m_yMax >= 0)
	{
		painter.drawLine(m_plotRect.left(), yAxisAtZero, m_plotRect.right(), yAxisAtZero);
	}

	// 绘制X轴箭头
	if (m_yMin <= 0 && m_yMax >= 0)
	{
		painter.setBrush(axisColor);
		QPointF xArrow[] = {
			QPointF(m_plotRect.right(), yAxisAtZero),
			QPointF(m_plotRect.right() - 10, yAxisAtZero - 5),
			QPointF(m_plotRect.right() - 10, yAxisAtZero + 5),
			QPointF(m_plotRect.right(), yAxisAtZero)
		};
		painter.drawPolygon(xArrow, 4);
	}

	// 绘制Y轴（在X=0的位置）
	painter.drawLine(xAxisAtZero, m_plotRect.top(), xAxisAtZero, m_plotRect.bottom());

	// 绘制Y轴箭头
	painter.setBrush(axisColor);
	QPointF yArrow[] = {
		QPointF(xAxisAtZero, m_plotRect.top()),
		QPointF(xAxisAtZero - 5, m_plotRect.top() + 10),
		QPointF(xAxisAtZero + 5, m_plotRect.top() + 10),
		QPointF(xAxisAtZero, m_plotRect.top())
	};
	painter.drawPolygon(yArrow, 4);

	// 绘制坐标轴标签
	painter.setPen(QColor(70, 70, 70));
	QFont labelFont = painter.font();
	labelFont.setPointSize(10);
	painter.setFont(labelFont);
	QFontMetrics fm(labelFont);

	// X轴标签
	int xLabelWidth = fm.width(m_xLabel);
	painter.drawText(m_plotRect.right() - xLabelWidth - 5,
		yAxisAtZero + (m_yMin <= 0 && m_yMax >= 0 ? 35 : -20), m_xLabel);

	// Y轴标签
	painter.save();
	painter.translate(m_plotRect.left() - 40, m_plotRect.top() + m_plotRect.height() / 2);
	painter.rotate(-90);
	painter.drawText(0, 0, m_yLabel);
	painter.restore();

	// 绘制刻度
	painter.setPen(QPen(axisColor, 1));

	// X轴刻度（只显示整数）
	int xMinInt = static_cast<int>(ceil(m_xMin));
	int xMaxInt = static_cast<int>(floor(m_xMax));

	for (int x = xMinInt; x <= xMaxInt; x++)
	{
		double xPos = m_plotRect.left() + (x - m_xMin) / (m_xMax - m_xMin) * m_plotRect.width();

		// 绘制刻度
		painter.drawLine(QPointF(xPos, yAxisAtZero - 3), QPointF(xPos, yAxisAtZero + 3));

		// 刻度值
		QString label = QString::number(x);
		int labelWidth = fm.width(label);
		painter.drawText(QPointF(xPos - labelWidth / 2, yAxisAtZero + 20), label);
	}

	// Y轴刻度：保证刻度是0.1的整数倍（10%的整数倍）
	double tickStep = 0.1; // 10%的步长
	double firstTick = ceil(m_yMin / tickStep) * tickStep;
	double lastTick = floor(m_yMax / tickStep) * tickStep;

	for (double tick = firstTick; tick <= lastTick; tick += tickStep)
	{
		double y = m_plotRect.bottom() - (tick - m_yMin) / (m_yMax - m_yMin) * m_plotRect.height();
		painter.drawLine(QPointF(xAxisAtZero - 3, y), QPointF(xAxisAtZero + 3, y));

		// 刻度值（乘以100显示百分比）
		double tickPercent = tick * 100.0;
		QString label = QString::number(tickPercent, 'f', 1) + QStringLiteral("%");
		int labelWidth = fm.width(label);
		painter.drawText(QPointF(xAxisAtZero - labelWidth - 8, y + 5), label);
	}

	// 在Y轴位置标注"当前周期"
	if (xAxisAtZero >= m_plotRect.left() && xAxisAtZero <= m_plotRect.right())
	{
		painter.setPen(QColor(200, 50, 50));
		painter.drawText(xAxisAtZero + 15, yAxisAtZero - 5, QStringLiteral("当前周期"));
	}

	painter.restore();
}

int CustomPlotWidget::calculateYAxisAtZero() const
{
	// 如果Y轴范围包含0，则计算0在绘图区域中的Y坐标
	if (m_yMin <= 0 && m_yMax >= 0)
	{
		double zeroYRatio = (0.0 - m_yMin) / (m_yMax - m_yMin);
		return m_plotRect.bottom() - static_cast<int>(zeroYRatio * m_plotRect.height());
	}
	else
	{
		// 如果0不在Y范围内，绘制在底部
		return m_plotRect.bottom();
	}
}

int CustomPlotWidget::calculateXAxisAtZero() const
{
	// 如果X轴范围包含0，则计算0在绘图区域中的X坐标
	if (m_xMin <= 0 && m_xMax >= 0)
	{
		double zeroXRatio = (0.0 - m_xMin) / (m_xMax - m_xMin);
		return m_plotRect.left() + static_cast<int>(zeroXRatio * m_plotRect.width());
	}
	else
	{
		// 如果0不在X范围内，绘制在左边
		return m_plotRect.left();
	}
}

void CustomPlotWidget::drawGrid(QPainter& painter)
{
	painter.save();

	QColor gridColor(230, 230, 230);
	painter.setPen(QPen(gridColor, 1));

	// X轴网格线（只显示整数位置）
	int xMinInt = static_cast<int>(ceil(m_xMin));
	int xMaxInt = static_cast<int>(floor(m_xMax));

	for (int x = xMinInt; x <= xMaxInt; x++)
	{
		double xPos = m_plotRect.left() + (x - m_xMin) / (m_xMax - m_xMin) * m_plotRect.width();
		painter.drawLine(QPointF(xPos, m_plotRect.top()), QPointF(xPos, m_plotRect.bottom()));
	}

	// Y轴网格线（0.1的整数倍）
	double tickStep = 0.1;
	double firstTick = ceil(m_yMin / tickStep) * tickStep;
	double lastTick = floor(m_yMax / tickStep) * tickStep;

	for (double tick = firstTick; tick <= lastTick; tick += tickStep)
	{
		double y = m_plotRect.bottom() - (tick - m_yMin) / (m_yMax - m_yMin) * m_plotRect.height();
		painter.drawLine(QPointF(m_plotRect.left(), y), QPointF(m_plotRect.right(), y));
	}

	// 在X=0的位置画一条明显的线
	int xAxisAtZero = calculateXAxisAtZero();
	if (xAxisAtZero >= m_plotRect.left() && xAxisAtZero <= m_plotRect.right())
	{
		painter.setPen(QPen(QColor(180, 180, 220), 1, Qt::DashLine));
		painter.drawLine(xAxisAtZero, m_plotRect.top(), xAxisAtZero, m_plotRect.bottom());
	}

	// 在Y=0的位置画一条明显的线
	int yAxisAtZero = calculateYAxisAtZero();
	if (yAxisAtZero >= m_plotRect.top() && yAxisAtZero <= m_plotRect.bottom())
	{
		painter.setPen(QPen(QColor(180, 220, 180), 1, Qt::DashLine));
		painter.drawLine(m_plotRect.left(), yAxisAtZero, m_plotRect.right(), yAxisAtZero);
	}

	painter.restore();
}

void CustomPlotWidget::drawSeries(QPainter& painter)
{
	painter.save();

	// 绘制所有系列（正常线条，线条粗细使用全局变量）
	for (int seriesIndex = 0; seriesIndex < m_series.size(); seriesIndex++)
	{
		const Series& series = m_series[seriesIndex];
		if (!series.visible || series.data.size() < 2)
		{
			continue;
		}

		// 绘制折线（线条粗细使用全局变量）
		QPen linePen(series.color, g_normalLineWidth);
		painter.setPen(linePen);

		// 连接点绘制折线
		for (int i = 0; i < series.data.size() - 1; i++)
		{
			QPointF p1 = dataToWidget(series.data[i]);
			QPointF p2 = dataToWidget(series.data[i + 1]);

			// 只在有效点之间绘制线
			if (!qIsNaN(p1.x()) && !qIsNaN(p1.y()) && !qIsNaN(p2.x()) && !qIsNaN(p2.y()))
			{
				painter.drawLine(p1, p2);
			}
		}

		// 绘制数据点（小圆点）
		painter.setBrush(series.color);
		for (int i = 0; i < series.data.size(); i++)
		{
			QPointF p = dataToWidget(series.data[i]);

			// 跳过无效点
			if (qIsNaN(p.x()) || qIsNaN(p.y()))
			{
				continue;
			}

			// 绘制小圆点（线条粗细的1.1倍）
			double radius = g_normalLineWidth * 1.1;
			painter.drawEllipse(p, radius, radius);
		}
	}

	painter.restore();
}

void CustomPlotWidget::drawHoverSeries(QPainter& painter)
{
	// 绘制悬停的系列（在最上面）
	if (m_hoveredSeries >= 0 && m_hoveredSeries < m_series.size())
	{
		const Series& series = m_series[m_hoveredSeries];
		if (series.visible && series.data.size() >= 2)
		{
			painter.save();

			// 悬停时使用悬停颜色（蓝色）
			QColor hoverColor(g_hoverColor[0], g_hoverColor[1], g_hoverColor[2]);
			QPen linePen(hoverColor, g_hoverLineWidth); // 悬停时线更粗
			painter.setPen(linePen);

			// 绘制折线
			for (int i = 0; i < series.data.size() - 1; i++)
			{
				QPointF p1 = dataToWidget(series.data[i]);
				QPointF p2 = dataToWidget(series.data[i + 1]);

				if (!qIsNaN(p1.x()) && !qIsNaN(p1.y()) && !qIsNaN(p2.x()) && !qIsNaN(p2.y()))
				{
					painter.drawLine(p1, p2);
				}
			}

			// 绘制数据点
			painter.setBrush(hoverColor);
			for (int i = 0; i < series.data.size(); i++)
			{
				QPointF p = dataToWidget(series.data[i]);
				if (!qIsNaN(p.x()) && !qIsNaN(p.y()))
				{
					double radius = g_hoverLineWidth * 1.1; // 悬停时点更大
					painter.drawEllipse(p, radius, radius);
				}
			}

			painter.restore();
		}
	}
}

QPointF CustomPlotWidget::dataToWidget(const QPointF& dataPoint) const
{
	// 将数据坐标转换为界面坐标
	double x = m_plotRect.left() +
		(dataPoint.x() - m_xMin) / (m_xMax - m_xMin) * m_plotRect.width();
	double y = m_plotRect.bottom() -
		(dataPoint.y() - m_yMin) / (m_yMax - m_yMin) * m_plotRect.height();
	return QPointF(x, y);
}

QPointF CustomPlotWidget::widgetToData(const QPoint& widgetPoint) const
{
	// 将界面坐标转换为数据坐标
	double x = m_xMin + (widgetPoint.x() - m_plotRect.left()) /
		(double)m_plotRect.width() * (m_xMax - m_xMin);
	double y = m_yMin + (m_plotRect.bottom() - widgetPoint.y()) /
		(double)m_plotRect.height() * (m_yMax - m_yMin);
	return QPointF(x, y);
}

void CustomPlotWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (m_isDragging)
	{
		// 只处理Y轴方向
		int deltaY = event->pos().y() - m_lastMousePos.y();

		// 平移视图（只影响Y轴）
		double dataDeltaY = -deltaY / (double)m_plotRect.height() * (m_yMax - m_yMin);

		m_yMin -= dataDeltaY;
		m_yMax -= dataDeltaY;

		m_lastMousePos = event->pos();
		update();
	}
	else
	{
		// 悬停检测
		int oldSeries = m_hoveredSeries;

		int seriesIndex = -1;
		if (findNearestSeries(event->pos(), seriesIndex))
		{
			if (m_hoveredSeries != seriesIndex)
			{
				m_hoveredSeries = seriesIndex;

				// 生成悬停信息
				const Series& series = m_series[m_hoveredSeries];
				QString hoverInfo = series.name; // 只传递策略名称，详细信息在StrategyPlotWidget中生成

				// 发射信号，包含系列索引
				emit hoverInfoChanged(m_hoveredSeries, hoverInfo);
				update();
			}
		}
		else
		{
			if (m_hoveredSeries != -1)
			{
				m_hoveredSeries = -1;
				// 发射空信息
				emit hoverInfoChanged(-1, QStringLiteral(""));
				update();
			}
		}
	}

	QWidget::mouseMoveEvent(event);
}

void CustomPlotWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton && m_plotRect.contains(event->pos()))
	{
		m_isDragging = true;
		m_lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
	}

	QWidget::mousePressEvent(event);
}

void CustomPlotWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_isDragging = false;
		setCursor(Qt::ArrowCursor);
	}

	QWidget::mouseReleaseEvent(event);
}

void CustomPlotWidget::wheelEvent(QWheelEvent* event)
{
	QPointF mouseDataPos = widgetToData(event->pos());

	// 缩放因子（只影响Y轴）
	double scaleFactor = 1.1;
	if (event->angleDelta().y() < 0)
	{
		scaleFactor = 1.0 / scaleFactor;
	}

	// 计算新的Y轴范围（以鼠标位置为中心）
	double newYRange = (m_yMax - m_yMin) * scaleFactor;

	m_yMin = mouseDataPos.y() - (mouseDataPos.y() - m_yMin) * scaleFactor;
	m_yMax = m_yMin + newYRange;

	// 调整Y轴范围，使刻度为0.1的整数倍（只在缩放时调整）
	adjustYRange();

	update();
	event->accept();
}

void CustomPlotWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	update();
}

void CustomPlotWidget::onResetButtonClicked()
{
	resetYAxis();
}