#include "CExpressionPicker.h"
#include <QHeaderView>
#include <QMouseEvent>
#include "CExpressionPickerDelegate.h"

#include <QDebug>

CExpressionPicker::CExpressionPicker(QWidget *parent)
    :QTableView(parent)
    ,mModel(new QStandardItemModel(this))
    ,mPreView(new QLabel(this))
    ,mShowPreView(false)
    ,mShowIconRect(true)
    ,mMaxColumnCount(12)
    ,mMaxRowCount(4)
    ,mMovie(new QMovie(this))
{
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->horizontalHeader()->setVisible(false);
    this->verticalHeader()->setVisible(false);

    this->horizontalHeader()->setDefaultSectionSize(33);
    this->verticalHeader()->setDefaultSectionSize(33);

    this->setEditTriggers(QTableView::NoEditTriggers);
    this->setSelectionMode(QTableView::SingleSelection);
    this->setShowGrid(false);
    this->setMouseTracking(true);
    this->setStyleSheet("QTableView{background-color:rgba(0,0,0,0);border:none;}");

    this->setModel(mModel);
    connect(this, &CExpressionPicker::clicked, this, &CExpressionPicker::onClicked);

    //delegate
    CExpressionPickerDelegate* tdelegate = new CExpressionPickerDelegate;
    tdelegate->setParent(this);
    this->setItemDelegate(tdelegate);

    //pre viewer
    mPreView->setMouseTracking(true);
    mPreView->hide();
    mPreView->resize(33*2,33*2);
    mPreView->setStyleSheet("background-color:white;");
    mPreView->installEventFilter(this);
    mMovie->setScaledSize(mPreView->size());
	mMovie->setCacheMode(QMovie::CacheAll);
    mPreView->setMovie(mMovie);

}

CExpressionPicker::~CExpressionPicker()
{

}

void CExpressionPicker::setShowPreView(bool s)
{
    mShowPreView = s;
}

bool CExpressionPicker::isShowPreView()
{
    return mShowPreView;
}

void CExpressionPicker::setShowIconRect(bool s)
{
    mShowIconRect = s;
}

bool CExpressionPicker::isShowIconRect()
{
    return mShowIconRect;
}

void CExpressionPicker::setExpressionList(const CExpressionPicker::ExpressionList &li)
{
    mExpressionList = li;

    mModel->clear();

    int capacity = this->maxRowCount()*this->maxColumnCount();
    int tSize = capacity <= li.count() ? capacity : li.count();

    for(int i = 0; i < tSize; i++)
    {
        Expression exp = li[i];
        QStandardItem* item = new QStandardItem("expression");
        item->setData(exp.desc     , ExpressionRole_Desc);
        item->setData(exp.fileName , ExpressionRole_FileName);
        item->setData(exp.groupid  , ExpressionRole_GroupId);
        item->setData(exp.id       , ExpressionRole_Id);
        item->setData(exp.shortcut , ExpressionRole_Shortcut);
        item->setData(exp.tooltip  , ExpressionRole_Tooltip);

        item->setToolTip(exp.tooltip);

        int currentColumn = i % this->maxColumnCount();
        int currentRow    = i / this->maxColumnCount();
        mModel->setItem(currentRow, currentColumn, item);
    }
}

CExpressionPicker::ExpressionList CExpressionPicker::expressionList()
{
    return mExpressionList;
}

void CExpressionPicker::setMaxColumnCount(int n)
{
    mMaxColumnCount = n;
}

void CExpressionPicker::setMaxRowCount(int n)
{
    mMaxRowCount = n;
}

int CExpressionPicker::maxColumnCount()
{
    return mMaxColumnCount;
}

int CExpressionPicker::maxRowCount()
{
    return mMaxRowCount;
}

void CExpressionPicker::leaveEvent(QEvent *e)
{
    QTableView::leaveEvent(e);
    mPreView->hide();
}

void CExpressionPicker::mouseMoveEvent(QMouseEvent *e)
{
    QTableView::mouseMoveEvent(e);

    if(!this->isShowPreView())
    {
        mPreView->hide();
        return;
    }

    //movie
    QModelIndex tindex = this->indexAt(e->pos());
    if(!tindex.isValid() || !tindex.data(ExpressionRole_Id).isValid())
    {
        mPreView->hide();
        return;
    }

    if(e->x() <= mPreView->width())
    {
        mPreView->move(this->width() - mPreView->width() - 1,
                       0 + 1);
    }
    else
    {
        mPreView->move(1,1);
    }
    mPreView->show();

    QString filename = tindex.data(ExpressionRole_FileName).toString();
    if(filename != mMovie->fileName())
    {
        mMovie->stop();
        mMovie->setFileName(filename);
        mMovie->start();
    }
}

bool CExpressionPicker::eventFilter(QObject *obj, QEvent *e)
{
    bool res = QTableView::eventFilter(obj,e);
	if (obj == nullptr || e == nullptr)
	{
		return res;
	}
    if(obj == mPreView)
    {
        if(e->type() == QEvent::MouseMove)
        {
            if(mPreView->pos() != QPoint(0,0))
            {
                mPreView->move(0,0);
            }
            else
            {
                mPreView->move(this->width() - mPreView->width(),
                               0);
            }
        }

		if(e->type() == QEvent::Hide)
		{
			mMovie->stop();
		}
		if(e->type() == QEvent::Show)
		{
			mMovie->start();
		}
    }

    return res;
}

void CExpressionPicker::onClicked(const QModelIndex &index)
{
    if(!index.isValid())
        return;

    if(!index.data(ExpressionRole_Id).isValid())
        return;

    Expression exp;
    exp.desc     = index.data(ExpressionRole_Desc     ).toString();
    exp.fileName = index.data(ExpressionRole_FileName ).toString();
    exp.groupid  = index.data(ExpressionRole_GroupId  ).toString();
    exp.id       = index.data(ExpressionRole_Id       ).toString();
    exp.shortcut = index.data(ExpressionRole_Shortcut ).toString();
    exp.tooltip  = index.data(ExpressionRole_Tooltip  ).toString();

    emit expressionClicked(exp);
}

