#pragma once
#include <stdint.h>
#include <map>
#include <QWidget>
#include "WidgetFactoryMacro.h"

/** Widget构造工厂
*/
class WidgetFactoryAPI WidgetFactory : public QObject
{
	Q_OBJECT
protected:
	/** 构造函数
	*/
	WidgetFactory();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static WidgetFactory& instance();
	
public:
	/** 获取widget，没有会创建，有则返回创建过的
	@param [in] widgetId widgetID号
	*/
	template<typename WidgetT>
	WidgetT* getWidget(int32_t widgetId);

	/** 销毁widget，会将父窗口设为空后delete
	@param [in] widgetId widgetID号
	*/
	template<typename WidgetT>
	void destroyWidget(int32_t widgetId);

	/** 获取widget，没有会创建，有则删除并创建
	@param [in] widgetId widgetID号
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