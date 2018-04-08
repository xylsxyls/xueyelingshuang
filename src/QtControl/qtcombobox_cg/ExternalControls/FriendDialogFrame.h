#pragma once
#include "../CW3LModalFrame.h"
#include <list>
#include <stdint.h>

class TreeWidget;
class COriginalButton;
class QTreeWidgetItem;

/** �����б��
*/
class FriendDialogFrame : public CW3LModalFrame
{
	Q_OBJECT
public:
	/** �û���Ϣ�ṹ
	*/
	struct User
	{
		//�û�ID
		int32_t m_userId;
		//ͷ��ͼƬ·��
		QString m_userPicPath;
		//�û���
		QString m_userName;
		//�Ƿ���VIP
		bool m_isVip;
		//�û���ǰ״̬�����ߣ��뿪���������ߣ��������룬Ԥ��ֵ����ʱδ�õ�
		int32_t m_userState;
	};

	/** ����Ϣ�ṹ
	*/
	struct Group
	{
		//��ID
		int32_t m_groupId;
		//����
		QString m_groupName;
		//�����û��б�
		std::list<User> m_userList;
	};
	
public:
	/** ���캯��
	@param [in] parent ������ָ��
	*/
	FriendDialogFrame(QWindow* parent = nullptr);

	/** ��������
	*/
	void popUp();

	/** ��ʼ������
	*/
	//void initUIData();

	/** ���һ����
	@param [in] groupList ����Ϣ
	*/
	void addGroupList(const Group& groupList);

	/** ���
	*/
	void clear();

	/** �����û���Ϣ��������û��������򲻻��иĶ�
	@param [in] groupId ��ID
	@param [in] userInfo �û���Ϣ
	*/
	void updateUserInfo(int32_t groupId, const FriendDialogFrame::User& userInfo);

	/** ����û���Ϣ���������𲻴����򲻻��иĶ�
	@param [in] groupId ��ID
	@param [in] userInfo �û���Ϣ
	*/
	void addUserInfo(int32_t groupId, const FriendDialogFrame::User& userInfo);

	/** �Ƴ��û���������ɾ����
	@param [in] userId �û�ID���û�IDΪΨһֵ����ͬ��ҲΨһ
	*/
	void removeUser(int32_t userId);

	/** �Ƴ���
	@param [in] groupId ��ID
	*/
	void removeGroup(int32_t groupId);

	/** ���ö����ĸ�����
	@param [in] parent ������ָ��
	*/
	void setParentWindow(QWindow* parent);

Q_SIGNALS:
	/** ���밴ť����������ź�
	@param [in] groupId ��ID
	@param [in] userId �û�ID
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
	//void onFriendInsert(const YY::W3L::FriendData& data);
	//void onFriendUpdate(const YY::W3L::FriendData& data);
	void onFriendRemove(int32_t userId);

private:
	TreeWidget* m_friendTree;
	bool m_highLight;
	COriginalButton* m_exit;
	COriginalButton* m_bottomExit;
	std::map<int32_t, QWidget*> m_groupData;
	std::map<int32_t, std::map<int32_t, QWidget*>> m_userData;
};