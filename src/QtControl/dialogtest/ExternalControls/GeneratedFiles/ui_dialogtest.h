/********************************************************************************
** Form generated from reading UI file 'dialogtest.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOGTEST_H
#define UI_DIALOGTEST_H

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

class Ui_dialogtestClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *dialogtestClass)
    {
        if (dialogtestClass->objectName().isEmpty())
            dialogtestClass->setObjectName(QStringLiteral("dialogtestClass"));
        dialogtestClass->resize(600, 400);
        menuBar = new QMenuBar(dialogtestClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        dialogtestClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(dialogtestClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        dialogtestClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(dialogtestClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        dialogtestClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(dialogtestClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        dialogtestClass->setStatusBar(statusBar);

        retranslateUi(dialogtestClass);

        QMetaObject::connectSlotsByName(dialogtestClass);
    } // setupUi

    void retranslateUi(QMainWindow *dialogtestClass)
    {
        dialogtestClass->setWindowTitle(QApplication::translate("dialogtestClass", "dialogtest", 0));
    } // retranslateUi

};

namespace Ui {
    class dialogtestClass: public Ui_dialogtestClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOGTEST_H
