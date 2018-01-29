#pragma once
#include "../CW3LModalFrame.h"
#include <list>
#include <stdint.h>

class TreeWidget;
class COriginalButton;
class QTreeWidgetItem;

/** 好友列表框
*/
class FriendDialogFrame : public CW3LModalFrame
{
	Q_OBJECT
public:
	/** 用户信息结构
	*/
	struct User
	{
		//用户ID
		int32_t m_userId;
		//头像图片路径
		QString m_userPicPath;
		//用户名
		QString m_userName;
		//是否是VIP
		bool m_isVip;
		//用户当前状态，在线，离开，隐身，下线，正在输入，预留值，暂时未用到
		int32_t m_userState;
	};

	/** 组信息结构
	*/
	struct Group
	{
		//组ID
		int32_t m_groupId;
		//组名
		QString m_groupName;
		//组内用户列表
		std::list<User> m_userList;
	};
	
public:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	FriendDialogFrame(QWindow* parent = nullptr);

	/** 弹出窗口
	*/
	void popUp();

	/** 初始化数据
	*/
	void initUIData();

	/** 添加一个组
	@param [in] groupList 组信息
	*/
	void addGroupList(const Group& groupList);

	/** 清空
	*/
	void clear();

	/** 更新用户信息，如果该用户不存在则不会有改动
	@param [in] groupId 组ID
	@param [in] userInfo 用户信息
	*/
	void updateUserInfo(int32_t groupId, const FriendDialogFrame::User& userInfo);

	/** 添加用户信息，如果该组别不存在则不会有改动
	@param [in] groupId 组ID
	@param [in] userInfo 用户信息
	*/
	void addUserInfo(int32_t groupId, const FriendDialogFrame::User& userInfo);

	/** 移除用户，但不会删除组
	@param [in] userId 用户ID，用户ID为唯一值，不同组也唯一
	*/
	void removeUser(int32_t userId);

	/** 移除组
	@param [in] groupId 组ID
	*/
	void removeGroup(int32_t groupId);

	/** 设置堵塞的父窗口
	@param [in] parent 父窗口指针
	*/
	void setParentWindow(QWindow* parent);

Q_SIGNALS:
	/** 邀请按钮被点击后发送信号
	@param [in] groupId 组ID
	@param [in] userId 用户ID
	*/
	void inviteClicked(int32_t groupId, int32_t userId);

private:
	virtual void paintEvent(QPaintEvent* eve);
	virtual bool nativeEvent(const QByteArray& eventType, void* message, long* result);

private:
	void activeChanged(int32_t wParam);

private slots:
	void onIndiClicked();
	void onInviteClicked();
	void onItemExpanded(QTreeWidgetItem* item);
	void onItemCollapsed(QTreeWidgetItem* item);
	void onFriendInsert(const YY::W3L::FriendData& data);
	void onFriendUpdate(const YY::W3L::FriendData& data);
	void onFriendRemove(int32_t userId);

private:
	TreeWidget* m_friendTree;
	bool m_highLight;
	COriginalButton* m_exit;
	COriginalButton* m_bottomExit;
	std::map<int32_t, QWidget*> m_groupData;
	std::map<int32_t, std::map<int32_t, QWidget*>> m_userData;
};