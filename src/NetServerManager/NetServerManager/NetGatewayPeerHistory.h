#pragma once
#include <stdint.h>
#include <array>

/** 单个物理代次的不可复用编号水位；只在活跃通道索引未命中后判断。
编号跳过的间隙也不可能在将来合法复用；丢弃旧编号不授予通道、身份或额度。
四个高位域分别记账，存储始终为32字节，不按连接寿命积累墓碑。
*/
class NetGatewayPeerHistory
{
public:
    /** 创建四个尚未使用的编号域。
    */
    NetGatewayPeerHistory();

    /** 记录已接纳或已明确拒绝并消耗的通道号。
    @param [in] channel 本物理代次的不可复用编号；0忽略
    */
    void remember(uint64_t channel);

    /** 判断非活跃编号是否已过期；不会允许未知未来编号。
    @param [in] channel 必须先查询活跃路由，不能用本方法跳过活跃路由校验
    @return 非0且不超过本编号域既有水位
    */
    bool obsolete(uint64_t channel) const;

private:
    // 按channel最高两位索引，借用者不得跨物理代次复用本对象。
    std::array<uint64_t, 4> m_highWater;
};