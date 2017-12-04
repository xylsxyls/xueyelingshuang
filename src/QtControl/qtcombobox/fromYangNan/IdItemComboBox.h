#pragma once
#include "ComboBox.h"

/** 定做类，节点对应ID编号，当点击框内容改变时发射信号
*/
class IdItemComboBox : public ComboBox
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	IdItemComboBox(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~IdItemComboBox();

public:
	/** 添加带有ID的节点
	@param [in] id 节点ID
	@param [in] text 节点内容
	*/
	void addItem(qint64 id, const QString& text);

	/** 添加一组带有ID的节点
	@param [in] idList 节点ID列表
	@param [in] textList 节点文字列表
	*/
	void addItems(const QList<qint64>& idList, const QStringList& textList);

public slots:
	void curIndexChanged(const QString& str);

signals:
	/** 当点击框内文字发生改变时发送信号
	@param [in] id 节点ID
	@param [in] text 当前点击框内文本内容
	*/
	void currentItemChanged5(qint64 id, const QString& text);

public:
	std::map<void*, qint64> m_mapIndex;
};