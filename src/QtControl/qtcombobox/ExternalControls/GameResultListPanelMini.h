#ifndef GAMERESULTLISTPANELMINI_H
#define GAMERESULTLISTPANELMINI_H

#include "../CTreeViewEx.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include "game_result_view.h"
#include "RPGContentBase.h"

class GameResultListPanelMini_Ladder : public RPGContentBase
{
    Q_OBJECT
public:
    GameResultListPanelMini_Ladder(QWidget* parent = NULL);
    ~GameResultListPanelMini_Ladder();

    GameResultPanelSlotItem_Ladder* ladderItem();
	bool setGameResult(const GameResultType::GameResult& s);
private:
    GameResultPanelSlotItem_Ladder* mLadderItem;

    void  paintEvent(QPaintEvent *e);
};

class GameResultListPanelMini_MingJiang : public RPGContentBase
{
    Q_OBJECT
public:
    GameResultListPanelMini_MingJiang(QWidget* parent = NULL);
    ~GameResultListPanelMini_MingJiang();

    GameResultPanelSlotItem_MingJiang* ladderItem();
	bool setGameResult(const GameResultType::GameResult& s);
private:
    GameResultPanelSlotItem_MingJiang* mMingJiangItem;

    void paintEvent(QPaintEvent *e);
};

#endif // GAMERESULTLISTPANELMINI_H
