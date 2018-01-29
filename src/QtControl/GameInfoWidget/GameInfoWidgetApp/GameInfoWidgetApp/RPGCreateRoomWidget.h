#pragma once
#include <QWidget>
#include <QUrl>
#include <stdint.h>
#include "ExternalControls/ComboBox.h"
#include "../CW3LModalFrame.h"

class QEvent;
class QObject;
class Label;
class CWebViewEx;
class LineEdit;
class CheckBox;
class COriginalButton;
class CPasswordInputBox;
class RPGCreateRoomWidget : public CW3LModalFrame
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	RPGCreateRoomWidget(QWidget* parent = NULL);

	/** 析构函数
	*/
	virtual ~RPGCreateRoomWidget();

	/** 设置地图版本
	@param [in] mapVersion 地图版本
	*/
	void setMapVersion(const QString& mapVersion);

	/** 获取地图版本
	@return 返回地图版本
	*/
	QString getMapVersion();

	/** 设置地图版本是否可用
	@param [in] enable 是否可用
	*/
	void setMapVersionEnable(bool enable);

	/** 设置游戏名称
	@param [in] gameName 游戏名称
	*/
	void setGameName(const QString& gameName);

	/** 获取游戏名称
	@return 返回地图版本
	*/
	QString getGameName();

	/** 设置游戏名称是否可用
	@param [in] enable 是否可用
	*/
	void setGameNameEnable(bool enable);

	/** 设置游戏局密码
	@param [in] gamePassword 游戏局密码
	*/
	void setGamePassword(const QString& gamePassword);

	/** 获取游戏局密码
	@return 返回游戏局密码
	*/
	QString getGamePassword();

	/** 设置游戏局密码是否可用
	@param [in] enable 是否可用
	*/
	void setGamePasswordEnable(bool enable);

	/** 设置游戏局模式
	@param [in] gameModeList 游戏局模式
	*/
	void setGameModeList(const QStringList& gameModeList);

	/** 设置当前游戏局模式
	@param [in] gameMode 游戏局模式
	*/
	void setCurGameMode(const QString& gameMode);

	/** 获取当前选择的游戏局模式
	@return 返回当前选择的游戏局模式
	*/
	QString getCurGameMode();

	/** 设置游戏局模式是否可用
	@param [in] enable 是否可用
	*/
	void setGameModeEnable(bool enable);

	/** 设置是否开启裁判位
	@param [in] judge 是否开启裁判位
	*/
	void setJudge(bool judge);

	/** 获取当前是否开启了裁判位
	@return 返回当前是否开启了裁判位
	*/
	bool getJudge();

	/** 设置裁判位是否可用
	@param [in] enable 是否可用
	*/
	void setJudgeEnable(bool enable);

	/** 设置保存是否可用
	@param [in] enable 是否可用
	*/
	void setSaveEnable(bool enable);

	/** 重置页面中所有控件
	*/
	void resetSettings();

	/** 点击游戏设置界面
	*/
	void clickGameSetting();

private:
	/** 默认数据初始化
	*/
	void init();

	/** 设置是否为主机，主机显示开始游戏，非主机显示准备游戏
	@param [in] isLeader 是否为主机
	*/
	void setLeader(bool isLeader);

	void initGameSettingButton();

	void initGameSettingWidget();

	//如果字符串传空则表示不设置正则表达式
	void setComboBoxAttri(ComboBox* pBox,
						  const std::wstring& pattern = L"",
						  QRegExp* ex = nullptr,
						  QRegExpValidator* rep = nullptr);

	void resizeEvent(QResizeEvent* eve);

	void paintEvent(QPaintEvent* eve);
	//所有控件统一布局
	void layout();

private slots:
	void onExitClicked();
	void onJudgeChanged(int state);
	
Q_SIGNALS:
	void exitClicked();
	void createRoomClicked();
	void gameNameChanged(const QString &);
	void gamePasswordChanged(const QString &);
	void gameModeChanged(const QString&);
	void judgeChanged(bool);
	
private:
	Label* m_gameSetting;

	QWidget* m_gameSettingWidget;

	LineEdit* m_gameNameEdit;
	LineEdit* m_mapVersionEdit;
	CPasswordInputBox* m_gamePasswordEdit;
	ComboBox* m_gameModeComboBox;
	CheckBox* m_judgeCheckBox;
	Label* m_judgeCheckName;

	COriginalButton* m_exit;

	Label* m_mapVersion;
	Label* m_gameName;
	Label* m_gamePassword;
	Label* m_gameMode;
	COriginalButton* m_createRoom;

	//窗口整体高度
	int32_t m_rpgCreateRoomWidgetHeight;

	//按钮对应的widget高度
	int32_t m_widgetHeight;

	//游戏设置网页起始高度
	int32_t m_gameSettingWebviewOrigin;

	//邀请好友按钮起始高度
	int32_t m_inviteFriendOrigin;

	// 开始游戏按钮纵轴起始位置
	int32_t m_startGameOrigin_y;

	//退出按钮纵轴起始位置
	int32_t m_exitOrigin_y;

	QString m_war3ResourcePath;
	bool m_isLeader;
};
