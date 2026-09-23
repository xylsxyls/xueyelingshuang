#pragma once
#include <QtWidgets/QMainWindow>
#include "QtControlsMacro.h"
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"

/** 主窗口容器，统一使用封装状态栏。
*/
class QtControlsAPI MainWindow :
	public ControlShow<QMainWindow>,
	public ControlSelf<QMainWindow>,
	public ControlFont<QMainWindow>,
	public ControlBorderForNormalHoverDisabled<QMainWindow>,
	public ControlBackgroundForNormalHoverDisabled<QMainWindow>
{
public:
	/** 构造函数。
	@param [in] parent 父控件，接管对象所有权。
	*/
	explicit MainWindow(QWidget* parent = nullptr);
    /** 设置窗口及子控件的附加 QSS，和通用样式接口生成的 QSS 合并。
    @param [in] styleSheet 附加样式文本。
    */
    void setStyleSheet(const QString& styleSheet);
    /** 应用通用样式和附加 QSS，首次显示及再次重画均保留附加样式。 */
    void repaint() override;
private:
    // 产品窗口对子控件提供的附加 QSS，在通用样式重画后继续保留
    QString m_contentStyle;
};
