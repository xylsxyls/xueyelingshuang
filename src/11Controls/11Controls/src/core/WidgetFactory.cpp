#include "WidgetFactory.h"

WidgetFactory::WidgetFactory()
{

}

WidgetFactory& WidgetFactory::instance()
{
	static WidgetFactory s_widgetFactory;
	return s_widgetFactory;
}

void WidgetFactory::onWidgetDestroyed()
{
	QWidget* widget = qobject_cast<QWidget*>(sender());
	if (widget == nullptr)
	{
		return;
	}
	for (auto itWidget = m_widget.begin(); itWidget != m_widget.end(); ++itWidget)
	{
		if (itWidget->second == widget)
		{
			m_widget.erase(itWidget);
			return;
		}
	}
}