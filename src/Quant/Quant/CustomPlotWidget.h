#pragma once

#include <QWidget>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>

// 全局颜色配置变量
// 注意：每个颜色数组中只有一个分量是255，其他两个是0
extern unsigned char g_initHighColor[3];  // 初始高颜色（收益率最高）
extern unsigned char g_initLowColor[3];   // 初始低颜色（收益率最低） 
extern unsigned char g_hoverColor[3];     // 悬停颜色

// 线条粗细全局变量
extern double g_normalLineWidth;          // 正常线条粗细
extern double g_hoverLineWidth;           // 悬停线条粗细

// 左右布局时图表高度放大系数
const double LAYOUT_HEIGHT_FACTOR = 1.5;

/** 验证颜色变量是否有效
*  验证规则：每个数组只有一个255，其他为0，且高颜色和低颜色的255分量不同
*  @return 验证通过返回true，否则返回false
*/
bool validateColorVariables();

/** 自定义绘图控件，用于绘制多条折线图
*  支持鼠标交互：垂直方向缩放、平移，悬停提示
*  支持多色折线显示，X轴为整数坐标
*/
class CustomPlotWidget : public QWidget
{
	Q_OBJECT

public:
	/** 构造函数
	*  @param [in] parent 父控件指针
	*/
	explicit CustomPlotWidget(QWidget* parent = nullptr);

	/** 添加一条折线
	*  @param [in] name 折线名称
	*  @param [in] points 数据点集合
	*  @param [in] color 折线颜色
	*/
	void addSeries(const QString& name, const QVector<QPointF>& points, const QColor& color);

	/** 设置坐标轴标签
	*  @param [in] xLabel X轴标签
	*  @param [in] yLabel Y轴标签
	*/
	void setAxisLabels(const QString& xLabel, const QString& yLabel);

	/** 设置图表标题
	*  @param [in] title 标题文本
	*/
	void setTitle(const QString& title);

	/** 清空所有数据
	*/
	void clear();

	/** 自动调整坐标轴范围以适应数据
	*/
	void autoRange();

	/** 设置X轴范围（整数范围）
	*  @param [in] min 最小值
	*  @param [in] max 最大值
	*/
	void setXRange(int min, int max);

	/** 设置是否显示该折线
	*  @param [in] seriesIndex 折线索引
	*  @param [in] visible 是否显示
	*/
	void setSeriesVisible(int seriesIndex, bool visible);

	/** 批量设置系列可见性
	*  @param [in] visibleList 可见性列表
	*/
	void setSeriesVisibleBatch(const QVector<bool>& visibleList);

	/** 重置Y轴位置，使Y=0在图表中间
	*/
	void resetYAxis();

	/** 设置悬停的系列
	*  @param [in] seriesIndex 系列索引
	*/
	void setHoveredSeries(int seriesIndex);

	/** 获取系列数量
	*  @return 系列数量
	*/
	int seriesCount() const { return m_series.size(); }

	/** 获取绘图区域矩形
	*  @return 绘图区域矩形
	*/
	QRect plotRect() const { return m_plotRect; }

	/** 获取系列数据
	*  @param [in] seriesIndex 系列索引
	*  @return 系列数据点集合，如果索引无效返回空集合
	*/
	QVector<QPointF> seriesData(int seriesIndex) const;

	/** 设置最小高度
	*  @param [in] height 最小高度
	*/
	void setMinHeight(int height);

signals:
	/** 鼠标悬停时发出的信号，包含策略详细信息和系列索引
	*  @param [in] seriesIndex 系列索引
	*  @param [in] info 详细信息文本
	*/
	void hoverInfoChanged(int seriesIndex, const QString& info);

protected:
	/** 绘制事件处理函数
	*  @param [in] event 绘制事件
	*/
	void paintEvent(QPaintEvent* event) override;

	/** 大小改变事件处理函数
	*  @param [in] event 大小改变事件
	*/
	void resizeEvent(QResizeEvent* event) override;

	/** 鼠标移动事件处理函数
	*  @param [in] event 鼠标事件
	*/
	void mouseMoveEvent(QMouseEvent* event) override;

	/** 鼠标按下事件处理函数
	*  @param [in] event 鼠标事件
	*/
	void mousePressEvent(QMouseEvent* event) override;

	/** 鼠标释放事件处理函数
	*  @param [in] event 鼠标事件
	*/
	void mouseReleaseEvent(QMouseEvent* event) override;

	/** 鼠标滚轮事件处理函数
	*  @param [in] event 滚轮事件
	*/
	void wheelEvent(QWheelEvent* event) override;

	/** 鼠标离开事件处理函数
	*  @param [in] event 鼠标事件
	*/
	void leaveEvent(QEvent* event) override;

	private slots:
	/** 重置按钮点击槽函数
	*/
	void onResetButtonClicked();

private:
	/** 绘制坐标轴
	*  @param [in] painter 绘图工具
	*/
	void drawAxes(QPainter& painter);

	/** 绘制折线（正常线条）
	*  @param [in] painter 绘图工具
	*/
	void drawSeries(QPainter& painter);

	/** 绘制悬停折线（在最上层）
	*  @param [in] painter 绘图工具
	*/
	void drawHoverSeries(QPainter& painter);

	/** 绘制网格
	*  @param [in] painter 绘图工具
	*/
	void drawGrid(QPainter& painter);

	/** 将数据坐标转换为界面坐标
	*  @param [in] dataPoint 数据坐标点
	*  @return 界面坐标点
	*/
	QPointF dataToWidget(const QPointF& dataPoint) const;

	/** 将界面坐标转换为数据坐标
	*  @param [in] widgetPoint 界面坐标点
	*  @return 数据坐标点
	*/
	QPointF widgetToData(const QPoint& widgetPoint) const;

	/** 查找最近的系列（通过折线）- 优化版本
	*  @param [in] mousePos 鼠标位置
	*  @param [out] seriesIndex 系列索引
	*  @return 是否找到最近的系列
	*/
	bool findNearestSeriesOptimized(const QPoint& mousePos, int& seriesIndex) const;

	/** 计算点到线段的距离
	*  @param [in] point 点坐标
	*  @param [in] lineStart 线段起点
	*  @param [in] lineEnd 线段终点
	*  @return 点到线段的距离
	*/
	double pointToLineDistance(const QPointF& point, const QPointF& lineStart, const QPointF& lineEnd) const;

	/** 计算Y轴在X=0时的Y坐标位置
	*  @return Y轴在X=0时的Y坐标
	*/
	int calculateYAxisAtZero() const;

	/** 计算Y轴在X=0时的X坐标位置
	*  @return Y轴在X=0时的X坐标
	*/
	int calculateXAxisAtZero() const;

	/** 调整Y轴范围，使Y轴刻度为10的整数倍（0.1的整数倍）
	*/
	void adjustYRange();

	/// 折线系列结构体
	struct Series
	{
		QString name;                       // 系列名称
		QVector<QPointF> data;              // 数据点集合
		QColor color;                       // 折线颜色
		bool visible;                       // 是否可见
		QRectF boundingBox;                 // 数据点的边界框（数据坐标系）
	};

	QVector<Series> m_series;               // 所有折线系列

	// 绘图缓存结构
	struct CachedLine
	{
		QVector<QPointF> screenPoints;      // 屏幕坐标点
		QRectF screenBoundingBox;           // 屏幕坐标系下的边界框
	};

	QVector<CachedLine> m_cachedLines;      // 缓存的屏幕坐标
	bool m_cacheDirty;                      // 缓存是否需要更新

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
	int m_lastHoveredSeries;                // 上次悬停的系列索引（用于避免重复更新）

	// 重置按钮
	QPushButton* m_resetButton;

	// 更新缓存
	void updateCache();
};