#pragma once
#include <QWidget>
#include "RPGContentBase.h"

class Label;
class ContentLabel;
/** RPG����ս����ʾ
*/
class RPGFourWidget : public RPGContentBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	RPGFourWidget(QWidget* parent = nullptr);

public:
	/** ��ʼ�����ú���Ҫ��ÿ��QStringList�ڱ�����8��ֵ�������ǲ��Ǵ���״̬��
	@param [in] titleLeft ����������
	@param [in] titleRight �Ҳ��������
	@param [in] header �����ַ�����
	@param [in] value �����ַ�����
	@param [in] progress �仯�ַ�����
	*/
	void init(const QString& titleLeft,
			  const QString& titleRight,
			  const QStringList& header,
			  const QStringList& value,
			  const QStringList& progress);

protected:
	void resizeEvent(QResizeEvent* eve);

private:
	enum
	{
		/** ����߶�
		*/
		TITLE_HEIGHT = 24,

		/** �ָ��߸߶�
		*/
		SEPARATOR_HEIGHT = 1,

		/** �ϲ����²��������߶�
		*/
		UP_DOWN_TITLE_HEIGHT = 23,

		/** ����
		*/
		COLUMN = 4
	};

private:
	void onState(int32_t state);
	bool check();

private:
	Label* m_titleLeft;
	Label* m_titleRight;
	Label* m_separator;
	Label* m_upTitleOne;
	Label* m_upTitleTwo;
	Label* m_upTitleThree;
	Label* m_upTitleFour;
	Label* m_downTitleOne;
	Label* m_downTitleTwo;
	Label* m_downTitleThree;
	Label* m_downTitleFour;
	ContentLabel* m_upContentOne;
	ContentLabel* m_upContentTwo;
	ContentLabel* m_upContentThree;
	ContentLabel* m_upContentFour;
	ContentLabel* m_downContentOne;
	ContentLabel* m_downContentTwo;
	ContentLabel* m_downContentThree;
	ContentLabel* m_downContentFour;
	Label* m_upError;
	Label* m_downError;
};