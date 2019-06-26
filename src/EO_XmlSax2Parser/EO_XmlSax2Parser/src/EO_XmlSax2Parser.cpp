#include "EO_XmlSax2Parser.h"
#include <QXmlSimpleReader>
#include <QDebug>

EO_XmlSax2Parser::EO_XmlSax2Parser()
    :
      mRoot(NULL),
      mCurrentNodeCount(0)
{

}

EO_XmlSax2Parser::~EO_XmlSax2Parser()
{
	if(mRoot != NULL)
	{
		delete mRoot;
		mRoot = NULL;
	}
}

bool EO_XmlSax2Parser::parseFromFile(QString filename)
{
    if(mRoot != NULL)
    {
        delete mRoot;
        mRoot = NULL;
    }

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "xml parser " << filename << " can not open";
        return false;
    }
    QXmlInputSource source(&file);
    QXmlSimpleReader reader;

    reader.setContentHandler(this);
    bool isOk = reader.parse(source);
    file.close();

    qDebug() << "xml parser " << filename << " parse Status:" << isOk;

    return isOk;
}

bool EO_XmlSax2Parser::parseFromData(QString data)
{
	if(mRoot != NULL)
	{
		delete mRoot;
		mRoot = NULL;
	}

	QXmlInputSource source;
	source.setData(data);
	QXmlSimpleReader reader;

	reader.setContentHandler(this);
	bool isOk = reader.parse(source);

	qDebug() << "xml parser parse Status:" << isOk;

	return isOk;
}

XMLNode *EO_XmlSax2Parser::root()
{
    return mRoot;
}

bool EO_XmlSax2Parser::startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts)
{
//    qDebug() << "start element:" << localName;

    XMLNode* newNode = new XMLNode;

    newNode->name = localName;

    XMLAttributes newAttributes;
    for(int i = 0; i < atts.count(); i++)
    {
        XMLAttribute newAttribute;
        newAttribute.name = atts.localName(i);
        newAttribute.value = atts.value(i);
		newAttributes << newAttribute;
    }

    newNode->attributes = newAttributes;


    if(mRoot == NULL)
    {
        mRoot = newNode;
    }
    else
    {
		newNode->parent = mRootStack.top();
        mRootStack.top()->children << newNode;
    }

    mRootStack.push(newNode);

    return true;
}

bool EO_XmlSax2Parser::endElement(const QString &namespaceURI, const QString &localName, const QString &qName)
{
//    qDebug() << "end element:" << localName;

    if(mRootStack.count() <= 0)
    {
        return false;
    }

    if(mRootStack.top()->name != localName)
    {
        return false;
    }

    mRootStack.pop();
	return true;
}

bool EO_XmlSax2Parser::characters(const QString &ch)
{
//    qDebug() << "characters:" << ch;

    if(mRootStack.count() <= 0)
    {
        return false;
    }

    mRootStack.top()->characters = ch.trimmed();
	return true;
}

bool EO_XmlSax2Parser::fatalError(const QXmlParseException &exception)
{
    qDebug() << exception.message();
    return false;
}

bool EO_XmlSax2Parser::error(const QXmlParseException &exception)
{
    qDebug() << exception.message();
    return false;
}

bool EO_XmlSax2Parser::warning(const QXmlParseException &exception)
{
    qDebug() << exception.message();
    return false;
}

