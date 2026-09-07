#include "CExpressionPicker.h"
#include <QHeaderView>
#include <QMouseEvent>
#include "CExpressionPickerDelegate.h"

#include <QDebug>

CExpressionPicker::CExpressionPicker(QWidget *parent)
    :QTableView(parent)
    ,m_model(new QStandardItemModel(this))
    ,m_preview(new QLabel(this))
    ,m_movie(new QMovie(this))
    ,m_expressionList()
    ,m_showPreview(false)
    ,m_showIconRect(true)
    ,m_maxColumnCount(12)
    ,m_maxRowCount(4)
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

    this->setModel(m_model);
    connect(this, &CExpressionPicker::clicked, this, &CExpressionPicker::onClicked);

    //delegate
    CExpressionPickerDelegate* tdelegate = new CExpressionPickerDelegate;
    tdelegate->setParent(this);
    this->setItemDelegate(tdelegate);

    //pre viewer
    m_preview->setMouseTracking(true);
    m_preview->hide();
    m_preview->resize(33*2,33*2);
    m_preview->setStyleSheet("background-color:white;");
    m_preview->installEventFilter(this);
    m_movie->setScaledSize(m_preview->size());
	m_movie->setCacheMode(QMovie::CacheAll);
    m_preview->setMovie(m_movie);

}

CExpressionPicker::~CExpressionPicker()
{

}

void CExpressionPicker::setShowPreView(bool show)
{
    m_showPreview = show;
}

bool CExpressionPicker::isShowPreView()
{
    return m_showPreview;
}

void CExpressionPicker::setShowIconRect(bool show)
{
    m_showIconRect = show;
}

bool CExpressionPicker::isShowIconRect()
{
    return m_showIconRect;
}

void CExpressionPicker::setExpressionList(const CExpressionPicker::ExpressionList &li)
{
    m_expressionList = li;

    m_model->clear();

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
        m_model->setItem(currentRow, currentColumn, item);
    }
}

CExpressionPicker::ExpressionList CExpressionPicker::expressionList()
{
    return m_expressionList;
}

void CExpressionPicker::setMaxColumnCount(int count)
{
    m_maxColumnCount = count;
}

void CExpressionPicker::setMaxRowCount(int count)
{
    m_maxRowCount = count;
}

int CExpressionPicker::maxColumnCount()
{
    return m_maxColumnCount;
}

int CExpressionPicker::maxRowCount()
{
    return m_maxRowCount;
}

void CExpressionPicker::leaveEvent(QEvent *e)
{
    QTableView::leaveEvent(e);
    m_preview->hide();
}

void CExpressionPicker::mouseMoveEvent(QMouseEvent *e)
{
    QTableView::mouseMoveEvent(e);

    if(!this->isShowPreView())
    {
        m_preview->hide();
        return;
    }

    //movie
    QModelIndex tindex = this->indexAt(e->pos());
    if(!tindex.isValid() || !tindex.data(ExpressionRole_Id).isValid())
    {
        m_preview->hide();
        return;
    }

    if(e->x() <= m_preview->width())
    {
        m_preview->move(this->width() - m_preview->width() - 1,
                       0 + 1);
    }
    else
    {
        m_preview->move(1,1);
    }
    m_preview->show();

    QString filename = tindex.data(ExpressionRole_FileName).toString();
    if(filename != m_movie->fileName())
    {
        m_movie->stop();
        m_movie->setFileName(filename);
        m_movie->start();
    }
}

bool CExpressionPicker::eventFilter(QObject *obj, QEvent *e)
{
    bool res = QTableView::eventFilter(obj,e);
	if (obj == nullptr || e == nullptr)
	{
		return res;
	}
    if(obj == m_preview)
    {
        if(e->type() == QEvent::MouseMove)
        {
            if(m_preview->pos() != QPoint(0,0))
            {
                m_preview->move(0,0);
            }
            else
            {
                m_preview->move(this->width() - m_preview->width(),
                               0);
            }
        }

		if(e->type() == QEvent::Hide)
		{
			m_movie->stop();
		}
		if(e->type() == QEvent::Show)
		{
			m_movie->start();
		}
    }

    return res;
}

void CExpressionPicker::onClicked(const QModelIndex &index)
{
    if(!index.isValid())
    {
        return;
    }

    if(!index.data(ExpressionRole_Id).isValid())
    {
        return;
    }

    Expression exp;
    exp.desc     = index.data(ExpressionRole_Desc     ).toString();
    exp.fileName = index.data(ExpressionRole_FileName ).toString();
    exp.groupid  = index.data(ExpressionRole_GroupId  ).toString();
    exp.id       = index.data(ExpressionRole_Id       ).toString();
    exp.shortcut = index.data(ExpressionRole_Shortcut ).toString();
    exp.tooltip  = index.data(ExpressionRole_Tooltip  ).toString();

    emit expressionClicked(exp);
}