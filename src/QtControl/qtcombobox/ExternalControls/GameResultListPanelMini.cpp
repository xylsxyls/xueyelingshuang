#include "GameResultListPanelMini.h"
#include <QPainter>
#include "../CGeneralStyle.h"

GameResultListPanelMini_Ladder::GameResultListPanelMini_Ladder(QWidget *parent)
	:RPGContentBase(parent)
{
	mLadderItem = new GameResultPanelSlotItem_Ladder;
	mLadderItem->setParent(this);

	connect(mLadderItem, SIGNAL(dataChanged()), this, SLOT(update()));
}

GameResultListPanelMini_Ladder::~GameResultListPanelMini_Ladder()
{

}

GameResultPanelSlotItem_Ladder *GameResultListPanelMini_Ladder::ladderItem()
{
	return mLadderItem;
}

bool GameResultListPanelMini_Ladder::setGameResult(const GameResultType::GameResult& s)
{
	GameResultType::Camp camp;
	GameResultType::Slot slot;
	for(int i = 0; i < s.campList.count(); i++)
	{
		GameResultType::Camp tc = s.campList[i];
		for(int j = 0; j < tc.slotList.count(); j ++)
		{
			GameResultType::Slot ts = tc.slotList[j];
			if(ts.isMe)
			{
				camp = tc;
				slot = ts;
				break;
			}
		}
	}

	if(slot.valueList.count() < 7)
	{
		return false;
	}

	mLadderItem->setCampName               (camp.campName     );
	mLadderItem->setPlayerName             (slot.playerName   );
	mLadderItem->setHero                   (slot.valueList [0]);
	mLadderItem->setKDA                    (slot.valueList [1]);
	mLadderItem->setHeroScore              (slot.valueList[2]);
	mLadderItem->setLadderScore            (slot.valueList[3]);
	mLadderItem->setHeroScoreProgress      (slot.valueProgressList[2]);
	mLadderItem->setLadderScoreProgress    (slot.valueProgressList[3]);

	QString tv = slot.valueList[6];
	quint64 tf = tv.toULongLong();
	mLadderItem->setTitleFlag              (tf);
	mLadderItem->setJun                    (tf& GameResultType::TitleFlags_BU );
	mLadderItem->setPo                     (tf& GameResultType::TitleFlags_PO );
	mLadderItem->setFu                     (tf& GameResultType::TitleFlags_FU );
	mLadderItem->setBu                     (tf& GameResultType::TitleFlags_BU );
	mLadderItem->setZhu                    (tf& GameResultType::TitleFlags_ZHU);
	mLadderItem->setHun                    (tf& GameResultType::TitleFlags_HUN);
	mLadderItem->setDataError              (s.dataError);
	mLadderItem->setInvalidGame            (s.invalidGame);

	return true;
}

void GameResultListPanelMini_Ladder::paintEvent(QPaintEvent *e)
{
	QPainter p(this);
	QRect itemRect(0, 24, width(), 41);
	QRect topRect(0,0,width(),24);

	p.fillRect(rect(), QColor(40,43,53));
	p.fillRect(itemRect, QColor(49,53,65));

	if(mLadderItem->invalidGame())
	{
		QFont f = CGeneralStyle::instance()->font();
		f.setPixelSize(12);

		p.setFont(f);
		p.setPen("white");
		p.drawText(itemRect.adjusted(13,0,0,0), Qt::AlignLeft|Qt::AlignVCenter, mLadderItem->playerName());

		p.setPen("red");
		p.drawText(itemRect.adjusted(140,0,0,0), Qt::AlignCenter, QStringLiteral("本局游戏无效"));
	}
	else if(mLadderItem->dataError())
	{
		QFont f = CGeneralStyle::instance()->font();
		f.setPixelSize(12);

		p.setFont(f);
		p.setPen("white");
		p.drawText(itemRect.adjusted(13,0,0,0), Qt::AlignLeft|Qt::AlignVCenter, mLadderItem->playerName());

		p.setPen("red");
		p.drawText(itemRect.adjusted(140,0,0,0), Qt::AlignCenter, QStringLiteral("本局数据出错"));
	}
	else
	{

		QFont f = CGeneralStyle::instance()->font();
		f.setPixelSize(12);
		p.setFont(f);

		if(mLadderItem->campName().contains(QStringLiteral("天灾军团")))
		{
			p.setPen("#e15e56");
		}
		else
		{
			p.setPen("#9fb740");
		}

		QRect nameRect = topRect.adjusted(0,0,-(this->width() - 99),0);
		p.drawText(nameRect,  Qt::AlignCenter,mLadderItem->campName());

		p.setPen(CGeneralStyle::instance()->fontColor());

		QRect heroRect = topRect.adjusted(99,0,-(this->width() - 99 - 50),0);
		p.drawText(heroRect, Qt::AlignCenter, QStringLiteral("英雄"));

		QRect kdaRect = topRect.adjusted(99 + 50,0,-(this->width() - 99 - 50 - 72),0);
		p.drawText(kdaRect, Qt::AlignCenter, QStringLiteral("K/D/A"));

		QRect heroScoreRect = topRect.adjusted(99 + 50 + 72,0,-(this->width() - 99 - 50 - 72 - 60),0);
		p.drawText(heroScoreRect, Qt::AlignCenter, QStringLiteral("英雄积分"));

		p.setPen("#7ea3ff");

		QRect ladderScoreRect = topRect.adjusted(99 + 50 + 72 + 60,0,0,0);
		p.drawText(ladderScoreRect, Qt::AlignCenter, QStringLiteral("天梯积分"));

		//-------------------

		p.setPen("white");
		f.setPixelSize(14);
		p.setFont(f);

		nameRect.setTop(itemRect.top());
		nameRect.setBottom(itemRect.bottom());
		p.drawText(nameRect,  Qt::AlignCenter,mLadderItem->playerName());

		kdaRect.setTop(itemRect.top());
		kdaRect.setBottom(itemRect.bottom());
		p.drawText(kdaRect        ,  Qt::AlignCenter,mLadderItem->KDA());

		f.setPixelSize(12);
		p.setFont(f);

		heroScoreRect.setTop(itemRect.top());
		heroScoreRect.setBottom(itemRect.bottom() - itemRect.height()/2);
		p.drawText(heroScoreRect  ,  Qt::AlignBottom | Qt::AlignHCenter,mLadderItem->heroScore());

		ladderScoreRect.setTop(itemRect.top());
		ladderScoreRect.setBottom(itemRect.bottom() - itemRect.height()/2);
		p.drawText(ladderScoreRect,  Qt::AlignBottom | Qt::AlignHCenter,mLadderItem->ladderScore());


		p.setPen("#e15e56");
		heroScoreRect.setTop(itemRect.top() + itemRect.height()/2);
		heroScoreRect.setBottom(itemRect.bottom());
		p.drawText(heroScoreRect  ,  Qt::AlignTop | Qt::AlignHCenter,mLadderItem->heroScoreProgress());

		ladderScoreRect.setTop(itemRect.top() + itemRect.height()/2);
		ladderScoreRect.setBottom(itemRect.bottom());
		p.drawText(ladderScoreRect,  Qt::AlignTop | Qt::AlignHCenter,mLadderItem->ladderScoreProgress());

		QPixmap titleIconList;
		titleIconList.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/TitleFlagIcon.png");

		int titleIconStep = titleIconList.height()/6;
		QList<QPixmap> plist;

		if(mLadderItem->po())
		{
			QPixmap p = titleIconList.copy(0, 0 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}

		if(mLadderItem->zhu())
		{
			QPixmap p = titleIconList.copy(0, 1 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mLadderItem->fu())
		{
			QPixmap p = titleIconList.copy(0, 2 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mLadderItem->jun())
		{
			QPixmap p = titleIconList.copy(0, 3 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mLadderItem->bu())
		{
			QPixmap p = titleIconList.copy(0, 4 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mLadderItem->hun())
		{
			QPixmap p = titleIconList.copy(0, 5 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}

		int titleWidth = (plist.count() - 1)*2 + plist.count() * titleIconList.width();
		int titleHeight = titleIconStep;
		QRect titleRect((width() - titleWidth)/2,itemRect.bottom() + 8, titleWidth, titleHeight);
		for(int i = 0; i < plist.count(); i++)
		{
			p.drawPixmap(titleRect.adjusted(i*titleIconStep,0, - titleRect.width() + (i+1)*titleIconStep,0), plist[i]);
		}

		QRect titleDescRect;
		titleDescRect.setLeft(0);
		titleDescRect.setRight(titleRect.left() - 8);
		titleDescRect.setTop(titleRect.top() - 5);
		titleDescRect.setBottom(titleRect.bottom());

		f.setPixelSize(14);
		p.setFont(f);
		p.setPen(CGeneralStyle::instance()->fontColor());

		if(!plist.isEmpty())
			p.drawText(titleDescRect, Qt::AlignVCenter|Qt::AlignRight, QStringLiteral("获得称号：") );
		else
			p.drawText(titleDescRect, Qt::AlignVCenter|Qt::AlignRight, QStringLiteral("获得称号：（无）") );
	}

}




////////////------------------
///
///


GameResultListPanelMini_MingJiang::GameResultListPanelMini_MingJiang(QWidget *parent)
	:RPGContentBase(parent)
{
	mMingJiangItem = new GameResultPanelSlotItem_MingJiang;
	mMingJiangItem->setParent(this);

	connect(mMingJiangItem, SIGNAL(dataChanged()), this, SLOT(update()));
}

GameResultListPanelMini_MingJiang::~GameResultListPanelMini_MingJiang()
{

}

GameResultPanelSlotItem_MingJiang *GameResultListPanelMini_MingJiang::ladderItem()
{
	return mMingJiangItem;
}

bool GameResultListPanelMini_MingJiang::setGameResult(const GameResultType::GameResult& s)
{
	GameResultType::Camp camp;
	GameResultType::Slot slot;
	for(int i = 0; i < s.campList.count(); i++)
	{
		GameResultType::Camp tc = s.campList[i];
		for(int j = 0; j < tc.slotList.count(); j ++)
		{
			GameResultType::Slot ts = tc.slotList[j];
			if(ts.isMe)
			{
				camp = tc;
				slot = ts;
				break;
			}
		}
	}

	if (slot.valueList.count() < 7)
	{
		return false;
	}

	mMingJiangItem->setCampName               (camp.campName     );
	mMingJiangItem->setPlayerName             (slot.playerName   );
	mMingJiangItem->setHero                   (slot.valueList [0]);
	mMingJiangItem->setKDA                    (slot.valueList [1]);
	mMingJiangItem->setHeroScore              (slot.valueList[2]);
	mMingJiangItem->setMingJiangScore         (slot.valueList[3]);
	mMingJiangItem->setHeroScoreProgress      (slot.valueProgressList[2]);
	mMingJiangItem->setMingJiangScoreProgress (slot.valueProgressList[3]);

	QString tv = slot.valueList[6];
	quint64 tf = tv.toULongLong();
	mMingJiangItem->setTitleFlag              (tf);
	mMingJiangItem->setJun                    (tf& GameResultType::TitleFlags_BU         );
	mMingJiangItem->setPo                     (tf& GameResultType::TitleFlags_PO         );
	mMingJiangItem->setFu                     (tf& GameResultType::TitleFlags_FU         );
	mMingJiangItem->setBu                     (tf& GameResultType::TitleFlags_BU         );
	mMingJiangItem->setZhu                    (tf& GameResultType::TitleFlags_ZHU        );
	mMingJiangItem->setHun                    (tf& GameResultType::TitleFlags_HUN        );
	mMingJiangItem->setDataError              (s.dataError);
	mMingJiangItem->setInvalidGame            (s.invalidGame);

	return true;
}

void GameResultListPanelMini_MingJiang::paintEvent(QPaintEvent *e)
{
	QPainter p(this);
	QRect itemRect(0, 24, width(), 41);
	QRect topRect(0,0,width(),24);

	p.fillRect(rect(), QColor(40,43,53));
	p.fillRect(itemRect, QColor(49,53,65));


	if(mMingJiangItem->invalidGame())
	{
		QFont f = CGeneralStyle::instance()->font();
		f.setPixelSize(12);

		p.setFont(f);
		p.setPen("white");
		p.drawText(itemRect.adjusted(13,0,0,0), Qt::AlignLeft|Qt::AlignVCenter, mMingJiangItem->playerName());

		p.setPen("red");
		p.drawText(itemRect.adjusted(140,0,0,0), Qt::AlignCenter, QStringLiteral("本局游戏无效"));
	}
	else if(mMingJiangItem->dataError())
	{
		QFont f = CGeneralStyle::instance()->font();
		f.setPixelSize(12);

		p.setFont(f);
		p.setPen("white");
		p.drawText(itemRect.adjusted(13,0,0,0), Qt::AlignLeft|Qt::AlignVCenter, mMingJiangItem->playerName());

		p.setPen("red");
		p.drawText(itemRect.adjusted(140,0,0,0), Qt::AlignCenter, QStringLiteral("本局数据出错"));
	}
	else
	{
		QFont f = CGeneralStyle::instance()->font();
		f.setPixelSize(12);
		p.setFont(f);

		if(mMingJiangItem->campName().contains(QStringLiteral("天灾军团")))
		{
			p.setPen("#e15e56");
		}
		else
		{
			p.setPen("#9fb740");
		}

		QRect nameRect = topRect.adjusted(0,0,-(this->width() - 99),0);
		p.drawText(nameRect,  Qt::AlignCenter,mMingJiangItem->campName());

		p.setPen(CGeneralStyle::instance()->fontColor());

		QRect heroRect = topRect.adjusted(99,0,-(this->width() - 99 - 105),0);
		p.drawText(heroRect, Qt::AlignCenter, QStringLiteral("英雄"));

		QRect kdaRect = topRect.adjusted(99 + 105,0,-(this->width() - 99 - 105 - 72),0);
		p.drawText(kdaRect, Qt::AlignCenter, QStringLiteral("K/D/A"));

		p.setPen("#7ea3ff");
		QRect mingJiangScoreRect = topRect.adjusted(99 + 105 + 72,0,0,0);
		p.drawText(mingJiangScoreRect, Qt::AlignCenter, QStringLiteral("名将值"));


		//-------------------

		p.setPen("white");
		f.setPixelSize(14);
		p.setFont(f);

		nameRect.setTop(itemRect.top());
		nameRect.setBottom(itemRect.bottom());
		p.drawText(nameRect,  Qt::AlignCenter,mMingJiangItem->playerName());

		kdaRect.setTop(itemRect.top());
		kdaRect.setBottom(itemRect.bottom());
		p.drawText(kdaRect        ,  Qt::AlignCenter,mMingJiangItem->KDA());

		f.setPixelSize(12);
		p.setFont(f);

		mingJiangScoreRect.setTop(itemRect.top());
		mingJiangScoreRect.setBottom(itemRect.bottom() - itemRect.height()/2);
		p.drawText(mingJiangScoreRect  ,  Qt::AlignBottom | Qt::AlignHCenter,mMingJiangItem->mingJiangScore());

		QPixmap titleIconList;
		titleIconList.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/TitleFlagIcon.png");

		int titleIconStep = titleIconList.height()/6;
		QList<QPixmap> plist;

		if(mMingJiangItem->po())
		{
			QPixmap p = titleIconList.copy(0, 0 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}

		if(mMingJiangItem->zhu())
		{
			QPixmap p = titleIconList.copy(0, 1 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mMingJiangItem->fu())
		{
			QPixmap p = titleIconList.copy(0, 2 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mMingJiangItem->jun())
		{
			QPixmap p = titleIconList.copy(0, 3 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mMingJiangItem->bu())
		{
			QPixmap p = titleIconList.copy(0, 4 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}
		if(mMingJiangItem->hun())
		{
			QPixmap p = titleIconList.copy(0, 5 * titleIconStep,titleIconList.width(), titleIconStep);
			plist << p;
		}

		int titleWidth = (plist.count() - 1)*2 + plist.count() * titleIconList.width();
		int titleHeight = titleIconStep;
		QRect titleRect((width() - titleWidth)/2,itemRect.bottom() + 8, titleWidth, titleHeight);
		for(int i = 0; i < plist.count(); i++)
		{
			p.drawPixmap(titleRect.adjusted(i*titleIconStep,0, - titleRect.width() + (i+1)*titleIconStep,0), plist[i]);
		}

		QRect titleDescRect;
		titleDescRect.setLeft(0);
		titleDescRect.setRight(titleRect.left() - 8);
		titleDescRect.setTop(titleRect.top() - 5);
		titleDescRect.setBottom(titleRect.bottom());

		f.setPixelSize(14);
		p.setFont(f);
		p.setPen(CGeneralStyle::instance()->fontColor());

		if(!plist.isEmpty())
			p.drawText(titleDescRect, Qt::AlignVCenter|Qt::AlignRight, QStringLiteral("获得称号：") );
		else
			p.drawText(titleDescRect, Qt::AlignVCenter|Qt::AlignRight, QStringLiteral("获得称号：（无）") );
	}

}
