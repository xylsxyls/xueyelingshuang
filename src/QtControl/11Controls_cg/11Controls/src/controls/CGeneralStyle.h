#ifndef CGENERALSTYLE_H
#define CGENERALSTYLE_H

#include <QObject>
#include <QColor>
#include <QFont>
#include <tchar.h>
#include "ControlsMacro.h"

class Label;
class QTextDocument;
class ControlsAPI CGeneralStyle : public QObject
{
    Q_OBJECT
private:
    CGeneralStyle();
    ~CGeneralStyle();

    static CGeneralStyle* gInstance;
    QString mWar3LobbyResourcePath;
	QString mPlatformResourcePath;
	QString mSystemAvatarDirectory;
	QString mCustomAvatarDirectory;


    QColor  mFontColr;
    QFont   mFont;

	Label* m_htmlRender;
	QTextDocument* m_doc;

	int mFontFaceId_BaiduZongYi;

	void initPath();
	void initCustomerFont();
	

public:
    enum PingLevel
    {
        PingLevel_Perfect = 0,
        PingLevel_Good    = 1,
        PingLevel_Normal  = 2,
        PingLevel_Bad     = 3,
    };

    static CGeneralStyle* instance();
    static void           release();

	QString fontFace_BaiDuZongYi();
    QString war3lobbyResourcePath();
	QString platformResourcePath();

    PingLevel getPingLevel(quint64 ping);
	QColor    getPingColor(quint64 ping);

    QColor  fontColor();
    QFont   font();

	void initSystemAvatarDirectory();
	void initCustomAvatarDirectory();
	QString getUserAvatar(quint64 userid, quint64 imgid); //* 获取id


	/** 画html文本，使用Label控件，字体，字体大小等通过painter获取
	@param [in] painter 绘画指针
	@param [in] rect 绘画区域
	@param [in] text 文本
	@param [in] flag 对齐方式
	*/
	void drawHtmlTextByLabel(QPainter* painter,
							 const QRect& rect,
							 const QString& html,
							 Qt::Alignment flag = Qt::AlignLeft | Qt::AlignTop);

	/** 画html文本，使用Label控件，字体，字体大小等通过painter获取
	@param [in] painter 绘画指针
	@param [in] rect 绘画区域
	@param [in] text 文本
	*/
	void drawHtmlTextByQTextDocument(QPainter* painter,
									 const QRect& rect,
									 const QString& html);

};

#endif // CGENERALSTYLE_H
