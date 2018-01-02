#include "CGeneralStyle.h"
#include "ExternalControls/Label.h"
#include <QDir>
#include <QApplication>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

CGeneralStyle* CGeneralStyle::gInstance = NULL;

void CGeneralStyle::initPath()
{
	// get w3l res path
	QString w3lPath = qApp->applicationDirPath();
	QDir w3lDir(w3lPath);

	if(!w3lDir.exists())
		mWar3LobbyResourcePath = "";

	bool res = true;
	res = w3lDir.cdUp() & res; //W3L
	res = w3lDir.cd("res") & res; //res

	if(res)
	{
		mWar3LobbyResourcePath = w3lDir.path();
	}

	//get platform res path
	w3lPath = qApp->applicationDirPath();
	w3lDir = QDir(w3lPath);

	if(!w3lDir.exists())
		mPlatformResourcePath = "";

	res = true;
	res = w3lDir.cdUp() & res; //W3L
	res = w3lDir.cdUp() & res; //output
	res = res = w3lDir.cd("resource") & res; //resource


	if(res)
	{
		mPlatformResourcePath = w3lDir.path();
	}
	mPlatformResourcePath = "E:/newClient/output/resource";
}

CGeneralStyle::CGeneralStyle()
{
	
	mWar3LobbyResourcePath = "E:/newClient/output/W3L/res";
	//TODO: init resoucre
    mFont.setFamily(QStringLiteral("Î¢ÈíÑÅºÚ"));
    mFont.setPixelSize(14);
    mFontColr = "#abb3d3";


	//TODO: init platform path
	initPath();

	//TODO: init HTML render
	m_htmlRender = new Label;
	m_htmlRender->setAttribute(Qt::WA_TranslucentBackground);

	m_doc = new QTextDocument;
}

CGeneralStyle::~CGeneralStyle()
{
	delete m_htmlRender;
	m_htmlRender = nullptr;
	delete m_doc;
	m_doc = nullptr;
}

CGeneralStyle *CGeneralStyle::instance()
{
    if(gInstance == NULL)
    {
        gInstance = new CGeneralStyle;
    }

    return gInstance;
}

void CGeneralStyle::release()
{
    if(gInstance)
    {
        delete gInstance;
        gInstance = NULL;
    }
}

QString CGeneralStyle::war3lobbyResourcePath()
{
    return mWar3LobbyResourcePath;
}

QString CGeneralStyle::platformResourcePath()
{
	return mPlatformResourcePath;
}

CGeneralStyle::PingLevel CGeneralStyle::getPingLevel(quint64 ping)
{
    PingLevel value = PingLevel_Bad;
    if((0 <= ping) && (ping < 200))
    {
        value = PingLevel_Perfect;
    }
    else if((200 <= ping) && (ping < 300) )
    {
        value = PingLevel_Good;
    }
    else if((300 <= ping) && (ping < 500) )
    {
        value = PingLevel_Normal;
    }
    else
    {
        value = PingLevel_Bad;
    }

    return value;
}

QColor CGeneralStyle::getPingColor(quint64 ping)
{
	QColor pingColor = this->fontColor();

	switch(this->getPingLevel(ping))
	{
	case CGeneralStyle::PingLevel_Perfect:
		{
			pingColor = QColor(93, 195, 44);
		}
		break;
	case CGeneralStyle::PingLevel_Good:
		{
			pingColor = QColor(230, 216, 57);
		}
		break;
	case CGeneralStyle::PingLevel_Normal:
		{
			pingColor = QColor(255, 82, 59);
		}
		break;
	case CGeneralStyle::PingLevel_Bad:
		{
			pingColor = Qt::red;
		}
		break;
	};

	
	return pingColor;
}

QColor CGeneralStyle::fontColor()
{
    return mFontColr;
}

QFont CGeneralStyle::font()
{
    return mFont;
}

void CGeneralStyle::drawHtmlTextByLabel(QPainter* painter,
										const QRect& rect,
										const QString& html,
										Qt::Alignment flag)
{
	painter->save();

	std::wstring htmlString2 = L"<div style=\"color:%s;font-size: %dpx;font-family:'%s'\">%s</div>";
	htmlString2 = CStringManager::Format(htmlString2.c_str(),
										painter->pen().color().name().toStdWString().c_str(),
										painter->font().pixelSize(),
										painter->font().family().toStdWString().c_str(),
										html.toStdWString().c_str());
	m_htmlRender->setText(QString::fromStdWString(htmlString2));

	m_htmlRender->setAlignment(flag);
	m_htmlRender->resize(rect.size());

	painter->translate(rect.topLeft());

	m_htmlRender->render(painter);

	painter->restore();
}

void CGeneralStyle::drawHtmlTextByQTextDocument(QPainter* painter,
												const QRect& rect,
												const QString& html)
{
	painter->save();
	std::wstring htmlString = L"<div style='color:%s;'>%s</div>";
	htmlString = CStringManager::Format(htmlString.c_str(),
										painter->pen().color().name().toStdWString().c_str(),
										html.toStdWString().c_str());
	m_doc->setHtml(QString::fromStdWString(htmlString));

	m_doc->setDocumentMargin(0);

	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	//textOption.setAlignment(Qt::AlignVCenter);
	m_doc->setDefaultTextOption(textOption);

	painter->translate(rect.topLeft());

	m_doc->setDefaultFont(painter->font());

	QAbstractTextDocumentLayout::PaintContext context;
	
	m_doc->documentLayout()->draw(painter, context);

	painter->restore();
}
