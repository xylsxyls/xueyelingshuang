#pragma once
#include "DialogShow.h"

class PopDialog : public DialogShow
{
    Q_OBJECT
public:
    PopDialog();
    /** ����Ĭ�Ͽؼ������йرհ�ť�����⣬�ָ��ߣ����С��������ɫ�������߿�
    @param [in] dialogWidth ���ڿ��
    @param [in] dialogHeight ���ڸ߶�
    */
    void init();

    /** �رմ���
    @param [in] result ���ڷ���ֵ
    */
    void done(int32_t result);

protected:
    void paintEvent(QPaintEvent* eve);
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);

private slots:
    void onNcActiveChanged(const bool& ncActive);

private:
    bool m_highLight;
};