#include "NetSubmissionSignalTest.h"
#include <thread>
#include <cstdio>
#include <vector>
#include <stdexcept>
#include "NetGatewayTest.h"

NetSubmissionSignalProbe::NetSubmissionSignalProbe() : m_count(0)
{

}

void NetSubmissionSignalProbe::notify()
{
    m_count.fetch_add(1);
}

bool NetSubmissionSignalTest::check()
{
    std::vector<std::weak_ptr<NetSubmissionSignalProbe>> lifetimes;
    lifetimes.reserve(3000);
    {
        NetSenderSubmission queue;
        if (queue.start(1, 101) != NI_OK || queue.ready(1, 128, 65536) != NI_OK) { std::fprintf(stderr, "SIGNAL_CHECK_FAILED line=%d\n", __LINE__); return false; }
        for (uint32_t iteration = 0; iteration < 3000; ++iteration)
        {
            netTestCheckpoint();
            uint64_t request = 0;
            uint64_t ticket = 0;
            const uint8_t data[6] = {'1','2','3','4','5','6'};
            if (queue.begin(101, 701, 10000, request) != NI_OK ||
                queue.sendSynchronous(request, data, sizeof(data), ticket) != NI_OK) { std::fprintf(stderr, "SIGNAL_CHECK_FAILED line=%d\n", __LINE__); return false; }
            std::shared_ptr<NetSubmissionSignalProbe> signal(new NetSubmissionSignalProbe());
            lifetimes.push_back(signal);
            int32_t registered = NI_READY;
            std::thread observer;
            if (iteration % 3 == 0)
            {
                registered = queue.observeSynchronous(ticket, signal);
            }
            if (iteration % 3 == 2)
            {
                // 输入均借用至join；只登记通知，不从第二线程驱动协议。
                observer = std::thread([&]() { registered = queue.observeSynchronous(ticket, signal); });
            }
            NetSenderSubmissionCommand command = {};
            if (queue.beginPoll() != NI_OK)
            {
                if (observer.joinable()) { observer.join(); }
                std::fprintf(stderr, "SIGNAL_CHECK_FAILED line=%d\n", __LINE__); return false;
            }
            while (queue.next(command) == NI_READY)
            {
                queue.finish(NI_OK);
            }
            queue.advanceSynchronous(NI_OK);
            if (iteration % 3 == 1)
            {
                registered = queue.observeSynchronous(ticket, signal);
            }
            if (observer.joinable()) { observer.join(); }
            if (registered != NI_OK || signal->m_count.load() != 1 ||
                queue.observeSynchronous(ticket, signal) != NI_INVALID_STATE ||
                queue.synchronousResult(ticket, true) != NI_OK ||
                queue.synchronousResult(ticket, true) != NI_NOT_FOUND ||
                !queue.complete(1, request, command)) { std::fprintf(stderr, "SIGNAL_CHECK_FAILED line=%d\n", __LINE__); return false; }
            signal.reset();
            queue.beginPoll();
        }
    }
    // 队列/索引可延迟回收已退役节点；整个接纳表析构后必须不再持有通知对象。
    for (const auto& lifetime : lifetimes)
    {
        if (!lifetime.expired()) { return false; }
    }
    std::puts("SUBMISSION_SIGNAL_PASS before=1000 after=1000 racing=1000 ownership=3000");
    return true;
}

void NetSubmissionSignalTest::run()
{
    if (!check()) { throw std::runtime_error("synchronous receipt signal"); }
}