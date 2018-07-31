#pragma once
#include "DialogBase.h"
#include <QPropertyAnimation>
#include "ControlsMacro.h"

class Label;
class COriginalButton;
class Separator;
enum DialogResult;
/** ���ڻ���ģ�ͣ�ʵ��ͨ�ò���
*/
class ControlsAPI DialogShow : public DialogBase
{
	Q_OBJECT
public:
	/** ���캯��
	*/
	DialogShow();

	/** ��������
	*/
	virtual ~DialogShow();

public:
    /** ����ʣ��ʱ���Ƿ���ʾ
    @param [in] visible �Ƿ���ʾ
    */
    void setTimeRestVisible(bool visible);

    /** �����û��Զ������
    @param [in] userResult �û��Զ������
    */
    void setUserResult(qint32 userResult);

    /** ��ȡ�û��Զ������
    @return �����û��Զ������
    */
    qint32 userResult();

	/** �����û��Զ������ָ��
	@param [in] userResult �û��Զ������
	*/
	void setUserResultPtr(qint32* userResult);

	/** ��ȡ�洢�û��Զ���ֵ��ָ��
	@return ���ش洢�û��Զ���ֵ��ָ��
	*/
	qint32* userResultPtr();

    /** ���õ����¿ո�ͻس��󴰿�Ĭ�ϵ���İ�ť
    @param [in] button Ĭ�ϵ���İ�ťָ��
    */
    void initAcceptButton(COriginalButton* button);

    /** ���÷���ֵ�洢��
    @param [in] result �ⲿ�洢��ָ��
    */
    void setWindowResultAddr(DialogResult* result);

    /** ���÷���ֵ
    @param [in] result ����ֵ�������ⲿ�洢��
    */
    void setWindowResult(DialogResult result);

Q_SIGNALS:
    /** �ڴ��ڹر�֮�����ź�
    @param [in] result ���ڹر�ʱ�����ķ���ֵ
    */
    void closedSignal(DialogResult* result);

protected slots:
    void onKeyboardAccept(QObject* tar, Qt::Key key);

protected:
    void showEvent(QShowEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool check();
    void escEvent();
    void altF4PressedEvent();

private slots:
	void onTimeUpdate(qint32 timeOut);
	
protected:
	COriginalButton* m_exit;
	qint32* m_userResult;
    Label* m_time;
	DialogResult* m_result;
    std::map<QWidget*, DialogResult> m_mapResult;
	//���ߺͿո�Ĭ��ִ�еİ�ť�����ഫ��ָ��
    COriginalButton* m_acceptButton;
};