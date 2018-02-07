#pragma once
#include <QWidget>
#include <QUrl>
#include <stdint.h>
#include "ExternalControls/ComboBox.h"

class QEvent;
class QObject;
class Label;
class CWebViewEx;
class LineEdit;
class CheckBox;
class COriginalButton;
class CPasswordInputBox;
class GameInfoWidget : public QWidget
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	GameInfoWidget(QWidget* parent = NULL);

	/** 析构函数
	*/
	virtual ~GameInfoWidget();

	/** 设置游戏局名称
	@param [in] gameName 游戏局名称
	*/
	void setGameName(const QString& gameName);

	/** 获取游戏局名称
	@return 返回游戏局名称
	*/
	QString getGameName();

	/** 设置游戏局名称是否可用
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

	/** 设置名将Lv限制是否可用
	@param [in] enable 是否可用
	*/
	void setGameLvEnable(bool enable);

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

	/** 设置MVP限制是否可用
	@param [in] enable 是否可用
	*/
	void setGameMVPEnable(bool enable);

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

	/** 设置网速限制是否可用
	@param [in] enable 是否可用
	*/
	void setGameNetEnable(bool enable);

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

	/** 设置掉线率限制是否可用
	@param [in] enable 是否可用
	*/
	void setGameLeaveEnable(bool enable);

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

	/** 设置裁判位是否显示
	@param [in] visible 是否显示
	*/
	void setJudgeVisible(bool visible);

	/** 设置保存是否可用
	@param [in] enable 是否可用
	*/
	void setSaveEnable(bool enable);

	/** 设置邀请好友是否可用
	@param [in] enable 是否可用
	*/
	void setInviteFriendEnable(bool enable);

	/** 邀请好友是否可用
	@return 返回邀请好友是否可用
	*/
	bool isInviteFriendEnable();

	/** 设置开始游戏是否可用
	@param [in] enable 是否可用
	*/
	void setStartGameEnable(bool enable);

	/** 开始游戏是否可用
	@return 返回开始游戏是否可用
	*/
	bool isStartGameEnable();

	/** 设置准备游戏是否可用
	@param [in] enable 是否可用
	*/
	void setPrepareGameEnable(bool enable);

	/** 设置准备游戏是否为按下状态
	@param [in] check 是否为按下状态
	*/
	void setPrepareGameCheck(bool check);

	/** 准备游戏是否可用
	@return 返回准备游戏是否可用
	*/
	bool isPrepareGameEnable();

	/** 设置退出是否可用
	@param [in] enable 是否可用
	*/
	void setExitEnable(bool enable);

	/** 退出是否可用
	@return 返回退出是否可用
	*/
	bool isExitEnable();

	/** 设置游戏设置界面网页内容
	@param [in] web 网址
	*/
	void setGameSettingWebView(const QString& web);

	/** 设置个人战绩界面网页内容
	@param [in] web 网址
	*/
	void setPersonalRecordWebView(const QString& web);

	/** 设置我的道具界面网页内容
	@param [in] web 网址
	*/
	void setMyToolWebView(const QString& web);

	/** 重置页面中所有控件
	*/
	void resetSettings();

	/** 设置是否为主机，主机显示开始游戏，非主机显示准备游戏
	@param [in] isLeader 是否为主机
	*/
	void setLeader(bool isLeader);

	/** 点击游戏设置界面
	*/
	void clickGameSetting();

	/** 点击个人战绩界面
	*/
	void clickPersonalRecord();

	/** 点击我的道具界面
	*/
	void clickMyTool();

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

	//如果字符串传空则表示不设置正则表达式
	void setComboBoxAttri(ComboBox* pBox,
						  const std::wstring& pattern = L"",
						  QRegExp* ex = nullptr,
						  QRegExpValidator* rep = nullptr);

	void comboBoxTextChanged(const QString& text,
							 ComboBox* pBox,
							 const QRegExp& exp,
							 const std::wstring& addString,
							 std::wstring& curText);

	void comboBoxEditFinish(ComboBox* pBox, const std::wstring& addString, std::wstring& curText, bool& finish);

	void comboBoxPress(QObject* target,
					   ComboBox* pBox,
					   const std::wstring& addString,
					   std::wstring& curText,
					   bool& finish);

	void comboBoxFocusOut(QObject* target,
						  ComboBox* pBox,
						  const std::wstring& addString,
						  std::wstring& curText,
						  bool& finish);

	void resizeEvent(QResizeEvent* eve);
	//所有控件统一布局
	void layout();

	bool eventFilter(QObject* target, QEvent* eve);

private slots:
	void onGameSettingClicked();
	void onPersonalRecordClicked();
	void onMyToolClicked();
	void onGameSettingUrlLinkClicked(const QUrl& url);
	void onPersonalRecordUrlLinkClicked(const QUrl& url);
	void onMyToolUrlLinkClicked(const QUrl& url);
	void onPrepareGameClicked();
	void onLvEditTextFinish();
	void onMVPEditTextFinish();
	void onNetEditTextFinish();
	void onLeaveEditTextFinish();
	void onGameLvTextChanged(const QString& text);
	void onGameMVPTextChanged(const QString& text);
	void onGameNetTextChanged(const QString& text);
	void onGameLeaveTextChanged(const QString& text);

Q_SIGNALS:
	void onSaveClicked();
	void onInviteFriendClicked();
	void onStartGameClicked();
	void prepareGameClicked();
	void cancelPrepareGameClicked();
	void onExitClicked();
	void onGameNameChanged(const QString &);
	void onGamePasswordChanged(const QString &);
	void onGameModeChanged(const QString&);
	void onGameLvChanged(const QString &);
	void onGameMVPChanged(const QString &);
	void onGameNetChanged(const QString &);
	void onGameLeaveChanged(const QString &);
	void onJudgeChanged(int);
	void onGameSettingLinkClicked(const QString&);
	void onPersonalRecordLinkClicked(const QString&);
	void onMyToolLinkClicked(const QString&);
	
private:
	COriginalButton* m_gameSetting;
	COriginalButton* m_personalRecord;
	COriginalButton* m_myTool;

	QWidget* m_gameSettingWidget;
	QWidget* m_personalRecordWidget;
	QWidget* m_myToolWidget;

	LineEdit* m_gameNameEdit;
	CPasswordInputBox* m_gamePasswordEdit;
	ComboBox* m_gameModeComboBox;
	ComboBox* m_gameLvComboBox;
	ComboBox* m_gameMVPComboBox;
	ComboBox* m_gameNetComboBox;
	ComboBox* m_gameLeaveComboBox;
	CheckBox* m_judgeCheckBox;

	COriginalButton* m_inviteFriend;
	COriginalButton* m_startGame;
	COriginalButton* m_prepareGame;
	COriginalButton* m_exit;

	CWebViewEx* m_myToolWebView;
	CWebViewEx* m_personalRecordWebView;
	CWebViewEx* m_gameSettingWebView;

	Label* m_gameName;
	Label* m_gamePassword;
	Label* m_gameMode;
	Label* m_gameLv;
	Label* m_gameMVP;
	Label* m_gameNet;
	Label* m_gameLeave;
	COriginalButton* m_save;

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

	QRegExpValidator m_lvRep;
	QRegExpValidator m_MVPRep;
	QRegExpValidator m_netRep;
	QRegExpValidator m_leaveRep;

	QRegExp m_lvExp;
	QRegExp m_MVPExp;
	QRegExp m_netExp;
	QRegExp m_leaveExp;

	std::wstring m_gameLvCurText;
	std::wstring m_gameMVPCurText;
	std::wstring m_gameNetCurText;
	std::wstring m_gameLeaveCurText;

	QString m_war3ResourcePath;
	bool m_isLeader;

	bool m_lvFinish;
	bool m_MVPFinish;
	bool m_netFinish;
	bool m_leaveFinish;
};
