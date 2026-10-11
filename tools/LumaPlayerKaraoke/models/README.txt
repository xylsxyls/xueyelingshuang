本目录保存客户端离线小模型及服务端大模型获取说明；不使用.gitignore隐藏必需模型。客户端构建和运行不依赖联网下载。
当前模型：UVR-MDX-NET-Inst_HQ_5.onnx，59074342字节（56.34MiB）。
SHA-256：811cb24095d865763752310848b7ec86aeede0626cb05749ab35350e46897000。
来源：https://github.com/TRvlvr/model_repo/releases/tag/all_public_uvr_models ，资产ID208872913。
参数来源：https://raw.githubusercontent.com/TRvlvr/application_data/main/mdx_model_data/model_data.json 。文件末10240000字节MD5为cb790d0c913647ced70fc6b38f5bea1a，与该参数表一致；这不是完整文件MD5。
2026-10-08从上游下载，校验完整大小、SHA-256、参数表识别值，并用本地ORT实际执行Test56完整推理、取消和短尾素材。参数表匹配不等于声学质量验收。
输入/输出：float32 [batch,4,2560,256]，batch运行取1，44.1kHz双声道，四平面为左实部/左虚部/右实部/右虚部；5120点FFT、1024 hop、periodic Hann、中心反射填充、261120帧窗口。输入最低3频点置零，输出为直接伴奏复数谱，不是人声掩码；重建应用1.010补偿。
当前重叠130560帧，单Session顺序处理；旧8/4秒配置和旧RoFormer张量不能混用。更换其他模型须先确认采样率、张量、FFT、输出语义和补偿，不只改文件名。
许可边界：2026-10-08补查UVR官方README模型使用声明及gui_data/constants.py内完整MIT文字，已保留Copyright (c) 2022 Ultimate Vocal Remover并署名UVR、Anjok07、aufr33和MDX-Net架构作者Kuielab、Woosung Choi。安装包在common/LumaPlayer/karaoke/licenses/UVR-MIT.txt及第三方使用说明.txt保留许可和署名；详细来源/授权核查在本目录UVR模型来源及授权说明.txt，不重复放进安装包。官方程序的NORMAL_REPO与本文件下载来源一致；这比仅引用代码许可或镜像MIT标签更明确，但尚未取得针对本权重SHA-256的个别商用再分发确认，不能宣称训练数据和所有第三方权利已审核。正式对外商业分发仍需核实，本机部署不等于授权审核通过。

服务端预留模型：mel_band_roformer_kim_T801.onnx。本体不留在仓库，不参与客户端构建、部署或启动；后续LumaPlayerServer按需下载一次并复用校验通过的缓存，客户端小模型不能删除。
使用说明：将模型下载到服务端模型数据目录，完成后核对下述大小和完整SHA-256，再交给RoFormer适配器。可使用curl -fL --retry 2 -o <临时文件> <下述直链>，或wget -O <临时文件> <下述直链>；下载到临时文件，校验通过后再替换目标。链接失效或校验失败时保留原有效文件、记录后台错误；未来服务端可用客户端小模型降级，但须使用对应适配器并标明结果所用模型，本轮不实现服务器或自动下载。
接口区别：float32输入[1,2050,801,2]，44.1kHz双声道8秒STFT，2048点FFT、441 hop、periodic Hann；输出[1,1,2050,801,2]是人声复数掩码，不是伴奏PCM。当前Core仅实现上述MDX-HQ5适配，不能只替换LUMAPLAYER_VOCAL_MODEL文件路径就使用此模型。正式服务端及商业使用前仍须核对权重许可。
大小：931054408字节；SHA-256：1b8afd7780d8a234527748821dee6bc746d346f2088751c12fd48e8c873f625a。
来源版本：models-v1.0，https://github.com/santiquiroz/port-bs-roformer-onnx/releases/tag/models-v1.0 。2026-10-09本地完整哈希与发布资产一致、直链HEAD返回200；不保证未来一直可下载，不以HEAD代替下载文件校验。
下载地址：https://github.com/santiquiroz/port-bs-roformer-onnx/releases/download/models-v1.0/mel_band_roformer_kim_T801.onnx