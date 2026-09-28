#pragma once
#include "PdfReaderSessionState.h"
#include <functional>
#include <map>

/** GUI端会话门面：缓存元数据、关联回调，不在GUI调用Core
*/
class PdfReaderSession
{
public:
    /** 复制初始化配置并绑定GUI接收者
    @param [in] receiver 提供processCoreResults槽的窗口
    */
    explicit PdfReaderSession(QObject* receiver);

    /** 断开接收者并有序提交资源关闭；不等待执行线程
    */
    ~PdfReaderSession();

    /** 接纳一次请求，业务变更同时最多一项，预览限制总数
    @param [in] request 请求参数，身份由会话分配
    @param [in] completion GUI线程终态回调
    @return 接受的请求序号；0表示未接纳，无副作用
    */
    uint64_t submit(PdfReaderRequest request, const std::function<void(const PdfReaderResult&)>& completion);

    /** GUI槽调用，取出终态并更新缓存后执行回调
    */
    void processResults();

    /** 使所有旧预览结果失效，不撤销已接纳的写入
    */
    void invalidateRenders();

    /** 有序异步关闭；允许重复调用
    @param [in] completion 关闭完成后GUI回调
    */
    void close(const std::function<void(const PdfReaderResult&)>& completion);

    /** 返回文档快照是否有效
    @return 存在页面返回true
    */
    bool isOpen() const;

    /** 返回GUI快照页数
    @return 已完成的页面数量
    */
    int32_t pageCount() const;

    /** 读取页面尺寸快照
    @param [in] index 页码
    @param [out] info 页面尺寸
    @return 有效时返回true
    */
    bool pageInfo(int32_t index, PdfReaderCoreCPageInfo* info) const;

    /** 是否有业务操作在途
    @return 初始化/渲染不计入业务互斥
    */
    bool busy() const;

    /** 全部已接纳请求是否已收到终态
    @return 邮箱回调均已处理返回true
    */
    bool idle() const;
private:
    // 任务持强引用；接收者销毁后仍存活到最终关闭
    std::shared_ptr<PdfReaderSessionState> m_state;
    // 以下成员仅GUI访问
    uint64_t m_nextId;
    uint64_t m_businessId;
    int32_t m_maxPending;
    // 邮箱与在途渲染共用像素预算，默认等于Core单次上限
    uint64_t m_maxPendingPixels;
    uint64_t m_pendingPixels;
    std::map<uint64_t, uint64_t> m_pixelReservations;
    QVector<PdfReaderCoreCPageInfo> m_pages;
    std::map<uint64_t, std::function<void(const PdfReaderResult&)>> m_pending;
};