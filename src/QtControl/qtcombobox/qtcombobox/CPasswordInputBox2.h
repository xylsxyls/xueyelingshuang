#ifndef CPASSWORDINPUTBOX2_H
#define CPASSWORDINPUTBOX2_H

#include "LineEdit.h"
#include <COriginalButton2.h>

class CPasswordInputBox2 : public QLineEdit
{
    Q_OBJECT
public:
    CPasswordInputBox2(QWidget* parent = NULL);
    ~CPasswordInputBox2();

private:
    COriginalButton2* mMaskButton;

    void layoutControl();
    void resizeEvent(QResizeEvent *e);

private slots:
    void onMaskButtonClicked();

};

#endif // CPASSWORDINPUTBOX_H
