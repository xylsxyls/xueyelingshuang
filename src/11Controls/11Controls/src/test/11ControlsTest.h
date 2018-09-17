#ifndef QTCOMBOBOX_H
#define QTCOMBOBOX_H

#include <QtWidgets/QMainWindow>
#include "ui_11ControlsTest.h"
#include "11Controls/include/controls/DialogType.h"
#include <QDialog>

class ControlsTest : public QMainWindow
{
	Q_OBJECT

public:
	ControlsTest(QWidget *parent = 0);
	~ControlsTest();

private slots:
	void onPopTipDialog();
	void onPopAskDialog();
	void onPopWaitDialog();
	void onPopInputDialog();
	void onShowTipDialog();
	void onShowLoginDialog();
	void onShowAskDialog();
	void onShowDestroyAll();

	void onDialogSignal(const SignalParam& param);
	void onCurrentItemChanged(qint32 index, qint64 id, const QString& text);

	void onCurrentItemChanged2(qint64 id, const QString& text);

	void onCurrentIndexChanged(const QString& text);

	void onClicked();

	void timerEvent(QTimerEvent* eve);
private:
	Ui::ControlsTestClass ui;
};



//#include <QtCore/QVariant>
//#include <QtWidgets/QAction>
//#include <QtWidgets/QApplication>
//#include <QtWidgets/QButtonGroup>
//#include <QtWidgets/QDialog>
//#include <QtWidgets/QFrame>
//#include <QtWidgets/QHeaderView>
//#include <QtWidgets/QPushButton>
//#include <QtWidgets/QVBoxLayout>
//
//class Ui_Dialog2
//{
//public:
//	QVBoxLayout *verticalLayout;
//	QFrame *frame;
//	QPushButton *pushButton;
//	QPushButton *pushButton_2;
//
//	void setupUi(QDialog *Dialog)
//	{
//		if (Dialog->objectName().isEmpty())
//			Dialog->setObjectName(QStringLiteral("Dialog"));
//		Dialog->resize(613, 327);
//		verticalLayout = new QVBoxLayout(Dialog);
//		verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
//		frame = new QFrame(Dialog);
//		frame->setObjectName(QStringLiteral("frame"));
//		frame->setStyleSheet(QLatin1String("QFrame\n"
//			"{\n"
//			"	background-color:#ffffff;\n"
//			"\n"
//			"	border:none;\n"
//			"	border-radius:10px;\n"
//			"}"));
//		frame->setFrameShape(QFrame::StyledPanel);
//		frame->setFrameShadow(QFrame::Raised);
//		pushButton = new QPushButton(frame);
//		pushButton->setObjectName(QStringLiteral("pushButton"));
//		pushButton->setGeometry(QRect(250, 200, 75, 23));
//		pushButton_2 = new QPushButton(frame);
//		pushButton_2->setObjectName(QStringLiteral("pushButton_2"));
//		pushButton_2->setGeometry(QRect(420, 40, 75, 23));
//
//		verticalLayout->addWidget(frame);
//
//
//		retranslateUi(Dialog);
//		QObject::connect(pushButton_2, SIGNAL(clicked()), Dialog, SLOT(reject()));
//
//		QMetaObject::connectSlotsByName(Dialog);
//	} // setupUi
//
//	void retranslateUi(QDialog *Dialog)
//	{
//		Dialog->setWindowTitle(QApplication::translate("Dialog", "Dialog", 0));
//		pushButton->setText(QApplication::translate("Dialog", "PushButton", 0));
//		pushButton_2->setText(QApplication::translate("Dialog", "\345\205\263\351\227\255", 0));
//	} // retranslateUi
//
//};
//
//namespace Ui {
//	class Dialog2 : public Ui_Dialog2 {};
//} // namespace Ui
//
//
//class Dialog2 : public QDialog
//{
//	Q_OBJECT
//
//public:
//	explicit Dialog2(QWidget *parent = 0);
//	~Dialog2();
//
//private:
//	Ui::Dialog2 *ui;
//};

#endif // QTCOMBOBOX_H
