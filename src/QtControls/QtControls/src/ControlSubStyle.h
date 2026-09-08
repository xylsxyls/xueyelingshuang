#pragma once
#include <map>
#include <string>
#include <QColor>
#include "QssString/QssStringAPI.h"
#include "QtControlsMacro.h"

/** QSS写入工具，集中处理控件本体、Qt内置subcontrol和后代控件选择器的样式拼接
*/
class QtControlsAPI ControlSubStyle
{
public:
	/** 写入控件本体QSS键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] key QSS键
	@param [in] value QSS值
	*/
	static void setWidgetKeyValue(QssString* controlStyle,
								  const std::wstring& key,
								  const std::wstring& value);

	/** 写入控件本体像素键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] key QSS键
	@param [in] valuePx 像素值，会自动追加px
	*/
	static void setWidgetPxValue(QssString* controlStyle,
								 const std::wstring& key,
								 qint32 valuePx);

	/** 写入控件本体像素边框键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] key QSS键
	@param [in] valuePx 边框像素值，会生成“像素值px solid”
	*/
	static void setWidgetPxSolidValue(QssString* controlStyle,
									  const std::wstring& key,
									  qint32 valuePx);

	/** 写入控件本体多状态颜色
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] colorStateMap 外层为第一状态，内层为第二状态的颜色表
	@param [in] key QSS键
	*/
	static void setWidgetColorStateMap(QssString* controlStyle,
									   const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
									   const std::wstring& key);

	/** 写入控件本体多状态切片图片
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] imagePath 图片路径，要求Qt可以直接加载
	@param [in] stateCount 图片纵向切片数量
	@param [in] imageStateMap 外层为第一状态，内层为第二状态的图片序号表
	@param [in] key QSS键，通常为border-image或image
	*/
	static void setWidgetImageStateMap(QssString* controlStyle,
									   const std::wstring& imagePath,
									   qint32 stateCount,
									   const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
									   const std::wstring& key);

	/** 写入子控件QSS键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] subControl Qt内置子控件名称，例如up-button、section
	@param [in] key QSS键
	@param [in] value QSS值
	*/
	static void setKeyValue(QssString* controlStyle,
							const std::wstring& subControl,
							const std::wstring& key,
							const std::wstring& value);

	/** 写入子控件像素键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] subControl Qt内置子控件名称
	@param [in] key QSS键
	@param [in] valuePx 像素值，会自动追加px
	*/
	static void setPxValue(QssString* controlStyle,
						   const std::wstring& subControl,
						   const std::wstring& key,
						   qint32 valuePx);

	/** 写入子控件像素边框键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] subControl Qt内置子控件名称
	@param [in] key QSS键
	@param [in] valuePx 边框像素值，会生成“像素值px solid”
	*/
	static void setPxSolidValue(QssString* controlStyle,
								const std::wstring& subControl,
								const std::wstring& key,
								qint32 valuePx);

	/** 写入子控件多状态颜色
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] subControl Qt内置子控件名称
	@param [in] colorStateMap 外层为第一状态，内层为第二状态的颜色表
	@param [in] key QSS键
	*/
	static void setColorStateMap(QssString* controlStyle,
								 const std::wstring& subControl,
								 const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
								 const std::wstring& key);

	/** 写入子控件多状态切片图片
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] subControl Qt内置子控件名称
	@param [in] imagePath 图片路径，要求Qt可以直接加载
	@param [in] stateCount 图片纵向切片数量
	@param [in] imageStateMap 外层为第一状态，内层为第二状态的图片序号表
	@param [in] key QSS键，通常为border-image或image
	*/
	static void setImageStateMap(QssString* controlStyle,
								 const std::wstring& subControl,
								 const std::wstring& imagePath,
								 qint32 stateCount,
								 const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
								 const std::wstring& key);

	/** 写入后代控件QSS键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] descendant 后代控件选择器，例如QHeaderView
	@param [in] key QSS键
	@param [in] value QSS值
	*/
	static void setDescendantKeyValue(QssString* controlStyle,
									  const std::wstring& descendant,
									  const std::wstring& key,
									  const std::wstring& value);

	/** 写入后代控件子控件QSS键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] descendant 后代控件选择器，例如QHeaderView
	@param [in] subControl 后代控件的Qt内置子控件名称，例如section
	@param [in] key QSS键
	@param [in] value QSS值
	*/
	static void setDescendantSubControlKeyValue(QssString* controlStyle,
												const std::wstring& descendant,
												const std::wstring& subControl,
												const std::wstring& key,
												const std::wstring& value);

	/** 写入后代控件子控件像素键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] descendant 后代控件选择器
	@param [in] subControl 后代控件的Qt内置子控件名称
	@param [in] key QSS键
	@param [in] valuePx 像素值，会自动追加px
	*/
	static void setDescendantSubControlPxValue(QssString* controlStyle,
											   const std::wstring& descendant,
											   const std::wstring& subControl,
											   const std::wstring& key,
											   qint32 valuePx);

	/** 写入后代控件子控件像素边框键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] descendant 后代控件选择器
	@param [in] subControl 后代控件的Qt内置子控件名称
	@param [in] key QSS键
	@param [in] valuePx 边框像素值，会生成“像素值px solid”
	*/
	static void setDescendantSubControlPxSolidValue(QssString* controlStyle,
													const std::wstring& descendant,
													const std::wstring& subControl,
													const std::wstring& key,
													qint32 valuePx);

private:
	/** 写入控件本体或子控件的普通QSS键值
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] hasSubControl true表示写入Qt内置子控件选择器
	@param [in] subControl Qt内置子控件名称
	@param [in] key QSS键
	@param [in] value QSS值
	*/
	static void writeKeyValue(QssString* controlStyle,
							  bool hasSubControl,
							  const std::wstring& subControl,
							  const std::wstring& key,
							  const std::wstring& value);

	/** 写入控件本体或子控件的多状态颜色
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] hasSubControl true表示写入Qt内置子控件选择器
	@param [in] subControl Qt内置子控件名称
	@param [in] colorStateMap 外层为第一状态，内层为第二状态的颜色表
	@param [in] key QSS键
	*/
	static void writeColorStateMap(QssString* controlStyle,
								   bool hasSubControl,
								   const std::wstring& subControl,
								   const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
								   const std::wstring& key);

	/** 写入控件本体或子控件的多状态切片图片
	@param [in,out] controlStyle 控件QSS管理对象，传入空指针时不写入
	@param [in] hasSubControl true表示写入Qt内置子控件选择器
	@param [in] subControl Qt内置子控件名称
	@param [in] imagePath 图片路径，要求Qt可以直接加载
	@param [in] stateCount 图片纵向切片数量
	@param [in] imageStateMap 外层为第一状态，内层为第二状态的图片序号表
	@param [in] key QSS键，通常为border-image或image
	*/
	static void writeImageStateMap(QssString* controlStyle,
								   bool hasSubControl,
								   const std::wstring& subControl,
								   const std::wstring& imagePath,
								   qint32 stateCount,
								   const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
								   const std::wstring& key);

private:
	/** 禁止构造工具类对象
	*/
	ControlSubStyle();
};