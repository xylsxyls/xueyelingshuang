#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mmreg.h>
#include <stdint.h>
#include <stddef.h>
#include <cstring>
#include <string>

/** 局部COM引用的释放守卫，接收接口已有引用并在离开作用域时Release
模板参数T须提供Release；仅用于本文件的同步编码调用
*/
template<class T> struct FFmpegCppComPtr
{
public:
    // 本守卫持有并负责Release的COM接口，可为空
    T* p;

public:
    /** 初始化为空引用
    */
    FFmpegCppComPtr() :
p(nullptr)
    {

    }

    /** 释放当前持有的COM引用
    */
    ~FFmpegCppComPtr()
    {
        if (p) p->Release();
    }

    /** 获取COM输出参数地址，调用前须保证尚未持有引用
    @return 内部接口指针的地址，借用期为本守卫的生命周期
    */
    T** address()
    {
        return &p;
    }

    /** 借用已持有的COM接口
    @return 当前接口指针；调用方须保证其非空且不单独Release
    */
    T* operator->()
    {
        return p;
    }
};

/** Windows编码会话状态，拥有SinkWriter及本次成功初始化的COM和MF资源
由FFmpegCppNativeMediaWriter持有，所有操作和析构由初始化线程串行执行
*/
struct FFmpegCppNativeMediaWriterState
{
public:
    // 本实例拥有的输出写入器，析构时释放
    IMFSinkWriter* m_writer;
    // 写入器返回的音频流编号
    DWORD m_audio;
    // 写入器返回的视频流编号，纯音频模式不使用
    DWORD m_video;
    // 最近一次系统编码调用的HRESULT
    HRESULT m_status;
    // 本实例是否成功调用CoInitializeEx，需要配对CoUninitialize
    bool m_com;
    // 本实例是否成功调用MFStartup，需要配对MFShutdown
    bool m_mf;
    // 最近一次编码阶段的静态字符串，只借用不释放
    const char* m_stage;

public:
    /** 初始化为空的Windows编码会话
    */
    FFmpegCppNativeMediaWriterState() :
m_writer(nullptr),
m_audio(0),
m_video(0),
m_status(S_OK),
m_com(false),
m_mf(false),
m_stage("initialize")
    {

    }

    /** 释放写入器并配对关闭本实例初始化的MF和COM资源
    */
    ~FFmpegCppNativeMediaWriterState()
    {
        if (m_writer) m_writer->Release();
        if (m_mf) MFShutdown();
        if (m_com) CoUninitialize();
    }

    /** 复制输入数据到MF样本并同步交给写入器
    @param [in] index 目标流编号
    @param [in] data 调用期间借用的数据，不可为空
    @param [in] bytes 数据长度，单位字节，范围1至MAXDWORD
    @param [in] time 样本时间戳，单位100纳秒
    @param [in] duration 样本持续时间，单位100纳秒
    @return true表示WriteSample成功，失败详情保存在m_status
    */
    bool write(DWORD index, const void* data, size_t bytes, int64_t time, int64_t duration)
    {
        if (!m_writer || !data || bytes == 0 || bytes > MAXDWORD) { m_status = E_INVALIDARG; return false; }
        FFmpegCppComPtr<IMFSample> sample;
        FFmpegCppComPtr<IMFMediaBuffer> buffer;
        m_status = MFCreateSample(sample.address());
        if (SUCCEEDED(m_status)) m_status = MFCreateMemoryBuffer(static_cast<DWORD>(bytes), buffer.address());
        BYTE* target = nullptr;
        if (SUCCEEDED(m_status)) m_status = buffer->Lock(&target, nullptr, nullptr);
        if (SUCCEEDED(m_status))
        {
            std::memcpy(target, data, bytes);
            m_status = buffer->Unlock();
        }
        if (SUCCEEDED(m_status)) m_status = buffer->SetCurrentLength(static_cast<DWORD>(bytes));
        if (SUCCEEDED(m_status)) m_status = sample->AddBuffer(buffer.p);
        if (SUCCEEDED(m_status)) m_status = sample->SetSampleTime(time);
        if (SUCCEEDED(m_status)) m_status = sample->SetSampleDuration(duration);
        if (SUCCEEDED(m_status)) m_status = m_writer->WriteSample(index, sample.p);
        return SUCCEEDED(m_status);
    }
};

/** FFmpeg缺少MP3、AAC或H.264编码器时使用的Windows系统后备编码器
独占会话状态，由调用方在同一线程完成创建、写入和销毁
*/
class FFmpegCppNativeMediaWriter
{
public:
    /** 创建未打开输出的系统编码会话
    */
    FFmpegCppNativeMediaWriter();

    /** 释放会话状态；完整输出须先显式调用finish
    */
    ~FFmpegCppNativeMediaWriter();

    /** 创建MP3文件或包含H.264和AAC的MP4文件
    @param [in] path UTF-8输出路径，不可为空
    @param [in] video true表示包含视频，false表示纯音频
    @param [in] width 视频宽度，单位像素，视频模式要求正偶数
    @param [in] height 视频高度，单位像素，视频模式要求正偶数
    @return true表示写入器已开始接受样本
    */
    bool open(const std::string& path, bool video, int width, int height);

    /** 写入48kHz双声道交错S16音频
    @param [in] data 调用期间借用的PCM，至少frames乘2个样本
    @param [in] frames 每声道帧数，数据字节数不得超过MAXDWORD
    @param [in] time 输出时间戳，单位100纳秒
    @return true表示样本已提交给写入器
    */
    bool audio(const int16_t* data, size_t frames, int64_t time);

    /** 写入一帧自上而下的BGRA视频
    @param [in] data 调用期间借用的像素，不可为空
    @param [in] bytes 数据字节数，由上层校验为完整帧大小
    @param [in] time 输出时间戳，单位100纳秒
    @param [in] duration 帧持续时间，单位100纳秒
    @return true表示样本已提交给写入器
    */
    bool video(const unsigned char* data, size_t bytes, int64_t time, int64_t duration);

    /** 完成编码并写入文件尾
    @return true表示系统写入器Finalize成功
    */
    bool finish();

    /** 获取系统编码阶段及最近一次HRESULT
    @return 错误说明副本，不清除会话状态
    */
    std::string error() const;

private:
    /** 禁止复制独占的系统编码会话
    @param [in] other 另一编码器
    */
    FFmpegCppNativeMediaWriter(const FFmpegCppNativeMediaWriter& other);

    /** 禁止赋值独占的系统编码会话
    @param [in] other 另一编码器
    @return 禁用操作，不提供实现
    */
    FFmpegCppNativeMediaWriter& operator=(const FFmpegCppNativeMediaWriter& other);

private:
    // 本对象独占的Windows编码状态，析构时删除
    FFmpegCppNativeMediaWriterState* m_impl;
};
#endif