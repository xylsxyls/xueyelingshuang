#pragma once
#include "KissFFTCppMacro.h"
#include <cstddef>
#include <stdint.h>

/** 不依赖第三方类型的双精度复数，缓冲区由调用方拥有
*/
struct KissFFTCppComplex
{
    // 频点实部
    double real;
    // 频点虚部
    double imaginary;
};

/** 双精度实数FFT封装；实例独占计划与工作区，同一实例的操作须串行
*/
class KissFFTCppAPI KissFFTCpp
{
public:
    /** 创建未初始化的变换实例
    */
    KissFFTCpp();

    /** 在本库释放计划及工作区，不释放调用方缓冲区
    */
    ~KissFFTCpp();

    /** 准备实数FFT计划，失败时保留原计划
    @param [in] length 正偶数采样点数，至少2；须满足底层整数及分配范围
    @param [in] inverse true为逆变换，false为正变换
    @return 计划与工作区均准备成功才返回true
    */
    bool open(int32_t length, bool inverse);

    /** 同步正变换，输出含DC和Nyquist的非负频率复数
    @param [in] input 非空实数采样缓冲区，借用至返回
    @param [in] inputCount 必须等于计划采样点数
    @param [out] output 非空复数缓冲区，输入检查失败时不修改
    @param [in] outputCount 必须等于采样点数的一半加1
    @return 计划方向及缓冲区大小正确且变换完成才返回true
    */
    bool forward(const double* input, size_t inputCount,
        KissFFTCppComplex* output, size_t outputCount);

    /** 同步逆变换，保持KissFFT原生未归一化结果，调用方按采样点数缩放
    @param [in] input 非空非负频率复数缓冲区，借用至返回
    @param [in] inputCount 必须等于采样点数的一半加1
    @param [out] output 非空实数缓冲区，输入检查失败时不修改
    @param [in] outputCount 必须等于计划采样点数
    @return 计划方向及缓冲区大小正确且变换完成才返回true
    */
    bool inverse(const KissFFTCppComplex* input, size_t inputCount,
        double* output, size_t outputCount);

private:
    /** 禁止复制独占的FFT计划与工作区
    @param [in] other 不允许复制的实例
    */
    KissFFTCpp(const KissFFTCpp& other);

    /** 禁止复制赋值
    @param [in] other 不允许复制的实例
    @return 不提供实现，不允许调用
    */
    KissFFTCpp& operator=(const KissFFTCpp& other);

private:
    // 本库创建和释放的第三方FFT计划，不向调用方暴露第三方类型
    void* m_plan;
    // 本库分配和释放的双精度复数工作区
    void* m_bins;
    // 已准备计划的实数采样点数，未初始化时为0
    int32_t m_length;
    // 已准备计划是否为逆变换
    bool m_inverse;
};