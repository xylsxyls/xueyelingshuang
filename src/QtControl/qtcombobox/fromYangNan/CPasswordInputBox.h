#ifndef CPASSWORDINPUTBOX_H
#define CPASSWORDINPUTBOX_H

#include "LineEdit.h"
#include "../COriginalButton.h"

class QControlVir : public QLineEdit
{
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	QControlVir(QWidget* parent = NULL) :QLineEdit(parent){}
};
class QControlBase : virtual public QLineEdit{};
class QControlFont : virtual public QControlBase{};
class QControlSelf : virtual public QControlBase{};


class CPasswordInputBox : public QControlVir,QControlFont, QControlSelf
{
    Q_OBJECT
public:
    CPasswordInputBox(QWidget* parent = NULL);
    ~CPasswordInputBox();

private:
    COriginalButton* mMaskButton;

    void layoutControl();
    void resizeEvent(QResizeEvent *e);

private slots:
    void onMaskButtonClicked();

};

#endif // CPASSWORDINPUTBOX_H
