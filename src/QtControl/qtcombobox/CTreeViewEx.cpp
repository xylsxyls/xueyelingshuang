#include "CTreeViewEx.h"

#include <QHeaderView>
#include <QMouseEvent>

CTreeViewEx::CTreeViewEx(QWidget *parent)
    :QTreeView(parent)
    ,mHoveredColumn(0)
    ,mHoveredRow   (0)
{
    this->setMouseTracking(true);
    this->header()->setMouseTracking(true);


    this->setStyleSheet("CTreeViewEx{                                      \n\
                             background-color:rgba(0,0,0,0);                   \n\
                             border:none;                                      \n\
                            }                                                  \n\
                            QScrollBar {									   \n\
                            border: none;									   \n\
                            background-color: rgba(0,0,0,0);				   \n\
                            height: 6px;									   \n\
                            width: 6px;										   \n\
                            }												   \n\
                            QScrollBar::handle {						       \n\
                            background-color:  #3a445e;				           \n\
                            border-radius:2px;                                 \n\
                            border:none;                                       \n\
                            min-width: 6px;								       \n\
                            min-height: 6px;								   \n\
                            }												   \n\
                            QScrollBar::handle:hover {				           \n\
                            background-color: #4486ff;    		               \n\
                            border-radius:2px;                                 \n\
                            border:none;                                       \n\
                            min-width: 6px;								       \n\
                            min-height: 6px;								   \n\
                            }												   \n\
                            QScrollBar::add-line {					           \n\
                            border: 0px;									   \n\
                            background: #32CC99;							   \n\
                            width: 0px;										   \n\
                            height: 0px;                                       \n\
                            }												   \n\
                            QScrollBar::sub-line {							   \n\
                            border: 0px;									   \n\
                            background: #32CC99;							   \n\
                            width: 0px;										   \n\
                            height: 0px;                                       \n\
                            }												   \n\
                            QScrollBar::add-page, QScrollBar::sub-page {	   \n\
                            background-color: none;							   \n\
                            border: none;									   \n\
                            }"
                            );
}

CTreeViewEx::~CTreeViewEx()
{

}

void CTreeViewEx::mouseMoveEvent(QMouseEvent *e)
{
    QTreeView::mouseMoveEvent(e);

    QModelIndex rIndex = this->indexAt(e->pos());
    if(rIndex.isValid())
    {

        mHoveredRow = rIndex.row();
    }
    else
    {
        if(mHoveredRow != -1)
        {
            mHoveredRow = -1;
            this->viewport()->update();
        }
    }

    QModelIndex cIndex = this->indexAt(e->pos());
    if(cIndex.isValid())
    {

        mHoveredColumn = rIndex.column();
    }
    else
    {
        if(mHoveredColumn != -1)
        {
            mHoveredColumn = -1;
            this->viewport()->update();
        }
    }

}

bool CTreeViewEx::eventFilter(QObject *obj, QEvent *e)
{
    if(obj == this->header())
    {
        switch(e->type())
        {
            case QEvent::Enter:
            case QEvent::MouseMove:
            case QEvent::HoverMove:
            case QEvent::HoverEnter:
            {//reset hovered column & hovered row
                if(mHoveredColumn != -1)
                {
                    mHoveredColumn = -1;
                    this->viewport()->update();
                }
                if(mHoveredRow != -1)
                {
                    mHoveredRow = -1;
                    this->viewport()->update();
                }
            }
                break;
            default:
            {
                //none ...
            }
                break;
        }
    }

    return QTreeView::eventFilter(obj,e);
}

void CTreeViewEx::setHeader(QHeaderView *header)
{
    if(header)
    {
        header->installEventFilter(this);
    }

    QTreeView::setHeader(header);
}

int CTreeViewEx::hoveredRow() const
{
    return mHoveredRow;
}

int CTreeViewEx::hoveredColumn() const
{
    return mHoveredColumn;
}

