#pragma once
#include <QtWidgets/QComboBox>
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

class QListWidgetItem;
class ListWidget;
class QtControlsAPI ComboBox :
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
	void setDropDownSize(qint32 width, qint32 height = -1, bool rePaint = false);

	/** ����������ͷ�ı߿���
	@param [in] width ������ͷ���
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setDropDownBorderWidth(qint32 width, bool rePaint = false);

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
						  qint32 dropDownImgStateCount = 8,
						  qint32 dropDownImgNormal = 1,
						  qint32 dropDownImgHover = 2,
						  qint32 dropDownImgDisabled = 4,
						  qint32 dropDownImgExpandNormal = 5,
						  qint32 dropDownImgExpandDisabled = 8,
						  bool rePaint = false);

	/** ����������ͷ�������Ͻǵı߾�
	@param [in] topOrigin �ϲ�ƫ����
	@param [in] rightOrigin �Ҳ�ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setDropDownTopRightOrigin(qint32 topOrigin, qint32 rightOrigin = -1, bool rePaint = false);

	/** ���������򵯳�ʱ�������������ƫ����
	@param [in] origin ����ƫ����
	*/
	void setListOrigin(qint32 origin);

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
	void setItemText(qint32 index, const QString& text);

	/** ���ñ�����ɫ
	@param [in] color ������ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListBackgroundColor(const QColor& color, bool rePaint = false);

	/** ����������߿�ֶ�
	@param [in] width �߿�ֶ�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListBorderWidth(qint32 width, bool rePaint = false);

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
	void setListItemBorderWidth(qint32 width, bool rePaint = false);

	/** ����������ڵ㱳��ͼƬ���ͱ߿���ɫ���ô�
	@param [in] borderImgPath ����ͼƬ·�������·���б���ʹ����б��
	@param [in] borderImgStateCount ����ƽ�ּ���
	@param [in] borderImgNormal ��ѡ�г�̬ͼƬ�������1��ʾ��ͼƬ����ָ�4�ݻ�8�ݣ�ѡ������ĵ�һ��
	@param [in] borderImgHover ��ѡ����ͣͼƬ
	@param [in] borderImgDisabled ��ѡ�н���ͼƬ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemBorderImage(const QString& borderImgPath,
								qint32 borderImgStateCount = 4,
								qint32 borderImgNormal = 1,
								qint32 borderImgHover = 2,
								qint32 borderImgDisabled = 4,
								bool rePaint = false);

	/** ����������ڵ�߶�
	@param [in] height �ڵ�߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setListItemHeight(qint32 height, bool rePaint = false);

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
	void setListFontSize(qint32 fontSize, bool rePaint = false);

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

	/** �������������߶�
	@param [in] maxHeight ���߶�
	*/
	void setListMaxHeight(qint32 maxHeight);

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
	void itemPressed(qint32 index);

protected:
	virtual void showEvent(QShowEvent* eve);
	virtual void mouseMoveEvent(QMouseEvent* eve);
	virtual void keyPressEvent(QKeyEvent* eve);
	virtual void showPopup();
	virtual void hidePopup();

protected:
	bool check();
	/** �ؼ���ʼ��
	*/
	void init();

private slots:
	void listItemEntered(QListWidgetItem* item);
	void listItemPressed(QListWidgetItem* item);

protected:
	ListWidget* m_listWidget;
	qint32 m_hoverIndex;

private:
	qint32 m_dropDownImgNormal;
	qint32 m_dropDownImgDisabled;
	qint32 m_dropDownImgExpandNormal;
	qint32 m_dropDownImgExpandDisabled;
	qint32 m_dropDownImgStateCount;
	std::map<qint32, std::map<qint32, qint32>> m_imageStateMap;
	std::wstring m_imagePath;
	qint32 m_listOrigin;
	qint32 m_dropDownWidth;
	qint32 m_dropDownHeight;
	qint32 m_dropDownBorderWidth;
	bool m_dropDownVisible;
};