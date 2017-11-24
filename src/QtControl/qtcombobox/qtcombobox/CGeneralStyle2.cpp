#include "CGeneralStyle2.h"

CGeneralStyle2* CGeneralStyle2::gInstance = NULL;

CGeneralStyle2::CGeneralStyle2()
{
    mFont.setFamily(QString::fromLocal8Bit("Î¢ÈíÑÅºÚ"));
    mFont.setPixelSize(14);
    mFontColr = "#abb3d3";

    mWar3LobbyResourcePath = "D:/SVN/output/W3L/res";
}

CGeneralStyle2::~CGeneralStyle2()
{

}

CGeneralStyle2 *CGeneralStyle2::instance()
{
    if(gInstance == NULL)
    {
        gInstance = new CGeneralStyle2;
    }

    return gInstance;
}

void CGeneralStyle2::release()
{
    if(gInstance)
    {
        delete gInstance;
        gInstance = NULL;
    }
}

QString CGeneralStyle2::war3lobbyResourcePath()
{
    return mWar3LobbyResourcePath;
}

CGeneralStyle2::PingLevel CGeneralStyle2::getPingLevel(quint64 ping)
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

QColor CGeneralStyle2::fontColor()
{
    return mFontColr;
}

QFont CGeneralStyle2::font()
{
    return mFont;
}

