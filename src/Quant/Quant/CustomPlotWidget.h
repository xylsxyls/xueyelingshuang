#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

/** 自定义绘图控件，用于绘制多条折线图
*  支持鼠标交互：垂直方向缩放、平移，悬停提示
*  支持多色折线显示，X轴为整数坐标
*/
class CustomPlotWidget : public QWidget
{
	Q_OBJECT

public:
	/** 构造函数
	@param [in] parent 父控件指针
	*/
	explicit CustomPlotWidget(QWidget *parent = nullptr);

	/** 添加一条折线
	@param [in] name 折线名称
	@param [in] points 数据点集合
	@param [in] color 折线颜色
	*/
	void addSeries(const QString &name, const QVector<QPointF> &points, const QColor &color);

	/** 设置坐标轴标签
	@param [in] xLabel X轴标签
	@param [in] yLabel Y轴标签
	*/
	void setAxisLabels(const QString &xLabel, const QString &yLabel);

	/** 设置图表标题
	@param [in] title 标题文本
	*/
	void setTitle(const QString &title);

	/** 清空所有数据
	*/
	void clear();

	/** 自动调整坐标轴范围以适应数据
	*/
	void autoRange();

	/** 设置X轴范围（整数范围）
	@param [in] min 最小值
	@param [in] max 最大值
	*/
	void setXRange(int min, int max);

	/** 设置是否显示该折线
	@param [in] seriesIndex 折线索引
	@param [in] visible 是否显示
	*/
	void setSeriesVisible(int seriesIndex, bool visible);

	/** 重置Y轴范围，使Y=0在中间位置
	*/
	void resetYAxis();

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

	private slots:
	/** 重置按钮点击槽函数
	*/
	void onResetButtonClicked();

private:
	/** 绘制坐标轴
	@param [in] painter 绘图工具
	*/
	void drawAxes(QPainter &painter);

	/** 绘制折线
	@param [in] painter 绘图工具
	*/
	void drawSeries(QPainter &painter);

	/** 绘制网格
	@param [in] painter 绘图工具
	*/
	void drawGrid(QPainter &painter);

	/** 将数据坐标转换为界面坐标
	@param [in] dataPoint 数据坐标点
	@return 界面坐标点
	*/
	QPointF dataToWidget(const QPointF &dataPoint) const;

	/** 将界面坐标转换为数据坐标
	@param [in] widgetPoint 界面坐标点
	@return 数据坐标点
	*/
	QPointF widgetToData(const QPoint &widgetPoint) const;

	/** 查找最近的数据点
	@param [in] mousePos 鼠标位置
	@param [out] seriesIndex 系列索引
	@param [out] pointIndex 数据点索引
	@return 是否找到最近点
	*/
	bool findNearestPoint(const QPoint &mousePos, int &seriesIndex, int &pointIndex) const;

	/** 计算Y轴在X=0时的Y坐标位置
	@return Y轴在X=0时的Y坐标
	*/
	int calculateYAxisAtZero() const;

	/// 折线系列结构体
	struct Series
	{
		QString name;                       // 系列名称
		QVector<QPointF> data;              // 数据点集合
		QColor color;                       // 折线颜色
		bool visible;                       // 是否可见
	};

	QVector<Series> m_series;               // 所有折线系列

	QString m_title;                        // 图表标题
	QString m_xLabel;                       // X轴标签
	QString m_yLabel;                       // Y轴标签

	// 坐标轴范围
	double m_xMin;                          // X轴最小值
	double m_xMax;                          // X轴最大值
	double m_yMin;                          // Y轴最小值
	double m_yMax;                          // Y轴最大值

	// 原始Y轴范围（用于重置）
	double m_originalYMin;                  // 原始Y轴最小值
	double m_originalYMax;                  // 原始Y轴最大值

	// 边距设置
	int m_leftMargin;                       // 左侧边距
	int m_rightMargin;                      // 右侧边距
	int m_topMargin;                        // 顶部边距
	int m_bottomMargin;                     // 底部边距

	QRect m_plotRect;                       // 绘图区域

	// 鼠标交互相关
	bool m_isDragging;                      // 是否正在拖拽
	QPoint m_lastMousePos;                  // 上次鼠标位置

	// 悬停信息
	int m_hoveredSeries;                    // 当前悬停的系列索引
	int m_hoveredPoint;                     // 当前悬停的数据点索引

	// 计算Y轴在X=0时的位置
	int m_yAxisAtZeroPos;                   // Y轴在X=0时的X坐标位置

	// 是否固定X轴范围
	bool m_fixedXRange;

	// 重置按钮
	QPushButton *m_resetButton;
};