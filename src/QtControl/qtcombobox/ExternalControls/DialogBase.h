#pragma once
#include <QDialog>
#include <stdint.h>

class Label;
class COriginalButton;
class QSplitter;
class LineEdit;
class CPasswordInputBox;
/** ��������࣬ʵ�ֻ����ӿ�
*/
class DialogBase : public QDialog
{
	Q_OBJECT
public:
	/** ���캯��
	*/
	DialogBase();

	/** չʾ����
	@param [out] dialogId ����IDֵ
	@param [in] timeOut �Զ��ر�ʱ�䣬��λ�룬����������ʾ�����Զ��ر�
	@param [in] isCountDownVisible �Ƿ���ʾ����ʱ
	@return �رմ���ʱ���ķ���ֵ
	*/
	int32_t exec(int32_t& dialogId, int32_t timeOut = -1, bool isCountDownVisible = false);

	/** չʾ����
	@param [out] dialogId ����IDֵ
	@param [in] timeOut �Զ��ر�ʱ�䣬��λ�룬����������ʾ�����Զ��ر�
	@param [in] isCountDownVisible �Ƿ���ʾ����ʱ
	*/
	void show(int32_t& dialogId, int32_t timeOut = -1, bool isCountDownVisible = false);

	/** ������ǩ���ֲ�����
	@param [in] text �ı�����
	@param [in] rect ��ǩ��Ը���������λ��
	@param [in] textColor ������ɫ
	@return ���ر�ǩָ��
	*/
	Label* addLabel(const QString& text, const QRect& rect, const QColor& textColor);

	/** ������ʾ���ֲ����ã����У�
	@param [in] text �ı�����
	@param [in] rect ��ǩ��Ը���������λ��
	@param [in] textColor ������ɫ
	@return ���ر�ǩָ��
	*/
	Label* addTip(const QString& text, const QRect& rect, const QColor& textColor);

	/** ������ť�����ã���ť������رմ���
	@param [in] text ��ť���ı�����
	@param [in] rect ��ť��Ը���������λ��
	@param [in] result �رմ��ں�ķ���ֵ
	@return ���ذ�ťָ��
	*/
	COriginalButton* addButton(const QString& text, const QRect& rect, int32_t result);

	/** ���ӷָ���
	@param [in] point ��ʼ��
	@param [in] length ����
	@param [in] isHorizon �Ƿ�ˮƽ
	@param [in] upColor �ϲ���ɫ
	@param [in] downColor �²���ɫ
	@return ���طָ���ָ��
	*/
	Label* addSeparator(const QPoint& point, int32_t length, bool isHorizon, const QColor upColor, const QColor downColor);

	/** ���ӵ����ı���
	@param [in] rect ��ǩ��Ը���������λ��
	@param [in] defaultText Ĭ����ʾ����
	@return �����ı���ָ��
	*/
	LineEdit* addLineEdit(const QRect& rect, bool isPassword = false, const QString& defaultText = "");

	/** ���������
	@param [in] rect ��ǩ��Ը���������λ��
	@param [in] defaultText Ĭ����ʾ����
	@return �����ı���ָ��
	*/
	CPasswordInputBox* addPasswordInputBox(const QRect& rect, const QString& defaultText = "");

	/** ���õ���ⲿʱ����Ӧ������˸�ĸ�����
	@param [in] parent ������ָ��
	*/
	void setParentWindow(QWindow* parent);

Q_SIGNALS:
	void timeRest(int32_t seconds);

protected:
	void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
	

private slots:
	void endDialog();

protected:
	bool m_timeVisible;
	int32_t m_timeId;

private:
	int32_t m_timeRest;
	std::map<void*, int32_t> m_mapResult;
};