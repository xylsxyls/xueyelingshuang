FFmpeg依赖及重建说明

当前Windows库为FFmpeg 6.1.6，按x86/x64及MT/MD目录存放。产品只使用avformat/avcodec/avutil/swresample/swscale五个共享库及相应导入库；历史static32/64.lib保留对照，但FFmpegCpp DLL/LIB工程和CMake不再引用它们，不允许缺库时回退静态链接。before_FFmpegCpp.sh负责同时复制匹配头、导入库和运行库，不执行产品发布。
实际五个DLL的license/configuration接口均报告LGPL version 2.1 or later、--enable-shared --disable-static；不含--enable-gpl、--enable-nonfree或外部libx264/libx265。当前MP3/AAC/H.264导出仍由Windows Media Foundation后备完成，不为解决许可问题偷偷禁用原有导出功能。

源码：保留本目录已提交的ffmpeg-6.1.6.tar.xz，大小10497884字节，SHA-256：d4fcb164028dd3beee5d92c0ac72e46aac6973c75ea12dc14de07bf8f407370a。重建优先使用本地归档，缺失时才按固定上游版本下载并校验；常规version依赖准备及VS2013编译播放器仍使用已有SDK，不依赖下载源码。
来源：https://ffmpeg.org/releases/ffmpeg-6.1.6.tar.xz
源码许可：完整归档内保留COPYING.LGPLv2.1、LICENSE.md及各源文件作者和许可；随包许可仍保留在common/LumaPlayer/karaoke/licenses/FFmpeg-COPYING.LGPLv2.1.txt及FFmpeg-LICENSE.md，不因开发材料合并而删除。
历史配置：configuration.txt保留2026-10-08从五个实际DLL读取的版本、许可和一致的构建配置，内容原样迁入；不是本次build.sh重建成功的证明。本目录统一保存SDK、源码包、构建脚本及溯源资料，不再另存tools/LumaPlayerKaraoke/sources/FFmpeg。
构建：build.sh为MSVC共享库重建入口，在匹配位数的VS开发者命令环境中运行Git Bash，以32或64、MT或MD为参数，需要GNU Make及sha256sum；仅本地源码包缺失时还需curl或wget。MAKE和JOBS可指定工具与并发度。优先读取本目录归档，缺失时下载到build；本地、缓存或下载校验失败即停止，不使用损坏源码。禁用GPL/nonfree外部库及静态产物，输出导入库、DLL及PDB。不在此脚本执行产品发布。
详细说明：旧DLL中的构建路径指向已不存在的src/temp/ffmpeg_windows_build，没有保留源码补丁及全部构建记录；不能仅凭版本和配置证明官方归档完全对应这些旧DLL，不虚构changes.diff或“无修改”结论。此前GNU Make工具执行受阻，至本次材料合并仍未完成实际重建，不能记作新DLL已验证。完成可追溯重建后，必须保留实际修改、完整源码、最终配置及脚本再交付，对外商业发布前该事项必须解决。
This software is based in part on the work of the Independent JPEG Group. 完整归档保留其DCT文件条款；后续重建若有修改，应明确记录文件、日期和修改内容。

其他平台：须分别生成目标ABI共享SDK；Linux/Android为.so，macOS/iOS为动态库或动态framework，不复用Windows头和库。Core的PortableCore.cmake拒绝FFmpeg/SoundTouch静态.a，要求对应公开接口和目标编码能力。平台SDK和签名/商店许可未验证，不能将Windows验证当作五端通过。
分发：common/LumaPlayer/karaoke/licenses保留许可正文、状态说明、源码获取说明和用户替换权利；详细构建配置及核查材料统一在tools/ffmpeg。本机功能测试和材料整理不代表已经完成对外分发义务：仍须核实并提供与实际DLL对应的完整源码、修改和构建说明，在关于框、下载页面及用户条款说明FFmpeg和LGPL，不能用通用上游链接冒充已经核实的对应源码。动态链接不等于只带LICENSE；用户兼容替换及必要的调试逆向权利、下载网站/EULA和专利条件仍须分别审核。
依据：https://www.ffmpeg.org/legal.html