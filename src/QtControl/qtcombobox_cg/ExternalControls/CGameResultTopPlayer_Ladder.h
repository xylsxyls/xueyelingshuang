#ifndef CGAMERESULTTOPPLAYER_LADDER_H
#define CGAMERESULTTOPPLAYER_LADDER_H

#include <QWidget>

class CGameResultTopPlayer_Ladder: public QWidget
{
public:
    CGameResultTopPlayer_Ladder(QWidget* parent = NULL);
    ~CGameResultTopPlayer_Ladder();

private:
    QImage mCampFramePix;
    QImage mCampFramePixRight;
    QImage mWinPix;
    QString mLeftName;
    QString mLeftScore;
    QString mRightName;
    QString mRightScore;
    bool    mIsLeftWin;

    void paintEvent(QPaintEvent *e);

public:
    void setLeftName(const QString& s);
    void setLeftScore(const QString& s);
    void setRightName(const QString& s);
    void setRightScore(const QString& s);
    void setIsLeftWin(bool s);
};

#endif // CGAMERESULTTOPPLAYER_LADDER_H
