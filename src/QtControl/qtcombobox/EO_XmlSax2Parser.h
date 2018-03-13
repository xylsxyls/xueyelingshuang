#ifndef EO_XMLSAX2PARSER_H
#define EO_XMLSAX2PARSER_H

#include <QVector>
#include <QStack>
#include <QXmlDefaultHandler>


/** SAX2 方式解析XML文件，并提供了简单的数据结构（如果有需要可以继续扩展和完善） **/

typedef struct xml_attribute_t{
    QString name;
    QString value;
}XMLAttribute;

typedef QVector<XMLAttribute> XMLAttributes;

typedef struct xml_node_t{
    XMLAttributes attributes;
    QVector<xml_node_t*> children;
    QString name;
    QString characters;
    xml_node_t* parent;

    xml_node_t():parent(0){

    }
    ~xml_node_t(){
		xml_node_t* p = this->parent;
		if(p)
		{
			for(int i = p->children.count() - 1; i >=0; i-- )
			{
				if(p->children[i] == this)
					p->children.remove(i);
			}
		}

		for(int i = 0; i < this->children.count(); i++){
			xml_node_t* child = this->children[i];
			if(child)
			{
				child->parent = 0;
				delete child;
			}
		}
    }

    QString getAttribute(const QString& name){
        QString value = "";
        for(int i = 0; i < attributes.count(); i++){
            if(attributes[i].name == name){
                value = attributes[i].value;
                break;
            }
        }

        return value;
    }

    void setAttribute(const QString& name, const QString& value){
        for(int i = 0; i < attributes.count(); i++){
            if(attributes[i].name == name){
                attributes[i].value = value;
                return;
            }
        }

        XMLAttribute attr;
        attr.name = name;
        attr.value = value;

        attributes << attr;
    }

    QVector<xml_node_t*> getChildrenByName(QString name){

        QVector<xml_node_t*> result;
        for(int i = 0; i < children.count(); i++){
            if(children[i]->name == name){
                result << children[i];
            }
        }
        return result;
    }

}XMLNode;

typedef QVector<XMLNode*> XMLNodeVector;


class EO_XmlSax2Parser : public QXmlDefaultHandler
{
private:
    quint64 mCurrentNodeCount;
    QStack<XMLNode*> mRootStack;
    XMLNode* mRoot;

public:
    explicit EO_XmlSax2Parser();
	~EO_XmlSax2Parser();

    bool parseFromFile(QString filename);
	bool parseFromData(QString data);
    XMLNode* root();

    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);

    bool characters(const QString &ch);

    bool fatalError(const QXmlParseException &exception);
    bool error(const QXmlParseException &exception);
    bool warning(const QXmlParseException &exception);

};

#endif // EO_XMLSAX2PARSER_H
