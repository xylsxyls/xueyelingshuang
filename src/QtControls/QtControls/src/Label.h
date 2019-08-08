#pragma once
#include <QtWidgets/QLabel>
#include "ControlShow.h"
#include "ControlFont.h"
#include "ControlSelf.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"

/** ������ͼƬ�ؼ����ÿؼ���setBackground��غ���û�зָ�Ч��������ʹ��setBorder��غ���
*/
class QtControlsAPI Label :
	public ControlShow < QLabel >,
	public ControlFont < QLabel >,
	public ControlSelf < QLabel >,
	public ControlBorderForNormalHoverDisabled < QLabel >,
	public ControlBackgroundForNormalHoverDisabled < QLabel >
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	Label(QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~Label();

public:
	/** �����о�
	@param [in] lineHeight �о�
	*/
	void setLineHeight(qint32 lineHeight);

	/** ����˫��ȫ����һ�����ò����˻�
	*/
	void setDoubleClickFullScreen();

Q_SIGNALS:
	/** ������������ź�
	*/
	void leftClicked();

	/** �Ҽ����������ź�
	*/
	void rightClicked();

	/** ˫��֮�����ź�
	*/
	void doubleClicked();

	/** ����ƶ��ź�
	@param [in] ����ڸ����ڵ����λ��
	*/
	void mouseMoved(QPoint point);

protected:
	void mouseMoveEvent(QMouseEvent* eve);
	bool eventFilter(QObject* tar, QEvent* eve);

protected:
	void onDoubleClicked();

private:
	//��ǰ�Ƿ�ȫ��
	bool m_full;
	//�Ƿ��Ѿ����ù�����˫��ȫ������
	bool m_hasSetFullScreen;
	//ȫ��֮ǰ��λ��
	QRect m_rect;
};