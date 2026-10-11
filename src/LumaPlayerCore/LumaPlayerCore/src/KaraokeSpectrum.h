#pragma once
#include <vector>

/** UVR-MDX-NET-Inst_HQ_5 transform; stereo 44100 Hz, periodic Hann, centered STFT
*/
class KaraokeSpectrum
{
public:
    // 固定模型的FFT长度，采样点
    static const int FftSize = 5120;
    // STFT相邻帧步长，采样点
    static const int Hop = 1024;
    // 单声道频率格数
    static const int Bins = 2560;
    // 模型时域帧数
    static const int Steps = 256;
    // 单次推理的双声道PCM帧数
    static const int Frames = Hop * (Steps - 1);
    // 模型输入输出浮点元素数
    static const int Elements = 4 * Bins * Steps;

    /** Interleaved PCM to [1,4,2560,256]: left real/imaginary, right real/imaginary
    @param [out] spectrum MDX浮点张量，布局为1×4×2560×256
    @param [in] audio 44100Hz双声道交错浮点PCM，输入窗口帧数须匹配模型
    @return 输入形状正确且STFT完成才返回true
    */
    static bool analyze(const std::vector<float>& audio, std::vector<float>* spectrum);

    /** Direct instrumental spectrum to PCM, without mask multiply or vocal subtraction
    @param [in] spectrum MDX浮点张量，布局为1×4×2560×256
    @param [out] audio 44100Hz双声道交错浮点PCM，输入窗口帧数须匹配模型
    @return 频谱形状正确且重建完成才返回true
    */
    static bool synthesize(const std::vector<float>& spectrum, std::vector<float>* audio);
};