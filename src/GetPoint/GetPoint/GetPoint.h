#ifndef QTTEST_H
#define QTTEST_H

#include <QtWidgets/QMainWindow>
#include "ui_GetPoint.h"
#include "QtControls/DialogShow.h"

class COriginalButton;
class LineEdit;

class GetPoint : public DialogShow
{
	Q_OBJECT
public:
	GetPoint(QWidget* parent = nullptr);
	~GetPoint();

protected:
	void init();
	bool check();

protected:
	void resizeEvent(QResizeEvent* eve);
	void closeEvent(QCloseEvent* eve);

private slots:
	void onPoint1ButtonClicked();
	void onPoint2ButtonClicked();
	void updateModelWindow();

private:
	Ui::GetPointClass ui;
	COriginalButton* m_point1;
	LineEdit* m_text1;
	LineEdit* m_text2;
	COriginalButton* m_point2;
};

#endif // QTTEST_H