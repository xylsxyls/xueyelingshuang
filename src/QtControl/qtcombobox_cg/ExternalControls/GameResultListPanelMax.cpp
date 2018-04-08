#include "GameResultListPanelMax.h"
#include "game_result_view.h"
#include <QHeaderView>
#include <QPainter>
#include "GameResultListPanelMaxDelegate.h"
#include "GameResultListPanelExDelegate.h"


GameResultListPanelMax::GameResultListPanelMax(QWidget *parent)
    :RPGContentBase(parent)
    ,mTreeView(new CTreeViewEx(this))
    ,mModel(new QStandardItemModel(this))
	,mType(GameResultType::ResultType_RPG)
    ,mModelEx(new QStandardItemModel(this))
    ,mTreeViewEx(new CTreeViewEx(this))
    ,mTopPlayer_Ladder(new CGameResultTopPlayer_Ladder(this))
{
    mTreeView->setModel(mModel);
    mTreeView->header()->setVisible(false);
    mTreeView->setExpandsOnDoubleClick(false);
    mTreeView->setIndentation(0);
    mTreeView->setSelectionBehavior(CTreeViewEx::SelectRows);
    mTreeView->setSelectionMode(CTreeViewEx::SingleSelection);
    GameResultListPanelMaxDelegate* tDelegate = new GameResultListPanelMaxDelegate(mTreeView);
    mTreeView->setItemDelegate(tDelegate);
//    mTreeView->setMouseTracking(true);
//    this->setMouseTracking(true);

    mTreeViewEx->setModel(mModelEx);
    mTreeViewEx->header()->setVisible(false);
    mTreeViewEx->setExpandsOnDoubleClick(false);
    mTreeViewEx->setIndentation(0);
    mTreeViewEx->setSelectionBehavior(CTreeViewEx::SelectRows);
    mTreeViewEx->setSelectionMode(CTreeViewEx::NoSelection);
    GameResultListPanelExDelegate* tDelegateEx = new GameResultListPanelExDelegate(mTreeViewEx);
    mTreeViewEx->setItemDelegate(tDelegateEx);

	GameResultType::CampList cli;
    for(int i = 0; i < 2; i++)
    {
        quint64 tff = 0;
		tff |= quint64(GameResultType::TitleFlags_JUN);
		tff |= quint64(GameResultType::TitleFlags_PO);
		tff |= quint64(GameResultType::TitleFlags_FU);
		tff |= quint64(GameResultType::TitleFlags_BU);
		tff |= quint64(GameResultType::TitleFlags_ZHU);
		tff |= quint64(GameResultType::TitleFlags_HUN);

		GameResultType::Camp c;
        c.isVictory = ((i % 2) == 0);
        c.campName = QString("camp %1").arg(i);
        c.headerList << "D:/SVN/output/W3L/res/Data/War3/GameChannel/6.png" << "2" << "3"<< "4"<< "5"
                     << "6" << QString::number(
                            tff
                            )<< "8";

        for(int j = 0; j < 5; j++)
        {
			GameResultType::Slot s;
            s.playerName  = QString("player %1").arg(j);


            s.valueList = c.headerList;
            //s.headerList = c.headerList;
            QStringList valueProgressLi;
            valueProgressLi << "+1" << "-2" << "+3"<< ""/*"-4"*/<< "+5"<< "+6"<< "-7"<< "+8";


            s.valueProgressList = valueProgressLi;
            s.icon = "D:/SVN/output/W3L/res/Data/War3/GameChannel/6.png";
            s.isDropNet = true;
            s.isEscap = true;
            s.isFriend =  ((j % 2) == 0) ? true : false;
            s.isNew = true;
            s.isBiaoZhang = j % 2 == 0;
            if(j == 0)
                s.isMe = true;

            if(j == 3)
                s.isMvp = true;

            //d
            s.isMvp = true;


            c.slotList << s;
        }

        cli << c;
    }

    this->setCampList(cli);

	this->setResultType(GameResultType::ResultType_Ladder);
}

GameResultListPanelMax::~GameResultListPanelMax()
{

}

void GameResultListPanelMax::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    this->layoutControls();
}

void GameResultListPanelMax::layoutControls()
{
//    mTopPlayer_Ladder->setVisible(false);
//    mTreeView->setVisible(false);

	QRect topPlayerLadderRect;
	topPlayerLadderRect.setLeft(0);
	topPlayerLadderRect.setRight(width());
	topPlayerLadderRect.setTop(0);
	topPlayerLadderRect.setBottom(71);

	QRect treeViewExRect;
	treeViewExRect.setLeft(0);
	treeViewExRect.setRight(width());
	treeViewExRect.setTop(topPlayerLadderRect.bottom() + 2);
	treeViewExRect.setBottom(treeViewExRect.top() + mModelEx->rowCount() * 51);

	QRect treeViewRect;
	treeViewRect.setLeft(0);
	treeViewRect.setRight(width());
	treeViewRect.setTop(treeViewExRect.bottom());
	treeViewRect.setBottom(height());

	mTopPlayer_Ladder->setGeometry(topPlayerLadderRect);
	mTreeViewEx->setGeometry(treeViewExRect);
	mTreeView->setGeometry(treeViewRect);
}

void GameResultListPanelMax::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.fillRect(rect(), "#313541");
}

void GameResultListPanelMax::setCampList(const GameResultType::CampList &li)
{
    //表彰列表
    mModelEx->clear();
    for(int i = 0; i < li.count(); i++)
    {
		GameResultType::Camp camp = li[i];

        for(int j = 0; j < camp.slotList.count(); j++)
        {
			GameResultType::Slot slot = camp.slotList[j];
            if(!slot.isBiaoZhang)
                continue;

            GameResultPanelSlotItem_Max* slotItem = new GameResultPanelSlotItem_Max;
            slotItem->setIsMvp            (slot.isMvp            );
            slotItem->setIsFriend         (slot.isFriend         );
            slotItem->setIcon             (slot.icon             );
            slotItem->setPlayerName       (slot.playerName       );
            slotItem->setIsEscap          (slot.isEscap          );
            slotItem->setIsNew            (slot.isNew            );
            slotItem->setIsDropNet        (slot.isDropNet        );
            //slotItem->setHeaderList       (slot.headerList       );
            slotItem->setValueList        (slot.valueList        );
            slotItem->setValueProgressList(slot.valueProgressList);
            slotItem->setAccountId        (slot.accountId        );
            slotItem->setEx               (slot.titleFlags       );
            slotItem->setIsBiaoZhang      (slot.isBiaoZhang      );
            slotItem->setIsMe             (slot.isMe             );

            slotItem->setSizeHint(QSize(0,51));
            mModelEx->appendRow(slotItem);

        }

    }
    mTreeViewEx->expandAll();



    // 正常列表

    mModel->clear();
    for(int i = 0; i < li.count(); i++)
    {
		GameResultType::Camp camp = li[i];

        GameResultPanelCampItem_Max* campItem = new GameResultPanelCampItem_Max;
        campItem->setCampName(camp.campName);
        campItem->setIsVictory(camp.isVictory);
        campItem->setHeaderList(camp.headerList);
        campItem->setSizeHint(QSize(0,34));

        for(int j = 0; j < camp.slotList.count(); j++)
        {
			GameResultType::Slot slot = camp.slotList[j];
            GameResultPanelSlotItem_Max* slotItem = new GameResultPanelSlotItem_Max;
            slotItem->setIsMvp            (slot.isMvp            );
            slotItem->setIsFriend         (slot.isFriend         );
            slotItem->setIcon             (slot.icon             );
            slotItem->setPlayerName       (slot.playerName       );
            slotItem->setIsEscap          (slot.isEscap          );
            slotItem->setIsNew            (slot.isNew            );
            slotItem->setIsDropNet        (slot.isDropNet        );
            //slotItem->setHeaderList       (slot.headerList       );
            slotItem->setValueList        (slot.valueList        );
            slotItem->setValueProgressList(slot.valueProgressList);
            slotItem->setAccountId        (slot.accountId        );
            slotItem->setEx               (slot.titleFlags       );
            slotItem->setIsBiaoZhang      (slot.isBiaoZhang      );
            slotItem->setIsMe             (slot.isMe             );

            slotItem->setSizeHint(QSize(0,45));

            campItem->appendRow(slotItem);
        }

        mModel->appendRow(campItem);
    }

    mTreeView->expandAll();

    for(int i = 0; i < mModel->rowCount(); i++)
    {
        QModelIndex campIndex = mModel->index(i,0);
        for(int j = 0; j < mModel->rowCount(campIndex); j++)
        {
            QModelIndex slotIndex = mModel->index(j,0,campIndex);
            mTreeView->openPersistentEditor(slotIndex);
        }
        mTreeView->openPersistentEditor(campIndex);
    }
}

void GameResultListPanelMax::setResultType(GameResultType::ResultType t)
{
    mType = t;
    mTreeView->update();
}

GameResultType::ResultType GameResultListPanelMax::resultType()
{
    return mType;
}

bool GameResultListPanelMax::setGameResult(const GameResultType::GameResult &s)
{
    mGameResult = s;

    this->setCampList(s.campList);
    this->setResultType(s.type);

    this->update();

	return true;
}


