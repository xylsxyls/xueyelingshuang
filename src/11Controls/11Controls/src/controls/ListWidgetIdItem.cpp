#include "ListWidgetIdItem.h"

ListWidgetIdItem::ListWidgetIdItem(QListWidget* view)
{

}

void ListWidgetIdItem::setId(qint64 id)
{
	m_id = id;
}

qint64 ListWidgetIdItem::getId()
{
	return m_id;
}