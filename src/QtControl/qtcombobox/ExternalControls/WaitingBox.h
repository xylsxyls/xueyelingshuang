#pragma once
#include <QDialog>
#include <stdint.h>
#include "../COriginalDialog.h"

class Label;
/** �����࣬���������뵫����������ĵȴ���
*/
class WaitingBox : public COriginalDialog
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	WaitingBox(QDialog* parent = nullptr);

	/** ��������
	*/
	virtual ~WaitingBox();

public:
	enum ExitEnum
	{
		/** ����ر�
		*/
		CODE_EXIT = -1,

		/** ��ʱ�ر�
		*/
		TIMEOUT_EXIT = 0,

		/** �û�ʹ��Esc��Alt+F4�ر�
		*/
		USER_EXIT = 1
	};

public:
	/** ����Ĭ�Ͽؼ������йرհ�ť�����⣬�ָ��ߣ����С��������ɫ�������߿�
	@param [in] dialogWidth ���ڿ��
	@param [in] dialogHeight ���ڸ߶�
	*/
	void initForWaitingBox(int32_t dialogWidth, int32_t dialogHeight);

	/** �������ڵ���ʱ��������Ϊ���봰�ڵĶ�����ʱ�����ڣ�����ⲿʱ����Ӧ������˸�ĸ����ڣ����봰�ڻ�ִ��winId
	@param [in] parent ���봰�ڵľ��
	*/
	void setParentWindow(QWindow* parent);

	/** ���ñ���
	@param [in] title ����
	*/
	void setTitle(const QString& title);

	/** ������ʾ
	@param [in] tip ��ʾ
	*/
	void setTip(const QString& tip);

	/** ���õȴ�ʱ��
	@param [in] timeRest ʣ��ʱ�䣨���룩
	*/
	void setTimeRest(int32_t timeRest);

	/** �����û��Ƿ����ͨ��Esc��Alt+F4�ر�
	@param [in] userExit �û��Ƿ���������ر�
	*/
	void setUserExit(bool userExit);

	/** ���ô���رգ�֧�ֶ��߳�
	*/
	void exit();

Q_SIGNALS:
	void exitSignal();
	void waitingBoxDone(WaitingBox* box, WaitingBox::ExitEnum exitEnum);

protected:
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

	/** ��ӷָ���
	@param [in] point ��ʼ��
	@param [in] length ����
	@param [in] isHorizon �Ƿ�ˮƽ
	@param [in] upColor �ϲ���ɫ
	@param [in] downColor �²���ɫ
	@return ���طָ���ָ��
	*/
	Label* addSeparator(const QPoint& point, int32_t length, bool isHorizon, const QColor upColor, const QColor downColor);

	bool check();

protected:
	void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
	void keyPressEvent(QKeyEvent* eve);
	void closeEvent(QCloseEvent* eve);
	void paintEvent(QPaintEvent* eve);

private slots:
	void onNcActiveChanged(const bool& ncActive);
	void onExitSignal();

private:
	QWindow* topWindowHandle(QWindow* parent);

protected:
	int32_t m_timeId;
	Label* m_title;
	Label* m_tip;
	Label* m_sep;
	bool m_cancelEscAltF4;

private:
	int32_t m_timeRest;
	bool m_highLight;
	std::map<void*, int32_t> m_mapResult;
};