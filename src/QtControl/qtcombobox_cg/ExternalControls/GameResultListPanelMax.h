#ifndef GAMERESULTLISTPANELMAX_H
#define GAMERESULTLISTPANELMAX_H

#include <QWidget>
#include "../CTreeViewEx.h"
#include <QStandardItemModel>
#include <QStandardItem>
#include "CGameResultTopPlayer_Ladder.h"
#include "game_result_view.h"
#include "RPGContentBase.h"

class GameResultListPanelMax : public RPGContentBase
{
    Q_OBJECT
public:
    GameResultListPanelMax(QWidget* parent = NULL);
    ~GameResultListPanelMax();

private:
    CTreeViewEx*        mTreeView;
    QStandardItemModel* mModel   ;
	GameResultType::ResultType mType;
    CTreeViewEx*        mTreeViewEx;
    QStandardItemModel* mModelEx;
    CGameResultTopPlayer_Ladder* mTopPlayer_Ladder;
	GameResultType::GameResult mGameResult;

    void resizeEvent(QResizeEvent *e);
    void layoutControls();
    void paintEvent(QPaintEvent *e);

	void setCampList(const GameResultType::CampList& li);
	void setResultType(GameResultType::ResultType t);

public:
	GameResultType::ResultType resultType();
	bool setGameResult(const GameResultType::GameResult& s);
};

#endif // GAMERESULTLISTPANELMAX_H
