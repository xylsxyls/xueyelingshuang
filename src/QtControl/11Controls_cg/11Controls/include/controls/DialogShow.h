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
    void setTimeRestVisible(bool visible);

    void setUserParam(int32_t userParam);

    int32_t userParam();

    /** ���õ����¿ո�ͻس��󴰿�Ĭ�ϵ���İ�ť
    */
    void initKeyboardAccept(COriginalButton* button);

Q_SIGNALS:
	/** ��֪ͨ�򴰿ڹر�ʱ�����źţ��ڹرն���ִ��֮ǰ�����ź�
	@param [in] dialogId ����ID
    @param [in] userId �û��Զ���ID
	@param [in] result ���ڹر�ʱ�����ķ���ֵ
	@param [in] userParam �û��Զ���ֵ
	*/
	void dialogDone(int32_t dialogId, int32_t userId, int32_t result, int32_t userParam);

public:
    void endDialog();

protected:
    void showEvent(QShowEvent* eve);

private slots:
	void onTimeUpdate(int32_t timeOut);
    void onKeyboardAccept(QObject* tar, Qt::Key key);
	
protected:
	COriginalButton* m_exit;
	int32_t m_userParam;
    Label* m_time;
    std::map<QWidget*, int32_t> m_mapResult;
    int32_t m_result;

private:
    static std::wstring s_countDownString;
    COriginalButton* m_acceptButton;
};