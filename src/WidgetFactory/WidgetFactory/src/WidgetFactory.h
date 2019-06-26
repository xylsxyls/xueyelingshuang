#pragma once
#include <stdint.h>
#include <map>
#include <QWidget>
#include "WidgetFactoryMacro.h"

/** Widget���칤��
*/
class WidgetFactoryAPI WidgetFactory : public QObject
{
	Q_OBJECT
protected:
	/** ���캯��
	*/
	WidgetFactory();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static WidgetFactory& instance();
	
public:
	/** ��ȡwidget��û�лᴴ�������򷵻ش�������
	@param [in] widgetId widgetID��
	*/
	template<typename WidgetT>
	WidgetT* getWidget(int32_t widgetId);

	/** ����widget���Ὣ��������Ϊ�պ�delete
	@param [in] widgetId widgetID��
	*/
	template<typename WidgetT>
	void destroyWidget(int32_t widgetId);

	/** ��ȡwidget��û�лᴴ��������ɾ��������
	@param [in] widgetId widgetID��
	*/
	template<typename WidgetT>
	WidgetT* recreateWidget(int32_t widgetId);

private slots:
	void onWidgetDestroyed();

private:
	std::map<int32_t, QWidget*> m_widget;
};

template<typename WidgetT>
WidgetT* WidgetFactory::getWidget(int32_t widgetId)
{
	auto itWidget = m_widget.find(widgetId);
	if (itWidget != m_widget.end())
	{
		return itWidget->second;
	}
	WidgetT* widgetT = new WidgetT;
	QWidget* widget = qobject_cast<QWidget*>(widgetT);
	if (widget == nullptr)
	{
		return nullptr;
	}
	QObject::connect(widget, &QWidget::destroyed, this, &WidgetFactory::onWidgetDestroyed);
	m_widget[widgetId] = widget;
	return widget;
}

template<typename WidgetT>
void WidgetFactory::destroyWidget(int32_t widgetId)
{
	auto itWidget = m_widget.find(widgetId);
	if (itWidget == m_widget.end())
	{
		return;
	}
	QWidget* widget = (QWidget*)itWidget->second;
	m_widget.erase(itWidget);
	widget->setParent(nullptr);
	delete widget;
}

template<typename WidgetT>
WidgetT* WidgetFactory::recreateWidget(int32_t widgetId)
{
	destroyWidget<WidgetT>(widgetId);
	return getWidget<WidgetT>(widgetId);
}