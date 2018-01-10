#ifndef CW3LMODALFRAME_H
#define CW3LMODALFRAME_H

#include "COriginalDialog.h"

class CW3LModalFrame : public COriginalDialog
{
	Q_OBJECT
public:
	CW3LModalFrame(QWidget* parent = NULL);
	~CW3LModalFrame();

private:
	QRect   mTitleBarRect;
	int     mTitleBarHeight;

protected:

	void paintEvent(QPaintEvent *e);
	void closeEvent(QCloseEvent *e);

signals:
	void closed();

private slots:
	void onWindowTitleChanged(const QString& s);

};
#endif // CW3LMODALFRAME_H
