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

	/** 设置名将Lv限制
	@param [in] gameLvList 名将Lv限制
	*/
	void setGameLvList(const QStringList& gameLvList);

	/** 设置当前名将Lv限制
	@param [in] gameLv 名将Lv限制
	*/
	void setCurGameLv(const QString& gameLv);

	/** 获取当前选择的名将Lv限制
	@return 返回当前选择的名将Lv限制
	*/
	QString getCurGameLv();

	/** 设置MVP限制
	@param [in] gameMVPList MVP限制
	*/
	void setGameMVPList(const QStringList& gameMVPList);

	/** 添加一条MVP限制
	@param [in] gameMVP MVP限制
	*/
	void setCurGameMVP(const QString& gameMVP);

	/** 获取当前选择的MVP限制
	@return 返回当前选择的MVP限制
	*/
	QString getCurGameMVP();

	/** 设置网速限制
	@param [in] gameNetList 网速限制
	*/
	void setGameNetList(const QStringList& gameNetList);

	/** 设置当前网速限制
	@param [in] gameNet 网速限制
	*/
	void setCurGameNet(const QString& gameNet);

	/** 获取当前选择的网速限制
	@return 返回当前选择的网速限制
	*/
	QString getCurGameNet();

	/** 设置掉线率限制
	@param [in] gameLeaveList 掉线率限制
	*/
	void setGameLeaveList(const QStringList& gameLeaveList);

	/** 获取当前掉线率限制
	@param [in] gameLeave 掉线率限制
	*/
	void setCurGameLeave(const QString& gameLeave);

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

	public:
	//窗口整体高度
	int32_t m_gameInfoWidgetHeight;

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
};