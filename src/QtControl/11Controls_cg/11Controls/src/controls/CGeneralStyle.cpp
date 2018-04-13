#include "CGeneralStyle.h"
#include "Label.h"
#include <QDir>
#include <QApplication>
#include <QPainter>
#include <QTextDocument>
#include <QDebug>
#include <QStandardPaths>
#include <QAbstractTextDocumentLayout>

CGeneralStyle* CGeneralStyle::gInstance = NULL;

void CGeneralStyle::initPath()
{
	// get w3l res path
	QString exePath = qApp->applicationDirPath();
	QDir exeDir(exePath);

	exeDir.cdUp(); // w3l   or  output

	qDebug() << exeDir.path();
	if(exeDir.path().split("/").last() == "W3L")
	{
		bool res = true;
		res = exeDir.cd("res") && res; //res

		if(res)
		{
			mWar3LobbyResourcePath = exeDir.path();
		}

		res = true;

		//get platform res path
		res = exeDir.cdUp() && res; // w3l
		res = exeDir.cdUp() && res; //output
		res = exeDir.cd("resource") && res; //resource


		if(res)
		{
			mPlatformResourcePath = "D:/newClient/output/resource";
		}
	}
	else
	{
		bool res = true;
		res = exeDir.cd("W3L") && res; //w3l
		res = exeDir.cd("res") && res; //res

		if(res)
		{
			mWar3LobbyResourcePath = exeDir.path();
		}

		//get platform res path
		res = exeDir.cdUp() && res; //w3l
		res = exeDir.cdUp() && res; //output
		res = exeDir.cd("resource") && res; //resource

		if(res)
		{
			mPlatformResourcePath = "D:/newClient/output/resource";
		}
	}

	
}

void CGeneralStyle::initCustomerFont()
{
	QString fontFileName = war3lobbyResourcePath() + "/Font/baidu_zongyi.ttf";

	QFile fontFile(fontFileName);
	if(!fontFile.open(QIODevice::ReadOnly))
	{
		//qDebug()<<"Open font file error";
		mFontFaceId_BaiduZongYi = -1;
	}
	else
	{
		mFontFaceId_BaiduZongYi = QFontDatabase::addApplicationFontFromData(fontFile.readAll());
		fontFile.close();
	}
}

CGeneralStyle::CGeneralStyle()
{
	
	//TODO: init resoucre
    mFont.setFamily(QStringLiteral("Î¢ÈíÑÅºÚ"));
    mFont.setPixelSize(14);
    mFontColr = "#abb3d3";


	//TODO: init platform path
	initPath();

	initCustomAvatarDirectory();
	initSystemAvatarDirectory();

	//TODO: init HTML render
	m_htmlRender = new Label;
	m_htmlRender->setAttribute(Qt::WA_TranslucentBackground);

	m_doc = new QTextDocument;

	//init font
	initCustomerFont();
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

QString CGeneralStyle::fontFace_BaiDuZongYi()
{
	QString fontFace;
	QStringList loadedFontFamilies = QFontDatabase::applicationFontFamilies(mFontFaceId_BaiduZongYi);
	if(!loadedFontFamilies.empty())
	{
		fontFace = loadedFontFamilies.at(0);
	}
	return fontFace;
}

QString CGeneralStyle::war3lobbyResourcePath()
{
    return mWar3LobbyResourcePath;
}

QString CGeneralStyle::platformResourcePath()
{
	return "D:/newClient/output/resource";
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

void CGeneralStyle::initSystemAvatarDirectory()
{
	if (mSystemAvatarDirectory.isEmpty())
	{
		// 11IM
		if (QDir(QApplication::applicationDirPath() + "/../resource/UserIcon").exists())
		{
			mSystemAvatarDirectory = QApplication::applicationDirPath() + "/../resource/UserIcon";
		}
		// War3Lobby
		else if (QDir(QApplication::applicationDirPath() + "/../../resource/UserIcon").exists())
		{
			mSystemAvatarDirectory = QApplication::applicationDirPath() + "/../../resource/UserIcon";
		}
	}
}

void CGeneralStyle::initCustomAvatarDirectory()
{
	const QString HomeDirectory = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

	if (!HomeDirectory.isEmpty())
	{
		if (QSysInfo::windowsVersion() != QSysInfo::WV_XP)
		{
			mCustomAvatarDirectory = HomeDirectory.left(HomeDirectory.lastIndexOf("/")+1) + "All Users/YiYi/11Client/CustomUserIcons";
		}
		else
		{
			mCustomAvatarDirectory = HomeDirectory.left(HomeDirectory.lastIndexOf("/")+1) + "All Users/Application Data/YiYi/11Client/CustomUserIcons";
		}
	}
}

QString CGeneralStyle::getUserAvatar(quint64 userid, quint64 imgid)
{
	QString avatarPath;

	if ((userid > 0) && (imgid >= 0))
	{
		if (imgid < 60000)
		{
			QString tp = QStringLiteral("%1/%2.jpg").arg(mSystemAvatarDirectory).arg(imgid);

			if (QFileInfo::exists(tp))
			{
				avatarPath = tp;
			}
			else
			{
				avatarPath =  mSystemAvatarDirectory + "/0.jpg";
			}
		}
		else
		{
			QString tp = QStringLiteral("%1/%2.jpg").arg(mCustomAvatarDirectory).arg(userid);

			if (QFileInfo::exists(tp))
			{
				avatarPath = tp;
			}
			else
			{
				avatarPath = mSystemAvatarDirectory + "/60000.jpg";
			}
		}
	}

	return avatarPath;
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
