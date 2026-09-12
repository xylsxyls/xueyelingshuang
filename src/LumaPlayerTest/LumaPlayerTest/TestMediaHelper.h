#pragma once
#include <QString>
#include <QByteArray>
#include <atomic>

/** 后台查找真实视频并生成确定性AVI，用户素材只读访问 */
class TestMediaHelper
{
public:
    /** 准备本轮素材，不递归扫描，发现一个可打开视频立即停止
    @param [in] directory 用户目录
    @param [in] output 自动素材目录
    @param [in] executable 隔离探测子程序
    @param [in] exit 协作退出标记
    @param [out] media 真实视频完整路径
    @param [out] fixture 自动AVI完整路径
    @return 空字符串成功，否则错误原因
    */
    static QString prepare(const QString& directory, const QString& output,
        const QString& executable, const std::atomic<bool>& exit, QString* media, QString* fixture);

private:
    /** 生成固定RIFF素材，每帧检查取消
    @param [in] exit 协作退出标记
    @return 完整AVI数据，取消时为空
    */
    static QByteArray generate(const std::atomic<bool>& exit);

    /** 封装RIFF块并处理偶数字节对齐
    @param [in] tag 四字节标识
    @param [in] data 内容
    @return 块数据
    */
    static QByteArray chunk(const QByteArray& tag, const QByteArray& data);

    /** 封装RIFF列表
    @param [in] tag 列表类型
    @param [in] data 子块
    @return 列表数据
    */
    static QByteArray list(const QByteArray& tag, const QByteArray& data);
};