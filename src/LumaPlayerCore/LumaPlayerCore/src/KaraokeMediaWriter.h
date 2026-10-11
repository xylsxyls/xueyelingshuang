#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include "FFmpegCpp/FFmpegCppAPI.h"

/** Worker-owned platform-neutral encoder; codec/backend selection belongs to FFmpegCpp
*/
class KaraokeMediaWriter
{
public:
    /** 初始化KaraokeMediaWriter对象的状态，不转移输入借用对象的所有权
    */
    KaraokeMediaWriter();

    /** 释放对象持有的资源，退出前按实现等待其任务或回调
    */
    ~KaraokeMediaWriter();

    /** 初始化输出编码流，失败通过返回值和诊断报告
    @param [in] pathUtf8 输出UTF-8路径，调用期间保持有效
    @param [in] video 是否包含原视频画面
    @param [in] width 视频宽度，像素
    @param [in] height 视频高度，像素
    @return true表示编码器打开成功，false时可读取error
    */
    bool open(const std::string& pathUtf8, bool video, int width, int height);

    /** 向输出编码器写入48000Hz双声道S16音频
    @param [in] stereo 借用的双声道S16交错输入，frames乘2个元素
    @param [in] frames PCM帧数，不是交错样本数或字节数
    @param [in] start100ns 帧起始时间，100纳秒单位
    @return true表示编码写入成功，false时可读取error
    */
    bool audio(const int16_t* stereo, size_t frames, int64_t start100ns);

    /** 向输出编码器写入BGRA视频帧
    @param [in] bgra 借用的BGRA像素缓冲区
    @param [in] bytes 缓冲区字节数
    @param [in] start100ns 帧起始时间，100纳秒单位
    @param [in] duration100ns 时长，100纳秒单位
    @return true表示视频帧编码写入成功，false时可读取error
    */
    bool video(const unsigned char* bgra, size_t bytes, int64_t start100ns, int64_t duration100ns);

    /** 刷新编码器并结束输出文件
    @return true表示编码器刷新和文件收尾成功
    */
    bool finish();

    /** 取得最近错误诊断文本
    @return 诊断文本，无错误时为空
    */
    std::string error() const;

private:
    /** 初始化KaraokeMediaWriter对象的状态，不转移输入借用对象的所有权
    @param [in] other 禁止拷贝的源对象，不取得所有权
    */
    KaraokeMediaWriter(const KaraokeMediaWriter& other);

    /** 禁止对象赋值，避免复制资源所有权
    @param [in] other 禁止拷贝的源对象，不取得所有权
    @return 禁止赋值，该函数没有实现
    */
    KaraokeMediaWriter& operator=(const KaraokeMediaWriter& other);

private:
    // 拥有的FFmpeg编码器适配对象
    FFmpegCppMediaWriter m_writer;
};