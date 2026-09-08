#include "CExpressionPicker.h"
#include <QHeaderView>
#include <QMouseEvent>
#include "CExpressionPickerDelegate.h"

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

	if (m_model == nullptr)
	{
		return;
	}
    m_model->clear();

    qint32 capacity = qMax(this->maxRowCount(), 1) * qMax(this->maxColumnCount(), 1);
    qint32 tSize = capacity <= li.count() ? capacity : li.count();

    for(qint32 i = 0; i < tSize; ++i)
    {
        Expression exp = li[i];
        QStandardItem* item = new QStandardItem("expression");
        item->setData(exp.m_desc     , ExpressionRole_Desc);
        item->setData(exp.m_fileName , ExpressionRole_FileName);
        item->setData(exp.m_groupId  , ExpressionRole_GroupId);
        item->setData(exp.m_id       , ExpressionRole_Id);
        item->setData(exp.m_shortcut , ExpressionRole_Shortcut);
        item->setData(exp.m_tooltip  , ExpressionRole_Tooltip);

        item->setToolTip(exp.m_tooltip);

        qint32 currentColumn = i % this->maxColumnCount();
        qint32 currentRow    = i / this->maxColumnCount();
        m_model->setItem(currentRow, currentColumn, item);
    }
}

CExpressionPicker::ExpressionList CExpressionPicker::expressionList()
{
    return m_expressionList;
}

void CExpressionPicker::setMaxColumnCount(int count)
{
    m_maxColumnCount = qMax(count, 1);
}

void CExpressionPicker::setMaxRowCount(int count)
{
    m_maxRowCount = qMax(count, 1);
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
	if (e == nullptr)
	{
		return;
	}
    QTableView::leaveEvent(e);
	if (m_preview != nullptr)
	{
		m_preview->hide();
	}
}

void CExpressionPicker::mouseMoveEvent(QMouseEvent *e)
{
	if (e == nullptr)
	{
		return;
	}
    QTableView::mouseMoveEvent(e);

    if(!this->isShowPreView())
    {
		if (m_preview != nullptr)
		{
			m_preview->hide();
		}
        return;
    }

    //movie
    QModelIndex tindex = this->indexAt(e->pos());
    if(!tindex.isValid() || !tindex.data(ExpressionRole_Id).isValid())
    {
		if (m_preview != nullptr)
		{
			m_preview->hide();
		}
        return;
    }

	if (m_preview == nullptr || m_movie == nullptr)
	{
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
	if (obj == nullptr || e == nullptr)
	{
		return false;
	}
    bool res = QTableView::eventFilter(obj,e);
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

		if(e->type() == QEvent::Hide && m_movie != nullptr)
		{
			m_movie->stop();
		}
		if(e->type() == QEvent::Show && m_movie != nullptr)
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
    exp.m_desc     = index.data(ExpressionRole_Desc     ).toString();
    exp.m_fileName = index.data(ExpressionRole_FileName ).toString();
    exp.m_groupId  = index.data(ExpressionRole_GroupId  ).toString();
    exp.m_id       = index.data(ExpressionRole_Id       ).toString();
    exp.m_shortcut = index.data(ExpressionRole_Shortcut ).toString();
    exp.m_tooltip  = index.data(ExpressionRole_Tooltip  ).toString();

    emit expressionClicked(exp);
}