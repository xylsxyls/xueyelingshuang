#pragma once
#include <QWidget>
#include "RPGContentBase.h"

class Label;
class ContentLabel;
/** RPG����ս����ʾ
*/
class RPGThreeWidget : public RPGContentBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	RPGThreeWidget(QWidget* parent = nullptr);

public:
	/** ��ʼ�����ú���Ҫ��ÿ��QStringList�ڱ�����6��ֵ�������ǲ��Ǵ���״̬��
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
		COLUMN = 3
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
	Label* m_downTitleOne;
	Label* m_downTitleTwo;
	Label* m_downTitleThree;
	ContentLabel* m_upContentOne;
	ContentLabel* m_upContentTwo;
	ContentLabel* m_upContentThree;
	ContentLabel* m_downContentOne;
	ContentLabel* m_downContentTwo;
	ContentLabel* m_downContentThree;
	Label* m_upError;
	Label* m_downError;
};