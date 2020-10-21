#pragma once
#include "MsgLinuxMacro.h"
#include <string>

/** ר������linux�µĽ���ͨ�ţ������Զ࣬������Ϣ���10240�ֽڣ�1��1û������
*/
class MsgLinuxAPI MsgLinux
{
public:
    /** ���캯��
    @param [in] path ���ڵ��ļ�·�������߽���Ҫ��ͬ
    @param [in] isCreate �Ƿ��Ǵ�����
    */
	MsgLinux(const std::string& path, bool isCreate);

    /** ��������
    */
    ~MsgLinux();

public:
    /** ����
    @param [in] src ���͵�����
    @param [in] type ���͵�����
    @return �����Ƿ��ͳɹ�
    */
    bool send(const std::string& src, long type = 1);

    /** ����
    @param [in] receive ���յ�����
    @param [in] type ���յ�����
    @return �����Ƿ���ճɹ�������false��ʾ�ö����ѱ�ɾ��
    */
    bool recv(std::string& receive, long type = 1);

    /** ���ٶ��У����ڽ��ն��˳�recv���������̵߳���
    */
    void destroy();

protected:
    //��Ϣ����ID����������Ϊ-1
    int m_msgid;
    //�Ƿ��Ƕ��д�����
    bool m_isCreate;
};