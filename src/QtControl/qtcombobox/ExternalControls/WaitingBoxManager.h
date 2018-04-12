#pragma once
#include <QObject>
#include <stdint.h>
#include <map>
#include "WaitingBox.h"

class QWindow;
class WaitingBox;
/** �����࣬�ȴ��������
*/
class WaitingBoxManager : public QObject
{
	Q_OBJECT
public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static WaitingBoxManager& instance();

public:
	/** ��ʾ�ȴ��򣬲��������뵫����������
	@param [in] key �û�����Ĵ���ID���û���֤Ψһ��
	@param [in] title ���ڱ���
	@param [in] tip ��ʾ
	@param [in] parent ��ʱ�����ھ��
	@param [in] timeOut �Զ���ʧʱ�䣨���룩
	@param [in] userExit �û��Ƿ���������ر�
	*/
	void showWaitingBox(int32_t key,
						const QString& title,
						const QString& tip,
						QWindow* parent = nullptr,
						int32_t timeOut = 3000,
						bool userExit = false);

	/** �رյ�����֧�ֶ��߳�
	@param [in] key �û�����keyֵ
	*/
	void closeWaitingBox(int32_t key);

	/** �������д����ĵȴ���
	*/
	void destroyAll();

Q_SIGNALS:
	/** ���ȴ���ر�ʱ�����ź�
	@param [in] key �û�����keyֵ
	@param [in] exitEnum �ر�����
	*/
	void waitingBoxDone(int32_t key, WaitingBox::ExitEnum exitEnum);

private slots:
	void onWaitingBoxDone(WaitingBox* box, WaitingBox::ExitEnum exitEnum);

private:
	std::map<int32_t, WaitingBox*> m_mapWaitingBox;

};