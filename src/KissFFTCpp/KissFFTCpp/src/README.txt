KissFFTCpp二次开发库
更新日期：2026-10-11
职责：封装KissFFT 131.1.0双精度实数正/逆FFT，不包含Qt、模型推理或产品STFT窗口规则。

调用：只包含KissFFTCpp/KissFFTCppAPI.h。实例通过open准备指定长度及方向，forward/inverse同步使用调用方缓冲区；长度至少2且为偶数，频点数为length/2+1。逆变换未归一化，调用方按length缩放。参数或分配失败返回false；open失败保留原计划。实例不可复制，同一实例须串行，不同实例各自拥有计划和工作区。
所有权：计划与内部工作区由库创建和释放，不跨模块释放调用方内存；公开接口不暴露kiss_fftr_cfg或上游复数类型。
构建：沿用version、before/after、DLL/LIB两套VS2013工程和CMake。上游C源码及私有头直接位于KissFFTCpp/src，不引用tools源码，也不把.c/.cpp复制到include。全部实现以kiss_fft_scalar=double编译，不混用float计划。
集成：before_LumaPlayerCore.sh先准备KissFFTCpp；LumaPlayerCore和Test通过公开API调用，PortableCore.cmake直接构建同一套库源码。Windows依赖准备后VS2013编译入口保持不变；新增KissFFTCpp.dll须随使用它的新Core一起部署，旧common程序在本次源码调整中不自动发布。
来源：https://github.com/mborgerding/kissfft/tree/131.1.0
许可：BSD-3-Clause，原始版权保留在上游源码，完整条款见LICENSE.txt；来源和原始许可独立存于tools/KissFFT，不再放在LumaPlayerKaraoke目录。上游C源码/私有头逐字保留，不按仓库业务代码风格重写。
验证：库边界及LumaPlayerTest稳定ID55验证计划方向、参数拒绝、失败保留、独立频点幅相、双声道STFT/iSTFT及零输入。实际已执行的构建/测试范围见common/LumaPlayerTest/测试文档.txt，不把跨平台源码目标写成五端已验证。