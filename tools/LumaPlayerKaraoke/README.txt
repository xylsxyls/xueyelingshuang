LumaPlayer K歌依赖与本机模型目录

miniaudio/：固定0.11.23，原生麦克风采集和独立监听，源码随Core编译，不使用Qt音频接口。来源、哈希和许可见该目录README及LICENSE。
KissFFT已独立至tools/KissFFT，仅保留来源说明及原始BSD-3-Clause许可；131.1.0的C源码和私有头归src/KissFFTCpp/KissFFTCpp/src，由独立二次开发库以double精度编译。Core/Test只包含KissFFTCpp/KissFFTCppAPI.h，不从tools或include引用第三方.c/.cpp。本目录不再保留kissfft子目录。
directml-1.24.4/：当前Windows x64 ONNX Runtime DirectML发行包，包含ORT 1.24.4、DirectML 1.15.4的头、三个运行DLL和完整许可。直接作为本目录下的依赖版本目录，不再嵌套onnxruntime父目录。Core通过KaraokeOrt.c按绝对路径动态加载，兼容旧VS2013编译器，不要求业务C++包含新版ORT头。
models/：UVR-MDX-NET-Inst_HQ_5.onnx小模型必须本地保留，客户端不依赖联网。931MB服务端mel_band_roformer_kim_T801.onnx只留使用说明、固定下载地址及校验信息，不作为客户端依赖；来源和两种适配器区别见models/README.txt。正式分发前另核查权重许可。

soundtouch/：固定2.4.0，以官方SoundTouchDLL C接口动态调用；算法、句柄分配和释放留在独立共享库，不静态合入FFmpegCpp。x86/x64的MT/MD与Debug/Release、源码和重建说明见soundtouch/使用说明.txt。
before_LumaPlayerCore.sh构建KissFFTCpp并准备miniaudio/ORT头及lib/karaoke所需的小模型，Windows x64另准备三个ORT/DirectML DLL及许可，不向common发布、不下载服务端模型。App/Test的version对Core使用same以执行该before；UI工程不直接拷贝第三方。FFmpeg共享库复用tools/ffmpeg，SoundTouch由before_FFmpegCpp.sh准备；旧Sonic仅保留历史对照，不属于当前运行依赖。
Core的MP3/MP4编码统一调用FFmpegCppMediaWriter；FFmpegCpp优先使用FFmpeg编码器，Windows既有FFmpeg缺MP3/AAC/H264编码器时使用库内Media Foundation后备，Core不再包含该平台实现。其他平台必须提供带对应编码器的目标FFmpeg SDK，不能复用Windows库；独立构建入口和完整链接检查见LumaPlayer开发文档。不将Windows构建通过记为五端验收。
正式产品默认读取自身karaoke/中的运行库和模型，不依赖临时试运行脚本；显式覆盖时LUMAPLAYER_ORT_RUNTIME为运行库完整文件路径，LUMAPLAYER_VOCAL_MODEL为小模型完整文件路径。不用Python参与产品运行。模型前后处理和非零音频测试已接入Core/Test，但数学和文件测试不等于真实音乐质量或声学延迟验收。

分发材料位置（2026-10-11整理）：
1. soundtouch/：完整源码归档、原始许可证、两份实际构建脚本、头文件、库及使用说明统一保存；重复sources/SoundTouch删除，旧说明已合并到soundtouch/使用说明.txt。
2. tools/ffmpeg/：统一保存FFmpeg SDK、完整源码归档、固定下载地址及SHA-256、历史configuration.txt、重建脚本和溯源说明。旧sources/FFmpeg已删除；历史源码/补丁对应关系未证实，新脚本未完成实际重建，不能把上游归档宣称为已核实对应当前DLL的完整源码。
3. 本目录第三方使用说明.txt：详细授权核查；models/UVR模型来源及授权说明.txt：模型来源及许可依据。旧license目录的两份正文移至上述位置，目录说明合并到本文。
4. common/LumaPlayer/karaoke/licenses/：随包适用条款、真实版权、模型署名、第三方通知、LGPL用户权利及源码获取说明，不含重复源码归档和开发核查材料。
5. sources目录已无独立材料，说明合并到本文后整目录删除。上述保留材料是长期开发与分发资料，不是结束工作流的清理对象；各依赖原始许可及源码归档内的LICENSE/COPYING不删除。

分发与更新规则：
1. 只复制安装包所需的声明及适用许可，不递归复制tools中的源码或详细核查材料。构建脚本只重建第三方库，不包含播放器发布操作；version/before/after不加入发布逻辑。
2. 单独提供的源码包须包含对应组件完整源码、实际修改、许可证、构建脚本和说明，并按版本关联已发布二进制。增量发布只有对应库改变才更新其源码及构建材料；未变库保持与旧二进制匹配的版本。
3. 在分发方自己的二进制下载位置同时提供所需对应源码，并在安装包、关于框及下载页面给出真实入口。仅有本地tools、上游链接或同版本未核实归档不等于履行源码提供义务；当前尚未建立公开下载服务。
4. miniaudio选择MIT No Attribution，安装包省略重复许可正文，tools/LumaPlayerKaraoke/miniaudio/LICENSE原件保留。KissFFT保留完整BSD条款及真实版权，省略COPYING索引；原件在tools/KissFFT。ORT完整第三方通知、DirectML官方条款、FFmpeg/SoundTouch许可及Qt既有许可/例外继续随包保留，未核实不适用的条款不按大小或名称猜测删除。
5. LGPL动态库仍有对应源码、用户权利及通知义务；Qt实际分发材料、模型权利来源、历史FFmpeg构建来源等未决项见第三方使用说明.txt。目录整理及保留LICENSE不能视为整套产品商业许可审核完成。