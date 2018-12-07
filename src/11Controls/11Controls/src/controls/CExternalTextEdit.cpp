#include "CExternalTextEdit.h"

#include "./EO_XmlSax2Parser.h"
#include <QDebug>
#include <QMimeData>
#include <QMouseEvent>
#include <QMovie>

CExternalTextEdit::CExternalTextEdit(QWidget *parent)
    :QTextEdit(parent)
    ,mExpressionTag("expression")
	,mExpressionSize(18)
{
    mRefresgGifTimer.setInterval(250);
    connect(&mRefresgGifTimer, &QTimer::timeout, this, &CExternalTextEdit::updateGifResourceForExpression);

	this->setContextMenuPolicy(Qt::NoContextMenu);
}

CExternalTextEdit::~CExternalTextEdit()
{

}

void CExternalTextEdit::showEvent(QShowEvent *e)
{
    QTextEdit::showEvent(e);
    mRefresgGifTimer.start();
	for(int i = 0; i < mGifResourceMapForExpression.keys().count(); i++)
	{
		QMovie *m = mGifResourceMapForExpression.value(mGifResourceMapForExpression.keys()[i], NULL);
		if(m)
		{
			m->start();
		}
	}
}

void CExternalTextEdit::hideEvent(QHideEvent *e)
{
    QTextEdit::hideEvent(e);
    mRefresgGifTimer.stop();
	for(int i = 0; i < mGifResourceMapForExpression.keys().count(); i++)
	{
		QMovie *m = mGifResourceMapForExpression.value(mGifResourceMapForExpression.keys()[i], NULL);
		if(m)
		{
			m->stop();
		}
	}
}

void CExternalTextEdit::keyPressEvent(QKeyEvent *e)
{
    // 主键盘 Enter
    if ((e->key() == Qt::Key_Return) && (e->modifiers() == Qt::NoModifier))
    {
        emit enterPressed();
        return ;
    }
    // 小键盘 Enter
    else if ((e->key() == Qt::Key_Enter) && (e->modifiers() == Qt::KeypadModifier))
    {
        emit enterPressed();
        return ;
    }

    // 主键盘 Ctrl + Enter
    else if((e->key() == Qt::Key_Return) && (e->modifiers() == Qt::ControlModifier))
    {
        emit ctrlEnterPressed();
        return ;
    }
    // 小键盘 Ctrl + Enter
    else if((e->key() == Qt::Key_Enter) && (e->modifiers() == (Qt::KeypadModifier|Qt::ControlModifier)))
    {
        emit ctrlEnterPressed();
        return ;
    }
    // Alt + C
    else if((e->key() == Qt::Key_C) && (e->modifiers() == Qt::AltModifier))
    {
        emit altCPressed();
        return ;
    }
    // Alt + S
    else if((e->key() == Qt::Key_S) && (e->modifiers() == Qt::AltModifier))
    {
        emit altSPressed();
        return ;
    }

    QTextEdit::keyPressEvent(e);
}

void CExternalTextEdit::mousePressEvent(QMouseEvent *e)
{
	QTextEdit::mousePressEvent(e);
	this->activateWindow();
	qDebug() << "edit: mouse press";
}

void CExternalTextEdit::updateGifResourceForExpression()
{
    for(int i = 0; i < mGifResourceMapForExpression.keys().count(); i++)
    {
        QMovie *m = mGifResourceMapForExpression.value(mGifResourceMapForExpression.keys()[i], NULL);
		if(m == NULL)
			continue;

        int currentFrame = m->currentFrameNumber();
        currentFrame = currentFrame < m->frameCount() - 1 ? currentFrame + 1 : 0;

        m->jumpToFrame(currentFrame);

        this->document()->addResource(QTextDocument::ImageResource, mGifResourceMapForExpression.keys()[i], m->currentPixmap());
    }

    setLineWrapColumnOrWidth(lineWrapColumnOrWidth());

}

void CExternalTextEdit::insertFromMimeData(const QMimeData *source)
{
    QMimeData* n_source = new QMimeData;

    QString text;
    if(source->hasHtml())
    {
        qDebug() << "insert mimdata hasHtml:" << source->html();

        text = source->html();

        //将img替换成shortcut
        text = filterExpressionToShortcut(text);

        ////TODO:先除去style
        //QRegExp ex("<[sS][tT][yY][lL][eE].*[sS][tT][yY][lL][eE]>");
        //text.replace(ex, "");
        //
        ////TODO:将html元素去除
        //ex.setPattern("<[^<]*>");
        //text.replace(ex, "");
        QTextDocument doc;
        doc.setHtml(text);
        text = doc.toPlainText();
        text = text.toHtmlEscaped();
        text = text.replace("\n", "<br/>");

        //TODO:将shortcut换成img
        text = parseShortcutToExpression(text);
        n_source->setHtml(text);

    }
    else if(source->hasText())
    {
        qDebug() << "insert mimdata hasText:" << source->text();

        text = source->text().toHtmlEscaped();
        text = text.replace("\n","<br/>");


        //TODO:将shortcut换成img
        text = parseShortcutToExpression(text);
        n_source->removeFormat("text/plain");


        n_source->setHtml(text);
    }
    //delete t_source;
    QTextEdit::insertFromMimeData(n_source);
    delete n_source;
}

QMimeData *CExternalTextEdit::createMimeDataFromSelection() const
{
    QMimeData* tData = QTextEdit::createMimeDataFromSelection();
    QString text;

    foreach(QString s, tData->formats())
    {
        qDebug() << s;
    }

    if(tData->hasHtml())
    {
        qDebug() << "create mimdata hasHtml:" << tData->html();
        text = tData->html();

        text = filterExpressionToShortcut(text);

        //QRegExp ex("<!--.*StartFragment.*-->.*<!--.*EndFragment.*-->");
        //if(ex.indexIn(text) != -1)
        //{
        //	//得到真正的内容
        //	text = ex.cap(0);
        //
        //	//TODO:将其中的img元素替换成shortcut，替换不了的直接消除
        //	text = imgToText(text);
        //
        //	////TODO:先除去style
        //	//QRegExp ex("<[sS][tT][yY][lL][eE].*[sS][tT][yY][lL][eE]>");
        //	//text.replace(ex, "");
        //	//
        //	////TODO:消除html元素
        //	//ex.setPattern("<[^<]*>");
        //	//text.replace(ex, "");
        //	QTextDocument doc;
        //	doc.setHtml(text);
        //	text = doc.toPlainText();
        //	text = text.toHtmlEscaped();
        //	text = text.replace("\n","<br/>");
        //}

        QTextDocument doc;
        doc.setHtml(text);
        text = doc.toPlainText();
        //text = text.toHtmlEscaped();
        //text = text.replace("\n","<br/>");

        qDebug() << "parse:" << text;
        tData->removeFormat("text/html");
        tData->setText(text);
    }
    else if(tData->hasText())
    {
        qDebug() << "create mimdata hasText:" << tData->text();
        text = tData->text();
    }


    return tData;
}

void CExternalTextEdit::loadExpressions(const QString& emotionPath)
{


    EO_XmlSax2Parser parser;
    if(!parser.parseFromFile(emotionPath + "/Emotions/emotion.xml"))
    {
        qDebug() << "load emotion error";
        return ;
    }

    XMLNode* config = parser.root();
    if(config == NULL)
    {
        qDebug() << "load emotion error";
        return ;
    }

    if(config->name != "config")
    {
        qDebug() << "load emotion error";
        return ;
    }

    XMLNodeVector groupNodeList = config->getChildrenByName("group");
    XMLNodeVector emotionNodeList = config->getChildrenByName("emotion");
    qDebug() << "group node:" << groupNodeList.count();
    qDebug() << "emotion node:" << emotionNodeList.count();

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

        QString tFilename;
        //file
        XMLNodeVector e_fileNode = emotionNode->getChildrenByName("file");
        if(e_fileNode.count() != 0)
        {
            tFilename = emotionPath + "/" + e_fileNode.first()->characters;
        }

        QString tShortcut;
        //shortcut
        XMLNodeVector e_shortcutNode = emotionNode->getChildrenByName("shortcut");
        if(e_shortcutNode.count() != 0)
        {
            tShortcut = e_shortcutNode.first()->characters;
        }

        mMapedExpression[tShortcut] = tFilename;
    }

    //init expressions gif resource
    for(int i = 0; i < mMapedExpression.keys().count(); i++)
    {
        QString gifFilename = mMapedExpression[mMapedExpression.keys()[i]];
        QMovie* m = new QMovie;
        m->setCacheMode(QMovie::CacheAll);
        m->setFileName(gifFilename);
		m->setScaledSize(QSize(mExpressionSize,mExpressionSize));
		if(this->isVisible())
			m->start();

        QString docResourceUrl(mExpressionTag + mMapedExpression.keys()[i]);
        this->document()->addResource(QTextDocument::ImageResource, docResourceUrl, m->currentPixmap());

        mGifResourceMapForExpression[docResourceUrl] = m;

        //qDebug() << "add:" <<docResourceUrl << " | file:" << gifFilename ;
    }


}

void CExternalTextEdit::insertExpressionShortcut(const QString &shortcut)
{
    QString uri = mExpressionTag + shortcut;
    QString element = QString("<img src='%1' />").arg(uri);
    this->insertHtml(element);
}

void CExternalTextEdit::appendText(const QString &text)
{
    QString tText = parseShortcutToExpression(text);
    QTextEdit::append(tText);
}

void CExternalTextEdit::clear()
{
    QTextEdit::clear();
    this->document()->clear();
}

QString CExternalTextEdit::message()
{
    //是有格式的数据
    QString msg = this->toHtml();
    QString t_msg = filterExpressionToShortcut(msg);

    ////出去html元素
    QTextDocument doc;
    doc.setHtml(t_msg.trimmed());

    return doc.toPlainText();
}

int CExternalTextEdit::testExpressionCount(QString msg)
{
    QString t_text = msg;

    QRegExp exp("/[a-zA-Z_]*");
    QStringList imgShortcutList;
    int pos = 0;

    while((pos = exp.indexIn(t_text, pos)) != -1)
    {
        imgShortcutList << exp.cap(0);
        pos += exp.matchedLength();
    }

    int count = 0;
    for(int i = 0; i < mMapedExpression.keys().count(); i++)
    {
        QString shortcut = mMapedExpression.keys()[i];
        for(int j = 0; j < imgShortcutList.count(); j++)
        {
            QString t_shortcut = imgShortcutList[j];
            if(t_shortcut == shortcut)
                count ++;
        }
    }

    return count;
}

int CExternalTextEdit::testWordCount(QString msg)
{
    QString t_text = msg;

    QRegExp exp("/[a-zA-Z_]*");
    QStringList imgShortcutList;
    int pos = 0;

    while((pos = exp.indexIn(t_text, pos)) != -1)
    {
        imgShortcutList << exp.cap(0);
        pos += exp.matchedLength();
    }

    //现将表情剔除
    for(int i = 0; i < mMapedExpression.keys().count(); i++)
    {
        QString shortcut = mMapedExpression.keys()[i];
        for(int j = 0; j < imgShortcutList.count(); j++)
        {
            QString t_shortcut = imgShortcutList[j];
            if(t_shortcut == shortcut)
                t_text.replace(t_shortcut, "");
        }
    }

    int count = t_text.length();

    return count;
}

QString CExternalTextEdit::removeExpressionShortcut(QString msg)
{
    QString t_text = msg;

    //add by Eosin, 在这里将表情转换一下！
    QRegExp exp("/[a-zA-Z_]*");
    QStringList imgShortcutList;
    int pos = 0;

    while((pos = exp.indexIn(t_text, pos)) != -1)
    {
        imgShortcutList << exp.cap(0);
        pos += exp.matchedLength();
    }


    for(int i = 0; i < mMapedExpression.keys().count(); i++)
    {
        QString shortcut = mMapedExpression.keys()[i];
        for(int j = 0; j < imgShortcutList.count(); j++)
        {
            QString t_shortcut = imgShortcutList[j];
            if(t_shortcut == shortcut)
                t_text.replace(t_shortcut, "");
        }
    }

    return t_text;

}

QString CExternalTextEdit::filterExpressionToShortcut(QString html) const
{
    //将表情替换成shortcut
    QString expString = "<img[^<]*/>";
    QRegExp exp(expString);
    exp.setMinimal(true);
    exp.indexIn(html);

    QString text = html;
    QStringList imgList = exp.capturedTexts();
    while(imgList.first() != "")
    {
        for(int i = 0; i < imgList.count(); i++)
        {
            QString imgElement = imgList[i];
            if(imgElement == "")
                continue;
            //找到imgElement的src

            QRegExp imgExp("\".*\"");
            imgExp.indexIn(imgElement);
            QString src = imgExp.capturedTexts().first();
            if(src == "")
                continue;

            src = src.mid(1, src.length() - 2);
            QString imgUri = src;

            //判断imgElement中是否能找到对应的shortcut，找到就替换
            bool isReplaced = false;
            for(int j = 0; j < mMapedExpression.keys().count(); j++)
            {
                QString shortcut = mMapedExpression.keys()[j];
                if(imgUri == (mExpressionTag + shortcut))
                {
                    text.replace(imgElement, shortcut);
                    isReplaced = true;
                    //qDebug() << "replace " << imgElement << " with " << shortcut;
                    break;
                }
            }

            if(!isReplaced) //* 不支持的img格式，直接删除<以后可以在这里添加忽略过去不删除>
            {
                text.replace(imgElement, "");
                qDebug() << "忽略" << imgElement;
            }
        }
        imgList.clear();
        exp.indexIn(text);
        imgList = exp.capturedTexts();
    }

    return text;
}

QString CExternalTextEdit::parseShortcutToExpression(QString text) const
{
    QString t_text = text;

    //在这里将表情转换一下！
    QRegExp exp("/[a-zA-Z_]*");
    QStringList imgShortcutList;
    int pos = 0;

    while((pos = exp.indexIn(t_text, pos)) != -1)
    {
        imgShortcutList << exp.cap(0);
        pos += exp.matchedLength();
    }


    //找到所有可能的shortcut
    QMap<QString, int> shortcutMap;
    for(int  i = 0; i < imgShortcutList.count(); i++)
    {
        shortcutMap[imgShortcutList[i]] ++;
    }

    //对比现有能识别的shortcut
    QMap<QString, int> findedShortcutMap;
    for(int i = 0; i < shortcutMap.keys().count(); i++)
    {
        QString shortcut = shortcutMap.keys()[i];
        if(shortcut == "")
        {
            continue;
        }

        //查找是否有shortcut(能匹配到的最大shortcut)
        QString findedShortcut;
        for(int i = 0; i < mMapedExpression.keys().count(); i++)
        {
            if(shortcut.contains(mMapedExpression.keys()[i]))
            {
                /*QString img = "<img src=\"Shortcut" + shortcut + "\" />";
                t_text.replace(shortcut, img);
                break;*/
                if(findedShortcut.length() < mMapedExpression.keys()[i].length())
                    findedShortcut = mMapedExpression.keys()[i];
            }
        }

        if(!findedShortcut.isEmpty())
        {
            /*
            QString img = "<img src=\"Shortcut" + findedShortcut + "\" />";
            t_text.replace(findedShortcut, img);
            */
            findedShortcutMap[findedShortcut] ++;
        }
    }

    for(int i = 0 ; i < findedShortcutMap.keys().count(); i++)
    {
        QString shortcut = findedShortcutMap.keys()[i];
        QString img = "<img src=\"" + mExpressionTag + shortcut + "\" />";
        t_text.replace(shortcut, img);
    }

    return t_text;
}

QString CExternalTextEdit::parseShortcutToHTMLExpression(QString text) const
{
	QString t_text = text;

    //add by Eosin, 在这里将表情转换一下！
    QRegExp exp("/[a-zA-Z_]*");
    QStringList imgShortcutList;
    int pos = 0;

    while((pos = exp.indexIn(t_text, pos)) != -1)
    {
        imgShortcutList << exp.cap(0);
        pos += exp.matchedLength();
    }


    QMap<QString, int> shortcutMap;
    for(int  i = 0; i < imgShortcutList.count(); i++)
    {
        shortcutMap[imgShortcutList[i]] ++;
    }

	QMap<QString, int> findedShortcutMap;
    for(int i = 0; i < shortcutMap.keys().count(); i++)
    {
        QString shortcut = shortcutMap.keys()[i];
        if(shortcut == "")
        {
            continue;
        }

        //查找是否有shortcut(能匹配到的最大shortcut)
        QString findedShortcut;
        for(int ii = 0; ii < mMapedExpression.keys().count(); ii++)
        {
            if(shortcut.contains(mMapedExpression.keys()[ii]))
            {
                if(findedShortcut.length() < mMapedExpression.keys()[ii].length())
                    findedShortcut = mMapedExpression.keys()[ii];
            }
        }

        if(!findedShortcut.isEmpty())
        {
			/*
            QString img = "<img src='" + mExpressionMap[findedShortcut] + "' />";
            t_text.replace(findedShortcut, img);
			*/
			findedShortcutMap[findedShortcut] ++;
        }
    }

	for(int i = 0 ; i < findedShortcutMap.keys().count(); i++)
	{
		QString shortcut = findedShortcutMap.keys()[i];
		QString img = "<div style='position:relative; display:inline;background-color:rgba(0,0,0,0); top:5px;'><img src='file:///" + mMapedExpression[shortcut] + QString("' width='%1' height='%2' style='top:3px;' /></div>").arg(mExpressionSize).arg(mExpressionSize);
		t_text.replace(shortcut, img);
	}

	return t_text;
}

