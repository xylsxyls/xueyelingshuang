#pragma once
#include <QWidget>
#include "RPGContentBase.h"
#include "game_result_view.h"

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
	/** ���ô�������
	@param [in] gameResult ��Ϸ���
	@return �����Ƿ����óɹ�
	*/
	virtual bool setGameResult(const GameResultType::GameResult& gameResult);

protected:
	void resizeEvent(QResizeEvent* eve);

private:
	/** ��ʼ��
	@param [in] titleLeft ����������
	@param [in] titleRight �Ҳ��������
	@param [in] header �����ַ�����
	@param [in] value �����ַ�����
	@param [in] progress �仯�ַ�����
	*/
	bool initWidget(const QString& titleLeft,
		const QString& titleRight,
		const QStringList& header,
		const QStringList& value,
		const QStringList& progress);

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