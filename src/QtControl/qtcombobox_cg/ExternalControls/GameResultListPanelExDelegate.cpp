#include "GameResultListPanelExDelegate.h"

#include <QPainter>
#include "game_result_view.h"
#include "../CTreeViewEx.h"
#include "../CGeneralStyle.h"
#include <QDebug>
#include "GameResultListPanelMax.h"
#include "../COriginalButton.h"

GameResultListPanelExDelegate::GameResultListPanelExDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{

}

GameResultListPanelExDelegate::~GameResultListPanelExDelegate()
{

}

void GameResultListPanelExDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    paintBkg(painter, option, index);

    paintRPGSlot(painter, option, index);

    paintHoverFrame(painter, option, index);
}


void GameResultListPanelExDelegate::paintBkg(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;

    GameResultListPanelMax* panel = qobject_cast<GameResultListPanelMax*>(view->parent());
    if(panel == NULL)
        return;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());


    //bkg
    if(!index.parent().isValid())
    {
        GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));

        QColor bkgColor = "#252832";
        painter->fillRect(option.rect, bkgColor);
    }
    else
    {
        GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));

        QColor bkgColor = "#313541";
        painter->fillRect(option.rect, bkgColor);

        QColor dashLineColor = "#444a5a";
        painter->setPen(QPen(dashLineColor,1,Qt::DashLine));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

        if(slotItem->isMe())
        {
            QPixmap isMePix;
            isMePix.load(CGeneralStyle::instance()->war3lobbyResourcePath()+ "/Image/GameResultView/MySlotFrame.png");
            painter->drawPixmap(option.rect.adjusted(0,0,0,-2), isMePix);
        }
    }
}

void GameResultListPanelExDelegate::paintHoverFrame(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;

    GameResultListPanelMax* panel = qobject_cast<GameResultListPanelMax*>(view->parent());
    if(panel == NULL)
        return;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());


    bool isHovered = option.state.testFlag(QStyle::State_MouseOver);
    bool isSelected = option.state.testFlag(QStyle::State_Selected);

    //bkg
    //if(!index.parent().isValid())
    //{
    //    GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));
    //
    //}
    //else
    //{
        GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));



        if(isHovered)
        {
            QPixmap hoverFrame;
            hoverFrame.load(CGeneralStyle::instance()->war3lobbyResourcePath()+ "/Image/GameResultView/HoveredSlotFrame.png");
            painter->drawPixmap(option.rect, hoverFrame);
        }
        else if(isSelected)
        {
            painter->setPen(QPen(QColor(84,93,118), 2, Qt::SolidLine));
            painter->drawRect(option.rect.adjusted(1,1,-1,-1));
        }

    //}
}

void GameResultListPanelExDelegate::paintRPGSlot(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;

    QRect tagRect = option.rect;
    QPixmap pix;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));



    QFont f = CGeneralStyle::instance()->font();

    //bkg
    pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/ExtraBox.png");
    painter->drawPixmap(tagRect.adjusted(0,0,0,-1), pix);

    //biaozhang flag
    pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/OutstandingIcon.png");
    QRect biaoZhangFlag(tagRect.left() + 37, tagRect.top() + (tagRect.height() - pix.height())/2, pix.width(), pix.height());
    painter->drawPixmap(biaoZhangFlag, pix);




    //icon
    QRect iconRect;
    iconRect.setLeft(tagRect.left() + 303);
    iconRect.setTop(tagRect.top() + (tagRect.height() - 36)/2);
    iconRect.setBottom(tagRect.bottom() - (tagRect.height() - 36)/2 - 3);
    iconRect.setRight(iconRect.left() + 36 - 3);


    pix.load(slotItem->icon());
    pix = pix.scaled(iconRect.size());

    QPainterPath path;
    path.addRoundedRect(iconRect,4,4);
    painter->setPen("#abb5d0");
    painter->drawPath(path);


    path = QPainterPath();
    path.addRoundedRect(iconRect.adjusted(1,1,-1,-1),4,4);
    painter->save();
    painter->setClipPath(path);
    painter->drawPixmap(iconRect.adjusted(1,1,-1,-1), pix);
    painter->restore();


    //desc
    QRect descRect;
    descRect.setLeft(tagRect.left() + 126);
    descRect.setRight(iconRect.left());
    descRect.setTop(tagRect.top());
    descRect.setBottom(tagRect.bottom());
    f.setPixelSize(12);
    painter->setPen("#ffd080");
    painter->setFont(f);
    painter->drawText(descRect, Qt::AlignLeft|Qt::AlignVCenter, QStringLiteral("（本局名将值超过3的玩家）"));



    //name
    QRect nameRect;
    nameRect.setLeft(tagRect.left() + 360);
    nameRect.setTop(iconRect.top());
    nameRect.setBottom(iconRect.bottom());
    nameRect.setRight(tagRect.left()+513);


    bool isNew = slotItem->isNew();
    bool isDropNet = slotItem->isDropNet();
    bool isEscap = slotItem->isEscap();

    if(!isNew && !isDropNet && !isEscap)
    {
        f.setPixelSize(15);
        painter->setPen("white");
        painter->setFont(f);

        painter->drawText(nameRect, Qt::AlignLeft|Qt::AlignVCenter, slotItem->playerName());
    }
    else
    {
        painter->setPen("white");
        painter->drawText(nameRect, Qt::AlignLeft|Qt::AlignTop, slotItem->playerName());

        pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/PlayerFlagIcon.png");
        int pixStep = pix.height()/3;
        int pixWidth = pix.width();

        if(isEscap)
        {
            QPixmap tpix = pix.copy(0, 0 *pixStep,pixWidth, pixStep);
            painter->drawPixmap(QRect(nameRect.left(),nameRect.bottom() - pixStep,pixWidth,pixStep), tpix);
        }

        if(isNew)
        {
            QPixmap tpix = pix.copy(0, 2 *pixStep, pixWidth, pixStep);
            painter->drawPixmap(QRect(nameRect.left() + 2 + pixWidth,nameRect.bottom() - pixStep,pixWidth,pixStep), tpix);
        }

        if(isDropNet)
        {
            QPixmap tpix = pix.copy(0, 1 *pixStep, pixWidth, pixStep);
            painter->drawPixmap(QRect(nameRect.left() + 4 + 2*pixWidth,nameRect.bottom() - pixStep,pixWidth,pixStep), tpix);
        }
    }



    //is friend flag   && mvp flag && biaozhang

    if(slotItem->isMvp())
    {
        pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/SmallMVPIcon.png");
        QRect firendIcon;
        firendIcon.setLeft(tagRect.left() + 582);
        firendIcon.setTop(tagRect.top());
        firendIcon.setBottom(tagRect.bottom());
        firendIcon.setRight(firendIcon.left() + pix.width());
        painter->drawPixmap(firendIcon, pix);
    }
//    else if(slotItem->isBiaoZhang())
//    {

//    }
    else if(slotItem->isFriend())
    {
        pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/MeIcon.png");
        QRect firendIcon;
        firendIcon.setLeft(tagRect.left() + 1013);
        firendIcon.setTop(tagRect.top() + (tagRect.height() - pix.height())/2);
        firendIcon.setBottom(tagRect.bottom() - (tagRect.height() - pix.height())/2 - 3);
        firendIcon.setRight(firendIcon.left() + pix.width() - 3);
        painter->drawPixmap(firendIcon, pix);
    }



    // title flag
    if(slotItem->valueList().count() >= 6)
    {
        QString v = slotItem->valueList()[6];
        quint64 ttf =  v.toULongLong();
        pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/TitleFlagIcon.png");

        int titleIconStep = pix.height()/6;
        QList<QPixmap> plist;

        if(ttf & GameResultType::TitleFlags_PO)
        {
            QPixmap p = pix.copy(0, 0 * titleIconStep,pix.width(), titleIconStep);
            plist << p;
        }

		if (ttf & GameResultType::TitleFlags_ZHU)
        {
            QPixmap p = pix.copy(0, 1 * titleIconStep,pix.width(), titleIconStep);
            plist << p;
        }
		if (ttf & GameResultType::TitleFlags_FU)
        {
            QPixmap p = pix.copy(0, 2 * titleIconStep,pix.width(), titleIconStep);
            plist << p;
        }
		if (ttf & GameResultType::TitleFlags_JUN)
        {
            QPixmap p = pix.copy(0, 3 * titleIconStep,pix.width(), titleIconStep);
            plist << p;
        }
		if (ttf & GameResultType::TitleFlags_BU)
        {
            QPixmap p = pix.copy(0, 4 * titleIconStep,pix.width(), titleIconStep);
            plist << p;
        }
		if (ttf & GameResultType::TitleFlags_HUN)
        {
            QPixmap p = pix.copy(0, 5 * titleIconStep,pix.width(), titleIconStep);
            plist << p;
        }

        int titleWidth = (plist.count() - 1)*2 + plist.count() * pix.width();
        int titleHeight = titleIconStep;
        QRect titleRect(tagRect.left() + 680 + 2,tagRect.top() + (tagRect.height() - titleHeight)/2, titleWidth, titleHeight);
        for(int j = 0; j < plist.count(); j++)
        {
            painter->drawPixmap(titleRect.adjusted(j*pix.width(),0, - titleRect.width() + (j+1)*pix.width(),0), plist[j]);
        }
    }




//    //icon
//    QRect iconRect;
//    iconRect.setLeft(tagRect.left() + 63);
//    iconRect.setTop(tagRect.top() + (tagRect.height() - 36)/2);
//    iconRect.setBottom(tagRect.bottom() - (tagRect.height() - 36)/2 - 3);
//    iconRect.setRight(iconRect.left() + 36 - 3);


//    pix.load(slotItem->icon());
//    pix = pix.scaled(iconRect.size());

//    QPainterPath path;
//    path.addRoundedRect(iconRect,4,4);
//    painter->setPen("#abb5d0");
//    painter->drawPath(path);


//    path = QPainterPath();
//    path.addRoundedRect(iconRect.adjusted(1,1,-1,-1),4,4);
//    painter->save();
//    painter->setClipPath(path);
//    painter->drawPixmap(iconRect.adjusted(1,1,-1,-1), pix);
//    painter->restore();

//    //name
//    QRect nameRect;
//    nameRect.setLeft(tagRect.left() + 120);
//    nameRect.setTop(iconRect.top());
//    nameRect.setBottom(iconRect.bottom());
//    nameRect.setRight(tagRect.left()+266);


//    bool isNew = slotItem->isNew();
//    bool isDropNet = slotItem->isDropNet();
//    bool isEscap = slotItem->isEscap();

//    if(!isNew && !isDropNet && !isEscap)
//    {
//        f.setPixelSize(15);
//        painter->setPen("white");
//        painter->setFont(f);

//        painter->drawText(nameRect, Qt::AlignLeft|Qt::AlignVCenter, slotItem->playerName());
//    }
//    else
//    {
//        painter->drawText(nameRect, Qt::AlignLeft|Qt::AlignTop, slotItem->playerName());

//        pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/PlayerFlagIcon.png");
//        int pixStep = pix.height()/3;
//        int pixWidth = pix.width();

//        if(isEscap)
//        {
//            QPixmap tpix = pix.copy(0, 0 *pixStep,pixWidth, pixStep);
//            painter->drawPixmap(QRect(nameRect.left(),nameRect.bottom() - pixStep,pixWidth,pixStep), tpix);
//        }

//        if(isNew)
//        {
//            QPixmap tpix = pix.copy(0, 2 *pixStep, pixWidth, pixStep);
//            painter->drawPixmap(QRect(nameRect.left() + 2 + pixWidth,nameRect.bottom() - pixStep,pixWidth,pixStep), tpix);
//        }

//        if(isDropNet)
//        {
//            QPixmap tpix = pix.copy(0, 1 *pixStep, pixWidth, pixStep);
//            painter->drawPixmap(QRect(nameRect.left() + 4 + 2*pixWidth,nameRect.bottom() - pixStep,pixWidth,pixStep), tpix);
//        }
//    }


//    //score
//    if(slotItem->valueList().count() != slotItem->valueProgressList().count())
//        return;

//    QColor scoreColor = "white";
//    QColor scoreProgressColorAdd = "#ee584d";
//    QColor scoreProgressColorReduce = "#8de32c";;

//    QStringList valueList       = slotItem->valueList();
//    QStringList valueProgressList = slotItem->valueProgressList();
//    for(int i = 0; i < valueList.count(); i++)
//    {
//        QString v = valueList[i];
//        QString vp= valueProgressList[i];
//        QRect rt = tagRect;
//        rt.setTop(iconRect.top());
//        rt.setBottom(iconRect.bottom());
//        rt.setLeft(rt.left() + 302 + i*84);
//        rt.setRight(rt.left() + 84);
//        if(vp.isEmpty())
//        {
//            f.setPixelSize(15);
//            painter->setFont(f);
//            painter->setPen(scoreColor);
//            painter->drawText(rt, Qt::AlignCenter, v);
//        }
//        else
//        {
//            f.setPixelSize(15);
//            painter->setFont(f);
//            painter->setPen(scoreColor);
//            painter->drawText(rt, Qt::AlignHCenter|Qt::AlignTop, v);

//            f.setPixelSize(12);
//            painter->setFont(f);
//            painter->setPen(vp.contains("-") ? scoreProgressColorReduce : scoreProgressColorAdd);
//            painter->drawText(rt, Qt::AlignHCenter|Qt::AlignBottom, vp);
//        }

//    }

}
