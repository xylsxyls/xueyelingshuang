#pragma once
#include <QWidget>
#include "RPGContentBase.h"

class Label;
class ContentLabel;
/** RPG����ս����ʾ
*/
class RPGContentWidget : public RPGContentBase
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	RPGContentWidget(int32_t row, int32_t column, QWidget* parent = nullptr);

	/** ��������
	*/
	virtual ~RPGContentWidget();

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

	template <typename TypeClass>
	TypeClass** CreateDyadicArray(int32_t row, int32_t column);

	template <typename TypeClass>
	void DestroyDyadicArray(TypeClass** classPtr, int32_t row);

private:
	void onState(ContentState state);
	void hideError();
	void raiseError();
	bool check();

private:
	enum
	{
		/** ����߶�
		*/
		TITLE_HEIGHT = 24,

		/** �ָ��߸߶�
		*/
		SEPARATOR_HEIGHT = 1,

		/** ���ݲ��ֵı������߶�
		*/
		CONTENT_TITLE_HEIGHT = 23
	};

private:
	int32_t m_row;
	int32_t m_column;

	Label* m_titleLeft;
	Label* m_titleRight;
	Label* m_separator;

	Label*** m_szTitle;
	ContentLabel*** m_szContent;
	Label** m_szError;

	Label* m_errorLeft;
	Label* m_errorAllTitle;
	Label* m_errorAllContentTitle;
	Label** m_errorTitle;
	Label* m_errorAllContent;
};