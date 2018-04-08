#include "CGameResultTopPlayer_Ladder.h"

#include "../CGeneralStyle.h"

#include <QPainter>

CGameResultTopPlayer_Ladder::CGameResultTopPlayer_Ladder(QWidget *parent)
    :QWidget(parent)
    ,mIsLeftWin(true)
{
    QPixmap pix;
    pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/BigCampCell.png");

    mCampFramePixRight = pix.toImage();
    mCampFramePix = mCampFramePixRight.mirrored(true,false);

    pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/VictoryIcon.png");
    mWinPix = pix.toImage();


    mLeftName = "left name";
    mRightName = "left name";

    mLeftScore = "left score";
    mRightScore = "left score";
}

CGameResultTopPlayer_Ladder::~CGameResultTopPlayer_Ladder()
{

}

void CGameResultTopPlayer_Ladder::paintEvent(QPaintEvent *e)
{
 //42
    QRect leftRect;
    leftRect.setLeft(0);
    leftRect.setTop(0);
    leftRect.setBottom(this->height()- 1);
    leftRect.setRight(this->width()/2 - 21);

    QRect rightRect;
    rightRect.setLeft(this->width()/2 + 21);
    rightRect.setTop(0);
    rightRect.setBottom(this->height() - 1);
    rightRect.setRight(this->width());

    QRect leftWinRect(leftRect.right() - 33 - 21, leftRect.top() + 15, 21, 19);
    QRect rightWinRect(rightRect.left() + 33, leftRect.top() + 15, 21, 19);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), "#313541");

    p.drawImage(leftRect.adjusted(leftRect.width() - mCampFramePix.width(),0,0,0) , mCampFramePix     .copy(0, mIsLeftWin ? 1* mCampFramePix.height()/2 : 0, mCampFramePix.width(), mCampFramePix.height()/2));
    p.drawImage(rightRect.adjusted(0,0,-(rightRect.width() - mCampFramePixRight.width()),0), mCampFramePixRight.copy(0, mIsLeftWin ? 0 : 1* mCampFramePix.height()/2, mCampFramePix.width(), mCampFramePix.height()/2));

    if(mIsLeftWin)
    {
        p.drawImage(leftWinRect, mWinPix);
    }
    else
    {
        p.drawImage(rightWinRect, mWinPix);
    }



    QRect leftNameRect =  leftRect.adjusted(0,15, -57, -15);
    QRect rightNameRect = rightRect.adjusted(57,15, 0, -15);

    QFont f = CGeneralStyle::instance()->font();
    QColor red = "#e15e56";
    QColor green = "#9fb740";
    QColor white = "#ffffff";

    f.setPixelSize(18);

    p.setPen(red);
    p.setFont(f);
    p.drawText(leftNameRect, Qt::AlignRight | Qt::AlignTop, mLeftName);
    p.setPen(green);
    p.drawText(rightNameRect, Qt::AlignLeft | Qt::AlignTop, mRightName);

    p.setPen(white);
    p.drawText(leftNameRect, Qt::AlignRight | Qt::AlignBottom, mLeftScore);
    p.drawText(rightNameRect, Qt::AlignLeft | Qt::AlignBottom, mRightScore);
}

void CGameResultTopPlayer_Ladder::setLeftName(const QString &s)
{
    mLeftName = s;
    this->update();
}

void CGameResultTopPlayer_Ladder::setLeftScore(const QString &s)
{
    mLeftScore = s;
    this->update();
}

void CGameResultTopPlayer_Ladder::setRightName(const QString &s)
{
    mRightName = s;
    this->update();
}

void CGameResultTopPlayer_Ladder::setRightScore(const QString &s)
{
    mRightScore = s;
    this->update();
}

void CGameResultTopPlayer_Ladder::setIsLeftWin(bool s)
{
    mIsLeftWin = s;
    this->update();
}

