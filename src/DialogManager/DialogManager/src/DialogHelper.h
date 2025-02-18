#pragma once
#include <QString>
#include <QRect>
#include <QColor>
#include <map>
#include "DialogType.h"
#include "DialogManagerMacro.h"
#include <fstream>

#define setPopButtonConfig(button, buttonText, textColor, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, textColor, CGeneralStyle::instance()->platformResourcePath() + "res/Dialog/PopupButton.png", acceptDone, fontSize, &m_mapResult, 4, true)
#define setNotifyButtonConfig(button, buttonText, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, QColor(255, 255, 255, 255), "", acceptDone, fontSize, &m_mapResult, 0, false)

class Label;
class COriginalButton;
class LineEdit;
class CPasswordInputBox;
class DialogShow;
class QWindow;
class Separator;
/** 帮助窗口创建控件
*/
class DialogManagerAPI DialogHelper
{
public:
    /** 新增标签文字并设置
    @param [in] label 控件指针
    @param [in] text 文本内容
    @param [in] textColor 字体颜色
    @param [in] fontSize 字体大小
    @return 返回标签指针
    */
    static void setLabel(Label* label, const QString& text, const QColor& textColor, qint32 fontSize);

    /** 新增提示文字并设置（多行）
    @param [in] tip 控件指针
    @param [in] text 文本内容
    @param [in] textColor 字体颜色
    @param [in] fontSize 字体大小
    */
    static void setTip(Label* tip, const QString& text, const QColor& textColor, qint32 fontSize);

    /** 新增按钮并设置，按钮点击后会关闭窗口
    @param [in] dialog 窗口指针
    @param [in] button 按钮指针
    @param [in] text 按钮文本内容
    @param [in] textColor 按钮文字颜色
    @param [in] backgroundImage 按钮背景图片路径
    @param [in] result 按下后的返回值
    @param [in] fontSize 字体大小
    @param [in] mapResult 按钮返回值的存储容器
    @param [in] imageMargin 图片起始位置
    @param [in] isPop 是否是弹窗的按钮
    */
    static void setButton(DialogShow* dialog,
                          COriginalButton* button,
                          const QString& text,
                          const QColor& textColor,
                          const QString& backgroundImage,
                          DialogResult result,
                          qint32 fontSize,
                          std::map<QWidget*, DialogResult>* mapResult,
                          qint32 imageMargin,
                          bool isPop);

    /** 添加分割线
    @param [in] separator 分割线控件指针
    @param [in] isHorizon 是否水平
    @param [in] upColor 上层颜色
    @param [in] downColor 下层颜色
    */
    static void setSeparator(Separator* separator, bool isHorizon, const QColor& upColor, const QColor& downColor);

    /** 添加单行文本框
    @param [in] lineEdit 单行文本框指针
    @param [in] defaultText 默认输入内容
    @param [in] maxLength 文本框可输入的最大长度
    */
    static void setLineEdit(LineEdit* lineEdit, const QString& defaultText = "", qint32 maxLength = -1);

    /** 添加密码框
    @param [in] passwordInputBox 密码框指针
    @param [in] defaultText 默认输入内容
    @param [in] maxLength 文本框可输入的最大长度
    */
    static void setPasswordInputBox(CPasswordInputBox* passwordInputBox, const QString& defaultText = "", qint32 maxLength = -1);

    /** 激活临时父窗口
    @param [in] 需要激活窗口的子窗口句柄
    */
	static void activeTransientParentWindow(QWindow* window);

	/** 设置日志路径
	@param [in] path 日志路径
	*/
	static void setLogPathFrom11Client(const std::string& path);

	/** 设置日志路径
	@param [in] path 日志路径
	*/
	static void setLogPathFromWar3(const std::string& path);

	/** 获取日志文件
	@return 获取日志文件
	*/
	static std::ofstream& logFile();

	/** 矩形转换，将小于0的点转换为0
	@param [in] rect 原矩形
	@return 返回转换后的矩形
	*/
	static QRect rectValid(const QRect& rect);
};