#pragma once

// 透明隧道帧与额度测试集合，逐用例独立建立预期并检查实际结果
class NetTunnelTestCases
{
public:
    /** 原生/Wasm同源外层格式、完整64位通道号、分片背压和拒绝边界
    */
    static void portableRecords();
};