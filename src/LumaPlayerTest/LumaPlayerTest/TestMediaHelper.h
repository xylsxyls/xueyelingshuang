#pragma once
#include <QString>
#include <QByteArray>
#include <atomic>

/** 后台查找真实视频并生成确定性AVI，用户素材只读访问
*/
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

    /** 生成固定RIFF素材，每帧检查取消
    @param [in] exit 协作退出标记
    @param [in] asymmetric 是否生成非对称首帧，以检出错误居中或翻转
    @param [in] odd 是否生成奇数尺寸，用于布局边界断言
    @return 完整AVI数据，取消时为空
    */
    static QByteArray generate(const std::atomic<bool>& exit, bool asymmetric = false, bool odd = false);

    /** 生成44.1kHz双声道非零PCM，帧数明确，用于分离时长和尾段测试
    @param [in] path 输出路径，调用期间保持有效
    @param [in] frames PCM帧数，不是交错样本数或字节数
    @return 文件完整写入成功才返回true
    */
    static bool writeSeparationWave(const QString& path, int32_t frames);

private:
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