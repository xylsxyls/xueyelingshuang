#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"
#include <QStringList>
#include <QColor>
#include <QPainterPath>
#include <QSize>
#include <atomic>
#include <string>
#include <stdint.h>

/** 播放器进程配置，main最先初始化，所有窗口共享默认参数
*/
class Config
{
public:
	/** 获取进程配置实例
	@return 配置单例引用
	*/
	static Config& instance();

	/** 解析启动参数并初始化默认Core配置
	@param [in] arguments QApplication提供的Unicode参数列表，首项为程序名
	*/
	void init(const QStringList& arguments);

	/** 清理配置运行状态
	*/
	void uninit();

    /** 按共享颜色及留白生成菜单样式，不在绘制处散落常量
    @return Qt菜单样式字符串
    */
    QString menuStyleSheet() const;

private:
	/** 构造配置默认值
	*/
	Config();

	/** 禁止复制进程配置
	@param [in] other 其他配置对象
	*/
	Config(const Config& other);

	/** 禁止覆盖配置实例
	@param [in] other 其他配置对象
	@return 当前对象引用
	*/
	Config& operator=(const Config& other);

public:
    // 窗口标题
    QString m_windowTitle;
    // 加载按钮和文件对话框标题
    QString m_loadText;
    // 文件选择过滤器
    QString m_videoFileFilter;
    // 设置A点菜单文案
    QString m_setAText;
    // 设置B点菜单文案
    QString m_setBText;
    // 清除AB菜单文案
    QString m_clearABText;
    // A点标记文字
    QString m_aLabel;
    // B点标记文字
    QString m_bLabel;
    // 弹窗关闭文字
    QString m_closeText;
    // 弹窗确认文字
    QString m_confirmText;
    // 时间标签宽度测量样例
    QString m_timeMeasureText;
    // 缩放百分比格式
    std::string m_zoomTextFormat;
    // 整秒时间格式
    std::string m_timeTextFormat;
    // 毫秒时间格式
    std::string m_preciseTimeTextFormat;
    // 菜单颜色格式
    std::string m_rgbaFormat;
    // 菜单样式格式
    std::string m_menuStyleFormat;
    // 空窗口加载按钮相对尺寸除数
    int32_t m_emptySizeDivisor;
    // 空窗口加载按钮最小边长，像素
    int32_t m_emptyMinSize;
    // 空窗口加载按钮最大边长，像素
    int32_t m_emptyMaxSize;
    // 加载圆按钮边框内缩，像素
    int32_t m_emptyBorderInset;
    // 加载加号和圆边框线宽，像素
    double m_emptyStroke;
    // 加载加号半臂长相对按钮边长除数
    int32_t m_emptyArmDivisor;
    // 中心播放提示相对尺寸除数
    int32_t m_centerTipSizeDivisor;
    // 中心播放提示最小边长，像素
    int32_t m_centerTipMinSize;
    // 中心播放提示最大边长，像素
    int32_t m_centerTipMaxSize;
    // 缩放提示宽度，像素
    int32_t m_zoomTipWidth;
    // 缩放提示高度，像素
    int32_t m_zoomTipHeight;
    // 缩放提示圆角，像素
    double m_zoomTipRadius;
    // 缩放提示字号，点
    int32_t m_zoomTipFontSize;
    // GUI呈现刷新间隔，毫秒
    int32_t m_uiRefreshMs;
    // 滚轮缩放步长，百分比
    int32_t m_zoomStep;
    // Ctrl滚轮缩放步长，百分比
    int32_t m_zoomFineStep;
    // Ctrl上下键倍率步长，1000为1倍
    int32_t m_rateStep;
    // 无法取得桌面区域时的备用尺寸
    QSize m_fallbackDesktopSize;
    // 初始窗口占桌面宽高的比例
    double m_initialDesktopFraction;
    // 大视频初始显示比例
    double m_largeMediaInitialScale;
    // AB重叠绘制判定距离，像素
    int32_t m_markerOverlapTolerance;
    // 菜单项上下留白，像素
    int32_t m_menuPaddingVertical;
    // 菜单项左留白，像素
    int32_t m_menuPaddingLeft;
    // 菜单项右留白，像素
    int32_t m_menuPaddingRight;
    // 菜单边框宽度，像素
    int32_t m_menuBorderWidth;
    // 菜单背景
    QColor m_menuBackgroundColor;
    // 菜单边框
    QColor m_menuBorderColor;
    // 菜单选中背景
    QColor m_menuSelectionColor;
    // 界面共享颜色RGBA (235, 235, 235, 230)
    QColor m_timeTextColor;
    // 界面共享颜色RGBA (255, 255, 255, 240)
    QColor m_playbackPositionColor;
    // 界面共享颜色RGBA (245, 245, 245, 220)
    QColor m_iconColor;
    // 界面共享颜色RGBA (255, 255, 255, 235)
    QColor m_zoomTextColor;
    // 界面共享颜色RGBA (255, 255, 255, 38)
    QColor m_buttonHoverColor;
    // 界面共享颜色RGBA (255, 255, 255, 8)
    QColor m_buttonColor;
    // 界面共享颜色RGBA (255, 255, 255, 18)
    QColor m_playButtonColor;
    // 界面共享颜色RGBA (0, 0, 0, 95)
    QColor m_centerTipBackgroundColor;
    // 界面共享颜色RGBA (70, 70, 70, 190)
    QColor m_emptyButtonColor;
    // 界面共享颜色RGBA (95, 95, 95, 210)
    QColor m_emptyHoverColor;
    // 界面共享颜色RGBA (210, 210, 210, 220)
    QColor m_emptyIconColor;
    // 界面共享颜色RGBA (0, 0, 0)
    QColor m_videoBackgroundColor;
    // 界面共享颜色RGBA (255, 255, 255, 55)
    QColor m_overlayBorderColor;
    // 界面共享颜色RGBA (16, 16, 16, 220)
    QColor m_overlayColor;
    // 界面共享颜色RGBA (255, 255, 255, 230)
    QColor m_textColor;
    // 界面共享颜色RGBA (255, 255, 255, 45)
    QColor m_playHoverColor;
    // 界面共享颜色RGBA (255, 255, 255, 70)
    QColor m_trackColor;
    // 界面共享颜色RGBA (255, 255, 255, 175)
    QColor m_centerTipIconColor;
    // 界面共享颜色RGBA (0, 0, 0, 180)
    QColor m_tipBackgroundColor;
    // 标题按钮边长，像素
    int32_t m_titleButtonSize;
    // 标题按钮间距，像素
    int32_t m_titleButtonGap;
    // 标题左右边距，像素
    int32_t m_titleMargin;
    // 标题按钮上边距，像素
    int32_t m_titleButtonTop;
    // 帮助与窗口按钮组的额外间距，像素
    int32_t m_windowButtonGroupGap;
    // 左右时间区最小宽度，像素
    int32_t m_timeWidth;
    // 时间文字到窗口边缘的距离，像素
    int32_t m_timeMargin;
    // 时间区到轨道间距，像素
    int32_t m_timeTrackGap;
    // 最小进度条宽度，像素
    int32_t m_minTrackWidth;
    // 固定工具栏时最小视频高度，像素
    int32_t m_minVideoHeight;
    // 可见轨道高度，像素
    int32_t m_trackHeight;
    // 轨道上下额外命中距离，像素，4+2+2=8
    int32_t m_trackHitExtra;
    // 播放按钮底边距，像素
    int32_t m_playButtonBottom;
    // 时间字体大小，点
    int32_t m_timeFontSize;
    // 默认界面字体大小，点
    int32_t m_fontSize;
    // 界面默认字体
    QString m_fontFamily;
    // AB竖线宽度，像素
    int32_t m_markerWidth;
    // AB悬浮竖线宽度，像素
    int32_t m_markerHoverWidth;
    // AB竖线高度，像素
    int32_t m_markerHeight;
    // AB水平命中半径，像素
    int32_t m_markerHitWidth;
    // AB命中上扩范围，像素
    int32_t m_markerHitTop;
    // AB命中下扩范围，像素
    int32_t m_markerHitBottom;
    // AB字母框宽，像素
    int32_t m_markerLabelWidth;
    // AB字母框高，像素
    int32_t m_markerLabelHeight;
    // AB字母到轨道间距，像素
    int32_t m_markerLabelGap;
    // AB时间戳内边距，像素
    int32_t m_timestampPadding;
    // AB时间戳到轨道间距，像素
    int32_t m_timestampGap;
    // 当前播放位置圆点半径，像素
    int32_t m_positionRadius;
    // 悬浮播放位置圆点半径，像素
    int32_t m_positionDragRadius;
    // 按钮圆角，像素
    double m_cornerRadius;
    // 矢量图标坐标基准边长
    double m_iconCanvasSize;
    // 矢量图标线宽
    double m_iconStroke;
    // 未固定图钉倾角，度
    double m_pinAngle;
    // 帮助弹窗宽度，像素
    int32_t m_helpWidth;
    // 帮助弹窗高度，像素
    int32_t m_helpHeight;
    // 帮助内容边距，像素
    int32_t m_helpMargin;
    // 帮助布局间距，像素
    int32_t m_helpSpacing;
    // 帮助标题字号，点
    int32_t m_helpTitleSize;
    // 确认按钮宽度，像素
    int32_t m_helpButtonWidth;
    // 确认按钮高度，像素
    int32_t m_helpButtonHeight;
    // 主体蓝色，共用于轨道/问号/固定状态
    QColor m_themeColor;
    // A点薄荷绿
    QColor m_loopAColor;
    // B点暖黄色
    QColor m_loopBColor;
    // 禁用图标颜色
    QColor m_disabledColor;
    // 帮助背景颜色
    QColor m_helpBackground;
    // 加载图标路径，坐标以iconCanvasSize为基准
    QPainterPath m_loadIcon;
    // 重置图标路径
    QPainterPath m_resetIcon;
    // 竖直图钉路径
    QPainterPath m_pinIcon;
    // 最小化图标路径
    QPainterPath m_minimizeIcon;
    // 最大化图标路径
    QPainterPath m_maximizeIcon;
    // 还原图标路径
    QPainterPath m_restoreIcon;
    // 关闭图标路径
    QPainterPath m_closeIcon;
    // 播放三角路径
    QPainterPath m_playIcon;
    // 暂停图标路径
    QPainterPath m_pauseIcon;
    // 问号路径
    QPainterPath m_questionIcon;
    // 时间文字相对居中位置的下移像素
    int32_t m_timeTextOffset;
    // 轨道左右额外延长像素
    int32_t m_trackExtension;
    // 普通按钮按下时主题蓝蒙层透明度
    double m_buttonPressOpacity;
    // 圆点与普通按钮共用的淡蓝按下蒙层
    QColor m_buttonPressColor;
    // 两个窗口共用的关闭按钮红色背景
    QColor m_closeButtonColor;
    // 关闭按钮按下色加深百分比
    int32_t m_closePressDarkening;
    // 重置提示
    QString m_resetText;
    // 未固定时图钉提示
    QString m_pinText;
    // 已固定时图钉提示
    QString m_unpinText;
    // 帮助标题栏文字
    QString m_helpTitle;
    // 帮助提示
    QString m_helpTooltip;
    // 帮助独立版本文字
    QString m_versionLabel;
    // 帮助使用说明
    QString m_helpText;
    // 普通左右键跳转秒数，支持小数
    double m_keyboardSeekSeconds;
    // 逻辑队列上限
    int32_t m_logicQueueLimit;
    // 尚未完成Core请求上限
    int32_t m_corePendingLimit;
	// 顶部悬浮条高度，单位像素
	int32_t m_topOverlayHeight;
	// 底部悬浮条高度，单位像素
	int32_t m_bottomOverlayHeight;
	// 底部播放按钮边长，单位像素
	int32_t m_playButtonSize;
	// 进度轨道到下浮框顶部的距离，单位像素
	int32_t m_progressTrackTop;
	// 悬浮条单次移动距离，单位像素
	int32_t m_overlayStep;
	// 鼠标静止隐藏延时，单位毫秒
	int32_t m_mouseIdleHideMs;
	// 窗口边缘缩放判定宽度，单位像素
	int32_t m_resizeMargin;
	// 进度条触发边距，单位像素
	int32_t m_progressTriggerMargin;
	// 顶部悬浮触发边距，单位像素
	int32_t m_topTriggerMargin;
	// 最小窗口宽度，单位像素
	int32_t m_minWindowWidth;
	// 最小窗口高度，单位像素
	int32_t m_minWindowHeight;
	// 最小缩放百分比
	int32_t m_minZoomPercent;
	// 最大缩放百分比
	int32_t m_maxZoomPercent;
	// 拖动预览提交间隔，单位毫秒
	int64_t m_previewDragIntervalMs;
	// 音频设备专用线程的服务间隔，单位毫秒
	int32_t m_audioServiceIntervalMs;
	// 每秒对应的100纳秒刻度数
	int64_t m_second100ns;
	// 每毫秒对应的100纳秒刻度数
	int64_t m_millisecond100ns;
	// 音频排队上限，单位100纳秒
	int64_t m_audioMaxQueue100ns;
	// 命令行是否包含debug诊断标记，基础日志始终开启
	bool m_debugEnabled;
	// 启动时加载的首个视频路径，空字符串表示显示空窗口
	QString m_startupMediaPath;
    // 单个日志文件上限，单位字节
    int64_t m_logMaxFileBytes;
    // 滚动日志保留文件数
    int32_t m_logMaxFileCount;
	// 日志服务是否已经启动，跨线程读取
	std::atomic<bool> m_logInitialized;
	// 注入Core的独立实例配置
	LumaPlayerCoreCConfig m_coreConfig;
};

#define g_config Config::instance()