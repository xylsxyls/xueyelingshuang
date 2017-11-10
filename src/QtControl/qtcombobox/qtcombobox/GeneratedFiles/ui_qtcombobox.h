/********************************************************************************
** Form generated from reading UI file 'qtcombobox.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QTCOMBOBOX_H
#define UI_QTCOMBOBOX_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_qtcomboboxClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *qtcomboboxClass)
    {
        if (qtcomboboxClass->objectName().isEmpty())
            qtcomboboxClass->setObjectName(QStringLiteral("qtcomboboxClass"));
        qtcomboboxClass->resize(600, 400);
        menuBar = new QMenuBar(qtcomboboxClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        qtcomboboxClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(qtcomboboxClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        qtcomboboxClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(qtcomboboxClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        qtcomboboxClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(qtcomboboxClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        qtcomboboxClass->setStatusBar(statusBar);

        retranslateUi(qtcomboboxClass);

        QMetaObject::connectSlotsByName(qtcomboboxClass);
    } // setupUi

    void retranslateUi(QMainWindow *qtcomboboxClass)
    {
        qtcomboboxClass->setWindowTitle(QApplication::translate("qtcomboboxClass", "qtcombobox", 0));
    } // retranslateUi

};

namespace Ui {
    class qtcomboboxClass: public Ui_qtcomboboxClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QTCOMBOBOX_H
