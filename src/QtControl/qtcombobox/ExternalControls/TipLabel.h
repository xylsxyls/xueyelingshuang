#pragma once
#include "Label.h"

/** �����࣬���ڽ�����ʾ
*/
class TipLabel : public Label
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
	void setExistTime(int32_t time);

	/** ���������
	@param [in] maxWidth �����
	*/
	void setMaxWidth(int32_t maxWidth);

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
	int32_t m_existTime;
	int32_t m_timeId;
	int32_t m_maxWidth;
	bool m_hasFocus;
	QPoint m_bottomRight;
};