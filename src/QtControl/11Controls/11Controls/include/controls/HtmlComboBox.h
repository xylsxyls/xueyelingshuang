#pragma once
#include "IdItemComboBox.h"
#include "ControlsMacro.h"

class QListWidgetItem;
/** �����֧࣬����addItem��ʱ������html��ʽ���ı�
*/
class ControlsAPI HtmlComboBox : public IdItemComboBox
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	HtmlComboBox(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~HtmlComboBox();

public:
	/** ����������������ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListTextColor(const QColor& normalColor,
						  const QColor& hoverColor = QColor(0, 0, 0, 0),
						  const QColor& disabledColor = QColor(0, 0, 0, 0),
						  bool rePaint = false);

	/** �����������ı�ƫ����
	@param [in] origin �ı�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListTextOrigin(qint32 origin, bool rePaint = false);

	/** �ڵ㵽�����򴰿ڵ��ĸ���߾�
	@param [in] leftOrigin ���ƫ����
	@param [in] topOrigin �ϲ�ƫ����
	@param [in] rightOrigin �Ҳ�ƫ����
	@param [in] bottomOrigin �²�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemAroundOrigin(qint32 leftOrigin,
								 qint32 topOrigin = -1,
								 qint32 rightOrigin = -1,
								 qint32 bottomOrigin = -1,
								 bool rePaint = false);

	/** ����������ڵ�߿�ֶ�
	@param [in] width �߿�ֶ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemBorderWidth(qint32 width, bool rePaint = false);

protected:
	void init();

public:
	QColor m_normalColor;
	QColor m_selectedColor;
	QColor m_disabledColor;
	qint32 m_origin;
	qint32 m_leftOrigin;
	qint32 m_topOrigin;
	qint32 m_rightOrigin;
	qint32 m_bottomOrigin;
	qint32 m_itemBorderWidth;
};