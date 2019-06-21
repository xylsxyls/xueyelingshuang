#pragma once
#include <QString>
#include <QPoint>
#include <QObject>
#include "QtControlsMacro.h"

class TipLabel;
class QtControlsAPI TipLabelManager : public QObject
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
	void popTip(const QPoint& pos, const QString& text, qint32 maxWidth = 500, qint32 timeOut = 10, bool hasFocus = true);

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