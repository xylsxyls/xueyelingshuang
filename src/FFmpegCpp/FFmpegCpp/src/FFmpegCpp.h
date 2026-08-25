#pragma once
#include "FFmpegCppMacro.h"
#include "FFmpegCppModels.h"

/** FFmpeg视频文件对象封装，一个对象代表一个已打开或准备打开的视频文件
*/
class FFmpegCppAPI FFmpegCpp
{
public:
    /** 构造函数，只初始化成员变量，不打开文件
    */
    FFmpegCpp();

    /** 析构函数，兜底释放当前视频上下文，重复close安全
    */
    ~FFmpegCpp();

private:
    /** 禁止拷贝构造，避免多个对象同时持有同一个ffmpeg上下文
    @param [in] other 另一个FFmpegCpp对象
    */
    FFmpegCpp(const FFmpegCpp& other);

    /** 禁止赋值，避免ffmpeg上下文生命周期被复制
    @param [in] other 另一个FFmpegCpp对象
    @return 返回当前对象引用
    */
    FFmpegCpp& operator=(const FFmpegCpp& other);

public:
    /** 打开视频文件并读取基础媒体信息
    @param [in] filePath 视频文件完整路径，支持mp4、mov、mkv、avi、flv等ffmpeg可识别格式
    @return 返回true表示文件打开成功且基础信息读取成功
    */
    bool open(const std::string& filePath);

    /** 关闭当前视频文件并释放ffmpeg上下文，重复调用安全
    */
    void close();

    /** 判断当前对象是否已经打开视频文件
    @return 返回true表示当前对象持有可用的视频上下文
    */
    bool isOpen() const;

    /** 获取当前打开的视频文件路径
    @return 返回open传入的文件路径，未打开时返回空字符串
    */
    std::string filePath() const;

    /** 获取最近一次失败原因
    @return 返回最近一次open、解析、截图、转码等操作失败的错误文本
    */
    std::string lastError() const;

    /** 重新读取当前视频文件的基础媒体信息
    @return 返回true表示刷新成功，失败时可通过lastError获取原因
    */
    bool reloadMediaInfo();

    /** 获取当前视频的基础媒体信息副本
    @return 返回媒体信息；未打开或解析失败时，返回默认空信息
    */
    FFmpegCppMediaInfo mediaInfo() const;

    /** 获取当前视频的基础媒体信息
    @param [out] mediaInfo 输出媒体信息，不能为空
    @return 返回true表示mediaInfo已经被写入有效信息
    */
    bool getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const;

    /** 获取主视频流信息
    @param [out] streamInfo 输出主视频流信息，不能为空
    @return 返回true表示找到主视频流并写入streamInfo
    */
    bool getMainVideoStreamInfo(FFmpegCppStreamInfo* streamInfo) const;

    /** 获取主音频流信息
    @param [out] streamInfo 输出主音频流信息，不能为空
    @return 返回true表示找到主音频流并写入streamInfo
    */
    bool getMainAudioStreamInfo(FFmpegCppStreamInfo* streamInfo) const;

    /** 判断当前视频是否包含可解码的视频流
    @return 返回true表示包含视频流
    */
    bool hasVideo() const;

    /** 判断当前视频是否包含音频流
    @return 返回true表示包含音频流
    */
    bool hasAudio() const;

    /** 获取视频总时长
    @return 返回视频总时长，单位毫秒；未知时返回0
    */
    int64_t durationMilliseconds() const;

    /** 从当前视频中截取首帧图片并写入内存
    @param [out] imageData 输出编码后的图片数据，不能为空；成功时会写入图片格式、宽高、mimeType和二进制数据
    @param [in] option 截图选项，timestampMilliseconds会被当作0处理
    @return 返回true表示首帧截图成功
    */
    bool extractFirstFrameToMemory(FFmpegCppImageData* imageData,
                                   const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption());

    /** 从当前视频指定时间位置截取一帧图片并写入内存
    @param [in] timestampMilliseconds 截图时间点，单位毫秒；小于0时按0处理，大于总时长时由实现层裁剪到可用范围
    @param [out] imageData 输出编码后的图片数据，不能为空；成功时会写入图片格式、宽高、mimeType和二进制数据
    @param [in] option 截图选项；option.timestampMilliseconds会被timestampMilliseconds参数覆盖
    @return 返回true表示截图成功
    */
    bool extractFrameToMemory(int64_t timestampMilliseconds,
                              FFmpegCppImageData* imageData,
                              const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption());

    /** 从当前视频中截取首帧图片并保存到文件
    @param [in] outputFilePath 输出图片完整路径；为空时实现层可按filePath和图片格式自动生成路径
    @param [in] option 截图选项，timestampMilliseconds会被当作0处理
    @param [out] savedFilePath 可选输出最终保存路径，传nullptr表示不需要
    @return 返回true表示首帧保存成功
    */
    bool extractFirstFrameToFile(const std::string& outputFilePath,
                                 const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption(),
                                 std::string* savedFilePath = nullptr);

    /** 从当前视频指定时间位置截取一帧图片并保存到文件
    @param [in] timestampMilliseconds 截图时间点，单位毫秒
    @param [in] outputFilePath 输出图片完整路径；为空时实现层可按filePath、时间点和图片格式自动生成路径
    @param [in] option 截图选项；option.timestampMilliseconds会被timestampMilliseconds参数覆盖
    @param [out] savedFilePath 可选输出最终保存路径，传nullptr表示不需要
    @return 返回true表示截图保存成功
    */
    bool extractFrameToFile(int64_t timestampMilliseconds,
                            const std::string& outputFilePath,
                            const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption(),
                            std::string* savedFilePath = nullptr);

    /** 转码当前视频到目标文件
    @param [in] outputFilePath 输出视频完整路径，不能为空
    @param [in] option 转码选项，包括目标容器、视频编码、音频编码、分辨率、码率、CRF等
    @param [in] progressCallback 可选进度回调，返回false表示请求取消当前转码任务
    @param [in,out] userData 调用方自定义上下文指针，会原样传给progressCallback，库内部不读取也不释放
    @return 返回true表示转码成功；失败或被取消时可通过lastError获取原因
    */
    bool transcode(const std::string& outputFilePath,
                   const FFmpegCppTranscodeOption& option,
                   FFmpegCppProgressCallback progressCallback = nullptr,
                   void* userData = nullptr);

    /** 请求取消当前正在执行的截图或转码任务
    @return 返回true表示取消请求已经记录；如果当前没有任务，也返回true
    */
    bool cancelCurrentTask();

private:
    // 当前打开的媒体文件路径
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_filePath;
    // 最近一次失败原因
    std::string m_lastError;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前媒体信息缓存
    FFmpegCppMediaInfo m_mediaInfo;
    // 是否已经打开媒体文件
    bool m_isOpen;
    // 当前长任务是否被请求取消
    bool m_cancelRequested;
    // AVFormatContext指针，头文件不暴露ffmpeg类型
    void* m_formatContext;
    // 预留视频解码上下文指针，close兜底释放
    void* m_videoCodecContext;
    // 预留音频解码上下文指针，close兜底释放
    void* m_audioCodecContext;
    // 预留缩放上下文指针，close兜底释放
    void* m_scaleContext;
};