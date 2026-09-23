#include "MainWindow.h"
#include <QVariant>
#include "StatusBar.h"

MainWindow::MainWindow(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setProperty("class", QVariant(QString("MainWindow")));
	setStatusBar(new StatusBar(this));
}

void MainWindow::setStyleSheet(const QString& styleSheet)
{
    m_contentStyle = styleSheet;
    repaint();
}

void MainWindow::repaint()
{
    initClassName();
    QMainWindow::setStyleSheet(QString::fromStdWString(m_controlStyle.toWString()) + m_contentStyle);
}
