#ifndef CGENERALSTYLE2_H
#define CGENERALSTYLE2_H

#include <QObject>
#include <QColor>
#include <QFont>

class CGeneralStyle2 : public QObject
{
    Q_OBJECT
private:
    CGeneralStyle2();
    ~CGeneralStyle2();

    static CGeneralStyle2* gInstance;
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

    static CGeneralStyle2* instance();
    static void           release();

    QString war3lobbyResourcePath();

    PingLevel getPingLevel(quint64 ping);

    QColor  fontColor();
    QFont   font();
};

#endif // CGENERALSTYLE_H
