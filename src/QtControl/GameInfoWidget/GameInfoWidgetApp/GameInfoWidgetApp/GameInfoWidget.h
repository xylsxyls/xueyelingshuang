#pragma once
#include <QWidget>
#include <QUrl>
#include <QtWebKitWidgets/QWebView>
#include <stdint.h>

class Label;
class QWebView;
class LineEdit;
class CheckBox;
class ComboBox;
class COriginalButton2;
class CPasswordInputBox2;
class GameInfoWidget : public QWidget
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	GameInfoWidget(QWidget* parent = NULL);

	virtual ~GameInfoWidget(){};

	/** 设置游戏局名称
	@param [in] gameName 游戏局名称
	*/
	void setGameName(const QString& gameName);

	/** 获取游戏局名称
	@return 返回游戏局名称
	*/
	QString getGameName();

	/** 设置游戏局密码
	@param [in] gamePassword 游戏局密码
	*/
	void setGamePassword(const QString& gamePassword);

	/** 获取游戏局密码
	@return 返回游戏局密码
	*/
	QString getGamePassword();

	/** 添加一条游戏局模式
	@param [in] gameMode 游戏局模式
	*/
	void addGameMode(const QString& gameMode);

	/** 获取当前选择的游戏局模式
	@return 返回当前选择的游戏局模式
	*/
	QString getCurGameMode();

	/** 添加一条名将Lv限制
	@param [in] gameLv 名将Lv限制
	*/
	void addGameLv(const QString& gameLv);

	/** 获取当前选择的名将Lv限制
	@return 返回当前选择的名将Lv限制
	*/
	QString getCurGameLv();

	/** 添加一条MVP限制
	@param [in] gameMVP MVP限制
	*/
	void addGameMVP(const QString& gameMVP);

	/** 获取当前选择的MVP限制
	@return 返回当前选择的MVP限制
	*/
	QString getCurGameMVP();

	/** 添加一条网速限制
	@param [in] gameNet 网速限制
	*/
	void addGameNet(const QString& gameNet);

	/** 获取当前选择的网速限制
	@return 返回当前选择的网速限制
	*/
	QString getCurGameNet();

	/** 添加一条掉线率限制
	@param [in] gameLeave 掉线率限制
	*/
	void addGameLeave(const QString& gameLeave);

	/** 获取当前选择的掉线率限制
	@return 返回当前选择的掉线率限制
	*/
	QString getCurGameLeave();

	/** 设置是否开启裁判位
	@param [in] judge 是否开启裁判位
	*/
	void setJudge(bool judge);

	/** 获取当前是否开启了裁判位
	@return 返回当前是否开启了裁判位
	*/
	bool getJudge();
	
private:
	/** 默认数据初始化
	*/
	void init();

	void initGameSettingButton();
	void initPersonalRecordButton();
	void initMyToolButton();

	void initGameSettingWidget();
	void initPersonalRecordWidget();
	void initMyToolButtonWidget();

	void resizeEvent(QResizeEvent* eve);
	//所有控件统一布局
	void layout();

private slots:
	void onGameSettingClicked();
	void onPersonalRecordClicked();
	void onMyToolClicked();

private:
	COriginalButton2* m_gameSetting;
	COriginalButton2* m_personalRecord;
	COriginalButton2* m_myTool;

	QWidget* m_gameSettingWidget;
	QWidget* m_personalRecordWidget;
	QWidget* m_myToolWidget;

	LineEdit* m_gameNameEdit;
	CPasswordInputBox2* m_gamePasswordEdit;
	ComboBox* m_gameModeComboBox;
	ComboBox* m_gameLvComboBox;
	ComboBox* m_gameMVPComboBox;
	ComboBox* m_gameNetComboBox;
	ComboBox* m_gameLeaveComboBox;
	CheckBox* m_judgeCheckBox;

	COriginalButton2* m_inviteFriend;
	COriginalButton2* m_startGame;
	COriginalButton2* m_exit;

	QWebView* m_myToolWebView;
	QWebView* m_personalRecordWebView;
	QWebView* m_gameSettingWebView;

	Label* m_gameName;
	Label* m_gamePassword;
	Label* m_gameMode;
	Label* m_gameLv;
	Label* m_gameMVP;
	Label* m_gameNet;
	Label* m_gameLeave;
	COriginalButton2* m_save;

	int32_t m_gameInfoWidgetHeight;
	int32_t m_widgetHeight;
};