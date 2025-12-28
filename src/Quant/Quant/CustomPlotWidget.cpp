#include "CustomPlotWidget.h"
#include <QPainter>
#include <QToolTip>
#include <QDebug>
#include <QFontMetrics>
#include <cmath>

CustomPlotWidget::CustomPlotWidget(QWidget *parent)
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
	, m_rightMargin(30)
	, m_topMargin(50)
	, m_bottomMargin(60)
	, m_isDragging(false)
	, m_hoveredSeries(-1)
	, m_hoveredPoint(-1)
	, m_yAxisAtZeroPos(0)
	, m_fixedXRange(false)
{
	setMouseTracking(true);
	setMinimumSize(500, 400);

	// 创建重置按钮
	m_resetButton = new QPushButton(QStringLiteral("复原"), this);
	m_resetButton->setFixedSize(60, 30);

	// 连接按钮点击信号
	connect(m_resetButton, &QPushButton::clicked, this, &CustomPlotWidget::onResetButtonClicked);
}

void CustomPlotWidget::addSeries(const QString &name, const QVector<QPointF> &points, const QColor &color)
{
	Series series;
	series.name = name;
	series.data = points;
	// 只使用G和B分量，R分量为0
	series.color = QColor(0, color.green(), color.blue());
	series.visible = true;

	m_series.append(series);

	// 更新坐标轴范围
	autoRange();
	update();
}

void CustomPlotWidget::setAxisLabels(const QString &xLabel, const QString &yLabel)
{
	m_xLabel = xLabel;
	m_yLabel = yLabel;
	update();
}

void CustomPlotWidget::setTitle(const QString &title)
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
	m_fixedXRange = false;
	update();
}

void CustomPlotWidget::autoRange()
{
	if (m_series.isEmpty())
	{
		return;
	}

	bool first = true;

	// 更新Y轴范围
	for (const auto &series : m_series)
	{
		if (!series.visible || series.data.isEmpty())
		{
			continue;
		}

		for (const auto &point : series.data)
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

void CustomPlotWidget::setXRange(int min, int max)
{
	m_xMin = static_cast<double>(min);
	m_xMax = static_cast<double>(max);
	m_fixedXRange = true;
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

void CustomPlotWidget::resetYAxis()
{
	// 重置Y轴范围到原始范围
	m_yMin = m_originalYMin;
	m_yMax = m_originalYMax;
	update();
}

void CustomPlotWidget::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	// 绘制背景
	painter.fillRect(rect(), QColor(250, 250, 250));

	// 计算绘图区域
	m_plotRect = rect().adjusted(m_leftMargin, m_topMargin, -m_rightMargin, -m_bottomMargin);

	// 计算Y轴在X=0时的位置
	if (m_xMax > m_xMin)
	{
		double zeroRatio = (0.0 - m_xMin) / (m_xMax - m_xMin);
		m_yAxisAtZeroPos = m_plotRect.left() + static_cast<int>(zeroRatio * m_plotRect.width());
	}
	else
	{
		m_yAxisAtZeroPos = m_plotRect.left() + m_plotRect.width() / 2;
	}

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

	// 绘制折线
	drawSeries(painter);

	// 更新重置按钮位置
	m_resetButton->move(m_plotRect.right() - m_resetButton->width() - 5, m_plotRect.top() + 5);
}

void CustomPlotWidget::drawAxes(QPainter &painter)
{
	painter.save();

	// 设置坐标轴颜色
	QColor axisColor(100, 100, 100);
	painter.setPen(QPen(axisColor, 2));

	// 计算Y轴在X=0时的Y坐标位置
	int yAxisAtZero = calculateYAxisAtZero();

	// 绘制X轴（在Y=0的位置，如果0在范围内）
	painter.drawLine(m_plotRect.left(), yAxisAtZero, m_plotRect.right(), yAxisAtZero);

	// 绘制X轴箭头
	painter.setBrush(axisColor);
	QPointF xArrow[] = {
		QPointF(m_plotRect.right(), yAxisAtZero),
		QPointF(m_plotRect.right() - 10, yAxisAtZero - 5),
		QPointF(m_plotRect.right() - 10, yAxisAtZero + 5),
		QPointF(m_plotRect.right(), yAxisAtZero)
	};
	painter.drawPolygon(xArrow, 4);

	// 绘制Y轴（在X=0的位置）
	painter.drawLine(m_yAxisAtZeroPos, m_plotRect.top(), m_yAxisAtZeroPos, m_plotRect.bottom());

	// 绘制Y轴箭头
	painter.setBrush(axisColor);
	QPointF yArrow[] = {
		QPointF(m_yAxisAtZeroPos, m_plotRect.top()),
		QPointF(m_yAxisAtZeroPos - 5, m_plotRect.top() + 10),
		QPointF(m_yAxisAtZeroPos + 5, m_plotRect.top() + 10),
		QPointF(m_yAxisAtZeroPos, m_plotRect.top())
	};
	painter.drawPolygon(yArrow, 4);

	// 绘制坐标轴标签
	painter.setPen(QColor(70, 70, 70));
	QFont labelFont = painter.font();
	labelFont.setPointSize(10);
	painter.setFont(labelFont);

	// X轴标签
	QFontMetrics fm(labelFont);
	int xLabelWidth = fm.width(m_xLabel);
	painter.drawText(m_plotRect.right() - xLabelWidth - 5, yAxisAtZero + 35, m_xLabel);

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

	// Y轴刻度
	int yTickCount = 8;
	for (int i = 0; i <= yTickCount; i++)
	{
		double y = m_plotRect.bottom() - i * m_plotRect.height() / yTickCount;
		painter.drawLine(QPointF(m_yAxisAtZeroPos - 3, y), QPointF(m_yAxisAtZeroPos + 3, y));

		// 刻度值（乘以100显示百分比）
		double dataY = m_yMin + i * (m_yMax - m_yMin) / yTickCount;
		double dataYPercent = dataY * 100.0;
		QString label = QString::number(dataYPercent, 'f', 2) + QStringLiteral("%");
		int labelWidth = fm.width(label);
		painter.drawText(QPointF(m_yAxisAtZeroPos - labelWidth - 8, y + 5), label);
	}

	// 在Y轴位置标注"当前周期"
	painter.setPen(QColor(200, 50, 50));
	painter.drawText(m_yAxisAtZeroPos + 15, yAxisAtZero - 5, QStringLiteral("当前周期"));

	painter.restore();
}

int CustomPlotWidget::calculateYAxisAtZero() const
{
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

void CustomPlotWidget::drawGrid(QPainter &painter)
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

	// Y轴网格线
	int yGridCount = 8;
	for (int i = 1; i < yGridCount; i++)
	{
		double y = m_plotRect.bottom() - i * m_plotRect.height() / yGridCount;
		painter.drawLine(QPointF(m_plotRect.left(), y), QPointF(m_plotRect.right(), y));
	}

	// 在Y轴位置画一条明显的线
	painter.setPen(QPen(QColor(180, 180, 220), 1, Qt::DashLine));
	painter.drawLine(m_yAxisAtZeroPos, m_plotRect.top(), m_yAxisAtZeroPos, m_plotRect.bottom());

	// 在Y=0的位置画一条明显的线
	int yAxisAtZero = calculateYAxisAtZero();
	painter.setPen(QPen(QColor(180, 220, 180), 1, Qt::DashLine));
	painter.drawLine(m_plotRect.left(), yAxisAtZero, m_plotRect.right(), yAxisAtZero);

	painter.restore();
}

void CustomPlotWidget::drawSeries(QPainter &painter)
{
	painter.save();

	for (int seriesIndex = 0; seriesIndex < m_series.size(); seriesIndex++)
	{
		const Series &series = m_series[seriesIndex];
		if (!series.visible || series.data.size() < 2)
		{
			continue;
		}

		// 绘制折线
		QColor lineColor = series.color;
		if (seriesIndex == m_hoveredSeries)
		{
			// 悬停时使用红色
			lineColor = QColor(255, 0, 0);
		}

		QPen linePen(lineColor, 1.5);
		painter.setPen(linePen);

		for (int i = 0; i < series.data.size() - 1; i++)
		{
			QPointF p1 = dataToWidget(series.data[i]);
			QPointF p2 = dataToWidget(series.data[i + 1]);
			painter.drawLine(p1, p2);
		}

		// 绘制数据点
		painter.setBrush(lineColor);
		for (int i = 0; i < series.data.size(); i++)
		{
			QPointF p = dataToWidget(series.data[i]);
			painter.drawEllipse(p, 3, 3);

			// 高亮当前悬停的点
			if (seriesIndex == m_hoveredSeries && i == m_hoveredPoint)
			{
				painter.setBrush(Qt::yellow);
				painter.drawEllipse(p, 5, 5);
				painter.setBrush(lineColor);
			}
		}
	}

	painter.restore();
}

QPointF CustomPlotWidget::dataToWidget(const QPointF &dataPoint) const
{
	double x = m_plotRect.left() +
		(dataPoint.x() - m_xMin) / (m_xMax - m_xMin) * m_plotRect.width();
	double y = m_plotRect.bottom() -
		(dataPoint.y() - m_yMin) / (m_yMax - m_yMin) * m_plotRect.height();
	return QPointF(x, y);
}

QPointF CustomPlotWidget::widgetToData(const QPoint &widgetPoint) const
{
	double x = m_xMin + (widgetPoint.x() - m_plotRect.left()) /
		(double)m_plotRect.width() * (m_xMax - m_xMin);
	double y = m_yMin + (m_plotRect.bottom() - widgetPoint.y()) /
		(double)m_plotRect.height() * (m_yMax - m_yMin);
	return QPointF(x, y);
}

bool CustomPlotWidget::findNearestPoint(const QPoint &mousePos, int &seriesIndex, int &pointIndex) const
{
	double minDistance = 15.0;  // 像素距离阈值
	bool found = false;

	for (int i = 0; i < m_series.size(); i++)
	{
		const Series &series = m_series[i];
		if (!series.visible)
		{
			continue;
		}

		for (int j = 0; j < series.data.size(); j++)
		{
			QPointF widgetPoint = dataToWidget(series.data[j]);
			double distance = QLineF(mousePos, widgetPoint).length();

			if (distance < minDistance)
			{
				minDistance = distance;
				seriesIndex = i;
				pointIndex = j;
				found = true;
			}
		}
	}

	return found;
}

void CustomPlotWidget::mouseMoveEvent(QMouseEvent *event)
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
		int oldPoint = m_hoveredPoint;

		if (findNearestPoint(event->pos(), m_hoveredSeries, m_hoveredPoint))
		{
			const Series &series = m_series[m_hoveredSeries];
			const QPointF &point = series.data[m_hoveredPoint];

			// 显示工具提示（宽度加大）
			QString tooltip = QStringLiteral("%1\n周期: %2\n值: %3%")
				.arg(series.name)
				.arg(point.x(), 0, 'f', 0)
				.arg(point.y() * 100, 0, 'f', 2);

			// 设置提示框样式，增加宽度
			QToolTip::showText(event->globalPos(), tooltip, this, QRect(), 1500);

			if (oldSeries != m_hoveredSeries || oldPoint != m_hoveredPoint)
			{
				update();
			}
		}
		else
		{
			m_hoveredSeries = m_hoveredPoint = -1;
			QToolTip::hideText();
			if (oldSeries != -1 || oldPoint != -1)
			{
				update();
			}
		}
	}

	QWidget::mouseMoveEvent(event);
}

void CustomPlotWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && m_plotRect.contains(event->pos()))
	{
		m_isDragging = true;
		m_lastMousePos = event->pos();
		setCursor(Qt::ClosedHandCursor);
	}

	QWidget::mousePressEvent(event);
}

void CustomPlotWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_isDragging = false;
		setCursor(Qt::ArrowCursor);
	}

	QWidget::mouseReleaseEvent(event);
}

void CustomPlotWidget::wheelEvent(QWheelEvent *event)
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

	update();
	event->accept();
}

void CustomPlotWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	update();
}

void CustomPlotWidget::onResetButtonClicked()
{
	resetYAxis();
}