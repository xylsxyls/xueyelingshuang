#include "CExpressionDialog.h"
#include <QStandardItemModel>
#include "EO_XmlSax2Parser/EO_XmlSax2ParserAPI.h"
#include <QDebug>
#include <QPainter>
#include <Windows.h>
#include "DialogHelper.h"

CExpressionDialog::CExpressionDialog(QWidget *parent)
    :QDialog(parent)
    ,mExpressionPicker(new CExpressionPicker(this))
    ,mExpressionGroupPicker(new CExpressionPicker(this))
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);

    ((QStandardItemModel*)(mExpressionPicker->model()))->setColumnCount(12);
    ((QStandardItemModel*)(mExpressionPicker->model()))->setRowCount(4);
    mExpressionPicker->resize(33*12 + 2,33*4 + 2);
    mExpressionPicker->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mExpressionPicker->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mExpressionPicker->setShowPreView(true);

    ((QStandardItemModel*)(mExpressionGroupPicker->model()))->setColumnCount(12);
    ((QStandardItemModel*)(mExpressionGroupPicker->model()))->setRowCount(1);
    mExpressionGroupPicker->setFixedSize(33*12 + 2,33*1 + 2);
    mExpressionGroupPicker->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mExpressionGroupPicker->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mExpressionGroupPicker->setShowIconRect(false);

   

    connect(mExpressionGroupPicker, &CExpressionPicker::expressionClicked, this, &CExpressionDialog::selectGroup);
    connect(mExpressionPicker     , &CExpressionPicker::expressionClicked, this, &CExpressionDialog::expressionChoosed);
    connect(this, &CExpressionDialog::expressionChoosed, [this](){
        this->accept();
    });
    connect(this, &CExpressionDialog::ncActiveChanged, [this](const bool& s)
    {
        if(!s)
            this->reject();
    });

	this->setFixedSize(416,178);
	this->layoutControl();
}

CExpressionDialog::~CExpressionDialog()
{

}

void CExpressionDialog::layoutControl()
{
    mExpressionPicker->move(10,10);
    mExpressionGroupPicker->move(10, this->height() - mExpressionGroupPicker->height());
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

    return res;
}

void CExpressionDialog::selectGroup(const CExpressionPicker::Expression &group)
{
    CExpressionPicker::ExpressionList texpList;
    for(int i = 0; i < mExpressions.count(); i++)
    {
        CExpressionPicker::Expression exp = mExpressions[i];
        if(exp.groupid == group.id)
            texpList << exp;
    }
    mExpressionPicker->setExpressionList(texpList);
}

bool CExpressionDialog::loadExpressions(const QString& emotionPath)
{
    EO_XmlSax2Parser parser;
    if(!parser.parseFromFile(emotionPath + "/Emotions/emotion.xml"))
    {
        qDebug() << "load emotion error";
        return false;
    }

    XMLNode* config = parser.root();
    if(config == NULL)
    {
        qDebug() << "load emotion error";
        return false;
    }

    if(config->name != "config")
    {
        qDebug() << "load emotion error";
        return false;
    }

    XMLNodeVector groupNodeList = config->getChildrenByName("group");
    XMLNodeVector emotionNodeList = config->getChildrenByName("emotion");
    qDebug() << "group node:" << groupNodeList.count();
    qDebug() << "emotion node:" << emotionNodeList.count();

    for(int i = 0; i < groupNodeList.count(); i++)
    {
        CExpressionPicker::Expression group;

        //id
        XMLNodeVector idNode = groupNodeList[i]->getChildrenByName("id");
        if(idNode.count() != 0)
        {
            group.id = idNode.first()->characters;
        }

        //desc
        XMLNodeVector descNode = groupNodeList[i]->getChildrenByName("desc");
        if(descNode.count() != 0)
        {
            group.desc = descNode.first()->characters;
        }


        if(group.id == "0")
        {
            group.fileName = emotionPath + "/Emotions\\def\\def_jingkong.gif";
        }
        else if(group.id == "1")
        {
            group.fileName = emotionPath + "/Emotions\\7f\\71_daxiao.gif";
        }
        else if(group.id == "2")
        {
            group.fileName = emotionPath + "/Emotions\\Smileys\\em4.bmp";
        }

        group.tooltip = group.desc;

        mGroups << group;
    }

    //expressions
    for(int j = 0; j < emotionNodeList.count(); j++)
    {
        XMLNode* emotionNode = emotionNodeList[j];

        //id
        XMLNodeVector e_groupidNode = emotionNode->getChildrenByName("groupid");
        if(e_groupidNode.count() <= 0)
        {
            continue;
        }

        //到了这里emotionNode才是有效的图标
        CExpressionPicker::Expression expression;

        //groupid
        expression.groupid = e_groupidNode.first()->characters;

        //id
        XMLNodeVector e_idNode = emotionNode->getChildrenByName("id");
        if(e_idNode.count() != 0)
        {
            expression.id = e_idNode.first()->characters;
        }

        //desc
        XMLNodeVector e_descNode = emotionNode->getChildrenByName("desc");
        if(e_descNode.count() != 0)
        {
            expression.desc = e_descNode.first()->characters;
        }

        //tooltip
        XMLNodeVector e_tooltipNode = emotionNode->getChildrenByName("tooltip");
        if(e_tooltipNode.count() != 0)
        {
            expression.tooltip = e_tooltipNode.first()->characters;
        }

        //file
        XMLNodeVector e_fileNode = emotionNode->getChildrenByName("file");
        if(e_fileNode.count() != 0)
        {
            expression.fileName = emotionPath + "/" + e_fileNode.first()->characters;
        }

        //shortcut
        XMLNodeVector e_shortcutNode = emotionNode->getChildrenByName("shortcut");
        if(e_shortcutNode.count() != 0)
        {
            expression.shortcut = e_shortcutNode.first()->characters;
        }

//        //audio
//        XMLNodeVector e_audioNode = emotionNode->getChildrenByName("audio");
//        if(e_audioNode.count() != 0)
//        {
//            expression.audio = e_audioNode.first()->characters;
//        }

//        qDebug() << expression.fileName;

        mExpressions << expression;
    }

    qDebug() << "load emotion ok";

    //mExpressionPicker->setExpressionList(mExpressions);
    mExpressionGroupPicker->setExpressionList(mGroups);
    this->selectGroup(mGroups.first());
    return true;
}

