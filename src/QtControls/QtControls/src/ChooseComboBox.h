#pragma once
#include "HtmlComboBox.h"
#include "QtControlsMacro.h"

class Label;
/** 定做类，下拉选择控件
*/
class QtControlsAPI ChooseComboBox : public HtmlComboBox
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ChooseComboBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
	~ChooseComboBox();

public:
	/** 初始化选择控件窗口文字，必须第一步执行
	@param [in] text 文字内容
	*/
	void setComboBoxText(const QString& text);

	/** 设置选择图标图片
	@param [in] borderImgPath 图片路径
	@param [in] borderImgStateCount 图片状态个数
	@param [in] borderImgNormal 常态
	@param [in] borderImgHover 悬停状态
	@param [in] borderImgDisabled 禁用状态
	@param [in] rePaint 是否立即绘制
	*/
	void setChooseImage(const QString& borderImgPath,
						qint32 borderImgStateCount = 4,
						qint32 borderImgNormal = 1,
						qint32 borderImgHover = 2,
						qint32 borderImgDisabled = 4,
						bool rePaint = false);

	/** 设置选择图标的偏移量
	@param [in] origin 偏移量
	*/
	void setChooseOrigin(qint32 origin);

	/** 设置选择的索引值，从1开始
	@param [in] index 索引值
	*/
	void setChooseByIndex(qint32 index);

	/** 设置选择
	@param [in] id 选项ID
	*/
	void setChooseById(qint64 id);

	/** 设置选择图标大小
	@param [in] width 宽度
	@param [in] height 高度
	*/
	void setChooseSize(qint32 width, qint32 height);

	/** 设置下拉框节点高度
	@param [in] height 节点高度
	@param [in] rePaint 是否立即重画
	*/
	void setListItemHeight(qint32 height, bool rePaint = false);

	/** 清空
	*/
	void clear();

	/** 获取选中项索引值
	@return 返回选中项索引值
	*/
	qint32 currentChoosedIndex();

	/** 获取选中项的ID
	@return 返回选中项ID
	*/
	qint64 currentChoosedId();

	/** 获取选中项内容
	@return 返回选中项内容
	*/
	QString currentChoosedText();

	/** 去除选择项的选择图标
	*/
	void clearChoose();

Q_SIGNALS:
	/** 选择项变化后发送信号
	@param [in] index 选中项索引
	@param [in] id 选中项ID
	@param [in] text 选中项文字
	*/
	void itemChoosed(qint32 index, qint64 id, const QString& text);

protected:
	/** 尺寸变化时同步选择图标的位置
	@param [in] eve Qt尺寸变化事件
	*/
	void resizeEvent(QResizeEvent* eve);

	/** 处理鼠标移动，保证下拉框悬停效果和选择图标区域一致
	@param [in] eve Qt鼠标事件
	*/
	void mouseMoveEvent(QMouseEvent* eve);

	/** 绘制当前选择框文本和选择图标
	@param [in] eve Qt绘制事件
	*/
	void paintEvent(QPaintEvent* eve);

protected:
	/** 更新选择图标的几何位置
	*/
	void updateChooseGeometry();

	/** 检测内部选择图标是否可用
	@return 返回true表示内部状态可用，false表示存在空指针或异常状态
	*/
	bool check();

	/** 初始化内部选择图标和信号连接
	*/
	void init();

private slots:
	/** 处理列表项按下事件，同步选择图标并发出itemChoosed信号
	@param [in] index 被按下的列表项索引
	*/
	void onItemPressed(qint32 index);

protected:
	// 当前选择图标
	Label* m_choose;
	// 选择图标相对右侧的偏移量
	qint32 m_chooseOrigin;
	// 下拉列表项高度
	qint32 m_listItemHeight;
	// 点击框中显示的默认文字
	QString m_comboBoxText;
};