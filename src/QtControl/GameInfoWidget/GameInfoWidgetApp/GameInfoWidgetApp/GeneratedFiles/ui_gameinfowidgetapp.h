/********************************************************************************
** Form generated from reading UI file 'gameinfowidgetapp.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GAMEINFOWIDGETAPP_H
#define UI_GAMEINFOWIDGETAPP_H

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

class Ui_GameInfoWidgetAppClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *GameInfoWidgetAppClass)
    {
        if (GameInfoWidgetAppClass->objectName().isEmpty())
            GameInfoWidgetAppClass->setObjectName(QStringLiteral("GameInfoWidgetAppClass"));
        GameInfoWidgetAppClass->resize(600, 400);
        menuBar = new QMenuBar(GameInfoWidgetAppClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        GameInfoWidgetAppClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(GameInfoWidgetAppClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        GameInfoWidgetAppClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(GameInfoWidgetAppClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        GameInfoWidgetAppClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(GameInfoWidgetAppClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        GameInfoWidgetAppClass->setStatusBar(statusBar);

        retranslateUi(GameInfoWidgetAppClass);

        QMetaObject::connectSlotsByName(GameInfoWidgetAppClass);
    } // setupUi

    void retranslateUi(QMainWindow *GameInfoWidgetAppClass)
    {
        GameInfoWidgetAppClass->setWindowTitle(QApplication::translate("GameInfoWidgetAppClass", "GameInfoWidgetApp", 0));
    } // retranslateUi

};

namespace Ui {
    class GameInfoWidgetAppClass: public Ui_GameInfoWidgetAppClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GAMEINFOWIDGETAPP_H
