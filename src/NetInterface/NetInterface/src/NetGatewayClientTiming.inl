/** 每次完整交付后计数，用尽预算时让宿主继续推进
@param [in] budget 本次可交付的正数上限
@param [in,out] step 执行一项工作，返回false时已有背压、终态或队列为空
*/
template<class Step>
inline void NetGatewayPollItems(uint32_t budget, const Step& step)
{
    for (uint32_t i = 0; i < budget; ++i)
    {
        if (!step(i)) { return; }
    }
    NetExecution::progress();
}

template<class Step>
inline void NetGatewayClientTiming::readFrames(const Step& step) const
{
    NetGatewayPollItems(m_readFrames, step);
}

template<class Step>
inline void NetGatewayClientTiming::writeRecords(const Step& step) const
{
    NetGatewayPollItems(m_writeRecords, step);
}

inline int32_t NetGatewayClientTiming::configurePollBudgets(uint32_t readFrames, uint32_t writeRecords)
{
    if (readFrames == 0 || writeRecords == 0) { return NI_INVALID_ARGUMENT; }
    m_readFrames = readFrames;
    m_writeRecords = writeRecords;
    return NI_OK;
}

inline int32_t NetGatewayClientTiming::configure(uint32_t startupMs, uint32_t backpressureMs)
{
    if (backpressureMs == 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_startupMs = startupMs;
    m_backpressureMs = backpressureMs;
    return NI_OK;
}

inline int32_t NetGatewayClientTiming::deliver(NetTransportListener& listener, const NetInterfaceFrame& frame, uint64_t now, uint64_t& deadline) const
{
    if (deadline != 0 && now >= deadline)
    {
        return NI_TIMEOUT;
    }
    if (listener.onFrame(1, frame))
    {
        deadline = 0;
        return NI_OK;
    }
    if (deadline == 0)
    {
        deadline = UINT64_MAX - now < m_backpressureMs ? UINT64_MAX : now + m_backpressureMs;
    }
    NetExecution::deadline(deadline, now);
    return NI_LIMIT_EXCEEDED;
}