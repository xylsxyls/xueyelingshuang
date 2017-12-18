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


	/** 画html文本，使用Label控件，字体，字体大小等通过painter获取
	@param [in] painter 绘画指针
	@param [in] rect 绘画区域
	@param [in] text 文本
	@param [in] flag 对齐方式
	*/
	void drawHtmlTextByLabel(QPainter* painter,
							 const QRect& rect,
							 const QString& text,
							 Qt::Alignment flag = Qt::AlignLeft | Qt::AlignTop);

	/** 画html文本，使用Label控件，字体，字体大小等通过painter获取
	@param [in] painter 绘画指针
	@param [in] rect 绘画区域
	@param [in] text 文本
	*/
	void drawHtmlTextByQTextDocument(QPainter* painter,
									 const QRect& rect,
									 const QString& text);

};

#endif // CGENERALSTYLE_H
