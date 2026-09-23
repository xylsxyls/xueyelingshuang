#pragma once
#include "Config.h"

class PushButton;

/** 使用 QtControls 样式接口应用产品配置，供主界面及测试界面复用。 */
class PdfReaderControlHelper
{
public:
    /** 设置按钮尺寸、四态颜色、圆角与点击行为。
    @param [in] button 待配置的按钮，不能为空。
    @param [in] config 产品样式配置。
    */
    static void configureButton(PushButton* button, const Config& config);
};
