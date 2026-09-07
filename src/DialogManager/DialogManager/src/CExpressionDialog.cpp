#include "CExpressionDialog.h"
#include "QtControls/ExpressionConfigParser.h"
#include <QStandardItemModel>
#include <QDebug>
#include <QPainter>
#include <stdint.h>
#ifdef _MSC_VER
#include <Windows.h>
#endif
#include "DialogHelper.h"

CExpressionDialog::CExpressionDialog(QWidget *parent)
    :QDialog(parent)
    ,m_expressionPicker(new CExpressionPicker(this))
    ,m_expressionGroupPicker(new CExpressionPicker(this))
    ,m_groups()
    ,m_expressions()
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);

    ((QStandardItemModel*)(m_expressionPicker->model()))->setColumnCount(12);
    ((QStandardItemModel*)(m_expressionPicker->model()))->setRowCount(4);
    m_expressionPicker->resize(33*12 + 2,33*4 + 2);
    m_expressionPicker->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_expressionPicker->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_expressionPicker->setShowPreView(true);

    ((QStandardItemModel*)(m_expressionGroupPicker->model()))->setColumnCount(12);
    ((QStandardItemModel*)(m_expressionGroupPicker->model()))->setRowCount(1);
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
    m_expressionPicker->move(10,10);
    m_expressionGroupPicker->move(10, this->height() - m_expressionGroupPicker->height());
}

void CExpressionDialog::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);
    this->layoutControl();
}

void CExpressionDialog::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.fillRect(this->rect(), QColor(44,52,74));
    painter.setPen(QColor("#4A5677"));
    painter.drawRect(DialogHelper::rectValid(rect().adjusted(0, 0, -1, -1)));
}

bool CExpressionDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    bool res = QDialog::nativeEvent(eventType, message, result);
#ifdef _MSC_VER
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG* msg = static_cast<MSG *>(message);
        switch (msg->message)
        {
        case WM_ACTIVATE:
            {
                emit ncActiveChanged(msg->wParam);
            }
            break;
        default:
            break;
        }
    }
#endif

    return res;
}

void CExpressionDialog::selectGroup(const CExpressionPicker::Expression &group)
{
    CExpressionPicker::ExpressionList texpList;
    for(int32_t i = 0; i < m_expressions.count(); ++i)
    {
        CExpressionPicker::Expression exp = m_expressions[i];
        if(exp.groupid == group.id)
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
        group.groupid = groupInfo.m_groupId;
        group.id = groupInfo.m_groupId;
        group.desc = groupInfo.m_desc;
        group.fileName = groupInfo.m_fileName;
        group.tooltip = groupInfo.m_tooltip;
        m_groups << group;
    }

    for (int32_t i = 0; i < config.m_expressions.count(); ++i)
    {
        const ExpressionInfo& expressionInfo = config.m_expressions[i];
        CExpressionPicker::Expression expression;
        expression.groupid = expressionInfo.m_groupId;
        expression.id = expressionInfo.m_id;
        expression.desc = expressionInfo.m_desc;
        expression.fileName = expressionInfo.m_fileName;
        expression.shortcut = expressionInfo.m_shortcut;
        expression.tooltip = expressionInfo.m_tooltip;
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
