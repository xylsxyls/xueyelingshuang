ONNX Runtime当前构建与运行依赖

版本：Windows x64 ONNX Runtime DirectML 1.24.4、DirectML 1.15.4；本版本目录直接位于tools/LumaPlayerKaraoke下，与miniaudio等依赖平级，内部保留include、lib/x64及licenses。目录名中的1.24.4是ORT发行包版本，不是DirectML库版本；这是同时包含ORT和DirectML的推理依赖包，不是单独的DirectML源码库。旧onnxruntime父目录及无用空目录已移除。
来源：NuGet Microsoft.ML.OnnxRuntime.DirectML 1.24.4和Microsoft.AI.DirectML 1.15.4。两个原始nupkg只用于下载提取，不是编译输入；11个已提取头文件、DLL及许可/通知与包内成员逐项SHA-256相同，核对后删除下载原包。
包地址：https://www.nuget.org/packages/Microsoft.ML.OnnxRuntime.DirectML/1.24.4 和 https://www.nuget.org/packages/Microsoft.AI.DirectML/1.15.4 。原包SHA-256分别为57e9f11b73437bef7a309496135d4c1f96b1a8e9ddba60013fa27bfc1d788681和4e7cb7ddce8cf837a7a75dc029209b520ca0101470fcdf275c1f49736a3615b9。

构建：before_LumaPlayerCore.sh复制官方C API头到include/onnxruntime，并为Windows x64复制三个DLL和当前许可到lib/karaoke；Core的KaraokeOrt.c按C编译，通过C API 24隔离VS2013 C++兼容边界。小模型由同一before从models复制，App/Test的version对Core使用same，不跳过资源准备。

运行：Windows至少需要当前支持的系统及系统MSVC运行库；应用提前选择System32 CRT以避免壳扩展旧CRT冲突。缺依赖必须报告实际失败，文件存在不能代替Session/Run验证。

平台：DirectML二进制仅用于Windows/Xbox；其他平台使用各自ORT目标SDK，不能复制Windows DLL。PortableCore.cmake仍读取固定C头；iOS另链接目标ORT。Windows通过不代表其他四端已编译或运行。
许可：保留当前五份官方LICENSE/ThirdPartyNotices原件，安装包也保留适用条款；ORT MIT与DirectML二进制许可不同，不以清理下载包为理由删通知。