#pragma once
#include "../CW3LModalFrame.h"
#include <list>
#include <stdint.h>

class TreeWidget;
class COriginalButton;
class QTreeWidgetItem;

/** 好友列表框
*/
class FriendDialog : public CW3LModalFrame
{
	Q_OBJECT
public:
	struct User
	{
		int32_t m_userId;
		QString m_userPicPath;
		QString m_userName;
		int32_t m_userState;
	};

	struct Group
	{
		int32_t m_groupId;
		QString m_groupName;
		std::list<User> m_userList;
	};
	
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	FriendDialog(QWidget* parent = nullptr);

	/** 添加一个组
	@param [in] groupList 组信息
	*/
	void addGroupList(const Group& groupList);

	/** 更新用户信息，如果该用户不存在则不会有改动
	@param [in] groupId 组ID
	@param [in] userInfo 用户信息
	*/
	void updateUserInfo(int32_t groupId, const FriendDialog::User& userInfo);

	/** 添加用户信息，如果该组别不存在则不会有改动
	@param [in] groupId 组ID
	@param [in] userInfo 用户信息
	*/
	void addUserInfo(int32_t groupId, const FriendDialog::User& userInfo);

	/** 移除用户，但不会删除组
	@param [in] groupId 组ID
	@param [in] userId 用户ID
	*/
	void removeUser(int32_t groupId, int32_t userId);

	/** 移除组
	@param [in] groupId 组ID
	*/
	void removeGroup(int32_t groupId);

private:
	bool eventFilter(QObject* obj, QEvent* eve);
	void paintEvent(QPaintEvent* eve);
	bool nativeEvent(const QByteArray& eventType, void* message, long* result);
	void ncActiveChanged(int32_t wParam);
	void setResponseHighlightDialog(QWidget* parent);

private slots:
	void onIndiClicked();
	void onInviteClicked();
	void onItemExpanded(QTreeWidgetItem* item);
	void onItemCollapsed(QTreeWidgetItem* item);

Q_SIGNALS:
	void inviteClicked(int32_t groupId, int32_t userId);

private:
	TreeWidget* m_friendTree;

	QWidget* m_lastHoverWidget;
	bool m_highLight;

private:
	COriginalButton* m_exit;
	COriginalButton* m_bottomExit;

	std::map<int32_t, QWidget*> m_groupData;
	std::map<int32_t, std::map<int32_t, QWidget*>> m_userData;
};