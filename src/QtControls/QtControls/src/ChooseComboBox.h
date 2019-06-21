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
	void itemChoosed(qint32 index, qint64 id, const QString& text);

protected:
	void resizeEvent(QResizeEvent* eve);
	void mouseMoveEvent(QMouseEvent* eve);
	void paintEvent(QPaintEvent* eve);

protected:
	void updateChooseGeometry();
	bool check();
	void init();

private slots:
	void onItemPressed(qint32 index);

protected:
	Label* m_choose;
	qint32 m_chooseOrigin;
	qint32 m_listItemHeight;
	QString m_comboBoxText;
};