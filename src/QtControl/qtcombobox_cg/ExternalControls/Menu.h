#pragma once
#include <QMenu>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlItem.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include <vector>

class QMouseEvent;
class MenuCustomStyle;
class Menu : 
	public ControlShow < QMenu >,
	public ControlFont < QMenu >,
	public ControlItem < QMenu >,
	public ControlSelf < QMenu >,
	public ControlBorderForNormal < QMenu >,
	public ControlBackgroundForNormal < QMenu >,
	public ControlItemBorderForNormalSelectedDisabled < QMenu >,
	public ControlItemBackgroundForNormalSelectedDisabled < QMenu >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	Menu(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~Menu();

public:
	/** ���캯��
	@param [in] title �˵�����
	@param [in] icon �˵�ͼ��
	@param [in] size �˵�ͼ���С
	@param [in] parent ������ָ��
	*/
	Menu(const QString& title, const QString& icon = QString(), const QSize& size = QSize(), QWidget* parent = nullptr);

public:
	/** ��ӽڵ�
	@param [in] text �ı�
	@param [in] uncheckIcon δѡ��ʱ��ͼƬ
	@param [in] uncheckIconSize δѡ��ʱ��ͼƬ��С
	@param [in] checkIcon ѡ��ʱ��ͼƬ
	@param [in] checkIconSize ѡ��ʱ��ͼƬ��С
	@return ������ӵĽڵ�ָ��
	*/
	QAction* addAction(const QString& text,
					   const QString& uncheckIcon = QString(),
					   const QSize& uncheckIconSize = QSize(0, 0),
					   const QString& checkIcon = QString(),
					   const QSize& checkIconSize = QSize(0, 0));

	/** ��Ӳ˵�
	@param [in] menu �˵�ָ��
	*/
	void addMenu(Menu* menu);

	/** ���÷ָ��ߴֶ�
	@param [in] height �ָ��߸߶�
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setSeparatorHeight(int32_t height, bool rePaint = false);

	/** ���÷ָ�����ɫ
	@param [in] color �ָ�����ɫ
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setSeparatorColor(const QColor& color, bool rePaint = false);

	/** ����ͼ��ƫ����
	@param [in] leftOrigin ͼ��ƫ����
	@param [in] rePaint �Ƿ������ػ�
	*/
	void setItemIconOrigin(int32_t leftOrigin, bool rePaint = false);

	/** �����˵�
	@param [in] point ����λ��
	@return ��ѡ����һ���˵��󷵻ظò˵�ָ�룬���������������򷵻ؿ�
	*/
	QAction* exec(const QPoint& point);

private:
	/** �ı�ͼ��
	@param [in] action ѡ�еĲ˵���
	@param [in] icon ��ǰͼ��
	@return ���ظı���ͼ��
	*/
	QIcon ChangeIcon(QAction* action, const QIcon& icon);
	
private:
	std::map<QAction*, QIcon> m_mapIconData;
	MenuCustomStyle* m_pCustomStyle;
	std::vector<Menu*> m_vecMenu;
};