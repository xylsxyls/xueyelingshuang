#pragma once
#include <stdint.h>
#include <QString>
#include <QPoint>
#include <QObject>
#include "ControlsMacro.h"

class TipLabel;
class ControlsAPI TipLabelManager : public QObject
{
	Q_OBJECT
public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static TipLabelManager& instance();

public:
	/** ������ʾ
	@param [in] pos �������½�
	@param [in] text ����
	@param [in] maxWidth �����
	@param [in] timeOut ���ʾʱ�䣬��λ��
	@param [in] hasFocus ����ʱ�Ƿ������㣬��������㣬����ʧȥ����ʱ�Զ���ʧ
	*/
	void popTip(const QPoint& pos, const QString& text, int32_t maxWidth = 500, int32_t timeOut = 10, bool hasFocus = true);

	/** �ر���ʾ��֧�ֶ��̣߳�
	*/
	void close();

Q_SIGNALS:
	void closeTip();

private:
	TipLabelManager();
	~TipLabelManager();

	void init();

private:
	TipLabel* m_tipLabel;
};