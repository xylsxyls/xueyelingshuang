#ifndef CGENERALSTYLE_H
#define CGENERALSTYLE_H

#include <QObject>
#include <QColor>
#include <QFont>

class CGeneralStyle : public QObject
{
    Q_OBJECT
private:
    CGeneralStyle();
    ~CGeneralStyle();

    static CGeneralStyle* gInstance;
    QString mWar3LobbyResourcePath;

    QColor  mFontColr;
    QFont   mFont;

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

    QString war3lobbyResourcePath();

    PingLevel getPingLevel(quint64 ping);

    QColor  fontColor();
    QFont   font();
};

#endif // CGENERALSTYLE_H
