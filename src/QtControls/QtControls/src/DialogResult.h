#pragma once

/** ���ڷ���ֵ
*/
enum DialogResult
{
    /** ����ֵ�����ڳ�ʼ��
    */
    ERROR_RESULT = 1000,

    /** ȷ�ϰ�ť
    */
    ACCEPT_BUTTON,

    /** ȡ����ť
    */
    IGNORE_BUTTON,

    /** ��ʱ
    */
    TIME_OUT,

    /** Esc�˳�
    */
    ESC_EXIT,

    /** Alt+F4�˳�
    */
    ALT_F4_EXIT,

    /** ���ϽǵĹر�
    */
    RIGHT_TOP_EXIT,

    /** ��������
    */
    CODE_DESTROY = -1,

    /** ������ΪUserId�ظ����ر�
    */
    BUSY = -2
};