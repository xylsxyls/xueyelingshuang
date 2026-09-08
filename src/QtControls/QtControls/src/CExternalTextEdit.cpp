#include "CExternalTextEdit.h"

#include "ExpressionConfigParser.h"
#include <QMimeData>
#include <QMouseEvent>
#include <QMovie>
#include <QtAlgorithms>
#include <stdint.h>

CExternalTextEdit::CExternalTextEdit(QWidget *parent)
    :QTextEdit(parent)
    ,m_expressionTag("expression")
	,m_expressionSize(18)
{
    m_refreshGifTimer.setInterval(250);
    connect(&m_refreshGifTimer, &QTimer::timeout, this, &CExternalTextEdit::updateGifResourceForExpression);

	this->setContextMenuPolicy(Qt::NoContextMenu);
}

CExternalTextEdit::~CExternalTextEdit()
{
	qDeleteAll(m_gifResourceMapForExpression);
	m_gifResourceMapForExpression.clear();
}

void CExternalTextEdit::showEvent(QShowEvent *e)
{
	if (e == nullptr)
	{
		return;
	}
    QTextEdit::showEvent(e);
    m_refreshGifTimer.start();
	QStringList resourceKeys = m_gifResourceMapForExpression.keys();
	for(int32_t i = 0; i < resourceKeys.count(); ++i)
	{
		QMovie* movie = m_gifResourceMapForExpression.value(resourceKeys[i], nullptr);
		if(movie != nullptr)
		{
			movie->start();
		}
	}
}

void CExternalTextEdit::hideEvent(QHideEvent *e)
{
	if (e == nullptr)
	{
		return;
	}
    QTextEdit::hideEvent(e);
    m_refreshGifTimer.stop();
	QStringList resourceKeys = m_gifResourceMapForExpression.keys();
	for(int32_t i = 0; i < resourceKeys.count(); ++i)
	{
		QMovie* movie = m_gifResourceMapForExpression.value(resourceKeys[i], nullptr);
		if(movie != nullptr)
		{
			movie->stop();
		}
	}
}

void CExternalTextEdit::keyPressEvent(QKeyEvent *e)
{
	if (e == nullptr)
	{
		return;
	}
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
	if (e == nullptr)
	{
		return;
	}
	QTextEdit::mousePressEvent(e);
	this->activateWindow();
}

void CExternalTextEdit::updateGifResourceForExpression()
{
	QTextDocument* textDocument = document();
	if (textDocument == nullptr)
	{
		return;
	}
	QStringList resourceKeys = m_gifResourceMapForExpression.keys();
    for(int32_t i = 0; i < resourceKeys.count(); ++i)
    {
        QMovie* movie = m_gifResourceMapForExpression.value(resourceKeys[i], nullptr);
        if(movie == nullptr)
		{
			continue;
		}

		const int32_t frameCount = movie->frameCount();
		if (frameCount <= 0)
		{
			continue;
		}

        int32_t currentFrame = qMax(movie->currentFrameNumber(), 0);
        currentFrame = currentFrame < frameCount - 1 ? currentFrame + 1 : 0;

        movie->jumpToFrame(currentFrame);

        textDocument->addResource(QTextDocument::ImageResource, resourceKeys[i], movie->currentPixmap());
    }

    setLineWrapColumnOrWidth(lineWrapColumnOrWidth());

}

void CExternalTextEdit::insertFromMimeData(const QMimeData *source)
{
	if (source == nullptr)
	{
		return;
	}
    QMimeData n_source;

    QString text;
    if(source->hasHtml())
    {
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
        n_source.setHtml(text);

    }
    else if(source->hasText())
    {
        text = source->text().toHtmlEscaped();
        text = text.replace("\n","<br/>");


        //TODO:将shortcut换成img
        text = parseShortcutToExpression(text);
        n_source.removeFormat("text/plain");


        n_source.setHtml(text);
    }
    QTextEdit::insertFromMimeData(&n_source);
}

QMimeData *CExternalTextEdit::createMimeDataFromSelection() const
{
    QMimeData* tData = QTextEdit::createMimeDataFromSelection();
	if (tData == nullptr)
	{
		return nullptr;
	}
    QString text;

    if(tData->hasHtml())
    {
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

        tData->removeFormat("text/html");
        tData->setText(text);
    }
    else if(tData->hasText())
    {
        text = tData->text();
    }


    return tData;
}

void CExternalTextEdit::loadExpressions(const QString& emotionPath)
{
	qDeleteAll(m_gifResourceMapForExpression);
	m_gifResourceMapForExpression.clear();
	m_mappedExpression.clear();

	ExpressionConfig config;
	ExpressionConfigParser parser;
	if (!parser.parse(emotionPath, &config))
	{
		return;
	}

	for (int32_t i = 0; i < config.m_expressions.count(); ++i)
	{
		const ExpressionInfo& expression = config.m_expressions[i];
		if (expression.isValid())
		{
			m_mappedExpression[expression.m_shortcut] = expression.m_fileName;
		}
	}

    QStringList shortcutKeys = m_mappedExpression.keys();
    for(int32_t i = 0; i < shortcutKeys.count(); ++i)
    {
        QString gifFilename = m_mappedExpression[shortcutKeys[i]];
        QMovie* m = new QMovie;
        m->setCacheMode(QMovie::CacheAll);
        m->setFileName(gifFilename);
		m->setScaledSize(QSize(m_expressionSize,m_expressionSize));
		if(this->isVisible())
		{
			m->start();
		}

        QString docResourceUrl(m_expressionTag + shortcutKeys[i]);
		QTextDocument* textDocument = document();
		if (textDocument != nullptr)
		{
			textDocument->addResource(QTextDocument::ImageResource, docResourceUrl, m->currentPixmap());
		}

        m_gifResourceMapForExpression[docResourceUrl] = m;

        //qDebug() << "add:" <<docResourceUrl << " | file:" << gifFilename ;
    }
}

void CExternalTextEdit::insertExpressionShortcut(const QString &shortcut)
{
    QString uri = m_expressionTag + shortcut;
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
	QTextDocument* textDocument = document();
	if (textDocument != nullptr)
	{
		textDocument->clear();
	}
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
	const QStringList shortcutKeys = m_mappedExpression.keys();
    for(int i = 0; i < shortcutKeys.count(); i++)
    {
        QString shortcut = shortcutKeys[i];
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
	const QStringList shortcutKeys = m_mappedExpression.keys();
    for(int i = 0; i < shortcutKeys.count(); i++)
    {
        QString shortcut = shortcutKeys[i];
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


	const QStringList shortcutKeys = m_mappedExpression.keys();
    for(int i = 0; i < shortcutKeys.count(); i++)
    {
        QString shortcut = shortcutKeys[i];
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
    while(imgList.value(0) != "")
    {
        for(int i = 0; i < imgList.count(); i++)
        {
            QString imgElement = imgList[i];
            if(imgElement == "")
                continue;
            //找到imgElement的src

            QRegExp imgExp("\".*\"");
            imgExp.indexIn(imgElement);
			const QStringList capturedSrcList = imgExp.capturedTexts();
			if (capturedSrcList.isEmpty())
			{
				continue;
			}
            QString src = capturedSrcList.value(0);
            if(src == "")
                continue;

            src = src.mid(1, src.length() - 2);
            QString imgUri = src;

            //判断imgElement中是否能找到对应的shortcut，找到就替换
            bool isReplaced = false;
			const QStringList shortcutKeys = m_mappedExpression.keys();
            for(int j = 0; j < shortcutKeys.count(); j++)
            {
                QString shortcut = shortcutKeys[j];
                if(imgUri == (m_expressionTag + shortcut))
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
	const QStringList shortcutMapKeys = shortcutMap.keys();
	const QStringList expressionKeys = m_mappedExpression.keys();
    for(int i = 0; i < shortcutMapKeys.count(); i++)
    {
        QString shortcut = shortcutMapKeys[i];
        if(shortcut == "")
        {
            continue;
        }

        //查找是否有shortcut(能匹配到的最大shortcut)
        QString findedShortcut;
        for(int j = 0; j < expressionKeys.count(); j++)
        {
            if(shortcut.contains(expressionKeys[j]))
            {
                /*QString img = "<img src=\"Shortcut" + shortcut + "\" />";
                t_text.replace(shortcut, img);
                break;*/
                if(findedShortcut.length() < expressionKeys[j].length())
                    findedShortcut = expressionKeys[j];
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

	const QStringList foundShortcutKeys = findedShortcutMap.keys();
    for(int i = 0 ; i < foundShortcutKeys.count(); i++)
    {
        QString shortcut = foundShortcutKeys[i];
        QString img = "<img src=\"" + m_expressionTag + shortcut + "\" />";
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
	const QStringList shortcutMapKeys = shortcutMap.keys();
	const QStringList expressionKeys = m_mappedExpression.keys();
    for(int i = 0; i < shortcutMapKeys.count(); i++)
    {
        QString shortcut = shortcutMapKeys[i];
        if(shortcut == "")
        {
            continue;
        }

        //查找是否有shortcut(能匹配到的最大shortcut)
        QString findedShortcut;
        for(int ii = 0; ii < expressionKeys.count(); ii++)
        {
            if(shortcut.contains(expressionKeys[ii]))
            {
                if(findedShortcut.length() < expressionKeys[ii].length())
                    findedShortcut = expressionKeys[ii];
            }
        }

        if(!findedShortcut.isEmpty())
        {
			findedShortcutMap[findedShortcut] ++;
        }
    }

	const QStringList foundShortcutKeys = findedShortcutMap.keys();
	for(int i = 0 ; i < foundShortcutKeys.count(); i++)
	{
		QString shortcut = foundShortcutKeys[i];
		QString img = "<div style='position:relative; display:inline;background-color:rgba(0,0,0,0); top:5px;'><img src='file:///" + m_mappedExpression[shortcut] + QString("' width='%1' height='%2' style='top:3px;' /></div>").arg(m_expressionSize).arg(m_expressionSize);
		t_text.replace(shortcut, img);
	}

	return t_text;
}