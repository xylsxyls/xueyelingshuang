#include "GameResultListPanelMaxDelegate.h"

#include <QPainter>
#include "game_result_view.h"
#include "../CTreeViewEx.h"
#include "../CGeneralStyle.h"
#include <QDebug>
#include "GameResultListPanelMax.h"
#include "../COriginalButton.h"

GameResultListPanelMaxDelegate::GameResultListPanelMaxDelegate(QObject *parent)
    :QStyledItemDelegate(parent)
{

}

GameResultListPanelMaxDelegate::~GameResultListPanelMaxDelegate()
{

}

void GameResultListPanelMaxDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;

    GameResultListPanelMax* panel = qobject_cast<GameResultListPanelMax*>(view->parent());
    if(panel == NULL)
        return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);


    paintBkg(painter, option, index);

	if (panel->resultType() == GameResultType::ResultType_RPG)
    {
        if(!index.parent().isValid())
        {
            paintRPGCamp(painter,option,index);
        }
        else
        {
            paintRPGSlot(painter,option,index);
        }
    }
	else if (panel->resultType() == GameResultType::ResultType_Ladder)
    {
        if(!index.parent().isValid())
        {
            paintLadderCamp(painter,option,index);
        }
        else
        {
            paintLadderSlot(painter,option,index);
        }

    }

    paintHoverFrame(painter, option, index);


    painter->restore();

}

void GameResultListPanelMaxDelegate::paintBkg(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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

void GameResultListPanelMaxDelegate::paintHoverFrame(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
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
    if(!index.parent().isValid())
    {
        GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));

    }
    else
    {
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

    }
}

void GameResultListPanelMaxDelegate::paintRPGCamp(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;


    QRect tagRect = option.rect;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));

    //camp name
    QColor campNameColor = "#ffd080";
    QFont  f = CGeneralStyle::instance()->font();
    f.setPixelSize(16);

    painter->setFont(f);
    painter->setPen(campNameColor);
    painter->drawText(tagRect.adjusted(54,0,0,0), Qt::AlignVCenter|Qt::AlignLeft, campItem->campName());

    //header
    QColor headerColor = "#969db6";
    f.setPixelSize(15);
    painter->setPen(headerColor);
    painter->setFont(f);

    QStringList headerList = campItem->headerList();
    for(int i = 0; i < headerList.count(); i++)
    {
        QString h = headerList[i];
        QRect rt = tagRect;
        rt.setLeft(rt.left() + 302 + i*84);
        rt.setRight(rt.left() + 84);
        painter->drawText(rt, Qt::AlignCenter, h);
    }
}

void GameResultListPanelMaxDelegate::paintRPGSlot(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;

    QRect tagRect = option.rect;
    QPixmap pix;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));



    QFont f = CGeneralStyle::instance()->font();

    //is friend flag   && mvp flag && biaozhang

    if(slotItem->isMvp())
    {
        pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/SmallMVPIcon.png");
        QRect firendIcon;
        firendIcon.setLeft(0);
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
        firendIcon.setLeft(tagRect.left() + 22);
        firendIcon.setTop(tagRect.top() + (tagRect.height() - pix.height())/2);
        firendIcon.setBottom(tagRect.bottom() - (tagRect.height() - pix.height())/2 - 3);
        firendIcon.setRight(firendIcon.left() + pix.width() - 3);
        painter->drawPixmap(firendIcon, pix);
    }



    //icon
    QRect iconRect;
    iconRect.setLeft(tagRect.left() + 63);
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

    //name
    QRect nameRect;
    nameRect.setLeft(tagRect.left() + 120);
    nameRect.setTop(iconRect.top());
    nameRect.setBottom(iconRect.bottom());
    nameRect.setRight(tagRect.left()+266);


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


    //score
    if(slotItem->valueList().count() != slotItem->valueProgressList().count())
        return;

    QColor scoreColor = "white";
    QColor scoreProgressColorAdd = "#ee584d";
    QColor scoreProgressColorReduce = "#8de32c";;

    QStringList valueList       = slotItem->valueList();
    QStringList valueProgressList = slotItem->valueProgressList();
    for(int i = 0; i < valueList.count(); i++)
    {
        QString v = valueList[i];
        QString vp= valueProgressList[i];
        QRect rt = tagRect;
        rt.setTop(iconRect.top());
        rt.setBottom(iconRect.bottom());
        rt.setLeft(rt.left() + 302 + i*84);
        rt.setRight(rt.left() + 84);
        if(vp.isEmpty())
        {
            f.setPixelSize(15);
            painter->setFont(f);
            painter->setPen(scoreColor);
            painter->drawText(rt, Qt::AlignCenter, v);
        }
        else
        {
            f.setPixelSize(15);
            painter->setFont(f);
            painter->setPen(scoreColor);
            painter->drawText(rt, Qt::AlignHCenter|Qt::AlignTop, v);

            f.setPixelSize(12);
            painter->setFont(f);
            painter->setPen(vp.contains("-") ? scoreProgressColorReduce : scoreProgressColorAdd);
            painter->drawText(rt, Qt::AlignHCenter|Qt::AlignBottom, vp);
        }

    }

}

void GameResultListPanelMaxDelegate::paintLadderCamp(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;


    QRect tagRect = option.rect;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));


    QPixmap victoryIcon;
    victoryIcon.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallResultIcon.png");

    QPixmap defiedIcon = victoryIcon.copy(0, victoryIcon.height()/2, victoryIcon.width(), victoryIcon.height()/2);
    victoryIcon = victoryIcon.copy(0, 0, victoryIcon.width(), victoryIcon.height()/2);

    //camp name
    QColor campNameColor = "#ffd080";
    QFont  f = CGeneralStyle::instance()->font();
    f.setPixelSize(16);

    painter->setFont(f);
    painter->setPen(campNameColor);
    painter->drawText(tagRect.adjusted(54,0,0,0), Qt::AlignVCenter|Qt::AlignLeft, campItem->campName());

    //header
    QColor headerColor = "#969db6";
    f.setPixelSize(15);
    painter->setPen(headerColor);
    painter->setFont(f);

    QStringList headerList = campItem->headerList();
    for(int i = 0; i < headerList.count(); i++)
    {
        QString h = headerList[i];
        QRect rt = tagRect;
        if(i == 0)
        {
            rt.setLeft(rt.left() + 302);
            rt.setRight(rt.left() + 66);
        }
        else if(i == 1)
        {
            rt.setLeft(rt.left() + 302 + 66);
            rt.setRight(rt.left() + 103);
        }
        else if(i == 2 || i == 3)
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (i - 2)*106);
            rt.setRight(rt.left() + 106);
        }
        else if(i == 4 || i == 5)
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (2)*106 + (i - 4)*75);
            rt.setRight(rt.left() + 75);
        }
        else if(i == 6)
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (2)*106 + (2)*75);
            rt.setRight(rt.left() + 106);
        }
        else
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (2)*106 + (2)*75 + (i-6)*106);
            rt.setRight(rt.left() + 88);
        }
        painter->drawText(rt, Qt::AlignCenter, h);
    }
}

void GameResultListPanelMaxDelegate::paintLadderSlot(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return;

    QRect tagRect = option.rect;
    QPixmap pix;

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));

    QFont f = CGeneralStyle::instance()->font();

    //is friend flag   && mvp flag && biaozhang

    if(slotItem->isMvp())
    {
        pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/SmallMVPIcon.png");
        QRect firendIcon;
        firendIcon.setLeft(0);
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
        firendIcon.setLeft(tagRect.left() + 22);
        firendIcon.setTop(tagRect.top() + (tagRect.height() - pix.height())/2);
        firendIcon.setBottom(tagRect.bottom() - (tagRect.height() - pix.height())/2 - 3);
        firendIcon.setRight(firendIcon.left() + pix.width() - 3);
        painter->drawPixmap(firendIcon, pix);
    }


    //icon
    QRect iconRect;
    iconRect.setLeft(tagRect.left() + 63);
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

    //name
    QRect nameRect;
    nameRect.setLeft(tagRect.left() + 120);
    nameRect.setTop(iconRect.top());
    nameRect.setBottom(iconRect.bottom());
    nameRect.setRight(tagRect.left()+266);


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


    //score
    if(slotItem->valueList().count() != slotItem->valueProgressList().count())
        return;

    QColor scoreColor = "white";
    QColor scoreProgressColorAdd = "#ee584d";
    QColor scoreProgressColorReduce = "#8de32c";;

    QStringList valueList       = slotItem->valueList();
    QStringList valueProgressList = slotItem->valueProgressList();
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

    for(int i = 0; i < valueList.count(); i++)
    {
        QString v = valueList[i];
        QString vp= valueProgressList[i];
        QRect rt = tagRect;
        rt.setTop(iconRect.top());
        rt.setBottom(iconRect.bottom());
        if(i == 0)
        {
            rt.setLeft(rt.left() + 302);
            rt.setRight(rt.left() + 66);
        }
        else if(i == 1)
        {
            rt.setLeft(rt.left() + 302 + 66);
            rt.setRight(rt.left() + 103);
        }
        else if(i == 2 || i == 3)
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (i - 2)*106);
            rt.setRight(rt.left() + 106);
        }
        else if(i == 4 || i == 5)
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (2)*106 + (i - 4)*75);
            rt.setRight(rt.left() + 75);
        }
        else if(i == 6)
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (2)*106 + (2)*75);
            rt.setRight(rt.left() + 106);
        }
        else
        {
            rt.setLeft(rt.left() + 302 + 66 + 103 + (2)*106 + (2)*75 + (i-6)*106);
            rt.setRight(rt.left() + 88);
        }


        if(i == 0)
        {
            painter->save();
            QRect heroRect = rt.adjusted((rt.width() - 30)/2, (rt.height() - 30)/2, - (rt.width() - 30)/2, -(rt.height() - 30)/2);

            QPainterPath path;
            path.addRoundedRect(heroRect,15,15);
            painter->setClipPath(path);

            pix.load(v);
            painter->drawPixmap(heroRect, pix);

            pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/BigHeroAvatarFrame.png");
            painter->drawPixmap(heroRect, pix);

            painter->restore();
        }
        else if(i == 6)
        {
            quint64 ttf = v.toULongLong();

            pix.load(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/TitleFlagIcon.png");

            int titleIconStep = pix.height()/6;
            QList<QPixmap> plist;

			if (ttf & GameResultType::TitleFlags_PO)
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
            QRect titleRect(rt.left() + 2 + (rt.width() - titleWidth)/2,rt.top() + (rt.height() - titleHeight)/2, titleWidth, titleHeight);
            for(int j = 0; j < plist.count(); j++)
            {
                painter->drawPixmap(titleRect.adjusted(j*pix.width(),0, - titleRect.width() + (j+1)*pix.width(),0), plist[j]);
            }
        }
        else
        {
            if(vp.isEmpty())
            {
                f.setPixelSize(15);
                painter->setFont(f);
                painter->setPen(scoreColor);
                painter->drawText(rt, Qt::AlignCenter, v);
            }
            else
            {
                f.setPixelSize(15);
                painter->setFont(f);
                painter->setPen(scoreColor);
                painter->drawText(rt, Qt::AlignHCenter|Qt::AlignTop, v);

                f.setPixelSize(12);
                painter->setFont(f);
                painter->setPen(vp.contains("-") ? scoreProgressColorReduce : scoreProgressColorAdd);
                painter->drawText(rt, Qt::AlignHCenter|Qt::AlignBottom, vp);
            }
        }

    }
}

QWidget* GameResultListPanelMaxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return QStyledItemDelegate::createEditor(parent, option, index);

    GameResultListPanelMax* panel = qobject_cast<GameResultListPanelMax*>(view->parent());
    if(panel == NULL)
        return QStyledItemDelegate::createEditor(parent, option, index);

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    if(!index.parent().isValid())
    {
        GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));

        COriginalButton* btn = new COriginalButton(parent);
        btn->setFocusPolicy(Qt::NoFocus);

        return btn;

    }
    else
    {
        GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));

        COriginalButton* btn = new COriginalButton(parent);
        btn->setFocusPolicy(Qt::NoFocus);

        return btn;

    }
}

void GameResultListPanelMaxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
    {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
        return;
    }

    GameResultListPanelMax* panel = qobject_cast<GameResultListPanelMax*>(view->parent());
    if(panel == NULL)
    {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
        return;
    }

    QStandardItemModel* model = (QStandardItemModel*)(view->model());

    if(!index.parent().isValid())
    {
        GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));

        QRect victoryFlagRect(4, option.rect.top(), 44,52);
        editor->setGeometry(victoryFlagRect);
    }
    else
    {
        GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));

        QRect isFrendFlagRect(slotItem->isFriend() ? 266 : -266, option.rect.top() + (option.rect.height() - 22)/2, 22,22);
        editor->setGeometry(isFrendFlagRect);
    }
}

void GameResultListPanelMaxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    CTreeViewEx* view = qobject_cast<CTreeViewEx*>(this->parent());
    if(view == NULL)
        return QStyledItemDelegate::setEditorData(editor, index);

    GameResultListPanelMax* panel = qobject_cast<GameResultListPanelMax*>(view->parent());
    if(panel == NULL)
        return QStyledItemDelegate::setEditorData(editor, index);

    QStandardItemModel* model = (QStandardItemModel*)(view->model());
    if(!index.parent().isValid())
    {
        GameResultPanelCampItem_Max* campItem = (GameResultPanelCampItem_Max*)(model->itemFromIndex(index));
        bool isVictory = campItem->isVictory();
        COriginalButton* btn = (COriginalButton*)editor;
        btn->setText("");
        if(isVictory)
        {
            btn->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallResultIcon.png",2,1,1,1,1,1,1,1,1);
        }
        else
        {
            btn->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallResultIcon.png",2,2,2,2,2,2,2,2,2);
        }
    }
    else
    {
        GameResultPanelSlotItem_Max* slotItem = (GameResultPanelSlotItem_Max*)(model->itemFromIndex(index));
        bool isFriend = slotItem->isFriend();
        quint64 accountId = slotItem->accountId();
        COriginalButton* btn = (COriginalButton*)editor;
        btn->setText("");
        btn->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/AddFriendButton.png",4,1,2,3,4,1,2,3,4);

        btn->setVisible(!isFriend);
        btn->setEnabled(!isFriend);

        btn->setProperty("accountId", accountId);
    }
}

