#pragma once
#include <QtWidgets/QComboBox>
#include <stdint.h>
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "ControlsMacro.h"

class QListWidgetItem;
class ListWidget;
class ControlsAPI ComboBox :
	public ControlShow < QComboBox >,
	public ControlFont < QComboBox >,
	public ControlSelf < QComboBox >,
	public ControlBorderForNormalHoverPressedDisabled < QComboBox >,
	public ControlBackgroundForNormalHoverPressedDisabled < QComboBox >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	ComboBox(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~ComboBox();

public:
	/** ����Ĭ��ֵ
	*/
	void setDefault();

	/** ����������ͷ�ĳߴ�
	@param [in] width ������ͷ���
	@param [in] height ������ͷ�߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setDropDownSize(int32_t width, int32_t height = -1, bool rePaint = false);

	/** ����������ͷ�ı߿���
	@param [in] width ������ͷ���
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setDropDownBorderWidth(int32_t width, bool rePaint = false);

	/** ����������ͷ��ͼƬ
	@param [in] dropDownImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] dropDownImgStateCount ����ƽ�ּ���
	@param [in] dropDownImgNormal ��ѡ�г�̬ͼƬ�������1��ʾ��ͼƬ����ָ�4�ݻ�8�ݣ�ѡ������ĵ�һ��
	@param [in] dropDownImgHover ��ѡ����ͣͼƬ
	@param [in] dropDownImgDisabled ��ѡ�н���ͼƬ
	@param [in] dropDownImgCkNormal ѡ�г�̬ͼƬ
	@param [in] dropDownImgCkDisabled ѡ�н���ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setDropDownImage(const QString& dropDownImgPath,
						  int32_t dropDownImgStateCount = 8,
						  int32_t dropDownImgNormal = 1,
						  int32_t dropDownImgHover = 2,
						  int32_t dropDownImgDisabled = 4,
						  int32_t dropDownImgExpandNormal = 5,
						  int32_t dropDownImgExpandDisabled = 8,
						  bool rePaint = false);

	/** ����������ͷ�������Ͻǵı߾�
	@param [in] topOrigin �ϲ�ƫ����
	@param [in] rightOrigin �Ҳ�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setDropDownTopRightOrigin(int32_t topOrigin, int32_t rightOrigin = -1, bool rePaint = false);

	/** ���������򵯳�ʱ�������������ƫ����
	@param [in] origin ����ƫ����
	*/
	void setListOrigin(int32_t origin);

	/** ����һ��
	@param [in] text �ı�
	*/
	void addItem(const QString& text);

	/** ��������
	@param [in] textList �ı��б�
	*/
	void addItems(const QStringList& textList);

	/** ����ָ���ڵ�����
	@param [in] index ����ֵ
	@param [in] text �ı�
	*/
	void setItemText(int32_t index, const QString& text);

	/** ���ñ�����ɫ
	@param [in] color ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListBackgroundColor(const QColor& color, bool rePaint = false);

	/** ����������߿�ֶ�
	@param [in] width �߿�ֶ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListBorderWidth(int32_t width, bool rePaint = false);

	/** ����������߿���ɫ
	@param [in] color ��̬
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListBorderColor(const QColor& color, bool rePaint = false);

	/** ����������ڵ�߿���ɫ��list�ؼ�û�а������ԣ�
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemBorderColor(const QColor& normalColor,
								const QColor& hoverColor = QColor(0, 0, 0, 0),
								const QColor& disabledColor = QColor(0, 0, 0, 0),
								bool rePaint = false);

	/** ����������ڵ㱳����ɫ��list�ؼ�û�а������ԣ�
	@param [in] normalColor ��̬��ɫ
	@param [in] hoverColor ��ͣ��ɫ
	@param [in] disabledColor ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemBackgroundColor(const QColor& normalColor,
									const QColor& hoverColor = QColor(0, 0, 0, 0),
									const QColor& disabledColor = QColor(0, 0, 0, 0),
									bool rePaint = false);

	/** ����������ڵ�߿�ֶ�
	@param [in] width �߿�ֶ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemBorderWidth(int32_t width, bool rePaint = false);

	/** ����������ڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��ѡ�г�̬ͼƬ�������1��ʾ��ͼƬ����ָ�4�ݻ�8�ݣ�ѡ������ĵ�һ��
	@param [in] borderImgHover ��ѡ����ͣͼƬ
	@param [in] borderImgDisabled ��ѡ�н���ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemBorderImage(const QString& borderImgPath,
								int32_t borderImgStateCount = 4,
								int32_t borderImgNormal = 1,
								int32_t borderImgHover = 2,
								int32_t borderImgDisabled = 4,
								bool rePaint = false);

	/** ����������ڵ�߶�
	@param [in] height �ڵ�߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemHeight(int32_t height, bool rePaint = false);

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

	/** ��������������
	@param [in] fontName ������
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListFontFace(const QString& fontName, bool rePaint = false);

	/** ���������������С
	@param [in] fontSize �����С
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListFontSize(int32_t fontSize, bool rePaint = false);

	/** �����������ı�ƫ����
	@param [in] origin �ı�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListTextOrigin(int32_t origin, bool rePaint = false);

	/** �ڵ㵽�����򴰿ڵ��ĸ���߾�
	@param [in] leftOrigin ���ƫ����
	@param [in] topOrigin �ϲ�ƫ����
	@param [in] rightOrigin �Ҳ�ƫ����
	@param [in] bottomOrigin �²�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemAroundOrigin(int32_t leftOrigin,
								 int32_t topOrigin = -1,
								 int32_t rightOrigin = -1,
								 int32_t bottomOrigin = -1,
								 bool rePaint = false);

	/** �������������߶�
	@param [in] maxHeight ���߶�
	*/
	void setListMaxHeight(int32_t maxHeight);

	/** ����������ͷ�Ƿ�����
	@param [in] enable �Ƿ�����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setDropDownVisible(bool enable, bool rePaint = false);

	/** ���õ���������Ƿ���Ч
	@param [in] enable �Ƿ���Ч
	*/
	void setSelectEnable(bool enable);

	/** �ػ�
	*/
	virtual void repaint();

Q_SIGNALS:
	/** �ڵ㱻����֮�����ź�
	@param [in] index �ڵ�����ֵ
	*/
	void itemPressed(int index);

private:
	virtual void showEvent(QShowEvent* eve);
	virtual void mouseReleaseEvent(QMouseEvent* eve);
	virtual void mouseMoveEvent(QMouseEvent* eve);
	virtual void showPopup();
	virtual void hidePopup();
	virtual void hideEvent(QHideEvent* eve);
	virtual void keyPressEvent(QKeyEvent* eve);

private slots:
	void listItemEntered(QListWidgetItem* item);
	void listItemPressed(QListWidgetItem* item);

protected:
	ListWidget* m_listWidget;
	int32_t m_hoverIndex;

private:
	int32_t m_dropDownImgNormal;
	int32_t m_dropDownImgDisabled;
	int32_t m_dropDownImgExpandNormal;
	int32_t m_dropDownImgExpandDisabled;
	int32_t m_dropDownImgStateCount;
	std::map<int32_t, std::map<int32_t, int32_t>> m_imageStateMap;
	std::wstring m_imagePath;
	int32_t m_listOrigin;
	int32_t m_dropDownWidth;
	int32_t m_dropDownHeight;
	int32_t m_dropDownBorderWidth;
	bool m_dropDownVisible;
};
