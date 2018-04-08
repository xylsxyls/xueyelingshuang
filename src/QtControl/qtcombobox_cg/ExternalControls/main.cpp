#include "qtcombobox.h"
#include <QtWidgets/QApplication>
#include "Label.h"

class GroupWidget :public Label
{
public:
	GroupWidget() :mIcon(new Label(this))

	{
		this->setMouseTracking(true);
		//this->resize(100, 50);
		//mHoverFrame->resize(this->size());
		//mHoverFrame->setBackgroundColor("lightblue", "lightblue", "lightblue");
		//mHoverFrame->setWindowFlags(mHoverFrame->windowFlags() | Qt::WindowStaysOnBottomHint);

		setBackgroundColor(QColor("lightblue"), QColor("lightblue"), QColor("lightblue"));
		mIcon->setBorderImage("E:/newClient/output/W3L/res/Data/War3/GameChannel/6.png", 1, 1, 1, 1);
		mIcon->resize(60, 60);
		this->setTextOrigin(65);
		//this->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		this->setText("<font size='14' color = 'red'>my name< / font><img src = 'E:/newClient/output/W3L/res/Image/MatchChat/vip.png' />");
	}


private:
	Label* mIcon;

	void enterEvent(QEvent *e)
	{
		QWidget::enterEvent(e);
		setBackgroundColor(QColor("lightblue").dark(), QColor("lightblue").dark(), QColor("lightblue").dark(),true);
		
	}

	void leaveEvent(QEvent *e)
	{
		QWidget::leaveEvent(e);
		setBackgroundColor(QColor("lightblue"), QColor("lightblue"), QColor("lightblue"), true);
	}

	
};

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	qtcombobox w;
	w.show();

	//GroupWidget * gw = new GroupWidget;
	//gw->show();

	return a.exec();
}
