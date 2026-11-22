#pragma once

/** 独立编译本提供方的接口回归，避免同名C/C++门面混用
*/
class NetSenderFacadeNetPeerCppTest
{
public:
    /** 检查NetPeer的Cpp入口、独立客户端、参数错误和配对释放
    */
    static void peerCpp();
};