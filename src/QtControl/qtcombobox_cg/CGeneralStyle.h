#ifndef CGENERALSTYLE_H
#define CGENERALSTYLE_H

#include <QObject>
#include <QColor>
#include <QFont>
#include <tchar.h>

class Label;
class QTextDocument;
class CGeneralStyle : public QObject
{
	Q_OBJECT
private:
	CGeneralStyle();
	~CGeneralStyle();

	static CGeneralStyle* gInstance;
	QString mWar3LobbyResourcePath;
	QString mPlatformResourcePath;

	QColor  mFontColr;
	QFont   mFont;

	Label* m_htmlRender;
	QTextDocument* m_doc;


	void initPath();

public:
	enum PingLevel
	{
		PingLevel_Perfect = 0,
		PingLevel_Good = 1,
		PingLevel_Normal = 2,
		PingLevel_Bad = 3,
	};

	static CGeneralStyle* instance();
	static void           release();

	QString war3lobbyResourcePath();
	QString platformResourcePath();

	PingLevel getPingLevel(quint64 ping);
	QColor    getPingColor(quint64 ping);

	QColor  fontColor();
	QFont   font();


	/** ��html�ı���ʹ��Label�ؼ������壬�����С��ͨ��painter��ȡ
	@param [in] painter �滭ָ��
	@param [in] rect �滭����
	@param [in] text �ı�
	@param [in] flag ���뷽ʽ
	*/
	void drawHtmlTextByLabel(QPainter* painter,
							 const QRect& rect,
							 const QString& html,
							 Qt::Alignment flag = Qt::AlignLeft | Qt::AlignTop);

	/** ��html�ı���ʹ��Label�ؼ������壬�����С��ͨ��painter��ȡ
	@param [in] painter �滭ָ��
	@param [in] rect �滭����
	@param [in] text �ı�
	*/
	void drawHtmlTextByQTextDocument(QPainter* painter,
									 const QRect& rect,
									 const QString& html);

};

#endif // CGENERALSTYLE_H
