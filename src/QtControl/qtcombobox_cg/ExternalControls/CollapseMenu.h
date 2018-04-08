#pragma once
#include "Menu.h"

class Label;
/** �����࣬�۵�ʽ�߿�˵�
*/
class CollapseMenu : public Menu
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	CollapseMenu(QWidget* parent = nullptr);

	/** ���캯��
	@param [in] title �˵�����
	@param [in] icon �˵�ͼ��
	@param [in] size �˵�ͼ���С
	@param [in] parent ������ָ��
	*/
	CollapseMenu(const QString& title, const QString& icon = QString(), const QSize& size = QSize(), QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~CollapseMenu();

public:
	/** ���ñ�����ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] selectedColor ��ͣ��ɫ������list��
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemBorderColor(const QColor& normalColor,
							const QColor& selectedColor = QColor(0, 0, 0, 0),
							const QColor& disabledColor = QColor(0, 0, 0, 0),
							bool rePaint = false);

private:
	/** ���߿�����Ϊ�۵�ʽ������Ϊ�����࣬Ĭ����Ϊ�۵�ʽ�������ֶ��ٴε���
	*/
	void setBorderCollapse();

private:
	void paintEvent(QPaintEvent *eve);
	void leaveEvent(QEvent* eve);
	void mouseMoveEvent(QMouseEvent *eve);

private:
	QColor m_normalColor;
	QColor m_selectedColor;

	bool m_hasSetSelectedLabelTop;
	Label* m_selectedLabelTop;
	bool m_hasSetSelectedLabelBottom;
	Label* m_selectedLabelBottom;
	Label* m_topLabel;
	bool m_hasSetTopLabel;
};