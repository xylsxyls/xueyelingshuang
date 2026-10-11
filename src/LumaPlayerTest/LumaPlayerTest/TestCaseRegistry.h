#pragma once
#include <QString>
#include <map>
#include <set>
#include <vector>
#include <stdint.h>

/** 稳定用例ID；新增追加，删除保留空缺
*/
enum TestCaseId
{
    // 未初始化、空路径及倍率边界应返回约定结果
    CaseCppLifecycle = 1,
    // 反复初始化20次并释放，不得残留初始化状态
    CaseRepeatedInit = 2,
    // C接口生命周期和非法参数返回值正确
    CaseCApiLifecycle = 3,
    // C接口打开、预览、逐帧及异步播放暂停正确
    CaseCApiMedia = 4,
    // 预览覆盖目标帧，短AB包括B帧，暂停不清音频
    CaseMediaControl = 5,
    // C和C++独立配置、错误描述及参数校验一致
    CaseConfiguration = 6,
    // 完整缓存循环接缝及单帧100次循环均正确
    CaseFullLoopCache = 7,
    // 小缓存循环不得跳过A帧或越过B边界
    CasePrefixLoopCache = 8,
    // 频繁拖动预览应保留最新目标且安全退出
    CaseLatestPreview = 9,
    // AB逐帧连续移动与取消不得残留旧操作
    CaseLoopMoveCancel = 10,
    // 时间溢出保护和实际PCM倍速转换正确
    CaseFrameAndRate = 11,
    // 回调重入应拒绝，关闭后不得再输出旧帧
    CaseReentry = 12,
    // AVI和MP4可打开，切镜颜色及错误后恢复正确
    CaseMediaAndPixels = 13,
    // 25fps独立帧表的AB左右边界与末帧正确
    CaseAbBoundary = 14,
    // 播放中设置AB不得暂停、清音频或重新启动
    CasePlayingAb = 15,
    // 手动跳出AB继续播放，重入恢复循环，结尾从零开始
    CaseAbReentry = 16,
    // 打开、定位、预览、逐帧及播放中立即退出各5次
    CaseExitRace = 17,
    // 两个Core同时播放，销毁一个不影响另一个
    CaseMultiCore = 18,
    // 持续播放2分钟，帧持续输出且结尾正常重播
    CaseLongPlayback = 19,
    // 短AB持续循环2分钟，输出不断流且循环正常
    CaseLongLoop = 20,
    // 空窗口只出现上浮框，中央加号无提示
    CaseEmptyUi = 21,
    // 单击判定后切换播放，完整双击序列不改变播放状态
    CasePlayPauseUi = 22,
    // 32次拖动进度即时跟手，松开定位到正确帧
    CaseDragUi = 23,
    // 普通左右键按默认2秒跳转，不受AB范围限制
    CaseSeekKeys = 24,
    // 配置为0.5秒时左右键按小数步长跳转
    CaseFractionalKeys = 25,
    // 播放中右键菜单及设置AB不改变播放状态
    CaseAbMenu = 26,
    // 暂停时右击定位，画面和进度同步
    CasePausedMenu = 27,
    // 悬停AB按方向键只移动一个实际帧并暂停
    CaseFrameKey = 28,
    // 平移缩放倍率重置，保留AB进度和播放状态
    CaseResetUi = 29,
    // Esc与双击退出全屏恢复目标窗口，重复Esc不反向切换且保持暂停、进度与图钉
    CaseWindowState = 30,
    // 固定浮框避让视频，取消固定后隐藏，Esc退出保持未固定状态
    CasePinnedUi = 31,
    // 关于框标题贴边、无描边带阴影、可拖动且两按钮关闭
    CaseHelpUi = 32,
    // 重置尚在执行时关闭窗口，应正常收敛退出
    CaseQueuedClose = 33,
    // 独立播放器重复启动，窗口可见并正常退出
    CaseStartup = 34,
    // 日志实际落盘并包含源码文件信息
    CaseDiagnostics = 35,
    // 指定范围去重、删除跳过和非法输入判定正确
    CaseSelectionRules = 36,
    // 工具提示、按钮按下态和进度圆点悬停按下态正确
    CaseButtonVisuals = 37,
    // 任务切换中停止当前任务并回收线程200次，不得崩溃或残留线程
    CaseThreadStopRace = 38,
    // 0.1至5倍流式音频连续、分块一致且实际PCM供给充足
    CaseRatePitch = 39,
    // 1/2/5倍进度单击后800ms内持续播放，无暂停往返
    CaseProgressClick = 40,
    // 播放2倍及暂停5倍AB实际完成及时，清除淘汰在途解析
    CaseAbRateLatency = 41,
    // AB与连续调速后24次前后跳转，持续解码不崩溃并正常关闭
    CaseRateSeekLoopRace = 42,
    // 超大视频初始尺寸不超半屏，等比例居中且保留窗口状态
    CaseInitialMediaFit = 43,
    // 失焦取消未完成单击、收尾实际拖动，旧手势不影响新媒体
    CaseInputCancel = 44,
    // 空素材目录不复用旧路径，报告落盘失败不得计通过
    CaseTestIoBoundary = 45,
    // 录音前补静音、跳过补空白、回录只覆盖新区间并正确映射变速时长
    CaseKaraokeTimeline = 46,
    // 录音PCM分段、跳过静音、覆盖保留、变速时长及内存边界
    CaseKaraokePcmTimeline = 47,
    // K歌PCM时间线C ABI生命周期、写入、静音读取和清理
    CaseKaraokeCApi = 48,
    // 合成纯音升降-7至+7半音，验证频率、范围、时长及Windows独立LGPL动态库
    CaseKaraokePitchShift = 49,
    // 麦克风0至5000%实际增益、静音、独立监听和混音饱和边界正确
    CaseKaraokePcmProcessing = 50,
    // 播放时录制写入时间线，暂停仍监听但不录，未录区间补静音且回录覆盖
    CaseKaraokeCaptureFlow = 51,
    // 监听输出适配不同采样率和声道，分块总时长稳定且非法输入拒绝
    CaseKaraokeMonitorPcmConversion = 52,
    // 真实麦克风默认关闭、逐设备采集、暂停未录仍监听及开关恢复
    CaseKaraokeDeviceMonitor = 53,
    // 原生采集空句柄、增益边界、未启动读取和重复停止
    CaseKaraokeNativeBoundary = 54,
    // FFT封装边界及分离频谱的独立幅相、双声道重建、零谱和非法尺寸
    CaseKaraokeSpectrum = 55,
    // 模型分离流水线/串行一致性、重叠配置、短片/尾段、取消与时长；需本地模型
    CaseKaraokeSeparation = 56,
    // 麦克风及混音MP3/MP4真实编码、解码、非零音频和变速时长
    CaseKaraokeExport = 57,
    // 人声分离进度弹窗、伴奏开关、保存选项、原子保存和取消保护
    CaseKaraokeCompleteUi = 58,
    // 录制时保留AB但不循环，5倍到EOF结束，退出录制恢复正常AB
    CaseKaraokeRecordingEof = 59,
    // 50分钟生成录音分块落盘、覆盖、快照生命周期及采样时长
    CaseKaraokeDiskRecording = 60,
    // 奇数宽高视频导出偶数画布、方向及变速同步
    CaseKaraokeOddVideo = 61,
    // 播放暂停调速和小幅跳转均更新录音位置变更标识
    CaseKaraokeRecordingClock = 62,
    // 小窗口完整浮窗、增益10倍映射、麦克风移除与稳定轨道身份
    CaseKaraokeSmallPopup = 63,
    // 通用编码器非法输入、编码器可用性、编码或明确错误及资源释放
    CaseKaraokePortableWriter = 64,
    // 录制停止与浮窗寿命、取消重开、原生保存及整曲MP4
    CaseKaraokeSaveLifecycle = 65,
    // 播放中保存框内单选、无额外弹窗、保持存活及取消
    CaseKaraokeSaveDropdown = 66,
    // K歌存储非法读写/溢出显式失败、保留内容并恢复；分离/导出错误和取消有界返回
    CaseKaraokeErrorReturns = 67
};

/** 测试分类与ID独立
*/
enum TestCaseType
{
    // 瞬时功能回归，覆盖正确性和边界，不执行持续压力场景
    TestInstant = 0,
    // 重复、持续或竞争压力场景，须按本轮授权单独选择
    TestPressure = 1
};

/** 注册元数据，中文说明直接用于界面和报告
*/
struct TestCaseMetadata
{
public:
    // 中文功能及预期
    QString m_description;
    // 瞬时或压力
    TestCaseType m_type;
    // legacy/core/gui/stress/player
    QString m_engine;
    // 整个子进程预算，毫秒
    int32_t m_timeoutMs;
};

extern const std::map<TestCaseId, TestCaseMetadata> kTestCases;
extern const std::set<int32_t> kDeletedTestIds;

/** 统一的命令行及按钮选择解析，不执行任何测试
*/
class TestCaseRegistry
{
public:
    /** 校验登记完整性
    @return 空字符串表示正确，否则中文原因
    */
    static QString validate();

    /** 解析数字及闭区间，未知ID拒绝，已删除ID跳过
    @param [in] text 1,3,8-12形式
    @param [in] active 当前有效ID
    @param [in] deleted 已删除ID
    @param [out] selected 去重排序后的有效ID
    @param [out] skipped 已删除ID
    @return 空字符串成功，否则中文原因
    */
    static QString parse(const QString& text, const std::set<int32_t>& active,
        const std::set<int32_t>& deleted, std::vector<int32_t>* selected, std::vector<int32_t>* skipped);

    /** 按按钮或命令行选择用例
    @param [in] mode specified/instant/pressure/all
    @param [in] text 指定模式输入
    @param [out] selected 有效ID，all先瞬时后压力
    @param [out] skipped 已删除ID
    @return 空字符串成功，否则中文原因
    */
    static QString select(const QString& mode, const QString& text,
        std::vector<int32_t>* selected, std::vector<int32_t>* skipped);
};