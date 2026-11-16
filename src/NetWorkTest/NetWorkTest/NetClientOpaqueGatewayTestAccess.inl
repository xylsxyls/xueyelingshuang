// 显式实例化只位于测试翻译单元，不改变生产类声明。
template<class Tag, typename Tag::Type Member>
struct NetOpaqueTestMember
{
public:
    /** 取得测试需要观察的成员，不修改其所有权
    @param [in] tag 类型选择器
    @return 原始成员指针
    */
    friend typename Tag::Type netOpaqueTestMember(Tag tag)
    {
        (void)tag;
        return Member;
    }
};

template struct NetOpaqueTestMember<NetOpaqueTestDiscovery, &NetClientOpaqueGateway::m_discovery>;