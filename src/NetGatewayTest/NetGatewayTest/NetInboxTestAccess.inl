// 显式模板实例化取得成员指针，仅测试使用；不改变MSVC私有成员的符号修饰。
template<class Tag, typename Tag::Type Member>
struct NetInboxTestMember
{
public:
    /** 取得测试指定的原成员地址，不复制实现
    @param [in] tag 仅用于选择成员类型
    @return 正式库内成员地址
    */
    friend typename Tag::Type netInboxTestMember(Tag tag)
    {
        (void)tag;
        return Member;
    }
};

template struct NetInboxTestMember<NetInboxTestStart, &NetSenderLocalInbox::start>;
template struct NetInboxTestMember<NetInboxTestAllow, &NetSenderLocalInbox::allowGeneration>;
template struct NetInboxTestMember<NetInboxTestRemove, &NetSenderLocalInbox::removeGeneration>;
template struct NetInboxTestMember<NetInboxTestTake, &NetSenderLocalInbox::take>;
template struct NetInboxTestMember<NetInboxTestState, &NetSenderLocalInbox::m_state>;