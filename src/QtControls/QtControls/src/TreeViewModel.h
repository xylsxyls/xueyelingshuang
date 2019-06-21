#pragma once
#include <QStandardItemModel>
#include <stdint.h>
#include "TreeText.h"
#include "QtControlsMacro.h"

/** TreeViewModel������
*/
class QtControlsAPI TreeViewModel : public QStandardItemModel
{
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	TreeViewModel(QObject* parent = nullptr);

	/** ��������
	*/
	virtual ~TreeViewModel();

public:
	/** ��ȡ�ڵ㣬û���򴴽�
	@param [in] row ������
	@param [in] column ������
	@return ���ؽڵ�ָ��
	*/
	QStandardItem* getItem(int32_t row, int32_t column);

	/** �������ؼ�ͷ������
	@param [in] index �ؼ�����ֵ
	@param [in] text ������ز���
	*/
	void setHeaderText(int32_t index, const TreeText& text = TreeText());

	/** ��ȡͷ�����ֲ���ָ��
	@return ����ͷ�����ֲ���ָ��
	*/
	std::map<int32_t, TreeText>* headerText();

	/** �������ؼ��ڲ�����
	@param [in] row ������ֵ
	@param [in] column ������ֵ
	@param [in] text ������ز���
	*/
	void setBodyText(int32_t row, int32_t column, const TreeText& text = TreeText());

	/** ��ȡ�ڲ����ֲ���ָ��
	@return �����ڲ����ֲ���ָ��
	*/
	std::map<int32_t, std::map<int32_t, TreeText>>* bodyText();

	/** ��ȡ�ڵ��б�����ɫ
	@return ���ؽڵ��б�����ɫ
	*/
	std::map<int32_t, std::vector<QColor>>* itemBackgroundColor();

	/** ���ñ�ͷ�߶�
	@param [in] height ��ͷ�߶�
	*/
	void setHeaderHeight(int32_t height);

	/** �����и�
	@param [in] height �и�
	@param [in] row ������ֵ��-1���ʾ������
	*/
	void setRowHeight(int32_t height, int32_t row = -1);

	/** �����п�
	@param [in] width �п�
	@param [in] row ������ֵ��-1���ʾ������
	*/
	void setColumnWidth(int32_t width, int32_t column = -1);

	/** �����б�ڵ㱳����ɫ��һ�У������С�ڵ���û��������ɫ�Ż�ʹ���������ɫ
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] selectedColor ������ɫ
	@param [in] disabledColor ������ɫ
	*/
	void setListItemColor(const QColor& normalColor,
						  const QColor& hoverColor = QColor(0, 0, 0, 0),
						  const QColor& selectedColor = QColor(0, 0, 0, 0),
						  const QColor& disabledColor = QColor(0, 0, 0, 0),
						  int32_t row = -1);

	/** ���������п���޷��ı��С
	*/
	void setColumnWidthFixedMode();

protected:
	/** ��ʼ��
	*/
	void init();

	/** ����ڲ�ָ���Ƿ���Ч
	@return �����ڲ�ָ���Ƿ���Ч
	*/
	bool check();

private:
	//ͷ�����ֲ���
	std::map<int32_t, TreeText> m_headerTextMap;
	//�ڲ����ֲ���
	std::map<int32_t, std::map<int32_t, TreeText>> m_bodyTextMap;
	//������ɫ
	std::map<int32_t, std::vector<QColor>> m_itemBackgroundColorMap;
};