#pragma once
#include "Label.h"
#include "QtControlsMacro.h"

/** �����࣬���ڽ�����ʾ
*/
class QtControlsAPI TipLabel : public Label
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	TipLabel(QWidget* parent = nullptr);

	/** ���ô���ʱ�䣨��λ�룩
	@param [in] time ʱ��
	*/
	void setExistTime(qint32 time);

	/** ���������
	@param [in] maxWidth �����
	*/
	void setMaxWidth(qint32 maxWidth);

	/** �����Ƿ��н���
	@param [in] hasFocus ����ʱ�Ƿ�������
	*/
	void setHasFocus(bool hasFocus);

	/** ���ô�������λ�ã����½ǣ�
	@param [in] bottomRight ���½�����
	*/
	void setBottomRight(const QPoint& bottomRight);

protected:
	virtual void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
	bool nativeEvent(const QByteArray& eventType, void* message, long* result);

private:
	qint32 m_existTime;
	qint32 m_timeId;
	qint32 m_maxWidth;
	bool m_hasFocus;
	QPoint m_bottomRight;
};