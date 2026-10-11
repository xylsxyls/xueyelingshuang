KissFFT 131.1.0
来源：https://github.com/mborgerding/kissfft/tree/131.1.0
许可：BSD-3-Clause，保留COPYING及LICENSES/BSD-3-Clause。
职责：独立保存KissFFT来源和原始许可，不属于LumaPlayer专用依赖。本目录不是编译输入；上游C源码和私有头位于src/KissFFTCpp/KissFFTCpp/src，原始文件内容不变。
构建：KissFFTCpp二次开发库统一以double编译，调用方只包含KissFFTCpp/KissFFTCppAPI.h。LumaPlayerCore人声分离使用其5120点实数FFT/iFFT，由before_LumaPlayerCore.sh准备封装库，不将C源码复制到include或编入Core。其他工程可独立复用该库。
本目录不包含模型权重，也不承担推理运行时部署。