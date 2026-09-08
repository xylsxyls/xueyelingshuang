#include "CExpressionDialog.h"
#include "QtControls/ExpressionConfigParser.h"
#include <QStandardItemModel>
#include <QDebug>
#include <QEvent>
#include <QPainter>
#include <stdint.h>
#include "DialogHelper.h"

CExpressionDialog::CExpressionDialog(QWidget *parent)
    :QDialog(parent)
    ,m_expressionPicker(new CExpressionPicker(this))
    ,m_expressionGroupPicker(new CExpressionPicker(this))
    ,m_groups()
    ,m_expressions()
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);

	QStandardItemModel* expressionModel = qobject_cast<QStandardItemModel*>(m_expressionPicker->model());
	if (expressionModel != nullptr)
	{
		expressionModel->setColumnCount(12);
		expressionModel->setRowCount(4);
	}
    m_expressionPicker->resize(33*12 + 2,33*4 + 2);
    m_expressionPicker->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_expressionPicker->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_expressionPicker->setShowPreView(true);

	QStandardItemModel* groupModel = qobject_cast<QStandardItemModel*>(m_expressionGroupPicker->model());
	if (groupModel != nullptr)
	{
		groupModel->setColumnCount(12);
		groupModel->setRowCount(1);
	}
    m_expressionGroupPicker->setFixedSize(33*12 + 2,33*1 + 2);
    m_expressionGroupPicker->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_expressionGroupPicker->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_expressionGroupPicker->setShowIconRect(false);



    connect(m_expressionGroupPicker, &CExpressionPicker::expressionClicked, this, &CExpressionDialog::selectGroup);
    connect(m_expressionPicker     , &CExpressionPicker::expressionClicked, this, &CExpressionDialog::expressionChoosed);
    connect(this, &CExpressionDialog::expressionChoosed, [this](){
        this->accept();
    });
    connect(this, &CExpressionDialog::ncActiveChanged, [this](const bool& s)
    {
        if(!s)
        {
            this->reject();
        }
    });

	this->setFixedSize(416,178);
	this->layoutControl();
}

CExpressionDialog::~CExpressionDialog()
{

}

void CExpressionDialog::layoutControl()
{
	if (m_expressionPicker == nullptr || m_expressionGroupPicker == nullptr)
	{
		return;
	}
    m_expressionPicker->move(10,10);
    m_expressionGroupPicker->move(10, this->height() - m_expressionGroupPicker->height());
}

void CExpressionDialog::resizeEvent(QResizeEvent *e)
{
	if (e == nullptr)
	{
		return;
	}
    QDialog::resizeEvent(e);
    this->layoutControl();
}

void CExpressionDialog::paintEvent(QPaintEvent *e)
{
	if (e == nullptr)
	{
		return;
	}
    QPainter painter(this);
    painter.fillRect(this->rect(), QColor(44,52,74));
    painter.setPen(QColor("#4A5677"));
    painter.drawRect(DialogHelper::rectValid(rect().adjusted(0, 0, -1, -1)));
}

void CExpressionDialog::changeEvent(QEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
	QDialog::changeEvent(eve);
	if (eve->type() == QEvent::ActivationChange)
	{
		emit ncActiveChanged(isActiveWindow());
	}
}

void CExpressionDialog::selectGroup(const CExpressionPicker::Expression &group)
{
    CExpressionPicker::ExpressionList texpList;
    for(int32_t i = 0; i < m_expressions.count(); ++i)
    {
        CExpressionPicker::Expression exp = m_expressions[i];
        if(exp.m_groupId == group.m_id)
        {
            texpList << exp;
        }
    }
    m_expressionPicker->setExpressionList(texpList);
}

bool CExpressionDialog::loadExpressions(const QString& emotionPath)
{
    m_groups.clear();
    m_expressions.clear();

    ExpressionConfig config;
    ExpressionConfigParser parser;
    if (!parser.parse(emotionPath, &config))
    {
        qDebug() << "load emotion error:" << parser.lastError();
        return false;
    }

    for (int32_t i = 0; i < config.m_groups.count(); ++i)
    {
        const ExpressionGroupInfo& groupInfo = config.m_groups[i];
        CExpressionPicker::Expression group;
        group.m_groupId = groupInfo.m_groupId;
        group.m_id = groupInfo.m_groupId;
        group.m_desc = groupInfo.m_desc;
        group.m_fileName = groupInfo.m_fileName;
        group.m_tooltip = groupInfo.m_tooltip;
        m_groups << group;
    }

    for (int32_t i = 0; i < config.m_expressions.count(); ++i)
    {
        const ExpressionInfo& expressionInfo = config.m_expressions[i];
        CExpressionPicker::Expression expression;
        expression.m_groupId = expressionInfo.m_groupId;
        expression.m_id = expressionInfo.m_id;
        expression.m_desc = expressionInfo.m_desc;
        expression.m_fileName = expressionInfo.m_fileName;
        expression.m_shortcut = expressionInfo.m_shortcut;
        expression.m_tooltip = expressionInfo.m_tooltip;
        m_expressions << expression;
    }

    qDebug() << "load emotion ok, group:" << m_groups.count() << "expression:" << m_expressions.count();
    m_expressionGroupPicker->setExpressionList(m_groups);
    if (m_groups.isEmpty())
    {
        m_expressionPicker->setExpressionList(CExpressionPicker::ExpressionList());
        qDebug() << "load emotion error: group list is empty";
        return false;
    }

    this->selectGroup(m_groups.first());
    return true;
}