#ifndef CORIGINALDIALOG_H
#define CORIGINALDIALOG_H

#include <QDialog>

class COriginalDialog : public QDialog
{
	Q_OBJECT
public:
	COriginalDialog(QWidget* parent = NULL);
	~COriginalDialog();

private:
	int     mTouchBorderWidth;
	int     mCustomerTitleBarHeight;
	QRect   mCustomerTitleBarRect;




protected:
	long onNcHitTest(QPoint pt);
	void resizeEvent(QResizeEvent *e);
	bool nativeEvent(const QByteArray &eventType, void *message, long *result);

signals:
	void ncActiveChanged(const bool& ncActive);


public :
	void setTouchBorderWidth(int n);
	int  touchBorderWidth();

	void setCustomerTitleBarHeight(int n);
	int  customerTitleBarHeight();

	QRect customerTitleBarRect();
};

#endif // CORIGINALDIALOG_H
