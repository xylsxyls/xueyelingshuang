#include "CGeneralStyle.h"

CGeneralStyle* CGeneralStyle::gInstance = NULL;

CGeneralStyle::CGeneralStyle()
{
    mFont.setFamily(QString::fromLocal8Bit("Î¢ÈíÑÅºÚ"));
    mFont.setPixelSize(14);
    mFontColr = "#abb3d3";

	mWar3LobbyResourcePath = "E:/newClient/output/W3L/res";// "D:/SVN/output/W3L/res";
}

CGeneralStyle::~CGeneralStyle()
{

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

CGeneralStyle::PingLevel CGeneralStyle::getPingLevel(quint64 ping)
{
    PingLevel value;
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

QColor CGeneralStyle::fontColor()
{
    return mFontColr;
}

QFont CGeneralStyle::font()
{
    return mFont;
}

