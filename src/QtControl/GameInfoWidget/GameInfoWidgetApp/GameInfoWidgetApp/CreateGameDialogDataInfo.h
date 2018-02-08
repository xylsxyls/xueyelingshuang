#pragma once

/** 空指针检查
*/
#define SAFE(p,method) if(p != nullptr){method;}

/** 字条字体颜色
*/
#define LABEL_TEXT_COLOR QColor(171, 179, 211)

/** 右侧控件文字颜色
*/
#define CONTROL_TEXT_COLOR QColor(233, 234, 237, 255)

/** 右侧控件禁用时文字颜色
*/
#define CONTROL_DISABLED_TEXT_COLOR QColor(82, 126, 185, 255)

/** 创建房间字条颜色
*/
#define CREATE_GAME_COLOR QColor(183, 187, 204, 255)

/** 创建房间按钮常态颜色
*/
#define CREATE_ROOM_NORMAL_COLOR QColor("#53be1e")

/** 创建房间按钮悬停颜色
*/
#define CREATE_ROOM_HOVER_COLOR QColor("#1da403")

/** 右侧控件边框颜色
*/
#define CONTROL_BORDER_COLOR QColor(67, 81, 117, 255)

/** 控件背景颜色
*/
#define CONTROL_BACKGROUND_COLOR QColor(39, 50, 83, 255)

/** 字体
*/
#define GAME_INFO_FONT_FACE CGeneralStyle::instance()->font().family()

/** 下拉框常态颜色
*/
#define LIST_NORMAL_COLOR CONTROL_BACKGROUND_COLOR

/** 下拉框hover颜色
*/
#define LIST_HOVER_COLOR QColor(80, 98, 146, 255)

/** 分割线颜色
*/
#define SEPARATOR_COLOR QColor(39, 50, 83, 255)

enum
{
	/** 整体窗口宽度
	*/
	GAME_INFO_WIDGET_WIDTH = 255,

	/** 上面3个按钮的高度
	*/
	WIDGET_BUTTON_HEIGHT = 29,

	/** widget起始位置
	*/
	WIDGET_ORIGIN = 0,

	/** widget起始位置
	*/
	WIDGET_ORIGIN_Y = 186,

	/** 每个按钮对应widget的宽度
	*/
	WIDGET_WIDTH = GAME_INFO_WIDGET_WIDTH,

	/** widget高度
	*/
	WIDGET_HEIGHT = 98,

	/** 每个字条的宽度
	*/
	LABEL_WIDTH = 77,

	/** 每个字条的高度
	*/
	LABEL_HEIGHT = 22,

	/** 第一个字条的起始高度
	*/
	FIRST_LABEL_BEGIN_HEIGHT = 52,

	/** 字条和右侧控件的间隔宽度
	*/
	LABEL_SPACING = 10,

	/** 右侧控件横轴起始位置
	*/
	CONTROL_BEGIN_ORIGIN_X = LABEL_WIDTH + LABEL_SPACING,

	/** 右侧控件纵轴起始位置
	*/
	CONTROL_BEGIN_ORIGIN_Y = FIRST_LABEL_BEGIN_HEIGHT,

	/** 右侧控件宽度
	*/
	CONTROL_WIDTH = 147,

	/** 右侧控件高度
	*/
	CONTROL_HEIGHT = LABEL_HEIGHT,

	/** 右侧控件圆角半径
	*/
	CONTROL_RADIUS = 5,

	/** 创建房间圆角半径
	*/
	CREATE_ROOM_RADIUS = 3,

	/** 控件间隔高度
	*/
	CONTROL_SPACING_HEIGHT = 10,

	/** 控件和间隔总高度
	*/
	CONTROL_ALL_SPACING = CONTROL_HEIGHT + CONTROL_SPACING_HEIGHT,

	/** 保存按钮宽度
	*/
	CREATE_ROOM_WIDTH = 164,

	/** 保存按钮高度
	*/
	CREATE_ROOM_HEIGHT = 38,

	/** 保存按钮横轴起始位置
	*/
	CREATE_ROOM_ORIGIN_X = (WIDGET_WIDTH - CREATE_ROOM_WIDTH) / 2,

	/** 保存按钮纵轴起始位置
	*/
	CREATE_ROOM_ORIGIN_Y = 366,

	/** 游戏设置网页宽度
	*/
	GAME_SETTING_WEBVIEW_WIDTH = WIDGET_WIDTH,

	/** 游戏设置网页高度
	*/
	GAME_SETTING_WEBVIEW_HEIGHT = 170,

	/** 创建游戏起始位置
	*/
	CREATE_GAME_ORIGIN_X = 11,

	/** 创建房间字条起始高度
	*/
	CREATE_GAME_ORIGIN_Y = 1,

	/** 创建房间字条宽度
	*/
	CREATE_GAME_WIDTH = 60,

	/** 创建游戏高度
	*/
	CREATE_GAME_HEIGHT = WIDGET_BUTTON_HEIGHT - 6,

	/** 分割线横轴起始位置
	*/
	SEPARATOR_ORIGIN_X = 10,

	/** 分割线横轴起始位置
	*/
	SEPARATOR_ORIGIN_Y = 2,

	/** 分割线横轴起始位置
	*/
	SEPARATOR_WIDTH = 237,

	/** 分割线横轴起始位置
	*/
	SEPARATOR_HEIGHT = 1,

	/** 挑战模式字条起始高度
	*/
	CHALLENGE_MODE_ORIGIN = 20,

	/** 邀请好友按钮宽度
	*/
	INVITE_FRIEND_WIDTH = WIDGET_WIDTH,

	/** 邀请好友按钮高度
	*/
	INVITE_FRIEND_HEIGHT = 34,

	/** 开始游戏按钮宽度
	*/
	START_GAME_WIDTH = 164,

	/** 开始游戏按钮高度
	*/
	START_GAME_HEIGHT = 38,

	/** 退出按钮宽度
	*/
	EXIT_WIDTH = 24,

	/** 退出按钮高度
	*/
	EXIT_HEIGHT = EXIT_WIDTH,

	/** 退出按钮横轴起始位置
	*/
	EXIT_ORIGIN_X = GAME_INFO_WIDGET_WIDTH - EXIT_WIDTH - 11,

	/** 退出按钮纵轴起始位置
	*/
	EXIT_ORIGIN_Y = 5,

	/** 按钮之前的间隔宽度
	*/
	BUTTONS_SPACING = 6,

	/** 开始游戏按钮横轴起始位置
	*/
	START_GAME_ORIGIN_X = (WIDGET_WIDTH - START_GAME_WIDTH - EXIT_WIDTH - BUTTONS_SPACING) / 2,

	/** 字体大小
	*/
	GAME_INFO_FONT_SIZE = 13,

	/** 下拉框字体大小
	*/
	LIST_FONT_SIZE = GAME_INFO_FONT_SIZE,

	/** 右侧控件文字偏移量
	*/
	CONTROL_TEXT_ORIGIN = 4,

	/** 游戏模式点击框文字偏移量
	*/
	GAME_MODE_TEXT_ORIGIN = 7,

	/** 下拉框节点高度
	*/
	LIST_ITEM_HEIGHT = 17,

	/** 下拉框最大高度
	*/
	LIST_MAX_HEIGHT = 145,

	/** 下拉框偏移量
	*/
	LIST_ORIGIN = 2,

	/** 下拉框边框粗度
	*/
	LIST_BORDER_WIDTH = 1,

	/** 下拉框节点边框粗度
	*/
	LIST_ITEM_BORDER_WIDTH = 0,

	/** 下拉箭头宽度
	*/
	DROP_DOWN_WIDTH = 10,

	/** 下拉箭头高度
	*/
	DROP_DOWN_HEIGHT = 5,

	/** 下拉箭头距离右上角横轴距离
	*/
	DROP_DOWN_ORIGIN_X = 7,

	/** 下拉箭头距离右上角纵轴距离
	*/
	DROP_DOWN_ORIGIN_Y = 5,

};