#pragma once

/** 字条字体颜色
*/
#define LABEL_TEXT_COLOR QColor(145, 169, 226)

/** 右侧控件边框颜色
*/
#define CONTROL_BORDER_COLOR QColor(67, 81, 117, 255)

/** 右侧控件文字颜色
*/
#define CONTROL_TEXT_COLOR QColor(233, 234, 237, 255)

/** 保存按钮边框颜色
*/
#define SAVE_BORDER_COLOR QColor(84, 97, 138, 255)

/** 字体
*/
#define GAME_INFO_FONT_FACE QString::fromStdWString(L"微软雅黑")

enum
{
	/** 整体窗口宽度
	*/
	GAME_INFO_WIDGET_WIDTH = 254,

	/** 整体窗口高度
	*/
	GAME_INFO_WIDGET_HEIGHT = 681,

	/** 上面3个按钮的高度
	*/
	WIDGET_BUTTON_HEIGHT = 29,

	/** 每个按钮对应widget的宽度
	*/
	WIDGET_WIDTH = GAME_INFO_WIDGET_WIDTH,

	/** 每个按钮对应widget的高度
	*/
	WIDGET_HEIGHT = 550,

	/** 每个字条的宽度
	*/
	LABEL_WIDTH = 88,

	/** 每个字条的高度
	*/
	LABEL_HEIGHT = 22,

	/** 第一个字条的起始高度
	*/
	FIRST_LABEL_BEGIN_HEIGHT = 15,

	/** 字条和右侧控件的间隔宽度
	*/
	LABEL_SPACING = 5,

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

	/** 控件间隔高度
	*/
	CONTROL_SPACING_HEIGHT = 12,

	/** 控件和间隔总高度
	*/
	CONTROL_ALL_SPACING = CONTROL_HEIGHT + CONTROL_SPACING_HEIGHT,

	/** 开启裁判位横轴起始位置
	*/
	JUDGE_ORIGIN_X = 16,

	/** 保存按钮横轴起始位置
	*/
	SAVE_ORIGIN_X = 77,

	/** 保存按钮纵轴起始位置
	*/
	SAVE_ORIGIN_Y = 319,

	/** 保存按钮宽度
	*/
	SAVE_WIDTH = 100,

	/** 保存按钮高度
	*/
	SAVE_HEIGHT = 28,

	/** 游戏设置网页宽度
	*/
	GAME_SETTING_WEBVIEW_WIDTH = WIDGET_WIDTH,

	/** 游戏设置网页高度
	*/
	GAME_SETTING_WEBVIEW_HEIGHT = 170,

	/** 游戏设置网页起始高度
	*/
	GAME_SETTING_WEBVIEW_ORIGIN = WIDGET_HEIGHT - GAME_SETTING_WEBVIEW_HEIGHT,

	/** 邀请好友按钮起始高度
	*/
	INVITE_FRIEND_ORIGIN = WIDGET_BUTTON_HEIGHT + WIDGET_HEIGHT,

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
	EXIT_WIDTH = 58,

	/** 退出按钮高度
	*/
	EXIT_HEIGHT = START_GAME_HEIGHT,

	/** 按钮之前的间隔宽度
	*/
	BUTTONS_SPACING = 6,

	/** 开始游戏按钮横轴起始位置
	*/
	START_GAME_ORIGIN_X = (WIDGET_WIDTH - START_GAME_WIDTH - EXIT_WIDTH - BUTTONS_SPACING) / 2,

	/** 开始游戏按钮纵轴起始位置
	*/
	START_GAME_ORIGIN_Y = GAME_INFO_WIDGET_HEIGHT - (GAME_INFO_WIDGET_HEIGHT - WIDGET_HEIGHT - WIDGET_BUTTON_HEIGHT - INVITE_FRIEND_HEIGHT - START_GAME_HEIGHT) / 2 - START_GAME_HEIGHT,

	/** 退出按钮横轴起始位置
	*/
	EXIT_ORIGIN_X = START_GAME_ORIGIN_X + BUTTONS_SPACING + START_GAME_WIDTH,

	/** 退出按钮纵轴起始位置
	*/
	EXIT_ORIGIN_Y = START_GAME_ORIGIN_Y,

	/** 字体大小
	*/
	GAME_INFO_FONT_SIZE = 14,

	/** 右侧控件文字偏移量
	*/
	CONTROL_TEXT_ORIGIN = 4,

	/** 游戏模式点击框文字偏移量
	*/
	GAME_MODE_TEXT_ORIGIN = 7,
};