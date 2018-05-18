#pragma once
#include <QString>
#include <QRect>
#include <QColor>
#include <map>
#include "DialogType.h"

#define setPopButtonConfig(button, buttonText, textColor, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, textColor, CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupButton.png", acceptDone, fontSize, &m_mapResult, 4, true)
#define setNotifyButtonConfig(button, buttonText, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, QColor(255, 255, 255, 255), "", acceptDone, fontSize, &m_mapResult, 0, false)

class Label;
class COriginalButton;
class LineEdit;
class CPasswordInputBox;
class DialogShow;
class QWindow;
class Separator;
/** �������ڴ����ؼ�
*/
class DialogHelper
{
public:
    /** ������ǩ���ֲ�����
    @param [in] label �ؼ�ָ��
    @param [in] text �ı�����
    @param [in] textColor ������ɫ
    @param [in] fontSize �����С
    @return ���ر�ǩָ��
    */
    static void setLabel(Label* label, const QString& text, const QColor& textColor, qint32 fontSize);

    /** ������ʾ���ֲ����ã����У�
    @param [in] tip �ؼ�ָ��
    @param [in] text �ı�����
    @param [in] textColor ������ɫ
    @param [in] fontSize �����С
    */
    static void setTip(Label* tip, const QString& text, const QColor& textColor, qint32 fontSize);

    /** ������ť�����ã���ť������رմ���
    @param [in] dialog ����ָ��
    @param [in] button ��ťָ��
    @param [in] text ��ť�ı�����
    @param [in] textColor ��ť������ɫ
    @param [in] backgroundImage ��ť����ͼƬ·��
    @param [in] result ���º�ķ���ֵ
    @param [in] fontSize �����С
    @param [in] mapResult ��ť����ֵ�Ĵ洢����
    @param [in] imageMargin ͼƬ��ʼλ��
    @param [in] isPop �Ƿ��ǵ����İ�ť
    */
    static void setButton(DialogShow* dialog,
                          COriginalButton* button,
                          const QString& text,
                          const QColor& textColor,
                          const QString& backgroundImage,
                          DialogResult result,
                          qint32 fontSize,
                          std::map<QWidget*, DialogResult>* mapResult,
                          qint32 imageMargin,
                          bool isPop);

    /** ��ӷָ���
    @param [in] separator �ָ��߿ؼ�ָ��
    @param [in] isHorizon �Ƿ�ˮƽ
    @param [in] upColor �ϲ���ɫ
    @param [in] downColor �²���ɫ
    */
    static void setSeparator(Separator* separator, bool isHorizon, const QColor& upColor, const QColor& downColor);

    /** ��ӵ����ı���
    @param [in] lineEdit �����ı���ָ��
    @param [in] defaultText Ĭ����������
    @param [in] maxLength �ı�����������󳤶�
    */
    static void setLineEdit(LineEdit* lineEdit, const QString& defaultText = "", qint32 maxLength = -1);

    /** ��������
    @param [in] passwordInputBox �����ָ��
    @param [in] defaultText Ĭ����������
    @param [in] maxLength �ı�����������󳤶�
    */
    static void setPasswordInputBox(CPasswordInputBox* passwordInputBox, const QString& defaultText = "", qint32 maxLength = -1);

    /** ������ʱ������
    @param [in] ��Ҫ����ڵ��Ӵ��ھ��
    */
	static void activeTransientParentWindow(QWindow* window);
};