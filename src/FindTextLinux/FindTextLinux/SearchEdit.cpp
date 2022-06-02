#include "SearchEdit.h"
#include "SendToMessageTest.h"
#include <QTextBlock>
#include <QMouseEvent>
#include "CSystem/CSystemAPI.h"

SearchEdit::SearchEdit(QWidget* parent):
TextEdit(parent),
m_rightClickRow(-1)
{

}

SearchEdit::~SearchEdit()
{

}

int32_t SearchEdit::rightClickRow()
{
	return m_rightClickRow;
}

void SearchEdit::mousePressEvent(QMouseEvent* eve)
{
	m_rightClickRow = -1;
	if (eve->button() == Qt::RightButton)
	{
		auto pos = eve->pos();
		QTextCursor cursor = cursorForPosition(pos);
		m_rightClickRow = cursor.blockNumber();
	}
	TextEdit::mousePressEvent(eve);
}

void SearchEdit::mouseDoubleClickEvent(QMouseEvent* eve)
{
	QTextCursor cursor = textCursor();
	int32_t row = cursor.blockNumber();
	QString str = document()->findBlockByLineNumber(row).text();

	if (!str.isEmpty() && str[0] == '/')
	{
		CSystem::OpenFile("gedit " + str.toStdString());
	}

	TextEdit::mouseDoubleClickEvent(eve);
}