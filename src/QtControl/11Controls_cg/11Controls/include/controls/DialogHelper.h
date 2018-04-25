#pragma once
#include <QString>
#include <QRect>
#include <QColor>
#include <stdint.h>
#include <map>

#define setPopButtonConfig(button, buttonText, textColor, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, textColor, CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupButton.png", acceptDone, fontSize, &m_mapResult, 4, true)
#define setNotifyButtonConfig(button, buttonText, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, QColor(255, 255, 255, 255), "", acceptDone, fontSize, &m_mapResult, 0, false)

class Label;
class COriginalButton;
class QSplitter;
class LineEdit;
class CPasswordInputBox;
class DialogShow;
class QWidget;
class QWindow;
class Separator;
/** �������ڴ����ؼ�
*/
class DialogHelper
{
public:
    /** ������ǩ���ֲ�����
    @param [in] text �ı�����
    @param [in] rect ��ǩ��Ը���������λ��
    @param [in] textColor ������ɫ
    @return ���ر�ǩָ��
    */
    static void setLabel(Label* label, const QString& text, const QColor& textColor, int32_t fontSize);

    /** ������ʾ���ֲ����ã����У�
    @param [in] text �ı�����
    @param [in] rect ��ǩ��Ը���������λ��
    @param [in] textColor ������ɫ
    @return ���ر�ǩָ��
    */
    static void setTip(Label* tip, const QString& text, const QColor& textColor, int32_t fontSize);

    /** ������ť�����ã���ť������رմ���
    @param [in] text ��ť���ı�����
    @param [in] rect ��ť��Ը���������λ��
    @param [in] result �رմ��ں�ķ���ֵ
    @return ���ذ�ťָ��
    */
    static void setButton(DialogShow* dialog,
                          COriginalButton* button,
                          const QString& text,
                          const QColor& textColor,
                          const QString& backgroundImage,
                          int32_t result,
                          int32_t fontSize,
                          std::map<QWidget*, int32_t>* mapResult,
                          int32_t imageMargin,
                          bool isPop);

    /** ��ӷָ���
    @param [in] �ָ��߿ؼ�ָ��
    @param [in] isHorizon �Ƿ�ˮƽ
    @param [in] upColor �ϲ���ɫ
    @param [in] downColor �²���ɫ
    @return ���طָ���ָ��
    */
    static void setSeparator(Separator* separator, bool isHorizon, const QColor& upColor, const QColor& downColor);

    /** ��ӵ����ı���
    @param [in] rect ��ǩ��Ը���������λ��
    @param [in] defaultText Ĭ����ʾ����
    @return �����ı���ָ��
    */
    static void setLineEdit(LineEdit* lineEdit, const QString& defaultText = "", int32_t maxLength = -1);

    /** ��������
    @param [in] rect ��ǩ��Ը���������λ��
    @param [in] defaultText Ĭ����ʾ����
    @return �����ı���ָ��
    */
    //static CPasswordInputBox* addPasswordInputBox(const QRect& rect, const QString& defaultText = "");

    /** ������ʱ������
    @param [in] ��Ҫ����ڵ��Ӵ��ھ��
    */
    void activeTransientParentWindow(QWindow* window);
};