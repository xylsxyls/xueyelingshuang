#pragma once
#include "DialogBase.h"
#include <QPropertyAnimation>
#include "ControlsMacro.h"

class Label;
class COriginalButton;
class Separator;
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
    @param [in] userParam �û��Զ������
    */
    void setUserParam(int32_t userParam);

    /** ��ȡ�û��Զ������
    @return �����û��Զ������
    */
    int32_t userParam();

    /** ���õ����¿ո�ͻس��󴰿�Ĭ�ϵ���İ�ť
    @param [in] button Ĭ�ϵ���İ�ťָ��
    */
    void initAcceptButton(COriginalButton* button);

Q_SIGNALS:
    /** �ڴ��ڹر�֮�����ź�
    @param [in] result ���ڹر�ʱ�����ķ���ֵ
    */
    void closedSignal(int32_t result);

public slots:
    void onKeyboardAccept(QObject* tar, Qt::Key key);

protected:
    void showEvent(QShowEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool check();

private slots:
	void onTimeUpdate(int32_t timeOut);
	
protected:
	COriginalButton* m_exit;
	int32_t m_userParam;
    Label* m_time;
    std::map<QWidget*, int32_t> m_mapResult;
    int32_t m_result;
    COriginalButton* m_acceptButton;
};